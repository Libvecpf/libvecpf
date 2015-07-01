#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# Version
m4_define([MAJOR], 1)
m4_define([MINOR], 0)
m4_define([PATCH], 0)


AC_INIT(libvecpf, [MAJOR.MINOR.PATCH],
	https://github.com/Libvecpf/libvecpf/issues)
AC_PREREQ(2.60)

#
# Automake initialization
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AM_MAINTAINER_MODE([disable])

AC_CONFIG_SRCDIR([vecpf.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Update this value for every release: (A:B:C will map to foo.so.(A-C).C.B)
# http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
LIBVECPF_SO_VERSION=MAJOR:MINOR:PATCH
AC_SUBST(LIBVECPF_SO_VERSION)


AC_USE_SYSTEM_EXTENSIONS([_GNU_SOURCE])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LN_S
AM_PROG_AR
AC_PROG_RANLIB
LT_INIT

#
# Pass libtool options to automake
AM_CONDITIONAL([TEST_SHARED], [test "x$enable_shared" = "xyes"])
AM_CONDITIONAL([TEST_STATIC], [test "x$enable_static" = "xyes"])

# Enable the use of non-default location of system headers
AC_ARG_WITH([headers],
	    AC_HELP_STRING([--with-headers=PATH],
			   [location of system headers to use (for example /usr/src/linux/include) @<:@default=compiler default@:>@]),
	    [sysheaders=$withval],
	    [sysheaders=])
AC_SUBST(sysheaders, [])
AM_CONDITIONAL([SYSHEADERS_SET], [test "x$sysheaders" != "x"])

# Checks for header files.
AC_CHECK_HEADERS([limits.h locale.h stdlib.h string.h printf.h altivec.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([memset])

AX_CFLAGS_WARN_ALL

# Check for GLIBC 2.10 headers
have_glibc_2_10_headers=yes

AC_MSG_CHECKING([for printf-hook register_printf_type () in printf.h to verify GLIBC 2.10])
AC_TRY_COMPILE([#include "printf.h"],[
 static int foo;
 void foo_va (void *mem, va_list *ap)
 {
   return;
 }
 foo = register_printf_type (foo_va);
 ], [AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no]); have_glibc_2_10_headers=no])

if test x$have_glibc_2_10_headers != xyes; then
  AC_MSG_ERROR([Building Libvecpf requires a GLIBC printf.h header that
		provides the register_printf_type () function which is
		available in GLIBC Version 2.10 or later.  The headers that
		this build is using don't provide this functionality.])
  exit 1;
fi

have_glibc_2_10=yes

# Check if the compiler supports vector instructions.
AX_APPEND_COMPILE_FLAGS([-maltivec], [CFLAGS])

# Check for vector __int128_t compiler support.
AC_CACHE_CHECK(for __int128_t, libvecpf_cv_int128_t,
 AC_TRY_COMPILE([ ], [
 vector __int128_t u = {0};
 return 0;
 ], [
 libvecpf_cv_int128_t=yes
 ], [
 libvecpf_cv_int128_t=no
 ])
)
if test x$libvecpf_cv_int128_t = xyes; then
 AC_DEFINE_UNQUOTED([HAVE_INT128_T], [1], ["Have vector __int128_t type"])
fi


AC_MSG_CHECKING([for register_printf_type() runtime support])
AC_TRY_LINK([#include "printf.h"],[
 static int foo;
 void foo_va (void *mem, va_list *ap)
 {
   return;
 }
 foo = register_printf_type (foo_va);
 ], [AC_MSG_RESULT([yes])], [AC_MSG_RESULT([no]); have_glibc_2_10=no])

if test x$have_glibc_2_10 != xyes; then
  AC_MSG_WARN([Executing Libdfp tests requires a GLIBC runtime that supports
	       the register_printf_type () function which is available in GLIBC
	       Version 2.10 or later.  The libc that this build is using
	       doesn't provide this functionality.  You will not be able to
	       successfully execute the test suite.])
fi

# Check if the compiler supports -Werror.
AX_APPEND_COMPILE_FLAGS([-Werror], [CFLAGS])

# It's ok to build the static lib with PIC.
picflag="-fpic"
AC_MSG_NOTICE(configuring with "$picflag")
AC_SUBST(picflag)

AC_DEFINE(_GNU_SOURCE)
AC_USE_SYSTEM_EXTENSIONS

AC_CONFIG_FILES([Makefile man/Makefile])
AC_OUTPUT
