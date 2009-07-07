#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# IMPORTANT: IF YOU CHANGE THESE VERSION NUMBERS YOU MUST
# ALSO CHANGE ANY CORRESPONDING VERSION NUMBERS IN THE
# JAVA build.xml FILE
m4_define([VSM4_LIB_MAJOR_VERSION], [3])
m4_define([VSM4_LIB_MINOR_VERSION], [1])

AC_INIT([xuggler-captive],[VSM4_LIB_MAJOR_VERSION.VSM4_LIB_MINOR_VERSION],[info@xuggle.com])
AC_PREREQ([2.63])
AM_INIT_AUTOMAKE([-Wall -Werror no-dependencies nostdinc tar-pax 1.10.2 ])
AM_MAINTAINER_MODE
AC_PREFIX_DEFAULT([${XUGGLE_HOME:-/usr/local}])

LIB_MAJOR_VERSION=VSM4_LIB_MAJOR_VERSION
LIB_MINOR_VERSION=VSM4_LIB_MINOR_VERSION
AC_SUBST([LIB_MAJOR_VERSION])
AC_SUBST([LIB_MINOR_VERSION])
AC_DEFINE(VS_LIB_MAJOR_VERSION, [VSM4_LIB_MAJOR_VERSION], [Major version of library])
AC_DEFINE(VS_LIB_MINOR_VERSION, [VSM4_LIB_MINOR_VERSION], [Minor version of library])

VS_CPPFLAGS=
VS_CFLAGS=
VS_CXXFLAGS=
VS_LDFLAGS=

AC_ARG_ENABLE([optimizations],
              [AS_HELP_STRING([--enable-optimizations],
                              [build libraries and executables with compiler optimizations (default is yes)])],
                              [],
                              [enable_optimizations=yes])
AC_MSG_CHECKING([whether to enable compiler optimizations])
AC_MSG_RESULT([$enable_optimizations])

AM_CONDITIONAL([VS_DEBUG], [ test "$x$enable_optimizations" = xno ])
if test "x$enable_optimizations" = xno; then
  VS_DEBUG=1
  AC_SUBST([VS_DEBUG])
fi

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_MSG_CHECKING([whether we saw a bunny])
AC_MSG_RESULT([no])

HOST_OS=$host
case $host in
  *cygwin*)
  HOST_TYPE=Windows
  ;;
  *mingw*)
  HOST_TYPE=Windows
  ;;
  *darwin*)
  HOST_TYPE=Mac
  ;;
  *)
  HOST_TYPE=Linux
  ;;
esac
enable_warningerrors=yes
case $HOST_TYPE in
  Windows)
  CC="gcc -mno-cygwin "
  CXX="g++ -mno-cygwin "
  # Don't ask.
  VS_LDFLAGS+=" -Wl,--add-stdcall-alias -Wl,--enable-runtime-pseudo-reloc -Wl,--enable-auto-image-base"
  HOST_SO_LIBPATH=[PATH]
  VS_OS_WINDOWS=1
  AC_SUBST([VS_OS_WINDOWS])
  dnl For libtool
  export need_lib_prefix=no
  dnl Turns out older versions of G++ on windows have warnings
  dnl in their std:: header files.  shame on them.
  enable_warningerrors=no
  ;;
  Mac)
  HOST_SO_LIBPATH=[DYLD_LIBRARY_PATH]
  ;;
  Linux)
  HOST_SO_LIBPATH=[LD_LIBRARY_PATH]
  ;;
esac

AC_SUBST([HOST_TYPE])
AC_SUBST([HOST_OS])
AC_SUBST([HOST_SO_LIBPATH])

LT_INIT([disable-static disable-fast-install win32-dll])
AC_MSG_CHECKING([whether we smell bunny poop])
AC_MSG_RESULT([no])

LT_PREREQ([2.2.6])
dnl AC_LIBTOOL_WIN32_DLL
dnl AC_PROG_LIBTOOL
dnl AM_PROG_LIBTOOL
AC_SUBST([LIBTOOL_DEPS])

# Do this BEFORE we start testing the copiler
AC_ARG_ENABLE([m64],
              [AS_HELP_STRING([--enable-m64],
                              [build 64 bit version of the library instead of 32 bit version (default is yes on OS-X, no everywhere else)])],
                              [],
                              [
                               case $HOST_TYPE in
                                 Mac)
                                  enable_m64=yes
                                  ;;
                                *)
                                  enable_m64=no
                                  ;;
                                esac
                                ])
AC_MSG_CHECKING([whether to build 64 bit version of package])
AC_MSG_RESULT([$enable_m64])
VS_ENABLE_64BIT=$enable_m64
AC_SUBST([VS_ENABLE_64BIT])
if test "x$enable_m64" = xyes; then
  AC_DEFINE(VS_ENABLE_64BIT, [1], [compiled for 64-bit OS support])
  VS_CFLAGS+=" -m64"
  VS_CXXFLAGS+=" -m64"
  VS_LDFLAGS+=" -m64"
  case $HOST_TYPE in
    Mac)
    # Mac requires an additional flag for 64 bit builds
    VS_CFLAGS+=" -arch x86_64"
    VS_CXXFLAGS+=" -arch x86_64"
    VS_LDFLAGS+=" -arch x86_64"
    ;;
    *)
    ;;
  esac
fi

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_LANG(C++)

AC_PATH_PROG([YASM],[yasm])
AC_ARG_VAR(YASM,
          [Location of the 'yasm' program; if found, we'll build with some cool assembly on x86/x86_64 systems.])
AM_CONDITIONAL(HAVE_YASM, [ test "x$YASM" != "x" ])

case $HOST_TYPE in
  Windows)
    AC_PATH_PROG([MSVC_LIB],[lib])
    if test "x$MSVC_LIB" = "x"; then
      AC_MSG_FAILURE([Could not find Microsoft Visual C++'s 'LIB' tool.  This is required to build FFmpeg.
                      You can download a free version from http://www.microsoft.com/express/download.
                      Also make sure you have defined the variables in vcvars32.bat before running configure.])
    fi
    ;;
  *)
    ;;
esac

AC_MSG_CHECKING([whether to compile FFmpeg with GPL V3 code])
AC_ARG_ENABLE([gpl],
              [AS_HELP_STRING([--enable-gpl],
                              [whether to enable GPL V3 dependent code (defaults to yes)])],
                              [],
                              [enable_gpl=yes])
VS_ENABLE_GPL=$enable_gpl
AC_MSG_RESULT([$enable_gpl])
if test "x$VS_ENABLE_GPL" = "xyes"; then
  AC_MSG_WARN([Compiling the GNU General Public License version of FFmpeg])
  AC_DEFINE(VS_ENABLE_GPL, 1, AC_PACKAGE_NAME [ will enable any code that depends on GPL code ])
else
  AC_MSG_WARN([ 

  vvvvvvvvvvvvvvvvvvvvv WARNING vvvvvvvvvvvvvvvvvvvvv

  FFmpeg in LGPL mode cannot use libx264 (H264 encoding),
  libfaac (AAC encoding), and several assembly optimizations

  ^^^^^^^^^^^^^^^^^^^^^ WARNING ^^^^^^^^^^^^^^^^^^^^^

  ])
fi
AC_SUBST(VS_ENABLE_GPL)
AM_CONDITIONAL(VS_ENABLE_GPL,
               [ test "x$VS_ENABLE_GPL" = "xyes" ])

#
# CAPTIVE building support
#
enable_captives=yes
AC_MSG_CHECKING([whether to use captive versions of dependent libraries])
AC_MSG_RESULT([$enable_captives])
CAPTIVES_ENABLED=yes
AC_SUBST([CAPTIVES_ENABLED])

#
# Captive ffmpeg support
#
enable_captive_ffmpeg=yes
AC_MSG_CHECKING([whether to use captive version of ffmpeg])
AC_MSG_RESULT([$enable_captive_ffmpeg])
AM_CONDITIONAL(CAPTIVE_ENABLE_FFMPEG,
               [ test "x$enable_captive_ffmpeg" = "xyes" ])
CAPTIVE_ENABLE_FFMPEG=$enable_captive_ffmpeg
AC_SUBST([CAPTIVE_ENABLE_FFMPEG])

#
# Captive libx264 support
#
AC_ARG_ENABLE([captive-libx264],
              [AS_HELP_STRING([--enable-captive-libx264],
                              [use our own captive versions of the X264 library encoder (default is yes)])],
                              [],
                              [enable_captive_libx264=yes])
AC_MSG_CHECKING([whether to use captive version of libx264])
if test "x$enable_captives" != "xyes"; then
  enable_captive_libx264=no
fi
if test "x$enable_captive_ffmpeg" != "xyes"; then
  # no point in making a captive libx264 if we're not
  # incarcerating ffmpeg
  enable_captive_libx264=no
fi
if test "x$VS_ENABLE_GPL" != "xyes"; then
  # can't use x264 unless you GPL this source
  AC_MSG_WARN([cannot use captive libx264 unless gpl is enabled])
  enable_captive_libx264=no
fi

AC_MSG_RESULT([$enable_captive_libx264])
AM_CONDITIONAL(CAPTIVE_ENABLE_LIBX264,
               [ test "x$enable_captive_libx264" = "xyes" ])
CAPTIVE_ENABLE_LIBX264=$enable_captive_libx264
AC_SUBST([CAPTIVE_ENABLE_LIBX264])

# X264 headers
if test "x$enable_captive_libx264" != "xyes"; then
  # We only check for x264 if we're not building it
AC_CHECK_HEADERS([x264.h], [HAVE_LIBX264=yes], [HAVE_LIBX264=no])
if test "x$HAVE_LIBX264" = xno; then
  AC_MSG_WARN([cannot find libx264; "ant run-tests" may not pass all tests.  To fix, recompile FFMPEG with X264 support.])
fi
fi


#
# Captive libmp3lame support
#
AC_ARG_ENABLE([captive-libmp3lame],
              [AS_HELP_STRING([--enable-captive-libmp3lame],
                              [use our own captive versions of the LAME mp3 encoder (default is yes)])],
                              [],
                              [enable_captive_libmp3lame=yes])
AC_MSG_CHECKING([whether to use captive version of libmp3lame])
if test "x$enable_captives" != "xyes"; then
  enable_captive_libmp3lame=no
fi
if test "x$enable_captive_ffmpeg" != "xyes"; then
  # no point in making a captive lame if we're not
  # incarcerating ffmpeg
  enable_captive_libmp3lame=no
fi
AC_MSG_RESULT([$enable_captive_libmp3lame])
AM_CONDITIONAL(CAPTIVE_ENABLE_LIBMP3LAME,
               [ test "x$enable_captive_libmp3lame" = "xyes" ])
CAPTIVE_ENABLE_LIBMP3LAME=$enable_captive_libmp3lame
AC_SUBST([CAPTIVE_ENABLE_LIBMP3LAME])

# LAME headers
if test "x$enable_captive_libmp3lame" != "xyes"; then
  # We only check for Lame if we're not building it
AC_CHECK_HEADERS([lame/lame.h], [HAVE_LIBMP3LAME=yes], [HAVE_LIBMP3LAME=no])
if test "x$HAVE_LIBMP3LAME" = xno; then
  AC_MSG_WARN([cannot find libmp3lame; "ant run-tests" may not pass all tests.  To fix, recompile FFMPEG with LAME support.])
fi
fi

#
# Captive libspeex support
#
AC_ARG_ENABLE([captive-libspeex],
              [AS_HELP_STRING([--enable-captive-libspeex],
                              [use our own captive versions of the Speex library encoder (default is yes)])],
                              [],
                              [enable_captive_libspeex=yes])
AC_MSG_CHECKING([whether to use captive version of libspeex])
if test "x$enable_captives" != "xyes"; then
  enable_captive_libspeex=no
fi
if test "x$enable_captive_ffmpeg" != "xyes"; then
  # no point in making a captive speex if we're not
  # incarcerating ffmpeg
  enable_captive_libspeex=no
fi

AC_MSG_RESULT([$enable_captive_libspeex])
AM_CONDITIONAL(CAPTIVE_ENABLE_LIBSPEEX,
               [ test "x$enable_captive_libspeex" = "xyes" ])
CAPTIVE_ENABLE_LIBSPEEX=$enable_captive_libspeex
AC_SUBST([CAPTIVE_ENABLE_LIBSPEEX])
# SPEEX headers
if test "x$enable_captive_libspeex" != "xyes"; then
  # We only check for Lame if we're not building it
AC_CHECK_HEADERS([speex/speex.h], [HAVE_LIBSPEEX=yes], [HAVE_LIBSPEEX=no])
if test "x$HAVE_LIBSPEEX" = xno; then
  AC_MSG_WARN([cannot find libspeex.  We will likely not be able to decode speex audio])
fi
fi

#
# Captive libfaac support
#
AC_ARG_ENABLE([captive-libfaac],
              [AS_HELP_STRING([--enable-captive-libfaac],
                              [use our own captive versions of the faac library encoder (default is yes)])],
                              [],
                              [enable_captive_libfaac=yes])
AC_MSG_CHECKING([whether to use captive version of libfaac])
if test "x$enable_captives" != "xyes"; then
  enable_captive_libfaac=no
fi
if test "x$enable_captive_ffmpeg" != "xyes"; then
  # no point in making a captive faac if we're not
  # incarcerating ffmpeg
  enable_captive_libfaac=no
fi

AC_MSG_RESULT([$enable_captive_libfaac])
AM_CONDITIONAL(CAPTIVE_ENABLE_LIBFAAC,
               [ test "x$enable_captive_libfaac" = "xyes" ])
if test "x$VS_ENABLE_GPL" != "xyes"; then
  # can't use faac unless you GPL this source
  AC_MSG_WARN([cannot use captive libfaac unless gpl is enabled])
  enable_captive_libfaac=no
fi

CAPTIVE_ENABLE_LIBFAAC=$enable_captive_libfaac
AC_SUBST([CAPTIVE_ENABLE_LIBFAAC])
# FAAC headers
if test "x$enable_captive_libfaac" != "xyes"; then
  # We only check for Lame if we're not building it
AC_CHECK_HEADERS([faac.h], [HAVE_LIBFAAC=yes], [HAVE_LIBFAAC=no])
if test "x$HAVE_LIBFAAC" = xno; then
  AC_MSG_WARN([cannot find libfaac.  We will likely not be able to encode aac audio])
fi
fi

#
# Captive libogg support
#
AC_ARG_ENABLE([captive-libogg],
              [AS_HELP_STRING([--enable-captive-libogg],
                              [use our own captive versions of the ogg library encoder (default is yes)])],
                              [],
                              [enable_captive_libogg=yes])
AC_MSG_CHECKING([whether to use captive version of libogg])
if test "x$enable_captives" != "xyes"; then
  enable_captive_libogg=no
fi
if test "x$enable_captive_ffmpeg" != "xyes"; then
  # no point in making a captive ogg if we're not
  # incarcerating ffmpeg
  enable_captive_libogg=no
fi

AC_MSG_RESULT([$enable_captive_libogg])
AM_CONDITIONAL(CAPTIVE_ENABLE_LIBOGG,
               [ test "x$enable_captive_libogg" = "xyes" ])
CAPTIVE_ENABLE_LIBOGG=$enable_captive_libogg
AC_SUBST([CAPTIVE_ENABLE_LIBOGG])
# OGG headers
if test "x$enable_captive_libogg" != "xyes"; then
  # We only check for Lame if we're not building it
AC_CHECK_HEADERS([ogg/ogg.h], [HAVE_LIBOGG=yes], [HAVE_LIBOGG=no])
if test "x$HAVE_LIBOGG" = xno; then
  AC_MSG_WARN([cannot find libogg.  We will likely not be able to encode vorbis video or theora audio])
fi
fi

#
# Captive libvorbis support
#
AC_ARG_ENABLE([captive-libvorbis],
              [AS_HELP_STRING([--enable-captive-libvorbis],
                              [use our own captive versions of the vorbis library encoder (default is yes)])],
                              [],
                              [enable_captive_libvorbis=yes])
AC_MSG_CHECKING([whether to use captive version of libvorbis])
if test "x$enable_captives" != "xyes"; then
  enable_captive_libvorbis=no
fi
if test "x$enable_captive_ffmpeg" != "xyes"; then
  # no point in making a captive vorbis if we're not
  # incarcerating ffmpeg
  enable_captive_libvorbis=no
fi

AC_MSG_RESULT([$enable_captive_libvorbis])
AM_CONDITIONAL(CAPTIVE_ENABLE_LIBVORBIS,
               [ test "x$enable_captive_libvorbis" = "xyes" ])
CAPTIVE_ENABLE_LIBVORBIS=$enable_captive_libvorbis
AC_SUBST([CAPTIVE_ENABLE_LIBVORBIS])
# vorbis headers
if test "x$enable_captive_libvorbis" != "xyes"; then
  # We only check for Lame if we're not building it
AC_CHECK_HEADERS([vorbis/vorbisenc.h], [HAVE_LIBVORBIS=yes], [HAVE_LIBVORBIS=no])
if test "x$HAVE_LIBVORBIS" = xno; then
  AC_MSG_WARN([cannot find libvorbis.  We will likely not be able to encode vorbis audio])
fi
fi

#
# Captive libtheora support
#
AC_ARG_ENABLE([captive-libtheora],
              [AS_HELP_STRING([--enable-captive-libtheora],
                              [use our own captive versions of the theora library encoder (default is yes)])],
                              [],
                              [enable_captive_libtheora=yes])
AC_MSG_CHECKING([whether to use captive version of libtheora])
if test "x$enable_captives" != "xyes"; then
  enable_captive_libtheora=no
fi
if test "x$enable_captive_ffmpeg" != "xyes"; then
  # no point in making a captive theora if we're not
  # incarcerating ffmpeg
  enable_captive_libtheora=no
fi

AC_MSG_RESULT([$enable_captive_libtheora])
AM_CONDITIONAL(CAPTIVE_ENABLE_LIBTHEORA,
               [ test "x$enable_captive_libtheora" = "xyes" ])
CAPTIVE_ENABLE_LIBTHEORA=$enable_captive_libtheora
AC_SUBST([CAPTIVE_ENABLE_LIBTHEORA])
# theora headers
if test "x$enable_captive_libtheora" != "xyes"; then
  # We only check for Lame if we're not building it
AC_CHECK_HEADERS([theora/theora.h], [HAVE_LIBTHEORA=yes], [HAVE_LIBTHEORA=no])
if test "x$HAVE_LIBTHEORA" = xno; then
  AC_MSG_WARN([cannot find libtheora.  We will likely not be able to encode theora audio])
fi
fi

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_FILES([Makefile
                 Makefile.global
                 libogg/Makefile
                 libogg/incarcerate
                 libvorbis/Makefile
                 libvorbis/incarcerate
                 libtheora/Makefile
                 libtheora/incarcerate
                 libspeex/Makefile
                 libspeex/incarcerate
                 libx264/Makefile
                 libx264/incarcerate
                 libmp3lame/Makefile
                 libmp3lame/incarcerate
                 libfaac/Makefile
                 libfaac/incarcerate
                 ffmpeg/Makefile
                 ffmpeg/incarcerate
                 ffmpeg/install.sh
                 ffmpeg/uninstall.sh
                 ])
AC_OUTPUT
