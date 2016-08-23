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
       'vl', 'vh', 'lv', 'hv', 'v'

     Valid modifiers and conversions (all else are undefined):

         vl or lv: integer conversions; vectors are composed of four byte vals
         vh or hv: integer conversions; vectors are composed of two byte vals
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

static int test_count = 0, failed = 0;

char expected_output[1024], actual_output[1024];

/* Tests are organized by data type.  Try to print a vector using a variety
   of different flags, precisions and conversions. */


/* Unsigned 32 bit word tests */

typedef struct {
  int src_line;                /* What line of code is the test on? */
  const char *format1;         /* Format string to use for standard scalars */
  const char *format2;         /* Format string to use for vectors */
  const char *format3;         /* Alternate form, if applicable */
} uint32_type;


#define UINT32_TEST_VECTOR { 4294967295U, 0, 39, 2147483647 }

uint32_type uint32_tests[] =
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



typedef struct {
  int src_line;
  const char *format1;
  const char *format2;
  const char *format3;
} int32_type;

#define INT32_TEST_VECTOR { (-INT_MAX - 1), 0, 39, 2147483647 }

int32_type int32_tests[] =
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




/* Unsigned 16 bit word tests */

typedef struct {
  int src_line;
  const char *format1;
  const char *format2;
  const char *format3;
} uint16_type;

#define UINT16_TEST_VECTOR { 65535U, 0, 39, 42, 101, 16384, 32767, 32768 }

uint16_type uint16_tests[] =
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



typedef struct {
  int src_line;
  const char *format1;
  const char *format2;
  const char *format3;
} int16_type;

#define INT16_TEST_VECTOR { (-SHRT_MAX -1), -127, -1, 0, 127, 256, 16384, SHRT_MAX }

int16_type int16_tests[] =
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

typedef struct {
  int src_line;
  const char *format1;
  const char *format2;
} float_type;

#define FLOAT_TEST_VECTOR { -(11.0f/9.0f), 0.123456789f, 42.0f, 9876543210.123456789f }

float_type float_tests[] =
{

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%f",  "%vf" },
  { __LINE__, "%-f", "%-vf" },
  { __LINE__, "%+f", "%+vf" },
  { __LINE__, "% f", "% vf" },
  { __LINE__, "%#f", "%#vf" },
  { __LINE__, "%'f", "%'vf" },
  { __LINE__, "%0f", "%0vf" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+f", "%-+vf" },
  { __LINE__, "%- f", "%- vf" },
  { __LINE__, "%-#f", "%-#vf" },
  { __LINE__, "%-'f", "%-'vf" },
  { __LINE__, "%-0f", "%-0vf" },
  { __LINE__, "%+ f", "%+ vf" },
  { __LINE__, "%+#f", "%+#vf" },
  { __LINE__, "%+'f", "%+'vf" },
  { __LINE__, "%+0f", "%+0vf" },
  { __LINE__, "% #f", "% #vf" },
  { __LINE__, "% 'f", "% 'vf" },
  { __LINE__, "% 0f", "% 0vf" },
  { __LINE__, "%#'f", "%#'vf" },
  { __LINE__, "%#0f", "%#0vf" },
  { __LINE__, "%'0f", "%'0vf" },

  /* Basic flags with precision. */
  { __LINE__, "%.9f",  "%.9vf" },
  { __LINE__, "%-.9f",  "%-.9vf" },
  { __LINE__, "%+.9f",  "%+.9vf" },
  { __LINE__, "% .9f",  "% .9vf" },
  { __LINE__, "%#.9f",  "%#.9vf" },
  { __LINE__, "%'.9f",  "%'.9vf" },
  { __LINE__, "%0.9f",  "%0.9vf" },

  /* Basic flags with field width. */
  { __LINE__, "%20f",  "%20vf" },
  { __LINE__, "%-20f",  "%-20vf" },
  { __LINE__, "%+20f",  "%+20vf" },
  { __LINE__, "% 20f",  "% 20vf" },
  { __LINE__, "%#20f",  "%#20vf" },
  { __LINE__, "%'20f",  "%'20vf" },
  { __LINE__, "%020f",  "%020vf" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%25.3f",  "%25.3vf" },
  { __LINE__, "%-25.3f",  "%-25.3vf" },
  { __LINE__, "%+25.3f",  "%+25.3vf" },
  { __LINE__, "% 25.3f",  "% 25.3vf" },
  { __LINE__, "%#25.3f",  "%#25.3vf" },
  { __LINE__, "%'25.3f",  "%'25.3vf" },
  { __LINE__, "%025.3f",  "%025.3vf" },

  /* By this point the code that handles flags, field width and precision
     probably works.  Go for the other conversions on unsigned integers. */

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%e",  "%ve" },
  { __LINE__, "%-e", "%-ve" },
  { __LINE__, "%+e", "%+ve" },
  { __LINE__, "% e", "% ve" },
  { __LINE__, "%#e", "%#ve" },
  { __LINE__, "%'e", "%'ve" },
  { __LINE__, "%0e", "%0ve" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+e", "%-+ve" },
  { __LINE__, "%- e", "%- ve" },
  { __LINE__, "%-#e", "%-#ve" },
  { __LINE__, "%-'e", "%-'ve" },
  { __LINE__, "%-0e", "%-0ve" },
  { __LINE__, "%+ e", "%+ ve" },
  { __LINE__, "%+#e", "%+#ve" },
  { __LINE__, "%+'e", "%+'ve" },
  { __LINE__, "%+0e", "%+0ve" },
  { __LINE__, "% #e", "% #ve" },
  { __LINE__, "% 'e", "% 've" },
  { __LINE__, "% 0e", "% 0ve" },
  { __LINE__, "%#'e", "%#'ve" },
  { __LINE__, "%#0e", "%#0ve" },
  { __LINE__, "%'0e", "%'0ve" },

  /* Basic flags with precision. */
  { __LINE__, "%.9e",  "%.9ve" },
  { __LINE__, "%-.9e",  "%-.9ve" },
  { __LINE__, "%+.9e",  "%+.9ve" },
  { __LINE__, "% .9e",  "% .9ve" },
  { __LINE__, "%#.9e",  "%#.9ve" },
  { __LINE__, "%'.9e",  "%'.9ve" },
  { __LINE__, "%0.9e",  "%0.9ve" },

  /* Basic flags with field width. */
  { __LINE__, "%20e",  "%20ve" },
  { __LINE__, "%-20e",  "%-20ve" },
  { __LINE__, "%+20e",  "%+20ve" },
  { __LINE__, "% 20e",  "% 20ve" },
  { __LINE__, "%#20e",  "%#20ve" },
  { __LINE__, "%'20e",  "%'20ve" },
  { __LINE__, "%020e",  "%020ve" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%25.3e",  "%25.3ve" },
  { __LINE__, "%-25.3e",  "%-25.3ve" },
  { __LINE__, "%+25.3e",  "%+25.3ve" },
  { __LINE__, "% 25.3e",  "% 25.3ve" },
  { __LINE__, "%#25.3e",  "%#25.3ve" },
  { __LINE__, "%'25.3e",  "%'25.3ve" },
  { __LINE__, "%025.3e",  "%025.3ve" },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%E",  "%vE" },
  { __LINE__, "%-E", "%-vE" },
  { __LINE__, "%+E", "%+vE" },
  { __LINE__, "% E", "% vE" },
  { __LINE__, "%#E", "%#vE" },
  { __LINE__, "%'E", "%'vE" },
  { __LINE__, "%0E", "%0vE" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+E", "%-+vE" },
  { __LINE__, "%- E", "%- vE" },
  { __LINE__, "%-#E", "%-#vE" },
  { __LINE__, "%-'E", "%-'vE" },
  { __LINE__, "%-0E", "%-0vE" },
  { __LINE__, "%+ E", "%+ vE" },
  { __LINE__, "%+#E", "%+#vE" },
  { __LINE__, "%+'E", "%+'vE" },
  { __LINE__, "%+0E", "%+0vE" },
  { __LINE__, "% #E", "% #vE" },
  { __LINE__, "% 'E", "% 'vE" },
  { __LINE__, "% 0E", "% 0vE" },
  { __LINE__, "%#'E", "%#'vE" },
  { __LINE__, "%#0E", "%#0vE" },
  { __LINE__, "%'0E", "%'0vE" },

  /* Basic flags with precision. */
  { __LINE__, "%.9E",  "%.9vE" },
  { __LINE__, "%-.9E",  "%-.9vE" },
  { __LINE__, "%+.9E",  "%+.9vE" },
  { __LINE__, "% .9E",  "% .9vE" },
  { __LINE__, "%#.9E",  "%#.9vE" },
  { __LINE__, "%'.9E",  "%'.9vE" },
  { __LINE__, "%0.9E",  "%0.9vE" },

  /* Basic flags with field width. */
  { __LINE__, "%20E",  "%20vE" },
  { __LINE__, "%-20E",  "%-20vE" },
  { __LINE__, "%+20E",  "%+20vE" },
  { __LINE__, "% 20E",  "% 20vE" },
  { __LINE__, "%#20E",  "%#20vE" },
  { __LINE__, "%'20E",  "%'20vE" },
  { __LINE__, "%020E",  "%020vE" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%25.3E",  "%25.3vE" },
  { __LINE__, "%-25.3E",  "%-25.3vE" },
  { __LINE__, "%+25.3E",  "%+25.3vE" },
  { __LINE__, "% 25.3E",  "% 25.3vE" },
  { __LINE__, "%#25.3E",  "%#25.3vE" },
  { __LINE__, "%'25.3E",  "%'25.3vE" },
  { __LINE__, "%025.3E",  "%025.3vE" },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%g",  "%vg" },
  { __LINE__, "%-g", "%-vg" },
  { __LINE__, "%+g", "%+vg" },
  { __LINE__, "% g", "% vg" },
  { __LINE__, "%#g", "%#vg" },
  { __LINE__, "%'g", "%'vg" },
  { __LINE__, "%0g", "%0vg" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+g", "%-+vg" },
  { __LINE__, "%- g", "%- vg" },
  { __LINE__, "%-#g", "%-#vg" },
  { __LINE__, "%-'g", "%-'vg" },
  { __LINE__, "%-0g", "%-0vg" },
  { __LINE__, "%+ g", "%+ vg" },
  { __LINE__, "%+#g", "%+#vg" },
  { __LINE__, "%+'g", "%+'vg" },
  { __LINE__, "%+0g", "%+0vg" },
  { __LINE__, "% #g", "% #vg" },
  { __LINE__, "% 'g", "% 'vg" },
  { __LINE__, "% 0g", "% 0vg" },
  { __LINE__, "%#'g", "%#'vg" },
  { __LINE__, "%#0g", "%#0vg" },
  { __LINE__, "%'0g", "%'0vg" },

  /* Basic flags with precision. */
  { __LINE__, "%.9g",  "%.9vg" },
  { __LINE__, "%-.9g",  "%-.9vg" },
  { __LINE__, "%+.9g",  "%+.9vg" },
  { __LINE__, "% .9g",  "% .9vg" },
  { __LINE__, "%#.9g",  "%#.9vg" },
  { __LINE__, "%'.9g",  "%'.9vg" },
  { __LINE__, "%0.9g",  "%0.9vg" },

  /* Basic flags with field width. */
  { __LINE__, "%20g",  "%20vg" },
  { __LINE__, "%-20g",  "%-20vg" },
  { __LINE__, "%+20g",  "%+20vg" },
  { __LINE__, "% 20g",  "% 20vg" },
  { __LINE__, "%#20g",  "%#20vg" },
  { __LINE__, "%'20g",  "%'20vg" },
  { __LINE__, "%020g",  "%020vg" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%25.3g",  "%25.3vg" },
  { __LINE__, "%-25.3g",  "%-25.3vg" },
  { __LINE__, "%+25.3g",  "%+25.3vg" },
  { __LINE__, "% 25.3g",  "% 25.3vg" },
  { __LINE__, "%#25.3g",  "%#25.3vg" },
  { __LINE__, "%'25.3g",  "%'25.3vg" },
  { __LINE__, "%025.3g",  "%025.3vg" },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%G",  "%vG" },
  { __LINE__, "%-G", "%-vG" },
  { __LINE__, "%+G", "%+vG" },
  { __LINE__, "% G", "% vG" },
  { __LINE__, "%#G", "%#vG" },
  { __LINE__, "%'G", "%'vG" },
  { __LINE__, "%0G", "%0vG" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+G", "%-+vG" },
  { __LINE__, "%- G", "%- vG" },
  { __LINE__, "%-#G", "%-#vG" },
  { __LINE__, "%-'G", "%-'vG" },
  { __LINE__, "%-0G", "%-0vG" },
  { __LINE__, "%+ G", "%+ vG" },
  { __LINE__, "%+#G", "%+#vG" },
  { __LINE__, "%+'G", "%+'vG" },
  { __LINE__, "%+0G", "%+0vG" },
  { __LINE__, "% #G", "% #vG" },
  { __LINE__, "% 'G", "% 'vG" },
  { __LINE__, "% 0G", "% 0vG" },
  { __LINE__, "%#'G", "%#'vG" },
  { __LINE__, "%#0G", "%#0vG" },
  { __LINE__, "%'0G", "%'0vG" },

  /* Basic flags with precision. */
  { __LINE__, "%.9G",  "%.9vG" },
  { __LINE__, "%-.9G",  "%-.9vG" },
  { __LINE__, "%+.9G",  "%+.9vG" },
  { __LINE__, "% .9G",  "% .9vG" },
  { __LINE__, "%#.9G",  "%#.9vG" },
  { __LINE__, "%'.9G",  "%'.9vG" },
  { __LINE__, "%0.9G",  "%0.9vG" },

  /* Basic flags with field width. */
  { __LINE__, "%20G",  "%20vG" },
  { __LINE__, "%-20G",  "%-20vG" },
  { __LINE__, "%+20G",  "%+20vG" },
  { __LINE__, "% 20G",  "% 20vG" },
  { __LINE__, "%#20G",  "%#20vG" },
  { __LINE__, "%'20G",  "%'20vG" },
  { __LINE__, "%020G",  "%020vG" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%25.3G",  "%25.3vG" },
  { __LINE__, "%-25.3G",  "%-25.3vG" },
  { __LINE__, "%+25.3G",  "%+25.3vG" },
  { __LINE__, "% 25.3G",  "% 25.3vG" },
  { __LINE__, "%#25.3G",  "%#25.3vG" },
  { __LINE__, "%'25.3G",  "%'25.3vG" },
  { __LINE__, "%025.3G",  "%025.3vG" },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%a",  "%va" },
  { __LINE__, "%-a", "%-va" },
  { __LINE__, "%+a", "%+va" },
  { __LINE__, "% a", "% va" },
  { __LINE__, "%#a", "%#va" },
  { __LINE__, "%'a", "%'va" },
  { __LINE__, "%0a", "%0va" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+a", "%-+va" },
  { __LINE__, "%- a", "%- va" },
  { __LINE__, "%-#a", "%-#va" },
  { __LINE__, "%-'a", "%-'va" },
  { __LINE__, "%-0a", "%-0va" },
  { __LINE__, "%+ a", "%+ va" },
  { __LINE__, "%+#a", "%+#va" },
  { __LINE__, "%+'a", "%+'va" },
  { __LINE__, "%+0a", "%+0va" },
  { __LINE__, "% #a", "% #va" },
  { __LINE__, "% 'a", "% 'va" },
  { __LINE__, "% 0a", "% 0va" },
  { __LINE__, "%#'a", "%#'va" },
  { __LINE__, "%#0a", "%#0va" },
  { __LINE__, "%'0a", "%'0va" },

  /* Basic flags with precision. */
  { __LINE__, "%.9a",  "%.9va" },
  { __LINE__, "%-.9a",  "%-.9va" },
  { __LINE__, "%+.9a",  "%+.9va" },
  { __LINE__, "% .9a",  "% .9va" },
  { __LINE__, "%#.9a",  "%#.9va" },
  { __LINE__, "%'.9a",  "%'.9va" },
  { __LINE__, "%0.9a",  "%0.9va" },

  /* Basic flags with field width. */
  { __LINE__, "%20a",  "%20va" },
  { __LINE__, "%-20a",  "%-20va" },
  { __LINE__, "%+20a",  "%+20va" },
  { __LINE__, "% 20a",  "% 20va" },
  { __LINE__, "%#20a",  "%#20va" },
  { __LINE__, "%'20a",  "%'20va" },
  { __LINE__, "%020a",  "%020va" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%25.3a",  "%25.3va" },
  { __LINE__, "%-25.3a",  "%-25.3va" },
  { __LINE__, "%+25.3a",  "%+25.3va" },
  { __LINE__, "% 25.3a",  "% 25.3va" },
  { __LINE__, "%#25.3a",  "%#25.3va" },
  { __LINE__, "%'25.3a",  "%'25.3va" },
  { __LINE__, "%025.3a",  "%025.3va" },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%A",  "%vA" },
  { __LINE__, "%-A", "%-vA" },
  { __LINE__, "%+A", "%+vA" },
  { __LINE__, "% A", "% vA" },
  { __LINE__, "%#A", "%#vA" },
  { __LINE__, "%'A", "%'vA" },
  { __LINE__, "%0A", "%0vA" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+A", "%-+vA" },
  { __LINE__, "%- A", "%- vA" },
  { __LINE__, "%-#A", "%-#vA" },
  { __LINE__, "%-'A", "%-'vA" },
  { __LINE__, "%-0A", "%-0vA" },
  { __LINE__, "%+ A", "%+ vA" },
  { __LINE__, "%+#A", "%+#vA" },
  { __LINE__, "%+'A", "%+'vA" },
  { __LINE__, "%+0A", "%+0vA" },
  { __LINE__, "% #A", "% #vA" },
  { __LINE__, "% 'A", "% 'vA" },
  { __LINE__, "% 0A", "% 0vA" },
  { __LINE__, "%#'A", "%#'vA" },
  { __LINE__, "%#0A", "%#0vA" },
  { __LINE__, "%'0A", "%'0vA" },

  /* Basic flags with precision. */
  { __LINE__, "%.9A",  "%.9vA" },
  { __LINE__, "%-.9A",  "%-.9vA" },
  { __LINE__, "%+.9A",  "%+.9vA" },
  { __LINE__, "% .9A",  "% .9vA" },
  { __LINE__, "%#.9A",  "%#.9vA" },
  { __LINE__, "%'.9A",  "%'.9vA" },
  { __LINE__, "%0.9A",  "%0.9vA" },

  /* Basic flags with field width. */
  { __LINE__, "%20A",  "%20vA" },
  { __LINE__, "%-20A",  "%-20vA" },
  { __LINE__, "%+20A",  "%+20vA" },
  { __LINE__, "% 20A",  "% 20vA" },
  { __LINE__, "%#20A",  "%#20vA" },
  { __LINE__, "%'20A",  "%'20vA" },
  { __LINE__, "%020A",  "%020vA" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%25.3A",  "%25.3vA" },
  { __LINE__, "%-25.3A",  "%-25.3vA" },
  { __LINE__, "%+25.3A",  "%+25.3vA" },
  { __LINE__, "% 25.3A",  "% 25.3vA" },
  { __LINE__, "%#25.3A",  "%#25.3vA" },
  { __LINE__, "%'25.3A",  "%'25.3vA" },
  { __LINE__, "%025.3A",  "%025.3vA" },

  { 0, NULL, NULL }
};

#ifdef __VSX__
typedef struct {
  int src_line;
  const char *format1;
  const char *format2;
} double_type;

#define DOUBLE_TEST_VECTOR { -(11.0f/9.0f), 9876543210.123456789f }

double_type double_tests[] =
{
  { __LINE__, "%f",  "%vvf" },
  { __LINE__, "%-f",  "%-vvf" },
  { __LINE__, "%+f",  "%+vvf" },
  { __LINE__, "% f",  "% vvf" },
  { __LINE__, "%#f",  "%#vvf" },
  { __LINE__, "%'f",  "%'vvf" },
  { __LINE__, "%0f",  "%0vvf" },

  /* Basic flags.  Not all flags are supported with this data type. */
  { __LINE__, "%f",  "%vvf" },
  { __LINE__, "%-f", "%-vvf" },
  { __LINE__, "%+f", "%+vvf" },
  { __LINE__, "% f", "% vvf" },
  { __LINE__, "%#f", "%#vvf" },
  { __LINE__, "%'f", "%'vvf" },
  { __LINE__, "%0f", "%0vvf" },

  /* All combinations of two flags, some of which don't make sense. */
  { __LINE__, "%-+f", "%-+vvf" },
  { __LINE__, "%- f", "%- vvf" },
  { __LINE__, "%-#f", "%-#vvf" },
  { __LINE__, "%-'f", "%-'vvf" },
  { __LINE__, "%-0f", "%-0vvf" },
  { __LINE__, "%+ f", "%+ vvf" },
  { __LINE__, "%+#f", "%+#vvf" },
  { __LINE__, "%+'f", "%+'vvf" },
  { __LINE__, "%+0f", "%+0vvf" },
  { __LINE__, "% #f", "% #vvf" },
  { __LINE__, "% 'f", "% 'vvf" },
  { __LINE__, "% 0f", "% 0vvf" },
  { __LINE__, "%#'f", "%#'vvf" },
  { __LINE__, "%#0f", "%#0vvf" },
  { __LINE__, "%'0f", "%'0vvf" },

  /* Basic flags with precision. */
  { __LINE__, "%.9f",  "%.9vvf" },
  { __LINE__, "%-.9f",  "%-.9vvf" },
  { __LINE__, "%+.9f",  "%+.9vvf" },
  { __LINE__, "% .9f",  "% .9vvf" },
  { __LINE__, "%#.9f",  "%#.9vvf" },
  { __LINE__, "%'.9f",  "%'.9vvf" },
  { __LINE__, "%0.9f",  "%0.9vvf" },

  /* Basic flags with field width. */
  { __LINE__, "%20f",  "%20vvf" },
  { __LINE__, "%-20f",  "%-20vvf" },
  { __LINE__, "%+20f",  "%+20vvf" },
  { __LINE__, "% 20f",  "% 20vvf" },
  { __LINE__, "%#20f",  "%#20vvf" },
  { __LINE__, "%'20f",  "%'20vvf" },
  { __LINE__, "%020f",  "%020vvf" },

  /* Basic flags with field width and precision. */
  { __LINE__, "%25.3f",  "%25.3vvf" },
  { __LINE__, "%-25.3f",  "%-25.3vvf" },
  { __LINE__, "%+25.3f",  "%+25.3vvf" },
  { __LINE__, "% 25.3f",  "% 25.3vvf" },
  { __LINE__, "%#25.3f",  "%#25.3vvf" },
  { __LINE__, "%'25.3f",  "%'25.3vvf" },
  { __LINE__, "%025.3f",  "%025.3vvf" },

  { 0, NULL, NULL }
};
#endif

typedef struct {
  int src_line;
  const char *format1;
  const char *format2;
} char_type;

/* vector unsigned char - Test just the 'character' range.  */
#define CHAR_TEST_VECTOR { 't', 'h', 'i', 's', ' ', 's', 'p', 'a', 'c', 'e', ' ', 'i', 's', ' ', 'f', 'o'  }
char_type char_tests[] =
{
  { __LINE__, "%c",  "%vc" },
  { 0, NULL, NULL }
};

/* vector unsigned char - Test 0 - 255. */
#define UNSIGNED_CHAR_TEST_VECTOR { 't', 'h', 'i', 's', ' ', 's', 'p', 'a', 'c', 'e', ' ', 0, 15, 127, 128, 255  }
char_type unsigned_char_tests[] =
{
  { __LINE__, "%hho",  "%vo" },
  { __LINE__, "%hhu",  "%vu" },
  { __LINE__, "%hhx",  "%vx" },
  { __LINE__, "%hhX",  "%vX" },
  { 0, NULL, NULL }
};

/* vector signed char  */
#define SIGNED_CHAR_TEST_VECTOR { -128, -120, -99, -61, -43, -38, -1, 0, 1, 19, 76, 85, 10l, 123, 126, 127 }

char_type signed_char_tests[] =
{
  { __LINE__, "%hhd",  "%vd" },
  { __LINE__, "%hhi",  "%vi" },
  { 0, NULL, NULL }
};

#ifdef HAVE_INT128_T

typedef struct {
  int src_line;
  const char *format1;
  const char *format2;
  const char *format3;
} int128_type;

#define INT128_TEST_VECTOR  (vector __int128_t) { ((((__int128_t)-0x0123456789abcdefUL << 64)) + ((__int128_t)0xfedcba9876543210UL))}

int128_type int128_tests[] =
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
gen_cmp_str (int data_type, void *vec_ptr, const char *format_str, char *out_buffer, int out_buffer_len)
{
  int index = 0;
  int i;
  vp_u_t u;

  memset (out_buffer, 0x0, out_buffer_len);

  /* Copy the vector to our union */
  memcpy (&u.v, vec_ptr, sizeof (vp_u_t));

  switch (data_type) {

    case VDT_unsigned_int: {
      for (i = 0; i < 3; i++) {
        index += sprintf (out_buffer + index, format_str, u.ui[i]);
        strcat (out_buffer + index, " ");
        index++;
      }
      sprintf (out_buffer + index, format_str, u.ui[3]);
      break;
    }
#ifdef HAVE_INT128_T
    case VDT_int128: {
# ifdef __LITTLE_ENDIAN__
      index += sprintf (out_buffer, format_str, u.ul[1]);
      sprintf (out_buffer + index, format_str, u.ul[0]);
# else
       index += sprintf (out_buffer, format_str, u.ul[0]);
       sprintf (out_buffer + index, format_str, u.ul[1]);
# endif
       break;
    }
#endif
    case VDT_signed_int: {
      for (i = 0; i < 3; i++) {
        index += sprintf (out_buffer + index, format_str, u.si[i]);
        strcat (out_buffer + index, " ");
        index++;
      }
      sprintf (out_buffer + index, format_str, u.si[3]);
      break;
    }

    case VDT_unsigned_short: {
      for (i = 0; i < 7; i++) {
        index += sprintf (out_buffer + index, format_str, u.sh[i]);
        strcat (out_buffer + index, " ");
        index++;
      }
      sprintf (out_buffer + index, format_str, u.sh[7]);
      break;
    }

    case VDT_signed_short: {
      for (i = 0; i < 7; i++) {
        index += sprintf (out_buffer + index, format_str, u.uh[i]);
        strcat (out_buffer + index, " ");
        index++;
      }
      sprintf (out_buffer + index, format_str, u.uh[7]);
      break;
    }

    case VDT_unsigned_char: {
      for (i = 0; i < 15; i++) {
        index += sprintf (out_buffer + index, format_str, u.uc[i]);
        /* Kind of a hack.  Since the 'c' conversion specifier doesn't get a
         * separator but 'd' and 'i' do we have to do a character check.  */
        if (format_str[1] != 'c')
          {
            strcat (out_buffer + index, " ");
            index++;
          }
      }
      sprintf (out_buffer + index, format_str, u.uc[15]);
      break;
    }

    case VDT_signed_char: {
      for (i = 0; i < 15; i++) {
        index += sprintf (out_buffer + index, format_str, u.sc[i]);
        /* This doesn't support the 'c' conversion.  */
        strcat (out_buffer + index, " ");
        index++;
      }
      sprintf (out_buffer + index, format_str, u.sc[15]);
      break;
    }

    case VDT_float: {
      for (i = 0; i < 3; i++) {
        index += sprintf (out_buffer + index, format_str, u.f[i]);
        strcat (out_buffer + index, " ");
        index++;
      }
      sprintf (out_buffer + index, format_str, u.f[3]);
      break;
    }

#ifdef __VSX__
    case VDT_double: {
      index += sprintf (out_buffer, format_str, u.d[0]);
      strcat (out_buffer + index, " ");
      index++;
      sprintf (out_buffer + index, format_str, u.d[1]);
      break;
    }
#endif
  }
}

#define _COMPARE(src_line, expected, actual ) {\
  if (strcmp( expected, actual )  ) { \
    fprintf (stderr, "Error:   Expected: \"%s\", got \"%s\"  source: %s:%d\n", expected, actual, __FILE__, src_line); \
    failed++; \
  } \
  else { \
    if (Verbose) fprintf (stdout, "Success: Expected: \"%s\", got \"%s\"  source: %s:%d\n", expected, actual, __FILE__, src_line); \
  } \
}

int
main (int argc, char *argv[])
{
  int Verbose = 0;

  uint32_type *uint32_ptr;
  int32_type  *int32_ptr;
  uint16_type *uint16_ptr;
  int16_type  *int16_ptr;
  float_type  *float_ptr;
  char_type   *char_ptr;
#ifdef __VSX__
  double_type  *double_ptr;
#endif
#ifdef HAVE_INT128_T

  int128_type *int128_ptr;

  puts ("\nint128 tests\n");
  for (int128_ptr = int128_tests; int128_ptr->format1; int128_ptr++)
    {
      vector __int128_t val = INT128_TEST_VECTOR;
      gen_cmp_str (VDT_int128, &val, int128_ptr->format1, expected_output, 1024);
      sprintf (actual_output, int128_ptr->format2, val);
      _COMPARE (int128_ptr->src_line, expected_output, actual_output);
      test_count++;
      if (int128_ptr->format3) {
        sprintf (actual_output, int128_ptr->format3, val);
        _COMPARE (int128_ptr->src_line, expected_output, actual_output);
        test_count++;
      }
    }
#endif

  puts ("\nUnsigned 32 bit integer tests\n");
  for (uint32_ptr = uint32_tests; uint32_ptr->format1; uint32_ptr++)
    {
      vector unsigned int val = UINT32_TEST_VECTOR;
      gen_cmp_str (VDT_unsigned_int, &val, uint32_ptr->format1, expected_output, 1024);
      sprintf (actual_output, uint32_ptr->format2, val);
      _COMPARE (uint32_ptr->src_line, expected_output, actual_output);
      test_count++;
      if (uint32_ptr->format3) {
        sprintf (actual_output, uint32_ptr->format3, val);
        _COMPARE (uint32_ptr->src_line, expected_output, actual_output);
        test_count++;
      }
    }

  puts ("\nSigned 32 bit integer tests\n");
  for (int32_ptr = int32_tests; int32_ptr->format1; int32_ptr++)
    {
      vector signed int val = INT32_TEST_VECTOR;
      gen_cmp_str (VDT_signed_int, &val, int32_ptr->format1, expected_output, 1024);
      sprintf (actual_output, int32_ptr->format2, val);
      _COMPARE (int32_ptr->src_line, expected_output, actual_output);
      test_count++;
      if (int32_ptr->format3) {
        sprintf (actual_output, int32_ptr->format3, val);
        _COMPARE (int32_ptr->src_line, expected_output, actual_output);
        test_count++;
      }
    }

  puts ("\nUnsigned 16 bit integer tests\n");
  for (uint16_ptr = uint16_tests; uint16_ptr->format1; uint16_ptr++)
    {
      vector unsigned short val = UINT16_TEST_VECTOR;
      gen_cmp_str (VDT_unsigned_short, &val, uint16_ptr->format1, expected_output, 1024);
      sprintf (actual_output, uint16_ptr->format2, val);
      _COMPARE (uint16_ptr->src_line, expected_output, actual_output);
      test_count++;
      if (uint16_ptr->format3) {
        sprintf (actual_output, uint16_ptr->format3, val);
        _COMPARE (uint16_ptr->src_line, expected_output, actual_output);
        test_count++;
      }
    }

  puts ("\nSigned 16 bit integer tests\n");
  for (int16_ptr = int16_tests; int16_ptr->format1; int16_ptr++)
    {
      vector signed short val = INT16_TEST_VECTOR;
      gen_cmp_str (VDT_signed_short, &val, int16_ptr->format1, expected_output, 1024);
      sprintf (actual_output, int16_ptr->format2, val);
      _COMPARE (int16_ptr->src_line, expected_output, actual_output);
      test_count++;
      if (int16_ptr->format3) {
        sprintf (actual_output, int16_ptr->format3, val);
        _COMPARE (int16_ptr->src_line, expected_output, actual_output);
        test_count++;
      }
    }

  puts ("\nFloat tests\n");
  for (float_ptr = float_tests; float_ptr->format1; float_ptr++)
    {
      vector float val = FLOAT_TEST_VECTOR;
      gen_cmp_str (VDT_float, &val, float_ptr->format1, expected_output, 1024);
      sprintf (actual_output, float_ptr->format2, val);
      _COMPARE (float_ptr->src_line, expected_output, actual_output);
      test_count++;
    }

#ifdef __VSX__
  puts ("\nDouble tests (VSX)\n");
  for (double_ptr = double_tests; double_ptr->format1; double_ptr++)
    {
      vector double val = DOUBLE_TEST_VECTOR;
      gen_cmp_str (VDT_double, &val, double_ptr->format1, expected_output, 1024);
      sprintf (actual_output, double_ptr->format2, val);
      _COMPARE (double_ptr->src_line, expected_output, actual_output);
      test_count++;
    }
#endif

  puts ("\nChar tests - test 'character' ouput.\n");
  for (char_ptr = char_tests; char_ptr->format1; char_ptr++)
    {
      vector unsigned char val = CHAR_TEST_VECTOR;
      gen_cmp_str (VDT_unsigned_char, &val, char_ptr->format1, expected_output, 1024);
      sprintf (actual_output, char_ptr->format2, val);
      _COMPARE (char_ptr->src_line, expected_output, actual_output);
      test_count++;
    }

  puts ("\nUnsigned Char tests - test 0 - 255.\n");
  for (char_ptr = unsigned_char_tests; char_ptr->format1; char_ptr++)
    {
      vector unsigned char val = UNSIGNED_CHAR_TEST_VECTOR;
      gen_cmp_str (VDT_unsigned_char, &val, char_ptr->format1, expected_output, 1024);
      sprintf (actual_output, char_ptr->format2, val);
      _COMPARE (char_ptr->src_line, expected_output, actual_output);
      test_count++;
    }

  puts ("\nSigned Char tests - test -128 - 127.\n");
  for (char_ptr = signed_char_tests; char_ptr->format1; char_ptr++)
    {
      vector unsigned char val = SIGNED_CHAR_TEST_VECTOR;
      gen_cmp_str (VDT_unsigned_char, &val, char_ptr->format1, expected_output, 1024);
      sprintf (actual_output, char_ptr->format2, val);
      _COMPARE (char_ptr->src_line, expected_output, actual_output);
      test_count++;
    }

  /*
  puts ("\nLocale printing tests\n");
  setlocale(LC_NUMERIC, "en_US");
  /+ Rerun signed tests +/
  for (int32_ptr = int32_tests; int32_ptr->format1; int32_ptr++)
    {
      gen_cmp_str (VDT_signed_int, &int32_ptr->val, int32_ptr->format1, expected_output, 1024);
      sprintf (actual_output, int32_ptr->format2, int32_ptr->val);
      _COMPARE (int32_ptr->src_line, expected_output, actual_output);
      test_count++;
    }
  */

  if (failed)
    {
      fprintf (stderr, "\nWarning: %d tests failed!\n", failed);
    }
  else
    {
      printf ("\nAll tests passed (%d tests)\n", test_count);
    }

  if (failed) return 1;

  return 0;
}
