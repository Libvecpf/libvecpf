/* This is part of libvecpf, the Vector Printf Library.

   Author(s): Michael Brutman <brutman@us.ibm.com>
              Ryan S. Arnold <rsa@linux.vnet.ibm.com>

   Copyright (c) 2010, 2011, IBM Corporation
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

#include <altivec.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <locale.h>

#include "config.h"
#include "vecpf.h"

/* What to test:

   The Altivec Programming Interface Manual extends printf in the following
   manner:

     New separator chars (used like flags):
       ',', ';', ':', '_'

       The default separator is a space unless the 'c' conversion is
       being used.  If 'c' is being used the default separator character
       is a null.  Only one separator character may be specified.

     New size modifiers:
       'vll', 'vl', 'vh', 'llv', 'lv', 'hv', 'v'

     Valid modifiers and conversions (all else are undefined):

         vll or llv: long integer conversions; vectors are composed of eight byte vals
         vl or lv: integer conversions; vectors are composed of four byte vals
         vh or hv: short integer conversions; vectors are composed of two byte vals
         v: integer conversions; vectors are composed of 1 byte vals
         v: float conversions; vectors are composed of 4 byte vals

   Deviations from the PIM

     We only support the default separator; as things are today we can't
     add flags to printf to support the other separators.

     We add a new modifier 'vv' to support vector double for VSX.

   Conversion chars to test:

   %d, %i - print an integer as a signed decimal number
   %o     - print an integer as an unsigned octal number
   %u     - print an integer as an unsigned decimal number
   %x, %X - print an integer as an unsigned hexadecimal number

   %f     - print a fp number in fixed-point notation
   %e, %E - print a fp number in exponential notation
   %g, %G - print a fp number in either normal or exponential notation
   %a, %A - print a fp number in hexadecimal fractional notation

   %c     - print a single char

   Notes for flag characters:

     '-': Left justify (instead of right justify)
     '+': for %d and %i print a plus sign if the value is positive
     ' ': For %d and %i if the result doesn't start with a plus or a minus
          sign then prefix it with a space.  (Ignored when used with '+'.)
     '#': For %o force the leading digit to be a 0.
          For %x or %X prefix with '0x' or '0X'
     ''': Separate digits into groups specified by the locale.
     '0': Pad with zeros instead of spaces.  Ignored if used with '-' or
          a precision is specified.
*/

#define to_string(type, format, data, output) \
  for (i = 0; i < 16/sizeof(type); i++) \
    { \
        output += sprintf (output, format, (*(vector type*) data)[i]); \
        strcat (output, " "); \
        output++; \
    } \
  *(output - 1) = 0;

#define test( format, id, val ) \
  for (ptr = format; ptr->format1; ptr++) \
    { \
      gen_cmp_str (id, &val, ptr->format1, expected_output); \
      sprintf (actual_output, ptr->format2, val); \
      compare (ptr->src_line, expected_output, actual_output); \
      if (ptr->format3) { \
        sprintf (actual_output, ptr->format3, val); \
        compare (ptr->src_line, expected_output, actual_output); \
      } \
    }


vector unsigned int UINT32_TEST_VECTOR = { 4294967295U, 0, 39, 2147483647 };
vector signed int INT32_TEST_VECTOR = { (-INT_MAX - 1), 0, 39, 2147483647 };
vector unsigned short UINT16_TEST_VECTOR = {65535U, 0, 39, 42, 101, 16384, 32767, 32768 };
vector signed short INT16_TEST_VECTOR = { (-SHRT_MAX -1), -127, -1, 0, 127, 256, 16384, SHRT_MAX };
vector float FLOAT_TEST_VECTOR = { -(11.0f/9.0f), 0.123456789f, 42.0f, 9876543210.123456789f };
vector signed char SIGNED_CHAR_TEST_VECTOR = { -128, -120, -99, -61, -43, -38, -1, 0, 1, 19, 76, 85, 10l, 123, 126, 127 };
vector unsigned char UNSIGNED_CHAR_TEST_VECTOR = { 't', 'h', 'i', 's', ' ', 's', 'p', 'a', 'c', 'e', ' ', 0, 15, 127, 128, 255  };
vector char CHAR_TEST_VECTOR = { 't', 'h', 'i', 's', ' ', 's', 'p', 'a', 'c', 'e', ' ', 'i', 's', ' ', 'f', 'o'  };

#ifdef __VSX__
vector double DOUBLE_TEST_VECTOR = { -(11.0f/9.0f), 9876543210.123456789f };
vector unsigned long long UINT64_TEST_VECTOR = { -1, 0x1ABCDE0123456789 };
vector long long INT64_TEST_VECTOR = { LONG_MIN, LONG_MAX };
vector unsigned long UINT64_TEST_VECTOR_2 = { 0x1ABCDE0123456789, -1 };
vector long INT64_TEST_VECTOR_2 = { LONG_MAX, LONG_MIN };
#endif

#ifdef HAVE_INT128_T
vector __int128_t INT128_TEST_VECTOR  = (vector __int128_t) { ((((__int128_t)-0x0123456789abcdefUL << 64)) + ((__int128_t)0xfedcba9876543210UL))};
#endif

int test_count = 0, failed = 0, verbose = 0;
char expected_output[1024], actual_output[1024];

/* Tests are organized by data type.  Try to print a vector using a variety
   of different flags, precisions and conversions. */
typedef struct {
  int src_line;                /* What line of code is the test on? */
  const char *format1;         /* Format string to use for standard scalars */
  const char *format2;         /* Format string to use for vectors */
  const char *format3;         /* Alternate form, if applicable */
} format_specifiers;

format_specifiers uint32_tests[] =
{
  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%u",  "%vlu", "%lvu" },
  { __LINE__, "%-u", "%-vlu", "%-lvu" },
  { __LINE__, "%+u", "%+vlu", "%+lvu" },
  { __LINE__, "% u", "% vlu", "% lvu" },
  { __LINE__, "%#u", "%#vlu", "%#lvu" },
  { __LINE__, "%'u", "%'vlu", "%'lvu" },
  { __LINE__, "%0u", "%0vlu", "%0lvu" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+u", "%-+vlu", "%-+lvu" },
  { __LINE__, "%- u", "%- vlu", "%- lvu" },
  { __LINE__, "%-#u", "%-#vlu", "%-#lvu" },
  { __LINE__, "%-'u", "%-'vlu", "%-'lvu" },
  { __LINE__, "%-0u", "%-0vlu", "%-0lvu" },
  { __LINE__, "%+ u", "%+ vlu", "%+ lvu" },
  { __LINE__, "%+#u", "%+#vlu", "%+#lvu" },
  { __LINE__, "%+'u", "%+'vlu", "%+'lvu" },
  { __LINE__, "%+0u", "%+0vlu", "%+0lvu" },
  { __LINE__, "% #u", "% #vlu", "% #lvu" },
  { __LINE__, "% 'u", "% 'vlu", "% 'lvu" },
  { __LINE__, "% 0u", "% 0vlu", "% 0lvu" },
  { __LINE__, "%#'u", "%#'vlu", "%#'lvu" },
  { __LINE__, "%#0u", "%#0vlu", "%#0lvu" },
  { __LINE__, "%'0u", "%'0vlu", "%'0lvu" },

  /* Basic flags with precision. */
  { __LINE__, "%.5u",  "%.5vlu", "%.5lvu" },
  { __LINE__, "%-.5u", "%-.5vlu", "%-.5lvu" },
  { __LINE__, "%+.5u", "%+.5vlu", "%+.5lvu" },
  { __LINE__, "% .5u", "% .5vlu", "% .5lvu" },
  { __LINE__, "%#.5u", "%#.5vlu", "%#.5lvu" },
  { __LINE__, "%'.5u", "%'.5vlu", "%'.5lvu" },
  { __LINE__, "%0.5u", "%0.5vlu", "%0.5lvu" },

  /* Basic flags with field width. */
  { __LINE__, "%12u",  "%12vlu", "%12lvu" },
  { __LINE__, "%-12u", "%-12vlu", "%-12lvu" },
  { __LINE__, "%+12u", "%+12vlu", "%+12lvu" },
  { __LINE__, "% 12u", "% 12vlu", "% 12lvu" },
  { __LINE__, "%#12u", "%#12vlu", "%#12lvu" },
  { __LINE__, "%'12u", "%'12vlu", "%'12lvu" },
  { __LINE__, "%012u", "%012vlu", "%012lvu" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%15.7u",  "%15.7vlu", "%15.7lvu" },
  { __LINE__, "%-15.7u", "%-15.7vlu", "%-15.7lvu" },
  { __LINE__, "%+15.7u", "%+15.7vlu", "%+15.7lvu" },
  { __LINE__, "% 15.7u", "% 15.7vlu", "% 15.7lvu" },
  { __LINE__, "%#15.7u", "%#15.7vlu", "%#15.7lvu" },
  { __LINE__, "%'15.7u", "%'15.7vlu", "%'15.7lvu" },
  { __LINE__, "%015.7u", "%015.7vlu", "%015.7lvu" },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%o",  "%vlo", "%lvo" },
  { __LINE__, "%-o", "%-vlo", "%-lvo" },
  { __LINE__, "%+o", "%+vlo", "%+lvo" },
  { __LINE__, "% o", "% vlo", "% lvo" },
  { __LINE__, "%#o", "%#vlo", "%#lvo" },
  { __LINE__, "%'o", "%'vlo", "%'lvo" },
  { __LINE__, "%0o", "%0vlo", "%0lvo" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+o", "%-+vlo", "%-+lvo" },
  { __LINE__, "%- o", "%- vlo", "%- lvo" },
  { __LINE__, "%-#o", "%-#vlo", "%-#lvo" },
  { __LINE__, "%-'o", "%-'vlo", "%-'lvo" },
  { __LINE__, "%-0o", "%-0vlo", "%-0lvo" },
  { __LINE__, "%+ o", "%+ vlo", "%+ lvo" },
  { __LINE__, "%+#o", "%+#vlo", "%+#lvo" },
  { __LINE__, "%+'o", "%+'vlo", "%+'lvo" },
  { __LINE__, "%+0o", "%+0vlo", "%+0lvo" },
  { __LINE__, "% #o", "% #vlo", "% #lvo" },
  { __LINE__, "% 'o", "% 'vlo", "% 'lvo" },
  { __LINE__, "% 0o", "% 0vlo", "% 0lvo" },
  { __LINE__, "%#'o", "%#'vlo", "%#'lvo" },
  { __LINE__, "%#0o", "%#0vlo", "%#0lvo" },
  { __LINE__, "%'0o", "%'0vlo", "%'0lvo" },

  /* Basic flags with precision. */
  { __LINE__, "%.5o",  "%.5vlo", "%.5lvo" },
  { __LINE__, "%-.5o", "%-.5vlo", "%-.5lvo" },
  { __LINE__, "%+.5o", "%+.5vlo", "%+.5lvo" },
  { __LINE__, "% .5o", "% .5vlo", "% .5lvo" },
  { __LINE__, "%#.5o", "%#.5vlo", "%#.5lvo" },
  { __LINE__, "%'.5o", "%'.5vlo", "%'.5lvo" },
  { __LINE__, "%0.5o", "%0.5vlo", "%0.5lvo" },

  /* Basic flags with field width. */
  { __LINE__, "%12o",  "%12vlo", "%12lvo" },
  { __LINE__, "%-12o", "%-12vlo", "%-12lvo" },
  { __LINE__, "%+12o", "%+12vlo", "%+12lvo" },
  { __LINE__, "% 12o", "% 12vlo", "% 12lvo" },
  { __LINE__, "%#12o", "%#12vlo", "%#12lvo" },
  { __LINE__, "%'12o", "%'12vlo", "%'12lvo" },
  { __LINE__, "%012o", "%012vlo", "%012lvo" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%15.7o",  "%15.7vlo", "%15.7lvo" },
  { __LINE__, "%-15.7o", "%-15.7vlo", "%-15.7lvo" },
  { __LINE__, "%+15.7o", "%+15.7vlo", "%+15.7lvo" },
  { __LINE__, "% 15.7o", "% 15.7vlo", "% 15.7lvo" },
  { __LINE__, "%#15.7o", "%#15.7vlo", "%#15.7lvo" },
  { __LINE__, "%'15.7o", "%'15.7vlo", "%'15.7lvo" },
  { __LINE__, "%015.7o", "%015.7vlo", "%015.7lvo" },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%x",  "%vlx", "%lvx" },
  { __LINE__, "%-x", "%-vlx", "%-lvx" },
  { __LINE__, "%+x", "%+vlx", "%+lvx" },
  { __LINE__, "% x", "% vlx", "% lvx" },
  { __LINE__, "%#x", "%#vlx", "%#lvx" },
  { __LINE__, "%'x", "%'vlx", "%'lvx" },
  { __LINE__, "%0x", "%0vlx", "%0lvx" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+x", "%-+vlx", "%-+lvx" },
  { __LINE__, "%- x", "%- vlx", "%- lvx" },
  { __LINE__, "%-#x", "%-#vlx", "%-#lvx" },
  { __LINE__, "%-'x", "%-'vlx", "%-'lvx" },
  { __LINE__, "%-0x", "%-0vlx", "%-0lvx" },
  { __LINE__, "%+ x", "%+ vlx", "%+ lvx" },
  { __LINE__, "%+#x", "%+#vlx", "%+#lvx" },
  { __LINE__, "%+'x", "%+'vlx", "%+'lvx" },
  { __LINE__, "%+0x", "%+0vlx", "%+0lvx" },
  { __LINE__, "% #x", "% #vlx", "% #lvx" },
  { __LINE__, "% 'x", "% 'vlx", "% 'lvx" },
  { __LINE__, "% 0x", "% 0vlx", "% 0lvx" },
  { __LINE__, "%#'x", "%#'vlx", "%#'lvx" },
  { __LINE__, "%#0x", "%#0vlx", "%#0lvx" },
  { __LINE__, "%'0x", "%'0vlx", "%'0lvx" },

  /* Basic flags with precision. */
  { __LINE__, "%.5x",  "%.5vlx", "%.5lvx" },
  { __LINE__, "%-.5x", "%-.5vlx", "%-.5lvx" },
  { __LINE__, "%+.5x", "%+.5vlx", "%+.5lvx" },
  { __LINE__, "% .5x", "% .5vlx", "% .5lvx" },
  { __LINE__, "%#.5x", "%#.5vlx", "%#.5lvx" },
  { __LINE__, "%'.5x", "%'.5vlx", "%'.5lvx" },
  { __LINE__, "%0.5x", "%0.5vlx", "%0.5lvx" },

  /* Basic flags with field width. */
  { __LINE__, "%12x",  "%12vlx", "%12lvx" },
  { __LINE__, "%-12x", "%-12vlx", "%-12lvx" },
  { __LINE__, "%+12x", "%+12vlx", "%+12lvx" },
  { __LINE__, "% 12x", "% 12vlx", "% 12lvx" },
  { __LINE__, "%#12x", "%#12vlx", "%#12lvx" },
  { __LINE__, "%'12x", "%'12vlx", "%'12lvx" },
  { __LINE__, "%012x", "%012vlx", "%012lvx" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%15.7x",  "%15.7vlx", "%15.7lvx" },
  { __LINE__, "%-15.7x", "%-15.7vlx", "%-15.7lvx" },
  { __LINE__, "%+15.7x", "%+15.7vlx", "%+15.7lvx" },
  { __LINE__, "% 15.7x", "% 15.7vlx", "% 15.7lvx" },
  { __LINE__, "%#15.7x", "%#15.7vlx", "%#15.7lvx" },
  { __LINE__, "%'15.7x", "%'15.7vlx", "%'15.7lvx" },
  { __LINE__, "%015.7x", "%015.7vlx", "%015.7lvx" },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%X",  "%vlX", "%lvX" },
  { __LINE__, "%-X", "%-vlX", "%-lvX" },
  { __LINE__, "%+X", "%+vlX", "%+lvX" },
  { __LINE__, "% X", "% vlX", "% lvX" },
  { __LINE__, "%#X", "%#vlX", "%#lvX" },
  { __LINE__, "%'X", "%'vlX", "%'lvX" },
  { __LINE__, "%0X", "%0vlX", "%0lvX" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+X", "%-+vlX", "%-+lvX" },
  { __LINE__, "%- X", "%- vlX", "%- lvX" },
  { __LINE__, "%-#X", "%-#vlX", "%-#lvX" },
  { __LINE__, "%-'X", "%-'vlX", "%-'lvX" },
  { __LINE__, "%-0X", "%-0vlX", "%-0lvX" },
  { __LINE__, "%+ X", "%+ vlX", "%+ lvX" },
  { __LINE__, "%+#X", "%+#vlX", "%+#lvX" },
  { __LINE__, "%+'X", "%+'vlX", "%+'lvX" },
  { __LINE__, "%+0X", "%+0vlX", "%+0lvX" },
  { __LINE__, "% #X", "% #vlX", "% #lvX" },
  { __LINE__, "% 'X", "% 'vlX", "% 'lvX" },
  { __LINE__, "% 0X", "% 0vlX", "% 0lvX" },
  { __LINE__, "%#'X", "%#'vlX", "%#'lvX" },
  { __LINE__, "%#0X", "%#0vlX", "%#0lvX" },
  { __LINE__, "%'0X", "%'0vlX", "%'0lvX" },

  /* Basic flags with precision. */
  { __LINE__, "%.5X",  "%.5vlX", "%.5lvX" },
  { __LINE__, "%-.5X", "%-.5vlX", "%-.5lvX" },
  { __LINE__, "%+.5X", "%+.5vlX", "%+.5lvX" },
  { __LINE__, "% .5X", "% .5vlX", "% .5lvX" },
  { __LINE__, "%#.5X", "%#.5vlX", "%#.5lvX" },
  { __LINE__, "%'.5X", "%'.5vlX", "%'.5lvX" },
  { __LINE__, "%0.5X", "%0.5vlX", "%0.5lvX" },

  /* Basic flags with field width. */
  { __LINE__, "%12X",  "%12vlX", "%12lvX" },
  { __LINE__, "%-12X", "%-12vlX", "%-12lvX" },
  { __LINE__, "%+12X", "%+12vlX", "%+12lvX" },
  { __LINE__, "% 12X", "% 12vlX", "% 12lvX" },
  { __LINE__, "%#12X", "%#12vlX", "%#12lvX" },
  { __LINE__, "%'12X", "%'12vlX", "%'12lvX" },
  { __LINE__, "%012X", "%012vlX", "%012lvX" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%15.7X",  "%15.7vlX", "%15.7lvX" },
  { __LINE__, "%-15.7X", "%-15.7vlX", "%-15.7lvX" },
  { __LINE__, "%+15.7X", "%+15.7vlX", "%+15.7lvX" },
  { __LINE__, "% 15.7X", "% 15.7vlX", "% 15.7lvX" },
  { __LINE__, "%#15.7X", "%#15.7vlX", "%#15.7lvX" },
  { __LINE__, "%'15.7X", "%'15.7vlX", "%'15.7lvX" },
  { __LINE__, "%015.7X", "%015.7vlX", "%015.7lvX" },

  { 0, NULL, NULL, NULL }
};


format_specifiers int32_tests[] =
{
  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%d",  "%vld",  "%lvd" },
  { __LINE__, "%-d", "%-vld", "%-lvd" },
  { __LINE__, "%+d", "%+vld", "%+lvd" },
  { __LINE__, "% d", "% vld", "% lvd" },
  { __LINE__, "%#d", "%#vld", "%#lvd" },
  { __LINE__, "%'d", "%'vld", "%'lvd" },
  { __LINE__, "%0d", "%0vld", "%0lvd" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+d", "%-+vld", "%-+lvd" },
  { __LINE__, "%- d", "%- vld", "%- lvd" },
  { __LINE__, "%-#d", "%-#vld", "%-#lvd" },
  { __LINE__, "%-'d", "%-'vld", "%-'lvd" },
  { __LINE__, "%-0d", "%-0vld", "%-0lvd" },
  { __LINE__, "%+ d", "%+ vld", "%+ lvd" },
  { __LINE__, "%+#d", "%+#vld", "%+#lvd" },
  { __LINE__, "%+'d", "%+'vld", "%+'lvd" },
  { __LINE__, "%+0d", "%+0vld", "%+0lvd" },
  { __LINE__, "% #d", "% #vld", "% #lvd" },
  { __LINE__, "% 'd", "% 'vld", "% 'lvd" },
  { __LINE__, "% 0d", "% 0vld", "% 0lvd" },
  { __LINE__, "%#'d", "%#'vld", "%#'lvd" },
  { __LINE__, "%#0d", "%#0vld", "%#0lvd" },
  { __LINE__, "%'0d", "%'0vld", "%'0lvd" },

  /* Basic flags with precision. */
  { __LINE__, "%.5d",  "%.5vld", "%.5lvd" },
  { __LINE__, "%-.5d", "%-.5vld", "%-.5lvd" },
  { __LINE__, "%+.5d", "%+.5vld", "%+.5lvd" },
  { __LINE__, "% .5d", "% .5vld", "% .5lvd" },
  { __LINE__, "%#.5d", "%#.5vld", "%#.5lvd" },
  { __LINE__, "%'.5d", "%'.5vld", "%'.5lvd" },
  { __LINE__, "%0.5d", "%0.5vld", "%0.5lvd" },

  /* Basic flags with field width. */
  { __LINE__, "%12d",  "%12vld", "%12lvd" },
  { __LINE__, "%-12d", "%-12vld", "%-12lvd" },
  { __LINE__, "%+12d", "%+12vld", "%+12lvd" },
  { __LINE__, "% 12d", "% 12vld", "% 12lvd" },
  { __LINE__, "%#12d", "%#12vld", "%#12lvd" },
  { __LINE__, "%'12d", "%'12vld", "%'12lvd" },
  { __LINE__, "%012d", "%012vld", "%012lvd" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%15.7d",  "%15.7vld", "%15.7lvd" },
  { __LINE__, "%-15.7d", "%-15.7vld", "%-15.7lvd" },
  { __LINE__, "%+15.7d", "%+15.7vld", "%+15.7lvd" },
  { __LINE__, "% 15.7d", "% 15.7vld", "% 15.7lvd" },
  { __LINE__, "%#15.7d", "%#15.7vld", "%#15.7lvd" },
  { __LINE__, "%'15.7d", "%'15.7vld", "%'15.7lvd" },
  { __LINE__, "%015.7d", "%015.7vld", "%015.7lvd" },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%i",  "%vli",  "%lvi" },
  { __LINE__, "%-i", "%-vli", "%-lvi" },
  { __LINE__, "%+i", "%+vli", "%+lvi" },
  { __LINE__, "% i", "% vli", "% lvi" },
  { __LINE__, "%#i", "%#vli", "%#lvi" },
  { __LINE__, "%'i", "%'vli", "%'lvi" },
  { __LINE__, "%0i", "%0vli", "%0lvi" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+i", "%-+vli", "%-+lvi" },
  { __LINE__, "%- i", "%- vli", "%- lvi" },
  { __LINE__, "%-#i", "%-#vli", "%-#lvi" },
  { __LINE__, "%-'i", "%-'vli", "%-'lvi" },
  { __LINE__, "%-0i", "%-0vli", "%-0lvi" },
  { __LINE__, "%+ i", "%+ vli", "%+ lvi" },
  { __LINE__, "%+#i", "%+#vli", "%+#lvi" },
  { __LINE__, "%+'i", "%+'vli", "%+'lvi" },
  { __LINE__, "%+0i", "%+0vli", "%+0lvi" },
  { __LINE__, "% #i", "% #vli", "% #lvi" },
  { __LINE__, "% 'i", "% 'vli", "% 'lvi" },
  { __LINE__, "% 0i", "% 0vli", "% 0lvi" },
  { __LINE__, "%#'i", "%#'vli", "%#'lvi" },
  { __LINE__, "%#0i", "%#0vli", "%#0lvi" },
  { __LINE__, "%'0i", "%'0vli", "%'0lvi" },

  /* Basic flags with precision. */
  { __LINE__, "%.5i",  "%.5vli", "%.5lvi" },
  { __LINE__, "%-.5i", "%-.5vli", "%-.5lvi" },
  { __LINE__, "%+.5i", "%+.5vli", "%+.5lvi" },
  { __LINE__, "% .5i", "% .5vli", "% .5lvi" },
  { __LINE__, "%#.5i", "%#.5vli", "%#.5lvi" },
  { __LINE__, "%'.5i", "%'.5vli", "%'.5lvi" },
  { __LINE__, "%0.5i", "%0.5vli", "%0.5lvi" },

  /* Basic flags with field width. */
  { __LINE__, "%12i",  "%12vli", "%12lvi" },
  { __LINE__, "%-12i", "%-12vli", "%-12lvi" },
  { __LINE__, "%+12i", "%+12vli", "%+12lvi" },
  { __LINE__, "% 12i", "% 12vli", "% 12lvi" },
  { __LINE__, "%#12i", "%#12vli", "%#12lvi" },
  { __LINE__, "%'12i", "%'12vli", "%'12lvi" },
  { __LINE__, "%012i", "%012vli", "%012lvi" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%15.7i",  "%15.7vli", "%15.7lvi" },
  { __LINE__, "%-15.7i", "%-15.7vli", "%-15.7lvi" },
  { __LINE__, "%+15.7i", "%+15.7vli", "%+15.7lvi" },
  { __LINE__, "% 15.7i", "% 15.7vli", "% 15.7lvi" },
  { __LINE__, "%#15.7i", "%#15.7vli", "%#15.7lvi" },
  { __LINE__, "%'15.7i", "%'15.7vli", "%'15.7lvi" },
  { __LINE__, "%015.7i", "%015.7vli", "%015.7lvi" },

  { 0, NULL, NULL, NULL }
};


format_specifiers uint16_tests[] =
{
  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%u",  "%vhu", "%hvu" },
  { __LINE__, "%-u", "%-vhu", "%-hvu" },
  { __LINE__, "%+u", "%+vhu", "%+hvu" },
  { __LINE__, "% u", "% vhu", "% hvu" },
  { __LINE__, "%#u", "%#vhu", "%#hvu" },
  { __LINE__, "%'u", "%'vhu", "%'hvu" },
  { __LINE__, "%0u", "%0vhu", "%0hvu" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+u", "%-+vhu", "%-+hvu" },
  { __LINE__, "%- u", "%- vhu", "%- hvu" },
  { __LINE__, "%-#u", "%-#vhu", "%-#hvu" },
  { __LINE__, "%-'u", "%-'vhu", "%-'hvu" },
  { __LINE__, "%-0u", "%-0vhu", "%-0hvu" },
  { __LINE__, "%+ u", "%+ vhu", "%+ hvu" },
  { __LINE__, "%+#u", "%+#vhu", "%+#hvu" },
  { __LINE__, "%+'u", "%+'vhu", "%+'hvu" },
  { __LINE__, "%+0u", "%+0vhu", "%+0hvu" },
  { __LINE__, "% #u", "% #vhu", "% #hvu" },
  { __LINE__, "% 'u", "% 'vhu", "% 'hvu" },
  { __LINE__, "% 0u", "% 0vhu", "% 0hvu" },
  { __LINE__, "%#'u", "%#'vhu", "%#'hvu" },
  { __LINE__, "%#0u", "%#0vhu", "%#0hvu" },
  { __LINE__, "%'0u", "%'0vhu", "%'0hvu" },

  /* Basic flags with precision. */
  { __LINE__, "%.5u",  "%.5vhu", "%.5hvu" },
  { __LINE__, "%-.5u", "%-.5vhu", "%-.5hvu" },
  { __LINE__, "%+.5u", "%+.5vhu", "%+.5hvu" },
  { __LINE__, "% .5u", "% .5vhu", "% .5hvu" },
  { __LINE__, "%#.5u", "%#.5vhu", "%#.5hvu" },
  { __LINE__, "%'.5u", "%'.5vhu", "%'.5hvu" },
  { __LINE__, "%0.5u", "%0.5vhu", "%0.5hvu" },

  /* Basic flags with field width. */
  { __LINE__, "%12u",  "%12vhu", "%12hvu" },
  { __LINE__, "%-12u", "%-12vhu", "%-12hvu" },
  { __LINE__, "%+12u", "%+12vhu", "%+12hvu" },
  { __LINE__, "% 12u", "% 12vhu", "% 12hvu" },
  { __LINE__, "%#12u", "%#12vhu", "%#12hvu" },
  { __LINE__, "%'12u", "%'12vhu", "%'12hvu" },
  { __LINE__, "%012u", "%012vhu", "%012hvu" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%15.7u",  "%15.7vhu", "%15.7hvu" },
  { __LINE__, "%-15.7u", "%-15.7vhu", "%-15.7hvu" },
  { __LINE__, "%+15.7u", "%+15.7vhu", "%+15.7hvu" },
  { __LINE__, "% 15.7u", "% 15.7vhu", "% 15.7hvu" },
  { __LINE__, "%#15.7u", "%#15.7vhu", "%#15.7hvu" },
  { __LINE__, "%'15.7u", "%'15.7vhu", "%'15.7hvu" },
  { __LINE__, "%015.7u", "%015.7vhu", "%015.7hvu" },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%o",  "%vho", "%hvo" },
  { __LINE__, "%-o", "%-vho", "%-hvo" },
  { __LINE__, "%+o", "%+vho", "%+hvo" },
  { __LINE__, "% o", "% vho", "% hvo" },
  { __LINE__, "%#o", "%#vho", "%#hvo" },
  { __LINE__, "%'o", "%'vho", "%'hvo" },
  { __LINE__, "%0o", "%0vho", "%0hvo" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+o", "%-+vho", "%-+hvo" },
  { __LINE__, "%- o", "%- vho", "%- hvo" },
  { __LINE__, "%-#o", "%-#vho", "%-#hvo" },
  { __LINE__, "%-'o", "%-'vho", "%-'hvo" },
  { __LINE__, "%-0o", "%-0vho", "%-0hvo" },
  { __LINE__, "%+ o", "%+ vho", "%+ hvo" },
  { __LINE__, "%+#o", "%+#vho", "%+#hvo" },
  { __LINE__, "%+'o", "%+'vho", "%+'hvo" },
  { __LINE__, "%+0o", "%+0vho", "%+0hvo" },
  { __LINE__, "% #o", "% #vho", "% #hvo" },
  { __LINE__, "% 'o", "% 'vho", "% 'hvo" },
  { __LINE__, "% 0o", "% 0vho", "% 0hvo" },
  { __LINE__, "%#'o", "%#'vho", "%#'hvo" },
  { __LINE__, "%#0o", "%#0vho", "%#0hvo" },
  { __LINE__, "%'0o", "%'0vho", "%'0hvo" },

  /* Basic flags with precision. */
  { __LINE__, "%.5o",  "%.5vho", "%.5hvo" },
  { __LINE__, "%-.5o", "%-.5vho", "%-.5hvo" },
  { __LINE__, "%+.5o", "%+.5vho", "%+.5hvo" },
  { __LINE__, "% .5o", "% .5vho", "% .5hvo" },
  { __LINE__, "%#.5o", "%#.5vho", "%#.5hvo" },
  { __LINE__, "%'.5o", "%'.5vho", "%'.5hvo" },
  { __LINE__, "%0.5o", "%0.5vho", "%0.5hvo" },

  /* Basic flags with field width. */
  { __LINE__, "%12o",  "%12vho", "%12hvo" },
  { __LINE__, "%-12o", "%-12vho", "%-12hvo" },
  { __LINE__, "%+12o", "%+12vho", "%+12hvo" },
  { __LINE__, "% 12o", "% 12vho", "% 12hvo" },
  { __LINE__, "%#12o", "%#12vho", "%#12hvo" },
  { __LINE__, "%'12o", "%'12vho", "%'12hvo" },
  { __LINE__, "%012o", "%012vho", "%012hvo" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%15.7o",  "%15.7vho", "%15.7hvo" },
  { __LINE__, "%-15.7o", "%-15.7vho", "%-15.7hvo" },
  { __LINE__, "%+15.7o", "%+15.7vho", "%+15.7hvo" },
  { __LINE__, "% 15.7o", "% 15.7vho", "% 15.7hvo" },
  { __LINE__, "%#15.7o", "%#15.7vho", "%#15.7hvo" },
  { __LINE__, "%'15.7o", "%'15.7vho", "%'15.7hvo" },
  { __LINE__, "%015.7o", "%015.7vho", "%015.7hvo" },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%x",  "%vhx", "%hvx" },
  { __LINE__, "%-x", "%-vhx", "%-hvx" },
  { __LINE__, "%+x", "%+vhx", "%+hvx" },
  { __LINE__, "% x", "% vhx", "% hvx" },
  { __LINE__, "%#x", "%#vhx", "%#hvx" },
  { __LINE__, "%'x", "%'vhx", "%'hvx" },
  { __LINE__, "%0x", "%0vhx", "%0hvx" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+x", "%-+vhx", "%-+hvx" },
  { __LINE__, "%- x", "%- vhx", "%- hvx" },
  { __LINE__, "%-#x", "%-#vhx", "%-#hvx" },
  { __LINE__, "%-'x", "%-'vhx", "%-'hvx" },
  { __LINE__, "%-0x", "%-0vhx", "%-0hvx" },
  { __LINE__, "%+ x", "%+ vhx", "%+ hvx" },
  { __LINE__, "%+#x", "%+#vhx", "%+#hvx" },
  { __LINE__, "%+'x", "%+'vhx", "%+'hvx" },
  { __LINE__, "%+0x", "%+0vhx", "%+0hvx" },
  { __LINE__, "% #x", "% #vhx", "% #hvx" },
  { __LINE__, "% 'x", "% 'vhx", "% 'hvx" },
  { __LINE__, "% 0x", "% 0vhx", "% 0hvx" },
  { __LINE__, "%#'x", "%#'vhx", "%#'hvx" },
  { __LINE__, "%#0x", "%#0vhx", "%#0hvx" },
  { __LINE__, "%'0x", "%'0vhx", "%'0hvx" },

  /* Basic flags with precision. */
  { __LINE__, "%.5x",  "%.5vhx", "%.5hvx" },
  { __LINE__, "%-.5x", "%-.5vhx", "%-.5hvx" },
  { __LINE__, "%+.5x", "%+.5vhx", "%+.5hvx" },
  { __LINE__, "% .5x", "% .5vhx", "% .5hvx" },
  { __LINE__, "%#.5x", "%#.5vhx", "%#.5hvx" },
  { __LINE__, "%'.5x", "%'.5vhx", "%'.5hvx" },
  { __LINE__, "%0.5x", "%0.5vhx", "%0.5hvx" },

  /* Basic flags with field width. */
  { __LINE__, "%12x",  "%12vhx", "%12hvx" },
  { __LINE__, "%-12x", "%-12vhx", "%-12hvx" },
  { __LINE__, "%+12x", "%+12vhx", "%+12hvx" },
  { __LINE__, "% 12x", "% 12vhx", "% 12hvx" },
  { __LINE__, "%#12x", "%#12vhx", "%#12hvx" },
  { __LINE__, "%'12x", "%'12vhx", "%'12hvx" },
  { __LINE__, "%012x", "%012vhx", "%012hvx" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%15.7x",  "%15.7vhx", "%15.7hvx" },
  { __LINE__, "%-15.7x", "%-15.7vhx", "%-15.7hvx" },
  { __LINE__, "%+15.7x", "%+15.7vhx", "%+15.7hvx" },
  { __LINE__, "% 15.7x", "% 15.7vhx", "% 15.7hvx" },
  { __LINE__, "%#15.7x", "%#15.7vhx", "%#15.7hvx" },
  { __LINE__, "%'15.7x", "%'15.7vhx", "%'15.7hvx" },
  { __LINE__, "%015.7x", "%015.7vhx", "%015.7hvx" },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%X",  "%vhX", "%hvX" },
  { __LINE__, "%-X", "%-vhX", "%-hvX" },
  { __LINE__, "%+X", "%+vhX", "%+hvX" },
  { __LINE__, "% X", "% vhX", "% hvX" },
  { __LINE__, "%#X", "%#vhX", "%#hvX" },
  { __LINE__, "%'X", "%'vhX", "%'hvX" },
  { __LINE__, "%0X", "%0vhX", "%0hvX" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+X", "%-+vhX", "%-+hvX" },
  { __LINE__, "%- X", "%- vhX", "%- hvX" },
  { __LINE__, "%-#X", "%-#vhX", "%-#hvX" },
  { __LINE__, "%-'X", "%-'vhX", "%-'hvX" },
  { __LINE__, "%-0X", "%-0vhX", "%-0hvX" },
  { __LINE__, "%+ X", "%+ vhX", "%+ hvX" },
  { __LINE__, "%+#X", "%+#vhX", "%+#hvX" },
  { __LINE__, "%+'X", "%+'vhX", "%+'hvX" },
  { __LINE__, "%+0X", "%+0vhX", "%+0hvX" },
  { __LINE__, "% #X", "% #vhX", "% #hvX" },
  { __LINE__, "% 'X", "% 'vhX", "% 'hvX" },
  { __LINE__, "% 0X", "% 0vhX", "% 0hvX" },
  { __LINE__, "%#'X", "%#'vhX", "%#'hvX" },
  { __LINE__, "%#0X", "%#0vhX", "%#0hvX" },
  { __LINE__, "%'0X", "%'0vhX", "%'0hvX" },

  /* Basic flags with precision. */
  { __LINE__, "%.5X",  "%.5vhX", "%.5hvX" },
  { __LINE__, "%-.5X", "%-.5vhX", "%-.5hvX" },
  { __LINE__, "%+.5X", "%+.5vhX", "%+.5hvX" },
  { __LINE__, "% .5X", "% .5vhX", "% .5hvX" },
  { __LINE__, "%#.5X", "%#.5vhX", "%#.5hvX" },
  { __LINE__, "%'.5X", "%'.5vhX", "%'.5hvX" },
  { __LINE__, "%0.5X", "%0.5vhX", "%0.5hvX" },

  /* Basic flags with field width. */
  { __LINE__, "%12X",  "%12vhX", "%12hvX" },
  { __LINE__, "%-12X", "%-12vhX", "%-12hvX" },
  { __LINE__, "%+12X", "%+12vhX", "%+12hvX" },
  { __LINE__, "% 12X", "% 12vhX", "% 12hvX" },
  { __LINE__, "%#12X", "%#12vhX", "%#12hvX" },
  { __LINE__, "%'12X", "%'12vhX", "%'12hvX" },
  { __LINE__, "%012X", "%012vhX", "%012hvX" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%15.7X",  "%15.7vhX", "%15.7hvX" },
  { __LINE__, "%-15.7X", "%-15.7vhX", "%-15.7hvX" },
  { __LINE__, "%+15.7X", "%+15.7vhX", "%+15.7hvX" },
  { __LINE__, "% 15.7X", "% 15.7vhX", "% 15.7hvX" },
  { __LINE__, "%#15.7X", "%#15.7vhX", "%#15.7hvX" },
  { __LINE__, "%'15.7X", "%'15.7vhX", "%'15.7hvX" },
  { __LINE__, "%015.7X", "%015.7vhX", "%015.7hvX" },

  { 0, NULL, NULL, NULL }
};

format_specifiers int16_tests[] =
{
  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%d",  "%vhd",  "%hvd" },
  { __LINE__, "%-d", "%-vhd", "%-hvd" },
  { __LINE__, "%+d", "%+vhd", "%+hvd" },
  { __LINE__, "% d", "% vhd", "% hvd" },
  { __LINE__, "%#d", "%#vhd", "%#hvd" },
  { __LINE__, "%'d", "%'vhd", "%'hvd" },
  { __LINE__, "%0d", "%0vhd", "%0hvd" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+d", "%-+vhd", "%-+hvd" },
  { __LINE__, "%- d", "%- vhd", "%- hvd" },
  { __LINE__, "%-#d", "%-#vhd", "%-#hvd" },
  { __LINE__, "%-'d", "%-'vhd", "%-'hvd" },
  { __LINE__, "%-0d", "%-0vhd", "%-0hvd" },
  { __LINE__, "%+ d", "%+ vhd", "%+ hvd" },
  { __LINE__, "%+#d", "%+#vhd", "%+#hvd" },
  { __LINE__, "%+'d", "%+'vhd", "%+'hvd" },
  { __LINE__, "%+0d", "%+0vhd", "%+0hvd" },
  { __LINE__, "% #d", "% #vhd", "% #hvd" },
  { __LINE__, "% 'd", "% 'vhd", "% 'hvd" },
  { __LINE__, "% 0d", "% 0vhd", "% 0hvd" },
  { __LINE__, "%#'d", "%#'vhd", "%#'hvd" },
  { __LINE__, "%#0d", "%#0vhd", "%#0hvd" },
  { __LINE__, "%'0d", "%'0vhd", "%'0hvd" },

  /* Basic flags with precision. */
  { __LINE__, "%.5d",  "%.5vhd", "%.5hvd" },
  { __LINE__, "%-.5d", "%-.5vhd", "%-.5hvd" },
  { __LINE__, "%+.5d", "%+.5vhd", "%+.5hvd" },
  { __LINE__, "% .5d", "% .5vhd", "% .5hvd" },
  { __LINE__, "%#.5d", "%#.5vhd", "%#.5hvd" },
  { __LINE__, "%'.5d", "%'.5vhd", "%'.5hvd" },
  { __LINE__, "%0.5d", "%0.5vhd", "%0.5hvd" },

  /* Basic flags with field width. */
  { __LINE__, "%12d",  "%12vhd", "%12hvd" },
  { __LINE__, "%-12d", "%-12vhd", "%-12hvd" },
  { __LINE__, "%+12d", "%+12vhd", "%+12hvd" },
  { __LINE__, "% 12d", "% 12vhd", "% 12hvd" },
  { __LINE__, "%#12d", "%#12vhd", "%#12hvd" },
  { __LINE__, "%'12d", "%'12vhd", "%'12hvd" },
  { __LINE__, "%012d", "%012vhd", "%012hvd" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%15.7d",  "%15.7vhd", "%15.7hvd" },
  { __LINE__, "%-15.7d", "%-15.7vhd", "%-15.7hvd" },
  { __LINE__, "%+15.7d", "%+15.7vhd", "%+15.7hvd" },
  { __LINE__, "% 15.7d", "% 15.7vhd", "% 15.7hvd" },
  { __LINE__, "%#15.7d", "%#15.7vhd", "%#15.7hvd" },
  { __LINE__, "%'15.7d", "%'15.7vhd", "%'15.7hvd" },
  { __LINE__, "%015.7d", "%015.7vhd", "%015.7hvd" },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%i",  "%vhi",  "%hvi" },
  { __LINE__, "%-i", "%-vhi", "%-hvi" },
  { __LINE__, "%+i", "%+vhi", "%+hvi" },
  { __LINE__, "% i", "% vhi", "% hvi" },
  { __LINE__, "%#i", "%#vhi", "%#hvi" },
  { __LINE__, "%'i", "%'vhi", "%'hvi" },
  { __LINE__, "%0i", "%0vhi", "%0hvi" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+i", "%-+vhi", "%-+hvi" },
  { __LINE__, "%- i", "%- vhi", "%- hvi" },
  { __LINE__, "%-#i", "%-#vhi", "%-#hvi" },
  { __LINE__, "%-'i", "%-'vhi", "%-'hvi" },
  { __LINE__, "%-0i", "%-0vhi", "%-0hvi" },
  { __LINE__, "%+ i", "%+ vhi", "%+ hvi" },
  { __LINE__, "%+#i", "%+#vhi", "%+#hvi" },
  { __LINE__, "%+'i", "%+'vhi", "%+'hvi" },
  { __LINE__, "%+0i", "%+0vhi", "%+0hvi" },
  { __LINE__, "% #i", "% #vhi", "% #hvi" },
  { __LINE__, "% 'i", "% 'vhi", "% 'hvi" },
  { __LINE__, "% 0i", "% 0vhi", "% 0hvi" },
  { __LINE__, "%#'i", "%#'vhi", "%#'hvi" },
  { __LINE__, "%#0i", "%#0vhi", "%#0hvi" },
  { __LINE__, "%'0i", "%'0vhi", "%'0hvi" },

  /* Basic flags with precision. */
  { __LINE__, "%.5i",  "%.5vhi", "%.5hvi" },
  { __LINE__, "%-.5i", "%-.5vhi", "%-.5hvi" },
  { __LINE__, "%+.5i", "%+.5vhi", "%+.5hvi" },
  { __LINE__, "% .5i", "% .5vhi", "% .5hvi" },
  { __LINE__, "%#.5i", "%#.5vhi", "%#.5hvi" },
  { __LINE__, "%'.5i", "%'.5vhi", "%'.5hvi" },
  { __LINE__, "%0.5i", "%0.5vhi", "%0.5hvi" },

  /* Basic flags with field width. */
  { __LINE__, "%12i",  "%12vhi", "%12hvi" },
  { __LINE__, "%-12i", "%-12vhi", "%-12hvi" },
  { __LINE__, "%+12i", "%+12vhi", "%+12hvi" },
  { __LINE__, "% 12i", "% 12vhi", "% 12hvi" },
  { __LINE__, "%#12i", "%#12vhi", "%#12hvi" },
  { __LINE__, "%'12i", "%'12vhi", "%'12hvi" },
  { __LINE__, "%012i", "%012vhi", "%012hvi" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%15.7i",  "%15.7vhi", "%15.7hvi" },
  { __LINE__, "%-15.7i", "%-15.7vhi", "%-15.7hvi" },
  { __LINE__, "%+15.7i", "%+15.7vhi", "%+15.7hvi" },
  { __LINE__, "% 15.7i", "% 15.7vhi", "% 15.7hvi" },
  { __LINE__, "%#15.7i", "%#15.7vhi", "%#15.7hvi" },
  { __LINE__, "%'15.7i", "%'15.7vhi", "%'15.7hvi" },
  { __LINE__, "%015.7i", "%015.7vhi", "%015.7hvi" },

  { 0, NULL, NULL, NULL }
};

format_specifiers float_tests[] =
{
  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%f",  "%vf", NULL },
  { __LINE__, "%-f", "%-vf", NULL },
  { __LINE__, "%+f", "%+vf", NULL },
  { __LINE__, "% f", "% vf", NULL },
  { __LINE__, "%#f", "%#vf", NULL },
  { __LINE__, "%'f", "%'vf", NULL },
  { __LINE__, "%0f", "%0vf", NULL },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+f", "%-+vf", NULL },
  { __LINE__, "%- f", "%- vf", NULL },
  { __LINE__, "%-#f", "%-#vf", NULL },
  { __LINE__, "%-'f", "%-'vf", NULL },
  { __LINE__, "%-0f", "%-0vf", NULL },
  { __LINE__, "%+ f", "%+ vf", NULL },
  { __LINE__, "%+#f", "%+#vf", NULL },
  { __LINE__, "%+'f", "%+'vf", NULL },
  { __LINE__, "%+0f", "%+0vf", NULL },
  { __LINE__, "% #f", "% #vf", NULL },
  { __LINE__, "% 'f", "% 'vf", NULL },
  { __LINE__, "% 0f", "% 0vf", NULL },
  { __LINE__, "%#'f", "%#'vf", NULL },
  { __LINE__, "%#0f", "%#0vf", NULL },
  { __LINE__, "%'0f", "%'0vf", NULL },

  /* Basic flags with precision. */
  { __LINE__, "%.9f",  "%.9vf", NULL },
  { __LINE__, "%-.9f",  "%-.9vf", NULL },
  { __LINE__, "%+.9f",  "%+.9vf", NULL },
  { __LINE__, "% .9f",  "% .9vf", NULL },
  { __LINE__, "%#.9f",  "%#.9vf", NULL },
  { __LINE__, "%'.9f",  "%'.9vf", NULL },
  { __LINE__, "%0.9f",  "%0.9vf", NULL },

  /* Basic flags with field width. */
  { __LINE__, "%20f",  "%20vf", NULL },
  { __LINE__, "%-20f",  "%-20vf", NULL },
  { __LINE__, "%+20f",  "%+20vf", NULL },
  { __LINE__, "% 20f",  "% 20vf", NULL },
  { __LINE__, "%#20f",  "%#20vf", NULL },
  { __LINE__, "%'20f",  "%'20vf", NULL },
  { __LINE__, "%020f",  "%020vf", NULL },

  /* Basic flags with field width and precision. */
  { __LINE__, "%25.3f",  "%25.3vf", NULL },
  { __LINE__, "%-25.3f",  "%-25.3vf", NULL },
  { __LINE__, "%+25.3f",  "%+25.3vf", NULL },
  { __LINE__, "% 25.3f",  "% 25.3vf", NULL },
  { __LINE__, "%#25.3f",  "%#25.3vf", NULL },
  { __LINE__, "%'25.3f",  "%'25.3vf", NULL },
  { __LINE__, "%025.3f",  "%025.3vf", NULL },

  /* By this point the code that handles flags, field width and precision
     probably works.  Go for the other conversions on unsigned integers. */

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%e",  "%ve", NULL },
  { __LINE__, "%-e", "%-ve", NULL },
  { __LINE__, "%+e", "%+ve", NULL },
  { __LINE__, "% e", "% ve", NULL },
  { __LINE__, "%#e", "%#ve", NULL },
  { __LINE__, "%'e", "%'ve", NULL },
  { __LINE__, "%0e", "%0ve", NULL },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+e", "%-+ve", NULL },
  { __LINE__, "%- e", "%- ve", NULL },
  { __LINE__, "%-#e", "%-#ve", NULL },
  { __LINE__, "%-'e", "%-'ve", NULL },
  { __LINE__, "%-0e", "%-0ve", NULL },
  { __LINE__, "%+ e", "%+ ve", NULL },
  { __LINE__, "%+#e", "%+#ve", NULL },
  { __LINE__, "%+'e", "%+'ve", NULL },
  { __LINE__, "%+0e", "%+0ve", NULL },
  { __LINE__, "% #e", "% #ve", NULL },
  { __LINE__, "% 'e", "% 've", NULL },
  { __LINE__, "% 0e", "% 0ve", NULL },
  { __LINE__, "%#'e", "%#'ve", NULL },
  { __LINE__, "%#0e", "%#0ve", NULL },
  { __LINE__, "%'0e", "%'0ve", NULL },

  /* Basic flags with precision. */
  { __LINE__, "%.9e",  "%.9ve", NULL },
  { __LINE__, "%-.9e",  "%-.9ve", NULL },
  { __LINE__, "%+.9e",  "%+.9ve", NULL },
  { __LINE__, "% .9e",  "% .9ve", NULL },
  { __LINE__, "%#.9e",  "%#.9ve", NULL },
  { __LINE__, "%'.9e",  "%'.9ve", NULL },
  { __LINE__, "%0.9e",  "%0.9ve", NULL },

  /* Basic flags with field width. */
  { __LINE__, "%20e",  "%20ve", NULL },
  { __LINE__, "%-20e",  "%-20ve", NULL },
  { __LINE__, "%+20e",  "%+20ve", NULL },
  { __LINE__, "% 20e",  "% 20ve", NULL },
  { __LINE__, "%#20e",  "%#20ve", NULL },
  { __LINE__, "%'20e",  "%'20ve", NULL },
  { __LINE__, "%020e",  "%020ve", NULL },

  /* Basic flags with field width and precision. */
  { __LINE__, "%25.3e",  "%25.3ve", NULL },
  { __LINE__, "%-25.3e",  "%-25.3ve", NULL },
  { __LINE__, "%+25.3e",  "%+25.3ve", NULL },
  { __LINE__, "% 25.3e",  "% 25.3ve", NULL },
  { __LINE__, "%#25.3e",  "%#25.3ve", NULL },
  { __LINE__, "%'25.3e",  "%'25.3ve", NULL },
  { __LINE__, "%025.3e",  "%025.3ve", NULL },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%E",  "%vE", NULL },
  { __LINE__, "%-E", "%-vE", NULL },
  { __LINE__, "%+E", "%+vE", NULL },
  { __LINE__, "% E", "% vE", NULL },
  { __LINE__, "%#E", "%#vE", NULL },
  { __LINE__, "%'E", "%'vE", NULL },
  { __LINE__, "%0E", "%0vE", NULL },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+E", "%-+vE", NULL },
  { __LINE__, "%- E", "%- vE", NULL },
  { __LINE__, "%-#E", "%-#vE", NULL },
  { __LINE__, "%-'E", "%-'vE", NULL },
  { __LINE__, "%-0E", "%-0vE", NULL },
  { __LINE__, "%+ E", "%+ vE", NULL },
  { __LINE__, "%+#E", "%+#vE", NULL },
  { __LINE__, "%+'E", "%+'vE", NULL },
  { __LINE__, "%+0E", "%+0vE", NULL },
  { __LINE__, "% #E", "% #vE", NULL },
  { __LINE__, "% 'E", "% 'vE", NULL },
  { __LINE__, "% 0E", "% 0vE", NULL },
  { __LINE__, "%#'E", "%#'vE", NULL },
  { __LINE__, "%#0E", "%#0vE", NULL },
  { __LINE__, "%'0E", "%'0vE", NULL },

  /* Basic flags with precision. */
  { __LINE__, "%.9E",  "%.9vE", NULL },
  { __LINE__, "%-.9E",  "%-.9vE", NULL },
  { __LINE__, "%+.9E",  "%+.9vE", NULL },
  { __LINE__, "% .9E",  "% .9vE", NULL },
  { __LINE__, "%#.9E",  "%#.9vE", NULL },
  { __LINE__, "%'.9E",  "%'.9vE", NULL },
  { __LINE__, "%0.9E",  "%0.9vE", NULL },

  /* Basic flags with field width. */
  { __LINE__, "%20E",  "%20vE", NULL },
  { __LINE__, "%-20E",  "%-20vE", NULL },
  { __LINE__, "%+20E",  "%+20vE", NULL },
  { __LINE__, "% 20E",  "% 20vE", NULL },
  { __LINE__, "%#20E",  "%#20vE", NULL },
  { __LINE__, "%'20E",  "%'20vE", NULL },
  { __LINE__, "%020E",  "%020vE", NULL },

  /* Basic flags with field width and precision. */
  { __LINE__, "%25.3E",  "%25.3vE", NULL },
  { __LINE__, "%-25.3E",  "%-25.3vE", NULL },
  { __LINE__, "%+25.3E",  "%+25.3vE", NULL },
  { __LINE__, "% 25.3E",  "% 25.3vE", NULL },
  { __LINE__, "%#25.3E",  "%#25.3vE", NULL },
  { __LINE__, "%'25.3E",  "%'25.3vE", NULL },
  { __LINE__, "%025.3E",  "%025.3vE", NULL },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%g",  "%vg", NULL },
  { __LINE__, "%-g", "%-vg", NULL },
  { __LINE__, "%+g", "%+vg", NULL },
  { __LINE__, "% g", "% vg", NULL },
  { __LINE__, "%#g", "%#vg", NULL },
  { __LINE__, "%'g", "%'vg", NULL },
  { __LINE__, "%0g", "%0vg", NULL },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+g", "%-+vg", NULL },
  { __LINE__, "%- g", "%- vg", NULL },
  { __LINE__, "%-#g", "%-#vg", NULL },
  { __LINE__, "%-'g", "%-'vg", NULL },
  { __LINE__, "%-0g", "%-0vg", NULL },
  { __LINE__, "%+ g", "%+ vg", NULL },
  { __LINE__, "%+#g", "%+#vg", NULL },
  { __LINE__, "%+'g", "%+'vg", NULL },
  { __LINE__, "%+0g", "%+0vg", NULL },
  { __LINE__, "% #g", "% #vg", NULL },
  { __LINE__, "% 'g", "% 'vg", NULL },
  { __LINE__, "% 0g", "% 0vg", NULL },
  { __LINE__, "%#'g", "%#'vg", NULL },
  { __LINE__, "%#0g", "%#0vg", NULL },
  { __LINE__, "%'0g", "%'0vg", NULL },

  /* Basic flags with precision. */
  { __LINE__, "%.9g",  "%.9vg", NULL },
  { __LINE__, "%-.9g",  "%-.9vg", NULL },
  { __LINE__, "%+.9g",  "%+.9vg", NULL },
  { __LINE__, "% .9g",  "% .9vg", NULL },
  { __LINE__, "%#.9g",  "%#.9vg", NULL },
  { __LINE__, "%'.9g",  "%'.9vg", NULL },
  { __LINE__, "%0.9g",  "%0.9vg", NULL },

  /* Basic flags with field width. */
  { __LINE__, "%20g",  "%20vg", NULL },
  { __LINE__, "%-20g",  "%-20vg", NULL },
  { __LINE__, "%+20g",  "%+20vg", NULL },
  { __LINE__, "% 20g",  "% 20vg", NULL },
  { __LINE__, "%#20g",  "%#20vg", NULL },
  { __LINE__, "%'20g",  "%'20vg", NULL },
  { __LINE__, "%020g",  "%020vg", NULL },

  /* Basic flags with field width and precision. */
  { __LINE__, "%25.3g",  "%25.3vg", NULL },
  { __LINE__, "%-25.3g",  "%-25.3vg", NULL },
  { __LINE__, "%+25.3g",  "%+25.3vg", NULL },
  { __LINE__, "% 25.3g",  "% 25.3vg", NULL },
  { __LINE__, "%#25.3g",  "%#25.3vg", NULL },
  { __LINE__, "%'25.3g",  "%'25.3vg", NULL },
  { __LINE__, "%025.3g",  "%025.3vg", NULL },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%G",  "%vG", NULL },
  { __LINE__, "%-G", "%-vG", NULL },
  { __LINE__, "%+G", "%+vG", NULL },
  { __LINE__, "% G", "% vG", NULL },
  { __LINE__, "%#G", "%#vG", NULL },
  { __LINE__, "%'G", "%'vG", NULL },
  { __LINE__, "%0G", "%0vG", NULL },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+G", "%-+vG", NULL },
  { __LINE__, "%- G", "%- vG", NULL },
  { __LINE__, "%-#G", "%-#vG", NULL },
  { __LINE__, "%-'G", "%-'vG", NULL },
  { __LINE__, "%-0G", "%-0vG", NULL },
  { __LINE__, "%+ G", "%+ vG", NULL },
  { __LINE__, "%+#G", "%+#vG", NULL },
  { __LINE__, "%+'G", "%+'vG", NULL },
  { __LINE__, "%+0G", "%+0vG", NULL },
  { __LINE__, "% #G", "% #vG", NULL },
  { __LINE__, "% 'G", "% 'vG", NULL },
  { __LINE__, "% 0G", "% 0vG", NULL },
  { __LINE__, "%#'G", "%#'vG", NULL },
  { __LINE__, "%#0G", "%#0vG", NULL },
  { __LINE__, "%'0G", "%'0vG", NULL },

  /* Basic flags with precision. */
  { __LINE__, "%.9G",  "%.9vG", NULL },
  { __LINE__, "%-.9G",  "%-.9vG", NULL },
  { __LINE__, "%+.9G",  "%+.9vG", NULL },
  { __LINE__, "% .9G",  "% .9vG", NULL },
  { __LINE__, "%#.9G",  "%#.9vG", NULL },
  { __LINE__, "%'.9G",  "%'.9vG", NULL },
  { __LINE__, "%0.9G",  "%0.9vG", NULL },

  /* Basic flags with field width. */
  { __LINE__, "%20G",  "%20vG", NULL },
  { __LINE__, "%-20G",  "%-20vG", NULL },
  { __LINE__, "%+20G",  "%+20vG", NULL },
  { __LINE__, "% 20G",  "% 20vG", NULL },
  { __LINE__, "%#20G",  "%#20vG", NULL },
  { __LINE__, "%'20G",  "%'20vG", NULL },
  { __LINE__, "%020G",  "%020vG", NULL },

  /* Basic flags with field width and precision. */
  { __LINE__, "%25.3G",  "%25.3vG", NULL },
  { __LINE__, "%-25.3G",  "%-25.3vG", NULL },
  { __LINE__, "%+25.3G",  "%+25.3vG", NULL },
  { __LINE__, "% 25.3G",  "% 25.3vG", NULL },
  { __LINE__, "%#25.3G",  "%#25.3vG", NULL },
  { __LINE__, "%'25.3G",  "%'25.3vG", NULL },
  { __LINE__, "%025.3G",  "%025.3vG", NULL },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%a",  "%va", NULL },
  { __LINE__, "%-a", "%-va", NULL },
  { __LINE__, "%+a", "%+va", NULL },
  { __LINE__, "% a", "% va", NULL },
  { __LINE__, "%#a", "%#va", NULL },
  { __LINE__, "%'a", "%'va", NULL },
  { __LINE__, "%0a", "%0va", NULL },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+a", "%-+va", NULL },
  { __LINE__, "%- a", "%- va", NULL },
  { __LINE__, "%-#a", "%-#va", NULL },
  { __LINE__, "%-'a", "%-'va", NULL },
  { __LINE__, "%-0a", "%-0va", NULL },
  { __LINE__, "%+ a", "%+ va", NULL },
  { __LINE__, "%+#a", "%+#va", NULL },
  { __LINE__, "%+'a", "%+'va", NULL },
  { __LINE__, "%+0a", "%+0va", NULL },
  { __LINE__, "% #a", "% #va", NULL },
  { __LINE__, "% 'a", "% 'va", NULL },
  { __LINE__, "% 0a", "% 0va", NULL },
  { __LINE__, "%#'a", "%#'va", NULL },
  { __LINE__, "%#0a", "%#0va", NULL },
  { __LINE__, "%'0a", "%'0va", NULL },

  /* Basic flags with precision. */
  { __LINE__, "%.9a",  "%.9va", NULL },
  { __LINE__, "%-.9a",  "%-.9va", NULL },
  { __LINE__, "%+.9a",  "%+.9va", NULL },
  { __LINE__, "% .9a",  "% .9va", NULL },
  { __LINE__, "%#.9a",  "%#.9va", NULL },
  { __LINE__, "%'.9a",  "%'.9va", NULL },
  { __LINE__, "%0.9a",  "%0.9va", NULL },

  /* Basic flags with field width. */
  { __LINE__, "%20a",  "%20va", NULL },
  { __LINE__, "%-20a",  "%-20va", NULL },
  { __LINE__, "%+20a",  "%+20va", NULL },
  { __LINE__, "% 20a",  "% 20va", NULL },
  { __LINE__, "%#20a",  "%#20va", NULL },
  { __LINE__, "%'20a",  "%'20va", NULL },
  { __LINE__, "%020a",  "%020va", NULL },

  /* Basic flags with field width and precision. */
  { __LINE__, "%25.3a",  "%25.3va", NULL },
  { __LINE__, "%-25.3a",  "%-25.3va", NULL },
  { __LINE__, "%+25.3a",  "%+25.3va", NULL },
  { __LINE__, "% 25.3a",  "% 25.3va", NULL },
  { __LINE__, "%#25.3a",  "%#25.3va", NULL },
  { __LINE__, "%'25.3a",  "%'25.3va", NULL },
  { __LINE__, "%025.3a",  "%025.3va", NULL },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%A",  "%vA", NULL },
  { __LINE__, "%-A", "%-vA", NULL },
  { __LINE__, "%+A", "%+vA", NULL },
  { __LINE__, "% A", "% vA", NULL },
  { __LINE__, "%#A", "%#vA", NULL },
  { __LINE__, "%'A", "%'vA", NULL },
  { __LINE__, "%0A", "%0vA", NULL },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+A", "%-+vA", NULL },
  { __LINE__, "%- A", "%- vA", NULL },
  { __LINE__, "%-#A", "%-#vA", NULL },
  { __LINE__, "%-'A", "%-'vA", NULL },
  { __LINE__, "%-0A", "%-0vA", NULL },
  { __LINE__, "%+ A", "%+ vA", NULL },
  { __LINE__, "%+#A", "%+#vA", NULL },
  { __LINE__, "%+'A", "%+'vA", NULL },
  { __LINE__, "%+0A", "%+0vA", NULL },
  { __LINE__, "% #A", "% #vA", NULL },
  { __LINE__, "% 'A", "% 'vA", NULL },
  { __LINE__, "% 0A", "% 0vA", NULL },
  { __LINE__, "%#'A", "%#'vA", NULL },
  { __LINE__, "%#0A", "%#0vA", NULL },
  { __LINE__, "%'0A", "%'0vA", NULL },

  /* Basic flags with precision. */
  { __LINE__, "%.9A",  "%.9vA", NULL },
  { __LINE__, "%-.9A",  "%-.9vA", NULL },
  { __LINE__, "%+.9A",  "%+.9vA", NULL },
  { __LINE__, "% .9A",  "% .9vA", NULL },
  { __LINE__, "%#.9A",  "%#.9vA", NULL },
  { __LINE__, "%'.9A",  "%'.9vA", NULL },
  { __LINE__, "%0.9A",  "%0.9vA", NULL },

  /* Basic flags with field width. */
  { __LINE__, "%20A",  "%20vA", NULL },
  { __LINE__, "%-20A",  "%-20vA", NULL },
  { __LINE__, "%+20A",  "%+20vA", NULL },
  { __LINE__, "% 20A",  "% 20vA", NULL },
  { __LINE__, "%#20A",  "%#20vA", NULL },
  { __LINE__, "%'20A",  "%'20vA", NULL },
  { __LINE__, "%020A",  "%020vA", NULL },

  /* Basic flags with field width and precision. */
  { __LINE__, "%25.3A",  "%25.3vA", NULL },
  { __LINE__, "%-25.3A",  "%-25.3vA", NULL },
  { __LINE__, "%+25.3A",  "%+25.3vA", NULL },
  { __LINE__, "% 25.3A",  "% 25.3vA", NULL },
  { __LINE__, "%#25.3A",  "%#25.3vA", NULL },
  { __LINE__, "%'25.3A",  "%'25.3vA", NULL },
  { __LINE__, "%025.3A",  "%025.3vA", NULL },

  { 0, NULL, NULL }
};

#ifdef __VSX__
format_specifiers double_tests[] =
{
  { __LINE__, "%f",  "%vvf", NULL },
  { __LINE__, "%-f",  "%-vvf", NULL },
  { __LINE__, "%+f",  "%+vvf", NULL },
  { __LINE__, "% f",  "% vvf", NULL },
  { __LINE__, "%#f",  "%#vvf", NULL },
  { __LINE__, "%'f",  "%'vvf", NULL },
  { __LINE__, "%0f",  "%0vvf", NULL },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%f",  "%vvf", NULL },
  { __LINE__, "%-f", "%-vvf", NULL },
  { __LINE__, "%+f", "%+vvf", NULL },
  { __LINE__, "% f", "% vvf", NULL },
  { __LINE__, "%#f", "%#vvf", NULL },
  { __LINE__, "%'f", "%'vvf", NULL },
  { __LINE__, "%0f", "%0vvf", NULL },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+f", "%-+vvf", NULL },
  { __LINE__, "%- f", "%- vvf", NULL },
  { __LINE__, "%-#f", "%-#vvf", NULL },
  { __LINE__, "%-'f", "%-'vvf", NULL },
  { __LINE__, "%-0f", "%-0vvf", NULL },
  { __LINE__, "%+ f", "%+ vvf", NULL },
  { __LINE__, "%+#f", "%+#vvf", NULL },
  { __LINE__, "%+'f", "%+'vvf", NULL },
  { __LINE__, "%+0f", "%+0vvf", NULL },
  { __LINE__, "% #f", "% #vvf", NULL },
  { __LINE__, "% 'f", "% 'vvf", NULL },
  { __LINE__, "% 0f", "% 0vvf", NULL },
  { __LINE__, "%#'f", "%#'vvf", NULL },
  { __LINE__, "%#0f", "%#0vvf", NULL },
  { __LINE__, "%'0f", "%'0vvf", NULL },

  /* Basic flags with precision. */
  { __LINE__, "%.9f",  "%.9vvf", NULL },
  { __LINE__, "%-.9f",  "%-.9vvf", NULL },
  { __LINE__, "%+.9f",  "%+.9vvf", NULL },
  { __LINE__, "% .9f",  "% .9vvf", NULL },
  { __LINE__, "%#.9f",  "%#.9vvf", NULL },
  { __LINE__, "%'.9f",  "%'.9vvf", NULL },
  { __LINE__, "%0.9f",  "%0.9vvf", NULL },

  /* Basic flags with field width. */
  { __LINE__, "%20f",  "%20vvf", NULL },
  { __LINE__, "%-20f",  "%-20vvf", NULL },
  { __LINE__, "%+20f",  "%+20vvf", NULL },
  { __LINE__, "% 20f",  "% 20vvf", NULL },
  { __LINE__, "%#20f",  "%#20vvf", NULL },
  { __LINE__, "%'20f",  "%'20vvf", NULL },
  { __LINE__, "%020f",  "%020vvf", NULL },

  /* Basic flags with field width and precision. */
  { __LINE__, "%25.3f",  "%25.3vvf", NULL },
  { __LINE__, "%-25.3f",  "%-25.3vvf", NULL },
  { __LINE__, "%+25.3f",  "%+25.3vvf", NULL },
  { __LINE__, "% 25.3f",  "% 25.3vvf", NULL },
  { __LINE__, "%#25.3f",  "%#25.3vvf", NULL },
  { __LINE__, "%'25.3f",  "%'25.3vvf", NULL },
  { __LINE__, "%025.3f",  "%025.3vvf", NULL },

  { 0, NULL, NULL, NULL }
};

format_specifiers uint64_tests[] = {
  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%llu",  "%vllu", "%llvu" },
  { __LINE__, "%-llu", "%-vllu", "%-llvu" },
  { __LINE__, "%+llu", "%+vllu", "%+llvu" },
  { __LINE__, "% llu", "% vllu", "% llvu" },
  { __LINE__, "%#llu", "%#vllu", "%#llvu" },
  { __LINE__, "%'llu", "%'vllu", "%'llvu" },
  { __LINE__, "%0llu", "%0vllu", "%0llvu" },
  { __LINE__, "%lu",  "%vllu", "%llvu" },
  { __LINE__, "%-lu", "%-vllu", "%-llvu" },
  { __LINE__, "%+lu", "%+vllu", "%+llvu" },
  { __LINE__, "% lu", "% vllu", "% llvu" },
  { __LINE__, "%#lu", "%#vllu", "%#llvu" },
  { __LINE__, "%'lu", "%'vllu", "%'llvu" },
  { __LINE__, "%0lu", "%0vllu", "%0llvu" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+llu", "%-+vllu", "%-+llvu" },
  { __LINE__, "%- llu", "%- vllu", "%- llvu" },
  { __LINE__, "%-#llu", "%-#vllu", "%-#llvu" },
  { __LINE__, "%-'llu", "%-'vllu", "%-'llvu" },
  { __LINE__, "%-0llu", "%-0vllu", "%-0llvu" },
  { __LINE__, "%+ llu", "%+ vllu", "%+ llvu" },
  { __LINE__, "%+#llu", "%+#vllu", "%+#llvu" },
  { __LINE__, "%+'llu", "%+'vllu", "%+'llvu" },
  { __LINE__, "%+0llu", "%+0vllu", "%+0llvu" },
  { __LINE__, "% #llu", "% #vllu", "% #llvu" },
  { __LINE__, "% 'llu", "% 'vllu", "% 'llvu" },
  { __LINE__, "% 0llu", "% 0vllu", "% 0llvu" },
  { __LINE__, "%#'llu", "%#'vllu", "%#'llvu" },
  { __LINE__, "%#0llu", "%#0vllu", "%#0llvu" },
  { __LINE__, "%'0llu", "%'0vllu", "%'0llvu" },
  { __LINE__, "%-+lu", "%-+vllu", "%-+llvu" },
  { __LINE__, "%- lu", "%- vllu", "%- llvu" },
  { __LINE__, "%-#lu", "%-#vllu", "%-#llvu" },
  { __LINE__, "%-'lu", "%-'vllu", "%-'llvu" },
  { __LINE__, "%-0lu", "%-0vllu", "%-0llvu" },
  { __LINE__, "%+ lu", "%+ vllu", "%+ llvu" },
  { __LINE__, "%+#lu", "%+#vllu", "%+#llvu" },
  { __LINE__, "%+'lu", "%+'vllu", "%+'llvu" },
  { __LINE__, "%+0lu", "%+0vllu", "%+0llvu" },
  { __LINE__, "% #lu", "% #vllu", "% #llvu" },
  { __LINE__, "% 'lu", "% 'vllu", "% 'llvu" },
  { __LINE__, "% 0lu", "% 0vllu", "% 0llvu" },
  { __LINE__, "%#'lu", "%#'vllu", "%#'llvu" },
  { __LINE__, "%#0lu", "%#0vllu", "%#0llvu" },
  { __LINE__, "%'0lu", "%'0vllu", "%'0llvu" },

  /* Basic flags with precision. */
  { __LINE__, "%.5llu",  "%.5vllu", "%.5llvu" },
  { __LINE__, "%-.5llu", "%-.5vllu", "%-.5llvu" },
  { __LINE__, "%+.5llu", "%+.5vllu", "%+.5llvu" },
  { __LINE__, "% .5llu", "% .5vllu", "% .5llvu" },
  { __LINE__, "%#.5llu", "%#.5vllu", "%#.5llvu" },
  { __LINE__, "%'.5llu", "%'.5vllu", "%'.5llvu" },
  { __LINE__, "%0.5llu", "%0.5vllu", "%0.5llvu" },
  { __LINE__, "%.5lu",  "%.5vllu", "%.5llvu" },
  { __LINE__, "%-.5lu", "%-.5vllu", "%-.5llvu" },
  { __LINE__, "%+.5lu", "%+.5vllu", "%+.5llvu" },
  { __LINE__, "% .5lu", "% .5vllu", "% .5llvu" },
  { __LINE__, "%#.5lu", "%#.5vllu", "%#.5llvu" },
  { __LINE__, "%'.5lu", "%'.5vllu", "%'.5llvu" },
  { __LINE__, "%0.5lu", "%0.5vllu", "%0.5llvu" },

  /* Basic flags with field width. */
  { __LINE__, "%12llu",  "%12vllu", "%12llvu" },
  { __LINE__, "%-12llu", "%-12vllu", "%-12llvu" },
  { __LINE__, "%+12llu", "%+12vllu", "%+12llvu" },
  { __LINE__, "% 12llu", "% 12vllu", "% 12llvu" },
  { __LINE__, "%#12llu", "%#12vllu", "%#12llvu" },
  { __LINE__, "%'12llu", "%'12vllu", "%'12llvu" },
  { __LINE__, "%012llu", "%012vllu", "%012llvu" },
  { __LINE__, "%12lu",  "%12vllu", "%12llvu" },
  { __LINE__, "%-12lu", "%-12vllu", "%-12llvu" },
  { __LINE__, "%+12lu", "%+12vllu", "%+12llvu" },
  { __LINE__, "% 12lu", "% 12vllu", "% 12llvu" },
  { __LINE__, "%#12lu", "%#12vllu", "%#12llvu" },
  { __LINE__, "%'12lu", "%'12vllu", "%'12llvu" },
  { __LINE__, "%012lu", "%012vllu", "%012llvu" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%15.7llu",  "%15.7vllu", "%15.7llvu" },
  { __LINE__, "%-15.7llu", "%-15.7vllu", "%-15.7llvu" },
  { __LINE__, "%+15.7llu", "%+15.7vllu", "%+15.7llvu" },
  { __LINE__, "% 15.7llu", "% 15.7vllu", "% 15.7llvu" },
  { __LINE__, "%#15.7llu", "%#15.7vllu", "%#15.7llvu" },
  { __LINE__, "%'15.7llu", "%'15.7vllu", "%'15.7llvu" },
  { __LINE__, "%015.7llu", "%015.7vllu", "%015.7llvu" },
  { __LINE__, "%15.7lu",  "%15.7vllu", "%15.7llvu" },
  { __LINE__, "%-15.7lu", "%-15.7vllu", "%-15.7llvu" },
  { __LINE__, "%+15.7lu", "%+15.7vllu", "%+15.7llvu" },
  { __LINE__, "% 15.7lu", "% 15.7vllu", "% 15.7llvu" },
  { __LINE__, "%#15.7lu", "%#15.7vllu", "%#15.7llvu" },
  { __LINE__, "%'15.7lu", "%'15.7vllu", "%'15.7llvu" },
  { __LINE__, "%015.7lu", "%015.7vllu", "%015.7llvu" },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%llo",  "%vllo", "%llvo" },
  { __LINE__, "%-llo", "%-vllo", "%-llvo" },
  { __LINE__, "%+llo", "%+vllo", "%+llvo" },
  { __LINE__, "% llo", "% vllo", "% llvo" },
  { __LINE__, "%#llo", "%#vllo", "%#llvo" },
  { __LINE__, "%'llo", "%'vllo", "%'llvo" },
  { __LINE__, "%0llo", "%0vllo", "%0llvo" },
  { __LINE__, "%lo",  "%vllo", "%llvo" },
  { __LINE__, "%-lo", "%-vllo", "%-llvo" },
  { __LINE__, "%+lo", "%+vllo", "%+llvo" },
  { __LINE__, "% lo", "% vllo", "% llvo" },
  { __LINE__, "%#lo", "%#vllo", "%#llvo" },
  { __LINE__, "%'lo", "%'vllo", "%'llvo" },
  { __LINE__, "%0lo", "%0vllo", "%0llvo" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+llo", "%-+vllo", "%-+llvo" },
  { __LINE__, "%- llo", "%- vllo", "%- llvo" },
  { __LINE__, "%-#llo", "%-#vllo", "%-#llvo" },
  { __LINE__, "%-'llo", "%-'vllo", "%-'llvo" },
  { __LINE__, "%-0llo", "%-0vllo", "%-0llvo" },
  { __LINE__, "%+ llo", "%+ vllo", "%+ llvo" },
  { __LINE__, "%+#llo", "%+#vllo", "%+#llvo" },
  { __LINE__, "%+'llo", "%+'vllo", "%+'llvo" },
  { __LINE__, "%+0llo", "%+0vllo", "%+0llvo" },
  { __LINE__, "% #llo", "% #vllo", "% #llvo" },
  { __LINE__, "% 'llo", "% 'vllo", "% 'llvo" },
  { __LINE__, "% 0llo", "% 0vllo", "% 0llvo" },
  { __LINE__, "%#'llo", "%#'vllo", "%#'llvo" },
  { __LINE__, "%#0llo", "%#0vllo", "%#0llvo" },
  { __LINE__, "%'0llo", "%'0vllo", "%'0llvo" },
  { __LINE__, "%-+lo", "%-+vllo", "%-+llvo" },
  { __LINE__, "%- lo", "%- vllo", "%- llvo" },
  { __LINE__, "%-#lo", "%-#vllo", "%-#llvo" },
  { __LINE__, "%-'lo", "%-'vllo", "%-'llvo" },
  { __LINE__, "%-0lo", "%-0vllo", "%-0llvo" },
  { __LINE__, "%+ lo", "%+ vllo", "%+ llvo" },
  { __LINE__, "%+#lo", "%+#vllo", "%+#llvo" },
  { __LINE__, "%+'lo", "%+'vllo", "%+'llvo" },
  { __LINE__, "%+0lo", "%+0vllo", "%+0llvo" },
  { __LINE__, "% #lo", "% #vllo", "% #llvo" },
  { __LINE__, "% 'lo", "% 'vllo", "% 'llvo" },
  { __LINE__, "% 0lo", "% 0vllo", "% 0llvo" },
  { __LINE__, "%#'lo", "%#'vllo", "%#'llvo" },
  { __LINE__, "%#0lo", "%#0vllo", "%#0llvo" },
  { __LINE__, "%'0lo", "%'0vllo", "%'0llvo" },

  /* Basic flags with precision. */
  { __LINE__, "%.5llo",  "%.5vllo", "%.5llvo" },
  { __LINE__, "%-.5llo", "%-.5vllo", "%-.5llvo" },
  { __LINE__, "%+.5llo", "%+.5vllo", "%+.5llvo" },
  { __LINE__, "% .5llo", "% .5vllo", "% .5llvo" },
  { __LINE__, "%#.5llo", "%#.5vllo", "%#.5llvo" },
  { __LINE__, "%'.5llo", "%'.5vllo", "%'.5llvo" },
  { __LINE__, "%0.5llo", "%0.5vllo", "%0.5llvo" },
  { __LINE__, "%.5lo",  "%.5vllo", "%.5llvo" },
  { __LINE__, "%-.5lo", "%-.5vllo", "%-.5llvo" },
  { __LINE__, "%+.5lo", "%+.5vllo", "%+.5llvo" },
  { __LINE__, "% .5lo", "% .5vllo", "% .5llvo" },
  { __LINE__, "%#.5lo", "%#.5vllo", "%#.5llvo" },
  { __LINE__, "%'.5lo", "%'.5vllo", "%'.5llvo" },
  { __LINE__, "%0.5lo", "%0.5vllo", "%0.5llvo" },

  /* Basic flags with field width. */
  { __LINE__, "%12llo",  "%12vllo", "%12llvo" },
  { __LINE__, "%-12llo", "%-12vllo", "%-12llvo" },
  { __LINE__, "%+12llo", "%+12vllo", "%+12llvo" },
  { __LINE__, "% 12llo", "% 12vllo", "% 12llvo" },
  { __LINE__, "%#12llo", "%#12vllo", "%#12llvo" },
  { __LINE__, "%'12llo", "%'12vllo", "%'12llvo" },
  { __LINE__, "%012llo", "%012vllo", "%012llvo" },
  { __LINE__, "%12lo",  "%12vllo", "%12llvo" },
  { __LINE__, "%-12lo", "%-12vllo", "%-12llvo" },
  { __LINE__, "%+12lo", "%+12vllo", "%+12llvo" },
  { __LINE__, "% 12lo", "% 12vllo", "% 12llvo" },
  { __LINE__, "%#12lo", "%#12vllo", "%#12llvo" },
  { __LINE__, "%'12lo", "%'12vllo", "%'12llvo" },
  { __LINE__, "%012lo", "%012vllo", "%012llvo" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%15.7llo",  "%15.7vllo", "%15.7llvo" },
  { __LINE__, "%-15.7llo", "%-15.7vllo", "%-15.7llvo" },
  { __LINE__, "%+15.7llo", "%+15.7vllo", "%+15.7llvo" },
  { __LINE__, "% 15.7llo", "% 15.7vllo", "% 15.7llvo" },
  { __LINE__, "%#15.7llo", "%#15.7vllo", "%#15.7llvo" },
  { __LINE__, "%'15.7llo", "%'15.7vllo", "%'15.7llvo" },
  { __LINE__, "%015.7llo", "%015.7vllo", "%015.7llvo" },
  { __LINE__, "%15.7lo",  "%15.7vllo", "%15.7llvo" },
  { __LINE__, "%-15.7lo", "%-15.7vllo", "%-15.7llvo" },
  { __LINE__, "%+15.7lo", "%+15.7vllo", "%+15.7llvo" },
  { __LINE__, "% 15.7lo", "% 15.7vllo", "% 15.7llvo" },
  { __LINE__, "%#15.7lo", "%#15.7vllo", "%#15.7llvo" },
  { __LINE__, "%'15.7lo", "%'15.7vllo", "%'15.7llvo" },
  { __LINE__, "%015.7lo", "%015.7vllo", "%015.7llvo" },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%llx",  "%vllx", "%llvx" },
  { __LINE__, "%-llx", "%-vllx", "%-llvx" },
  { __LINE__, "%+llx", "%+vllx", "%+llvx" },
  { __LINE__, "% llx", "% vllx", "% llvx" },
  { __LINE__, "%#llx", "%#vllx", "%#llvx" },
  { __LINE__, "%'llx", "%'vllx", "%'llvx" },
  { __LINE__, "%0llx", "%0vllx", "%0llvx" },
  { __LINE__, "%lx",  "%vllx", "%llvx" },
  { __LINE__, "%-lx", "%-vllx", "%-llvx" },
  { __LINE__, "%+lx", "%+vllx", "%+llvx" },
  { __LINE__, "% lx", "% vllx", "% llvx" },
  { __LINE__, "%#lx", "%#vllx", "%#llvx" },
  { __LINE__, "%'lx", "%'vllx", "%'llvx" },
  { __LINE__, "%0lx", "%0vllx", "%0llvx" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+llx", "%-+vllx", "%-+llvx" },
  { __LINE__, "%- llx", "%- vllx", "%- llvx" },
  { __LINE__, "%-#llx", "%-#vllx", "%-#llvx" },
  { __LINE__, "%-'llx", "%-'vllx", "%-'llvx" },
  { __LINE__, "%-0llx", "%-0vllx", "%-0llvx" },
  { __LINE__, "%+ llx", "%+ vllx", "%+ llvx" },
  { __LINE__, "%+#llx", "%+#vllx", "%+#llvx" },
  { __LINE__, "%+'llx", "%+'vllx", "%+'llvx" },
  { __LINE__, "%+0llx", "%+0vllx", "%+0llvx" },
  { __LINE__, "% #llx", "% #vllx", "% #llvx" },
  { __LINE__, "% 'llx", "% 'vllx", "% 'llvx" },
  { __LINE__, "% 0llx", "% 0vllx", "% 0llvx" },
  { __LINE__, "%#'llx", "%#'vllx", "%#'llvx" },
  { __LINE__, "%#0llx", "%#0vllx", "%#0llvx" },
  { __LINE__, "%'0llx", "%'0vllx", "%'0llvx" },
  { __LINE__, "%-+lx", "%-+vllx", "%-+llvx" },
  { __LINE__, "%- lx", "%- vllx", "%- llvx" },
  { __LINE__, "%-#lx", "%-#vllx", "%-#llvx" },
  { __LINE__, "%-'lx", "%-'vllx", "%-'llvx" },
  { __LINE__, "%-0lx", "%-0vllx", "%-0llvx" },
  { __LINE__, "%+ lx", "%+ vllx", "%+ llvx" },
  { __LINE__, "%+#lx", "%+#vllx", "%+#llvx" },
  { __LINE__, "%+'lx", "%+'vllx", "%+'llvx" },
  { __LINE__, "%+0lx", "%+0vllx", "%+0llvx" },
  { __LINE__, "% #lx", "% #vllx", "% #llvx" },
  { __LINE__, "% 'lx", "% 'vllx", "% 'llvx" },
  { __LINE__, "% 0lx", "% 0vllx", "% 0llvx" },
  { __LINE__, "%#'lx", "%#'vllx", "%#'llvx" },
  { __LINE__, "%#0lx", "%#0vllx", "%#0llvx" },
  { __LINE__, "%'0lx", "%'0vllx", "%'0llvx" },

  /* Basic flags with precision. */
  { __LINE__, "%.5llx",  "%.5vllx", "%.5llvx" },
  { __LINE__, "%-.5llx", "%-.5vllx", "%-.5llvx" },
  { __LINE__, "%+.5llx", "%+.5vllx", "%+.5llvx" },
  { __LINE__, "% .5llx", "% .5vllx", "% .5llvx" },
  { __LINE__, "%#.5llx", "%#.5vllx", "%#.5llvx" },
  { __LINE__, "%'.5llx", "%'.5vllx", "%'.5llvx" },
  { __LINE__, "%0.5llx", "%0.5vllx", "%0.5llvx" },
  { __LINE__, "%.5lx",  "%.5vllx", "%.5llvx" },
  { __LINE__, "%-.5lx", "%-.5vllx", "%-.5llvx" },
  { __LINE__, "%+.5lx", "%+.5vllx", "%+.5llvx" },
  { __LINE__, "% .5lx", "% .5vllx", "% .5llvx" },
  { __LINE__, "%#.5lx", "%#.5vllx", "%#.5llvx" },
  { __LINE__, "%'.5lx", "%'.5vllx", "%'.5llvx" },
  { __LINE__, "%0.5lx", "%0.5vllx", "%0.5llvx" },

  /* Basic flags with field width. */
  { __LINE__, "%12llx",  "%12vllx", "%12llvx" },
  { __LINE__, "%-12llx", "%-12vllx", "%-12llvx" },
  { __LINE__, "%+12llx", "%+12vllx", "%+12llvx" },
  { __LINE__, "% 12llx", "% 12vllx", "% 12llvx" },
  { __LINE__, "%#12llx", "%#12vllx", "%#12llvx" },
  { __LINE__, "%'12llx", "%'12vllx", "%'12llvx" },
  { __LINE__, "%012llx", "%012vllx", "%012llvx" },
  { __LINE__, "%12lx",  "%12vllx", "%12llvx" },
  { __LINE__, "%-12lx", "%-12vllx", "%-12llvx" },
  { __LINE__, "%+12lx", "%+12vllx", "%+12llvx" },
  { __LINE__, "% 12lx", "% 12vllx", "% 12llvx" },
  { __LINE__, "%#12lx", "%#12vllx", "%#12llvx" },
  { __LINE__, "%'12lx", "%'12vllx", "%'12llvx" },
  { __LINE__, "%012lx", "%012vllx", "%012llvx" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%15.7llx",  "%15.7vllx", "%15.7llvx" },
  { __LINE__, "%-15.7llx", "%-15.7vllx", "%-15.7llvx" },
  { __LINE__, "%+15.7llx", "%+15.7vllx", "%+15.7llvx" },
  { __LINE__, "% 15.7llx", "% 15.7vllx", "% 15.7llvx" },
  { __LINE__, "%#15.7llx", "%#15.7vllx", "%#15.7llvx" },
  { __LINE__, "%'15.7llx", "%'15.7vllx", "%'15.7llvx" },
  { __LINE__, "%015.7llx", "%015.7vllx", "%015.7llvx" },
  { __LINE__, "%15.7lx",  "%15.7vllx", "%15.7llvx" },
  { __LINE__, "%-15.7lx", "%-15.7vllx", "%-15.7llvx" },
  { __LINE__, "%+15.7lx", "%+15.7vllx", "%+15.7llvx" },
  { __LINE__, "% 15.7lx", "% 15.7vllx", "% 15.7llvx" },
  { __LINE__, "%#15.7lx", "%#15.7vllx", "%#15.7llvx" },
  { __LINE__, "%'15.7lx", "%'15.7vllx", "%'15.7llvx" },
  { __LINE__, "%015.7lx", "%015.7vllx", "%015.7llvx" },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%llX",  "%vllX", "%llvX" },
  { __LINE__, "%-llX", "%-vllX", "%-llvX" },
  { __LINE__, "%+llX", "%+vllX", "%+llvX" },
  { __LINE__, "% llX", "% vllX", "% llvX" },
  { __LINE__, "%#llX", "%#vllX", "%#llvX" },
  { __LINE__, "%'llX", "%'vllX", "%'llvX" },
  { __LINE__, "%0llX", "%0vllX", "%0llvX" },
  { __LINE__, "%lX",  "%vllX", "%llvX" },
  { __LINE__, "%-lX", "%-vllX", "%-llvX" },
  { __LINE__, "%+lX", "%+vllX", "%+llvX" },
  { __LINE__, "% lX", "% vllX", "% llvX" },
  { __LINE__, "%#lX", "%#vllX", "%#llvX" },
  { __LINE__, "%'lX", "%'vllX", "%'llvX" },
  { __LINE__, "%0lX", "%0vllX", "%0llvX" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+llX", "%-+vllX", "%-+llvX" },
  { __LINE__, "%- llX", "%- vllX", "%- llvX" },
  { __LINE__, "%-#llX", "%-#vllX", "%-#llvX" },
  { __LINE__, "%-'llX", "%-'vllX", "%-'llvX" },
  { __LINE__, "%-0llX", "%-0vllX", "%-0llvX" },
  { __LINE__, "%+ llX", "%+ vllX", "%+ llvX" },
  { __LINE__, "%+#llX", "%+#vllX", "%+#llvX" },
  { __LINE__, "%+'llX", "%+'vllX", "%+'llvX" },
  { __LINE__, "%+0llX", "%+0vllX", "%+0llvX" },
  { __LINE__, "% #llX", "% #vllX", "% #llvX" },
  { __LINE__, "% 'llX", "% 'vllX", "% 'llvX" },
  { __LINE__, "% 0llX", "% 0vllX", "% 0llvX" },
  { __LINE__, "%#'llX", "%#'vllX", "%#'llvX" },
  { __LINE__, "%#0llX", "%#0vllX", "%#0llvX" },
  { __LINE__, "%'0llX", "%'0vllX", "%'0llvX" },
  { __LINE__, "%-+lX", "%-+vllX", "%-+llvX" },
  { __LINE__, "%- lX", "%- vllX", "%- llvX" },
  { __LINE__, "%-#lX", "%-#vllX", "%-#llvX" },
  { __LINE__, "%-'lX", "%-'vllX", "%-'llvX" },
  { __LINE__, "%-0lX", "%-0vllX", "%-0llvX" },
  { __LINE__, "%+ lX", "%+ vllX", "%+ llvX" },
  { __LINE__, "%+#lX", "%+#vllX", "%+#llvX" },
  { __LINE__, "%+'lX", "%+'vllX", "%+'llvX" },
  { __LINE__, "%+0lX", "%+0vllX", "%+0llvX" },
  { __LINE__, "% #lX", "% #vllX", "% #llvX" },
  { __LINE__, "% 'lX", "% 'vllX", "% 'llvX" },
  { __LINE__, "% 0lX", "% 0vllX", "% 0llvX" },
  { __LINE__, "%#'lX", "%#'vllX", "%#'llvX" },
  { __LINE__, "%#0lX", "%#0vllX", "%#0llvX" },
  { __LINE__, "%'0lX", "%'0vllX", "%'0llvX" },

  /* Basic flags with precision. */
  { __LINE__, "%.5llX",  "%.5vllX", "%.5llvX" },
  { __LINE__, "%-.5llX", "%-.5vllX", "%-.5llvX" },
  { __LINE__, "%+.5llX", "%+.5vllX", "%+.5llvX" },
  { __LINE__, "% .5llX", "% .5vllX", "% .5llvX" },
  { __LINE__, "%#.5llX", "%#.5vllX", "%#.5llvX" },
  { __LINE__, "%'.5llX", "%'.5vllX", "%'.5llvX" },
  { __LINE__, "%0.5llX", "%0.5vllX", "%0.5llvX" },
  { __LINE__, "%.5lX",  "%.5vllX", "%.5llvX" },
  { __LINE__, "%-.5lX", "%-.5vllX", "%-.5llvX" },
  { __LINE__, "%+.5lX", "%+.5vllX", "%+.5llvX" },
  { __LINE__, "% .5lX", "% .5vllX", "% .5llvX" },
  { __LINE__, "%#.5lX", "%#.5vllX", "%#.5llvX" },
  { __LINE__, "%'.5lX", "%'.5vllX", "%'.5llvX" },
  { __LINE__, "%0.5lX", "%0.5vllX", "%0.5llvX" },

  /* Basic flags with field width. */
  { __LINE__, "%12llX",  "%12vllX", "%12llvX" },
  { __LINE__, "%-12llX", "%-12vllX", "%-12llvX" },
  { __LINE__, "%+12llX", "%+12vllX", "%+12llvX" },
  { __LINE__, "% 12llX", "% 12vllX", "% 12llvX" },
  { __LINE__, "%#12llX", "%#12vllX", "%#12llvX" },
  { __LINE__, "%'12llX", "%'12vllX", "%'12llvX" },
  { __LINE__, "%012llX", "%012vllX", "%012llvX" },
  { __LINE__, "%12lX",  "%12vllX", "%12llvX" },
  { __LINE__, "%-12lX", "%-12vllX", "%-12llvX" },
  { __LINE__, "%+12lX", "%+12vllX", "%+12llvX" },
  { __LINE__, "% 12lX", "% 12vllX", "% 12llvX" },
  { __LINE__, "%#12lX", "%#12vllX", "%#12llvX" },
  { __LINE__, "%'12lX", "%'12vllX", "%'12llvX" },
  { __LINE__, "%012lX", "%012vllX", "%012llvX" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%15.7llX",  "%15.7vllX", "%15.7llvX" },
  { __LINE__, "%-15.7llX", "%-15.7vllX", "%-15.7llvX" },
  { __LINE__, "%+15.7llX", "%+15.7vllX", "%+15.7llvX" },
  { __LINE__, "% 15.7llX", "% 15.7vllX", "% 15.7llvX" },
  { __LINE__, "%#15.7llX", "%#15.7vllX", "%#15.7llvX" },
  { __LINE__, "%'15.7llX", "%'15.7vllX", "%'15.7llvX" },
  { __LINE__, "%015.7llX", "%015.7vllX", "%015.7llvX" },
  { __LINE__, "%15.7lX",  "%15.7vllX", "%15.7llvX" },
  { __LINE__, "%-15.7lX", "%-15.7vllX", "%-15.7llvX" },
  { __LINE__, "%+15.7lX", "%+15.7vllX", "%+15.7llvX" },
  { __LINE__, "% 15.7lX", "% 15.7vllX", "% 15.7llvX" },
  { __LINE__, "%#15.7lX", "%#15.7vllX", "%#15.7llvX" },
  { __LINE__, "%'15.7lX", "%'15.7vllX", "%'15.7llvX" },
  { __LINE__, "%015.7lX", "%015.7vllX", "%015.7llvX" },

  { 0, NULL, NULL, NULL }
};

format_specifiers int64_tests[] = {
  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%lld",  "%vlld",  "%llvd" },
  { __LINE__, "%-lld", "%-vlld", "%-llvd" },
  { __LINE__, "%+lld", "%+vlld", "%+llvd" },
  { __LINE__, "% lld", "% vlld", "% llvd" },
  { __LINE__, "%#lld", "%#vlld", "%#llvd" },
  { __LINE__, "%'lld", "%'vlld", "%'llvd" },
  { __LINE__, "%0lld", "%0vlld", "%0llvd" },
  { __LINE__, "%ld",  "%vlld",  "%llvd" },
  { __LINE__, "%-ld", "%-vlld", "%-llvd" },
  { __LINE__, "%+ld", "%+vlld", "%+llvd" },
  { __LINE__, "% ld", "% vlld", "% llvd" },
  { __LINE__, "%#ld", "%#vlld", "%#llvd" },
  { __LINE__, "%'ld", "%'vlld", "%'llvd" },
  { __LINE__, "%0ld", "%0vlld", "%0llvd" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+lld", "%-+vlld", "%-+llvd" },
  { __LINE__, "%- lld", "%- vlld", "%- llvd" },
  { __LINE__, "%-#lld", "%-#vlld", "%-#llvd" },
  { __LINE__, "%-'lld", "%-'vlld", "%-'llvd" },
  { __LINE__, "%-0lld", "%-0vlld", "%-0llvd" },
  { __LINE__, "%+ lld", "%+ vlld", "%+ llvd" },
  { __LINE__, "%+#lld", "%+#vlld", "%+#llvd" },
  { __LINE__, "%+'lld", "%+'vlld", "%+'llvd" },
  { __LINE__, "%+0lld", "%+0vlld", "%+0llvd" },
  { __LINE__, "% #lld", "% #vlld", "% #llvd" },
  { __LINE__, "% 'lld", "% 'vlld", "% 'llvd" },
  { __LINE__, "% 0lld", "% 0vlld", "% 0llvd" },
  { __LINE__, "%#'lld", "%#'vlld", "%#'llvd" },
  { __LINE__, "%#0lld", "%#0vlld", "%#0llvd" },
  { __LINE__, "%'0lld", "%'0vlld", "%'0llvd" },
  { __LINE__, "%-+ld", "%-+vlld", "%-+llvd" },
  { __LINE__, "%- ld", "%- vlld", "%- llvd" },
  { __LINE__, "%-#ld", "%-#vlld", "%-#llvd" },
  { __LINE__, "%-'ld", "%-'vlld", "%-'llvd" },
  { __LINE__, "%-0ld", "%-0vlld", "%-0llvd" },
  { __LINE__, "%+ ld", "%+ vlld", "%+ llvd" },
  { __LINE__, "%+#ld", "%+#vlld", "%+#llvd" },
  { __LINE__, "%+'ld", "%+'vlld", "%+'llvd" },
  { __LINE__, "%+0ld", "%+0vlld", "%+0llvd" },
  { __LINE__, "% #ld", "% #vlld", "% #llvd" },
  { __LINE__, "% 'ld", "% 'vlld", "% 'llvd" },
  { __LINE__, "% 0ld", "% 0vlld", "% 0llvd" },
  { __LINE__, "%#'ld", "%#'vlld", "%#'llvd" },
  { __LINE__, "%#0ld", "%#0vlld", "%#0llvd" },
  { __LINE__, "%'0ld", "%'0vlld", "%'0llvd" },

  /* Basic flags with precision. */
  { __LINE__, "%.5lld",  "%.5vlld", "%.5llvd" },
  { __LINE__, "%-.5lld", "%-.5vlld", "%-.5llvd" },
  { __LINE__, "%+.5lld", "%+.5vlld", "%+.5llvd" },
  { __LINE__, "% .5lld", "% .5vlld", "% .5llvd" },
  { __LINE__, "%#.5lld", "%#.5vlld", "%#.5llvd" },
  { __LINE__, "%'.5lld", "%'.5vlld", "%'.5llvd" },
  { __LINE__, "%0.5lld", "%0.5vlld", "%0.5llvd" },
  { __LINE__, "%.5ld",  "%.5vlld", "%.5llvd" },
  { __LINE__, "%-.5ld", "%-.5vlld", "%-.5llvd" },
  { __LINE__, "%+.5ld", "%+.5vlld", "%+.5llvd" },
  { __LINE__, "% .5ld", "% .5vlld", "% .5llvd" },
  { __LINE__, "%#.5ld", "%#.5vlld", "%#.5llvd" },
  { __LINE__, "%'.5ld", "%'.5vlld", "%'.5llvd" },
  { __LINE__, "%0.5ld", "%0.5vlld", "%0.5llvd" },

  /* Basic flags with field width. */
  { __LINE__, "%12lld",  "%12vlld", "%12llvd" },
  { __LINE__, "%-12lld", "%-12vlld", "%-12llvd" },
  { __LINE__, "%+12lld", "%+12vlld", "%+12llvd" },
  { __LINE__, "% 12lld", "% 12vlld", "% 12llvd" },
  { __LINE__, "%#12lld", "%#12vlld", "%#12llvd" },
  { __LINE__, "%'12lld", "%'12vlld", "%'12llvd" },
  { __LINE__, "%012lld", "%012vlld", "%012llvd" },
  { __LINE__, "%12ld",  "%12vlld", "%12llvd" },
  { __LINE__, "%-12ld", "%-12vlld", "%-12llvd" },
  { __LINE__, "%+12ld", "%+12vlld", "%+12llvd" },
  { __LINE__, "% 12ld", "% 12vlld", "% 12llvd" },
  { __LINE__, "%#12ld", "%#12vlld", "%#12llvd" },
  { __LINE__, "%'12ld", "%'12vlld", "%'12llvd" },
  { __LINE__, "%012ld", "%012vlld", "%012llvd" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%15.7lld",  "%15.7vlld", "%15.7llvd" },
  { __LINE__, "%-15.7lld", "%-15.7vlld", "%-15.7llvd" },
  { __LINE__, "%+15.7lld", "%+15.7vlld", "%+15.7llvd" },
  { __LINE__, "% 15.7lld", "% 15.7vlld", "% 15.7llvd" },
  { __LINE__, "%#15.7lld", "%#15.7vlld", "%#15.7llvd" },
  { __LINE__, "%'15.7lld", "%'15.7vlld", "%'15.7llvd" },
  { __LINE__, "%015.7lld", "%015.7vlld", "%015.7llvd" },
  { __LINE__, "%15.7ld",  "%15.7vlld", "%15.7llvd" },
  { __LINE__, "%-15.7ld", "%-15.7vlld", "%-15.7llvd" },
  { __LINE__, "%+15.7ld", "%+15.7vlld", "%+15.7llvd" },
  { __LINE__, "% 15.7ld", "% 15.7vlld", "% 15.7llvd" },
  { __LINE__, "%#15.7ld", "%#15.7vlld", "%#15.7llvd" },
  { __LINE__, "%'15.7ld", "%'15.7vlld", "%'15.7llvd" },
  { __LINE__, "%015.7ld", "%015.7vlld", "%015.7llvd" },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%lli",  "%vlli",  "%llvi" },
  { __LINE__, "%-lli", "%-vlli", "%-llvi" },
  { __LINE__, "%+lli", "%+vlli", "%+llvi" },
  { __LINE__, "% lli", "% vlli", "% llvi" },
  { __LINE__, "%#lli", "%#vlli", "%#llvi" },
  { __LINE__, "%'lli", "%'vlli", "%'llvi" },
  { __LINE__, "%0lli", "%0vlli", "%0llvi" },
  { __LINE__, "%li",  "%vlli",  "%llvi" },
  { __LINE__, "%-li", "%-vlli", "%-llvi" },
  { __LINE__, "%+li", "%+vlli", "%+llvi" },
  { __LINE__, "% li", "% vlli", "% llvi" },
  { __LINE__, "%#li", "%#vlli", "%#llvi" },
  { __LINE__, "%'li", "%'vlli", "%'llvi" },
  { __LINE__, "%0li", "%0vlli", "%0llvi" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+lli", "%-+vlli", "%-+llvi" },
  { __LINE__, "%- lli", "%- vlli", "%- llvi" },
  { __LINE__, "%-#lli", "%-#vlli", "%-#llvi" },
  { __LINE__, "%-'lli", "%-'vlli", "%-'llvi" },
  { __LINE__, "%-0lli", "%-0vlli", "%-0llvi" },
  { __LINE__, "%+ lli", "%+ vlli", "%+ llvi" },
  { __LINE__, "%+#lli", "%+#vlli", "%+#llvi" },
  { __LINE__, "%+'lli", "%+'vlli", "%+'llvi" },
  { __LINE__, "%+0lli", "%+0vlli", "%+0llvi" },
  { __LINE__, "% #lli", "% #vlli", "% #llvi" },
  { __LINE__, "% 'lli", "% 'vlli", "% 'llvi" },
  { __LINE__, "% 0lli", "% 0vlli", "% 0llvi" },
  { __LINE__, "%#'lli", "%#'vlli", "%#'llvi" },
  { __LINE__, "%#0lli", "%#0vlli", "%#0llvi" },
  { __LINE__, "%'0lli", "%'0vlli", "%'0llvi" },
  { __LINE__, "%-+li", "%-+vlli", "%-+llvi" },
  { __LINE__, "%- li", "%- vlli", "%- llvi" },
  { __LINE__, "%-#li", "%-#vlli", "%-#llvi" },
  { __LINE__, "%-'li", "%-'vlli", "%-'llvi" },
  { __LINE__, "%-0li", "%-0vlli", "%-0llvi" },
  { __LINE__, "%+ li", "%+ vlli", "%+ llvi" },
  { __LINE__, "%+#li", "%+#vlli", "%+#llvi" },
  { __LINE__, "%+'li", "%+'vlli", "%+'llvi" },
  { __LINE__, "%+0li", "%+0vlli", "%+0llvi" },
  { __LINE__, "% #li", "% #vlli", "% #llvi" },
  { __LINE__, "% 'li", "% 'vlli", "% 'llvi" },
  { __LINE__, "% 0li", "% 0vlli", "% 0llvi" },
  { __LINE__, "%#'li", "%#'vlli", "%#'llvi" },
  { __LINE__, "%#0li", "%#0vlli", "%#0llvi" },
  { __LINE__, "%'0li", "%'0vlli", "%'0llvi" },

  /* Basic flags with precision. */
  { __LINE__, "%.5lli",  "%.5vlli", "%.5llvi" },
  { __LINE__, "%-.5lli", "%-.5vlli", "%-.5llvi" },
  { __LINE__, "%+.5lli", "%+.5vlli", "%+.5llvi" },
  { __LINE__, "% .5lli", "% .5vlli", "% .5llvi" },
  { __LINE__, "%#.5lli", "%#.5vlli", "%#.5llvi" },
  { __LINE__, "%'.5lli", "%'.5vlli", "%'.5llvi" },
  { __LINE__, "%0.5lli", "%0.5vlli", "%0.5llvi" },
  { __LINE__, "%.5li",  "%.5vlli", "%.5llvi" },
  { __LINE__, "%-.5li", "%-.5vlli", "%-.5llvi" },
  { __LINE__, "%+.5li", "%+.5vlli", "%+.5llvi" },
  { __LINE__, "% .5li", "% .5vlli", "% .5llvi" },
  { __LINE__, "%#.5li", "%#.5vlli", "%#.5llvi" },
  { __LINE__, "%'.5li", "%'.5vlli", "%'.5llvi" },
  { __LINE__, "%0.5li", "%0.5vlli", "%0.5llvi" },

  /* Basic flags with field width. */
  { __LINE__, "%12lli",  "%12vlli", "%12llvi" },
  { __LINE__, "%-12lli", "%-12vlli", "%-12llvi" },
  { __LINE__, "%+12lli", "%+12vlli", "%+12llvi" },
  { __LINE__, "% 12lli", "% 12vlli", "% 12llvi" },
  { __LINE__, "%#12lli", "%#12vlli", "%#12llvi" },
  { __LINE__, "%'12lli", "%'12vlli", "%'12llvi" },
  { __LINE__, "%012lli", "%012vlli", "%012llvi" },
  { __LINE__, "%12li",  "%12vlli", "%12llvi" },
  { __LINE__, "%-12li", "%-12vlli", "%-12llvi" },
  { __LINE__, "%+12li", "%+12vlli", "%+12llvi" },
  { __LINE__, "% 12li", "% 12vlli", "% 12llvi" },
  { __LINE__, "%#12li", "%#12vlli", "%#12llvi" },
  { __LINE__, "%'12li", "%'12vlli", "%'12llvi" },
  { __LINE__, "%012li", "%012vlli", "%012llvi" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%15.7lli",  "%15.7vlli", "%15.7llvi" },
  { __LINE__, "%-15.7lli", "%-15.7vlli", "%-15.7llvi" },
  { __LINE__, "%+15.7lli", "%+15.7vlli", "%+15.7llvi" },
  { __LINE__, "% 15.7lli", "% 15.7vlli", "% 15.7llvi" },
  { __LINE__, "%#15.7lli", "%#15.7vlli", "%#15.7llvi" },
  { __LINE__, "%'15.7lli", "%'15.7vlli", "%'15.7llvi" },
  { __LINE__, "%015.7lli", "%015.7vlli", "%015.7llvi" },
  { __LINE__, "%15.7li",  "%15.7vlli", "%15.7llvi" },
  { __LINE__, "%-15.7li", "%-15.7vlli", "%-15.7llvi" },
  { __LINE__, "%+15.7li", "%+15.7vlli", "%+15.7llvi" },
  { __LINE__, "% 15.7li", "% 15.7vlli", "% 15.7llvi" },
  { __LINE__, "%#15.7li", "%#15.7vlli", "%#15.7llvi" },
  { __LINE__, "%'15.7li", "%'15.7vlli", "%'15.7llvi" },
  { __LINE__, "%015.7li", "%015.7vlli", "%015.7llvi" },

  { 0, NULL, NULL, NULL }
};

#endif

format_specifiers char_tests[] =
{
  { __LINE__, "%c",  "%vc", NULL },
  { 0, NULL, NULL, NULL }
};

format_specifiers unsigned_char_tests[] =
{
  { __LINE__, "%hho",  "%vo", NULL },
  { __LINE__, "%hhu",  "%vu", NULL },
  { __LINE__, "%hhx",  "%vx", NULL },
  { __LINE__, "%hhX",  "%vX", NULL },
  { 0, NULL, NULL, NULL }
};

format_specifiers signed_char_tests[] =
{
  { __LINE__, "%hhd",  "%vd", NULL },
  { __LINE__, "%hhi",  "%vi", NULL },
  { 0, NULL, NULL, NULL }
};

#ifdef HAVE_INT128_T
format_specifiers int128_tests[] =
{
  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%lx",  "%vzx", "%zvx" },
  { __LINE__, "%ld",  "%vzd", "%zvd" },
  { __LINE__, "%lu",  "%vzu", "%zvu" },
  { __LINE__, "%li",  "%vzi", "%zvi" },
  { __LINE__, "%lX",  "%vzX", "%zvX" },
  { __LINE__, "%lo",  "%vzo", "%zvo" },
  { 0, NULL, NULL, NULL }
};
#endif

void
gen_cmp_str (int data_type, void* data, const char *format, char *output)
{
  int i;
  memset (output, 0x0, 1024);

  switch (data_type) {
    case VDT_unsigned_int:
      to_string (unsigned int, format, data, output);
      break;
    case VDT_signed_int:
      to_string (signed int, format, data, output);
      break;
    case VDT_unsigned_short:
      to_string (unsigned short, format, data, output);
      break;
    case VDT_signed_short:
      to_string (signed short, format, data, output);
      break;
    case VDT_unsigned_char:
      for (i = 0; i < 15; i++)
        {
          output += sprintf (output, format, ((unsigned char*)data)[i]);
          /* Kind of a hack.  Since the 'c' conversion specifier doesn't get a
           * separator but 'd' and 'i' do we have to do a character check.  */
          if (format[1] != 'c')
              strcat (output++, " ");
        }
      sprintf (output, format, ((unsigned char*)data)[15]);
      break;
    case VDT_signed_char:
      to_string (signed char, format, data, output);
      break;
    case VDT_float:
      to_string (float, format, data, output);
      break;
#ifdef __VSX__
    case VDT_double:
      to_string (double, format, data, output);
      break;
    case VDT_signed_long:
      to_string (signed long, format, data, output);
      break;
    case VDT_unsigned_long:
      to_string (unsigned long, format, data, output);
      break;
    case VDT_signed_long_long:
      to_string (signed long long, format, data, output);
      break;
    case VDT_unsigned_long_long:
      to_string (unsigned long long, format, data, output);
      break;
#endif
#ifdef HAVE_INT128_T
    case VDT_int128:
# ifdef __LITTLE_ENDIAN__
      output += sprintf (output, format, ((unsigned long long*) data)[1]);
      sprintf (output, format, ((unsigned long long*) data)[0]);
# else
      output += sprintf (output, format, ((unsigned long long*) data)[0]);
      sprintf (output, format, ((unsigned long long*) data)[1]);
# endif
      break;
#endif
  }
}

void
compare (int src_line, const char* expected, const char* actual)
{
  if (strcmp(expected, actual)) {
    fprintf (stderr, "Error:   Expected: \"%s\", got \"%s\"  source: %s:%d\n", expected, actual, __FILE__, src_line);
    failed++;
  }
  else if (verbose) {
    fprintf (stdout, "Success: Expected: \"%s\", got \"%s\"  source: %s:%d\n", expected, actual, __FILE__, src_line);
  }

  test_count++;
}

int
main (int argc, char *argv[])
{
  format_specifiers *ptr;

  puts ("\nUnsigned 32 bit integer tests.\n");
  test(uint32_tests, VDT_unsigned_int, UINT32_TEST_VECTOR)

  puts ("\nSigned 32 bit integer tests.\n");
  test(int32_tests, VDT_signed_int, INT32_TEST_VECTOR)

  puts ("\nUnsigned 16 bit integer tests.\n");
  test(uint16_tests, VDT_unsigned_short, UINT16_TEST_VECTOR)

  puts ("\nSigned 16 bit integer tests.\n");
  test(int16_tests, VDT_signed_short, INT16_TEST_VECTOR)

  puts ("\nFloat tests.\n");
  test(float_tests, VDT_float, FLOAT_TEST_VECTOR)

  puts ("\nChar tests - test 'character' ouput.\n");
  test(char_tests, VDT_unsigned_char, CHAR_TEST_VECTOR)

  puts ("\nUnsigned Char tests - test 0 - 255.\n");
  test(unsigned_char_tests, VDT_unsigned_char, UNSIGNED_CHAR_TEST_VECTOR)

  puts ("\nSigned Char tests - test -128 - 127.\n");
  test(signed_char_tests, VDT_signed_char, SIGNED_CHAR_TEST_VECTOR)

#ifdef HAVE_INT128_T
  puts ("\nint128 tests.\n");
  test(int128_tests, VDT_int128, INT128_TEST_VECTOR)
#endif

#ifdef __VSX__
  puts ("\nDouble tests (VSX).\n");
  test(double_tests, VDT_double, DOUBLE_TEST_VECTOR)

  puts ("\nSigned 64 bit integer tests (signed long long).\n");
  test(int64_tests, VDT_signed_long, INT64_TEST_VECTOR)

  puts ("\nUnsigned 64 bit integer tests (unsigned long long).\n");
  test(uint64_tests, VDT_unsigned_long, UINT64_TEST_VECTOR)

  puts ("\nSigned 64 bit integer tests (signed long).\n");
  test(int64_tests, VDT_signed_long, INT64_TEST_VECTOR_2)

  puts ("\nUnsigned 64 bit integer tests (unsigned long).\n");
  test(uint64_tests, VDT_unsigned_long, UINT64_TEST_VECTOR_2)
#endif

  if (failed) {
    fprintf (stderr, "\nWarning: %d tests failed!\n", failed);
    return 1;
  }

  printf ("\nAll tests passed (%d tests).\n", test_count);
  return 0;
}
