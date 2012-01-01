#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([opencore-amr], [0.1.2], [https://sourceforge.net/apps/trac/opencore-amr/])
AC_CONFIG_AUX_DIR(.)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([tar-ustar])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE

# Cross compiling support
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

# Various options for configure
AC_ARG_ENABLE([compile-c],
            [AS_HELP_STRING([--enable-compile-c],
               [enable compiling as C program (default is yes)])],
            [compile_as_c=$enableval],
            [compile_as_c=yes])

# Automake conditionals to set
AM_CONDITIONAL(COMPILE_AS_C, test x$compile_as_c = xyes)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Setup for libtool
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)

# Checks for libraries.
AC_CHECK_LIB([m], [main])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset])

# OpenCORE AMR soname version to use
# goes by ‘current[:revision[:age]]’ with the soname ending up as
# current.age.revision.
OPENCORE_AMRNB_VERSION=0:2:0
OPENCORE_AMRWB_VERSION=0:2:0
AC_SUBST(OPENCORE_AMRNB_VERSION)
AC_SUBST(OPENCORE_AMRWB_VERSION)

AC_CONFIG_FILES([Makefile
                 amrnb/Makefile
                 amrwb/Makefile
                 test/Makefile
                 amrnb/opencore-amrnb.pc
                 amrwb/opencore-amrwb.pc])
AC_OUTPUT
