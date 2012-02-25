ACLOCAL_AMFLAGS=-I m4
if VS_WINDOWS
 VS_LINK_LIBS = 
else
 VS_LINK_LIBS = -lpthread
endif

bin_PROGRAMS = rtmpdump rtmpgw rtmpsrv rtmpsuck

rtmpdump_SOURCES = \
  src/rtmpdump.c

rtmpdump_LDADD = librtmp.la

rtmpgw_SOURCES = \
 src/rtmpgw.c \
 src/thread.c

rtmpgw_LDADD = librtmp.la $(VS_LINK_LIBS)

rtmpsrv_SOURCES =  \
  src/rtmpsrv.c \
  src/thread.c

rtmpsrv_LDADD = librtmp.la $(VS_LINK_LIBS)

rtmpsuck_SOURCES = \
  src/rtmpsuck.c \
  src/thread.c

rtmpsuck_LDADD = librtmp.la $(VS_LINK_LIBS)

noinst_HEADERS = src/thread.h

lib_LTLIBRARIES = librtmp.la

AM_CFLAGS=-DRTMPDUMP_VERSION=\"@VERSION@\"
AM_CFLAGS+=-DUSE_OPENSSL

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = librtmp.pc

librtmp_la_LIBADD= -lssl -lcrypto -lz $(VS_LINK_LIBS)
librtmp_la_LDFLAGS=-version-number @LIB_MAJOR_VERSION@:@LIB_MINOR_VERSION@
if VS_WINDOWS
librtmp_la_LDFLAGS +=-shared -no-undefined -Wl,--out-implib,librtmp.dll.a -Wl,-Bsymbolic -Wl,--warn-common -Wl,--as-needed
endif

librtmp_la_SOURCES= \
  src/librtmp/amf.c \
  src/librtmp/log.c \
  src/librtmp/rtmp.c \
  src/librtmp/hashswf.c \
  src/librtmp/parseurl.c

librtmp_ladir=$(includedir)/librtmp
librtmp_la_HEADERS= \
  src/librtmp/amf.h \
  src/librtmp/dhgroups.h \
  src/librtmp/log.h \
  src/librtmp/bytes.h \
  src/librtmp/handshake.h \
  src/librtmp/rtmp.h \
  src/librtmp/dh.h \
  src/librtmp/http.h \
  src/librtmp/rtmp_sys.h

