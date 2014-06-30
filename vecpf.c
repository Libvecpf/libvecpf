/* This is part of libvecpf, the Vector Printf Library.

   Author(s): Michael Brutman <brutman@us.ibm.com>
              Ryan S. Arnold <rsa@linux.vnet.ibm.com>

   Copyright (c) 2010-2014 IBM Corporation
   All rights reserved.

   The Vector Printf Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License version
   2.1.

   The Vector Printf Library is distributed in the hope that it will be
   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
   General Public License version 2.1 for more details.

   You should have received a copy of the GNU Lesser General Public License
   version 2.1 along with the Vector Printf Library; if not, write to the
   Free Software Foundation, Inc.,59 Temple Place, Suite 330, Boston,
   MA 02111-1307 USA.

   Please see libvecpf/LICENSE for more information.  */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <printf.h>

#include <altivec.h>

#include "config.h"
#include "vecpf.h"

/* libvecpf

   This library extends printf so that it may print out vector data
   types.  The description of the extensions are in the AltiVec Technology
   Programming Interface Manual.  Below is a paraphrasing of the
   extensions:

     New separator chars (used like flags):
       ',', ';', ':', '_'

       The default separator is a space unless the 'c' conversion is
       being used.  If 'c' is being used the default separator character
       is a null.  Only one separator character may be specified.

     New size modifiers:
       'vl', 'vh', 'lv', 'hv', 'v'

     Valid modifiers and conversions (all else are undefined):

         vl or lv: integer conversions; vectors are composed of four byte vals
         vh or hv: integer conversions; vectors are composed of two byte vals
         v: integer conversions; vectors are composed of 1 byte vals
         v: float conversions; vectors are composed of 4 byte vals

   Deviations from the PIM:

   - We only support the default separator; as things are today we can't
     add flags to printf to support the other separators.

   - We add a new modifier 'vv' to support vector double for VSX, which
     is new in Power Instruction Set Architecture (ISA) Version 2.06.

   Quick intro to vectors:

   Vectors are 16 bytes long and can hold a variety of data types.  These
   are what we know of at the moment.

   Altivec and VSX:

     Type                            Elements  VMX  VSX
     unsigned and signed char          16       Y    Y
     bool char                         16       Y    Y
     unsigned and signed short          8       Y    Y
     bool short                         8       Y    Y
     unsigned and signed int            4       Y    Y
     bool int                           4       Y    Y
     float                              4       Y    Y
     pixel                              8       Y    Y

   VSX only:

     Type                            Elements  VMX  VSX
     double                             2       N    Y

   The Altivec Programming Interface Manual defines how to print all of these
   data types except for pixel, which is undefined and double, which is
   newer than the Altivec PIM.  We add support for double but leave pixel
   alone.

*/


/* Size of static buffer buffer to describe the vector internals.  */

#define FMT_STR_MAXLEN  64

/* A handle for the vector data type.  Filled in by register_printf_type
   when the library first initializes. */

static int printf_argtype_vec;

/* Table for our registered printf modifiers.  'bits' is filled in as
   we register each modifier.  */

typedef struct
{
  unsigned short bits;
  const wchar_t *modifier_string;
} vector_modifier_t;

static vector_modifier_t vector_mods[] =
{
  {-1, L"vl" },  /* Vector of words.  */
  {-1, L"lv" },  /* Vector of words (alias for vl).  */
  {-1, L"vh" },  /* Vector of halfwords.  */
  {-1, L"hv" },  /* Vector of halfwords (alias for vh).  */
  {-1, L"v"  },  /* Vector of char or single precision floats.  */

  {-1, L"vv" },  /* Vector of double precision floats.  */
};
static const int vector_mods_len = sizeof (vector_mods) /
				   sizeof (vector_mods[0]);

/* Master tables of valid specifiers and modifiers and how to handle them.
   We have one table for integer conversions and one table for floating
   point conversions.

     spec          - the original specifier character from the printf
     bits_index    - index into the table above
     mod_and_spec  - the combination of modifier and spec char that will be
                     used for output of each element
     element_size  - how many bytes in size is each element of the vector
     data_type     - used to pick elements out of the vector via a union

   At printf time we scan this table for a match on the incoming specifier and
   modifier bits.  If we get a match, the rest of the fields tell us how to
   generate the output.
*/

typedef struct
{
  const wchar_t spec;
  const wchar_t bits_index;
  const char *mod_and_spec;
  const int element_size;
  const int data_type;
} vector_types_rec_t;

/* The entries for VDT_signed_int, VDT_signed_short, VDT_unsigned_int, and
 * VDT_unsigned_short are duplicated due to the aliases, 'lv' -> 'vl' and 'hv'
 * -> 'vh'.  */
static const vector_types_rec_t int_types_table[] =
{

  {L'd', 0, "d",   4, VDT_signed_int},
  {L'd', 1, "d",   4, VDT_signed_int},
  {L'd', 2, "hd",  2, VDT_signed_short},
  {L'd', 3, "hd",  2, VDT_signed_short},
  {L'd', 4, "hhd", 1, VDT_signed_char},

  {L'i', 0, "i",   4, VDT_signed_int},
  {L'i', 1, "i",   4, VDT_signed_int},
  {L'i', 2, "hi",  2, VDT_signed_short},
  {L'i', 3, "hi",  2, VDT_signed_short},
  {L'i', 4, "hhi", 1, VDT_signed_char},

  {L'o', 0, "o",   4, VDT_unsigned_int},
  {L'o', 1, "o",   4, VDT_unsigned_int},
  {L'o', 2, "ho",  2, VDT_unsigned_short},
  {L'o', 3, "ho",  2, VDT_unsigned_short},
  {L'o', 4, "hho", 1, VDT_unsigned_char},

  {L'u', 0, "u",   4, VDT_unsigned_int},
  {L'u', 1, "u",   4, VDT_unsigned_int},
  {L'u', 2, "hu",  2, VDT_unsigned_short},
  {L'u', 3, "hu",  2, VDT_unsigned_short},
  {L'u', 4, "hhu", 1, VDT_unsigned_char},

  {L'x', 0, "x",   4, VDT_unsigned_int},
  {L'x', 1, "x",   4, VDT_unsigned_int},
  {L'x', 2, "hx",  2, VDT_unsigned_short},
  {L'x', 3, "hx",  2, VDT_unsigned_short},
  {L'x', 4, "hhx", 1, VDT_unsigned_char},

  {L'X', 0, "X",   4, VDT_unsigned_int},
  {L'X', 1, "X",   4, VDT_unsigned_int},
  {L'X', 2, "hX",  2, VDT_unsigned_short},
  {L'X', 3, "hX",  2, VDT_unsigned_short},
  {L'X', 4, "hhX", 1, VDT_unsigned_char},

  {L'c', 4, "c", 1, VDT_unsigned_char},
};
static const int int_types_table_len = sizeof (int_types_table) /
				       sizeof (int_types_table[0]);

static const vector_types_rec_t fp_types_table[] =
{
  {L'f', 4, "f", 4, VDT_float},
  {L'e', 4, "e", 4, VDT_float},
  {L'E', 4, "E", 4, VDT_float},
  {L'g', 4, "g", 4, VDT_float},
  {L'G', 4, "G", 4, VDT_float},
  {L'a', 4, "a", 4, VDT_float},
  {L'A', 4, "A", 4, VDT_float},

  {L'f', 5, "f", 8, VDT_double},
  {L'e', 5, "e", 8, VDT_double},
  {L'E', 5, "E", 8, VDT_double},
  {L'g', 5, "g", 8, VDT_double},
  {L'G', 5, "G", 8, VDT_double},
  {L'a', 5, "a", 8, VDT_double},
  {L'A', 5, "A", 8, VDT_double},
};
static const int fp_types_table_len = sizeof (fp_types_table) /
				      sizeof (fp_types_table[0]);

/* Variable argument handler registered with register_printf_type */
static void
vec_va (void *mem, va_list *ap)
{
  vector unsigned int v = va_arg (*ap, vector unsigned int);
  memcpy (mem, &v, sizeof(v));
}

/* Copy routine for one of our data elements. */
static int
vec_ais (const struct printf_info *info, size_t n, int *argtype, int *size)
{
  int i;
  for (i=0; i<vector_mods_len; ++i)
    {
      /* Only return '1' if we're supposed to be handling this data type.  */
      if ((info->user & vector_mods[i].bits))
  	{
	  argtype[0] = printf_argtype_vec;
	  size[0] = sizeof (vector unsigned int);
	  return 1;
	}
    }
  return -1;
}


static void
gen_fmt_str (const struct printf_info *info, const char *sz_flags_and_conv,
             char *fmt_str_buf)
{
  int fmt_str_idx = 0;

  fmt_str_buf[fmt_str_idx++] = '%';

  if (info->alt)
    fmt_str_buf[fmt_str_idx++] = '#';
  if (info->space)
    fmt_str_buf[fmt_str_idx++] = ' ';
  if (info->left)
    fmt_str_buf[fmt_str_idx++] = '-';
  if (info->showsign)
    fmt_str_buf[fmt_str_idx++] = '+';
  if (info->group)
    fmt_str_buf[fmt_str_idx++] = '\'';

  if (!info->left && info->pad == '0')
    fmt_str_buf[fmt_str_idx++] = '0';

  fmt_str_buf[fmt_str_idx++] = '*';
  fmt_str_buf[fmt_str_idx++] = '.';
  fmt_str_buf[fmt_str_idx++] = '*';

  strncpy (fmt_str_buf + fmt_str_idx, sz_flags_and_conv,
           FMT_STR_MAXLEN - fmt_str_idx);
}

static int
vec_printf_d (FILE *fp, const struct printf_info *info,
              const void *const *args)
{
  char fmt_str[FMT_STR_MAXLEN];
  int i;
  int limit;

  vp_u_t vp_u;

  /* Find entry in table. */
  int table_idx = -1;
  int j;

  for (j=0; j<int_types_table_len; ++j)
    {
      if ((info->spec == int_types_table[j].spec)
           && (info->user & vector_mods[int_types_table[j].bits_index].bits))
      {
        table_idx = j;
        break;
      }
    }

  if (table_idx == -1)
    return -2;

  gen_fmt_str (info, int_types_table[table_idx].mod_and_spec, fmt_str);

  limit = LIBVECTOR_VECTOR_WIDTH_BYTES /
          int_types_table[table_idx].element_size;

  for (i=0; i < limit; i++)
    {
      memcpy (&vp_u, *((void***)args)[0], sizeof( vp_u.v ));

      switch (int_types_table[table_idx].data_type)
      {
        case VDT_unsigned_int:
        {
          fprintf (fp, fmt_str, info->width, info->prec, vp_u.ui[i]);
          break;
        }
        case VDT_signed_int:
        {
          fprintf (fp, fmt_str, info->width, info->prec, vp_u.si[i]);
          break;
        }
        case VDT_unsigned_short:
        {
          fprintf (fp, fmt_str, info->width, info->prec, vp_u.uh[i]);
          break;
        }
        case VDT_signed_short:
        {
          fprintf (fp, fmt_str, info->width, info->prec, vp_u.sh[i]);
          break;
        }
        case VDT_unsigned_char:
        {
          fprintf (fp, fmt_str, info->width, info->prec, vp_u.uc[i]);
          break;
        }
        case VDT_signed_char:
        {
          fprintf (fp, fmt_str, info->width, info->prec, vp_u.sc[i]);
          break;
        }

      } /* end switch */

      if ((limit > 1 ) && (i < (limit-1)))
        {
          if (!((int_types_table[table_idx].data_type == VDT_unsigned_char)
             && (!strcmp(int_types_table[table_idx].mod_and_spec, "c"))))
          {
            fputs (" ", fp);
          }
        }
    } /* end for */

  return 0;
}

static int
vec_printf_f (FILE *fp, const struct printf_info *info,
              const void *const *args)
{
  char fmt_str[FMT_STR_MAXLEN];
  int i;
  int limit;

  vp_u_t vp_u;

  /* Find entry in table. */
  int table_idx = -1;
  int j;

  for (j=0; j<fp_types_table_len; ++j)
    {
      if ((info->spec == fp_types_table[j].spec)
           && (info->user & vector_mods[fp_types_table[j].bits_index].bits))
      {
        table_idx = j;
        break;
      }
    }

  if (table_idx == -1) {
    return -2;
  }

  gen_fmt_str (info, fp_types_table[table_idx].mod_and_spec, fmt_str);

  limit = LIBVECTOR_VECTOR_WIDTH_BYTES /
          fp_types_table[table_idx].element_size;

  for (i=0; i < limit; i++)
    {
      memcpy (&vp_u, *((void***)args)[0], sizeof( vp_u.v ));

      switch (fp_types_table[table_idx].data_type)
      {
        case VDT_float:
        {
          fprintf (fp, fmt_str, info->width, info->prec, vp_u.f[i]);
          break;
        }
        case VDT_double:
        {
          fprintf (fp, fmt_str, info->width, info->prec, vp_u.d[i]);
          break;
        }
      } /* end switch */

      if ((limit > 1 ) && (i < (limit-1)))
        {
	  /* This should never happen in the 'fp' path.  */
          if (!((int_types_table[table_idx].data_type == VDT_unsigned_char)
             && (!strcmp(int_types_table[table_idx].mod_and_spec, "c"))))
          {
            fputs (" ", fp);
          }
        }
    } /* end for */

  return 0;
}

static int
__register_printf_vec( void )
{
  int i;


  /* Register a new type, and a function to call to copy an element
     of that type when going through the varargs list.

     Register_printf_type doesn't acknowlege failure, so assume the
     the retun code is always good. */

  printf_argtype_vec = register_printf_type (vec_va);

  /* Register our modifiers */

  for (i=0; i<vector_mods_len; ++i)
    {
      vector_mods[i].bits
	= register_printf_modifier (vector_mods[i].modifier_string);
    }


  /* Indicate our interest in integer types */
  register_printf_specifier ('d', vec_printf_d, vec_ais);
  register_printf_specifier ('i', vec_printf_d, vec_ais);
  register_printf_specifier ('o', vec_printf_d, vec_ais);
  register_printf_specifier ('u', vec_printf_d, vec_ais);
  register_printf_specifier ('x', vec_printf_d, vec_ais);
  register_printf_specifier ('X', vec_printf_d, vec_ais);
  register_printf_specifier ('c', vec_printf_d, vec_ais);

  /* Indicate our interest in fp types */
  register_printf_specifier ('f', vec_printf_f, vec_ais);
  register_printf_specifier ('F', vec_printf_f, vec_ais);
  register_printf_specifier ('e', vec_printf_f, vec_ais);
  register_printf_specifier ('E', vec_printf_f, vec_ais);
  register_printf_specifier ('g', vec_printf_f, vec_ais);
  register_printf_specifier ('G', vec_printf_f, vec_ais);
  register_printf_specifier ('a', vec_printf_f, vec_ais);
  register_printf_specifier ('A', vec_printf_f, vec_ais);

  return 0;
}

void __attribute__ ((constructor)) __attribute__ ((visibility ("hidden") ))
__libvecpf_init (void)
{
  __register_printf_vec ();
}

