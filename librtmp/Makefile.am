ACLOCAL_AMFLAGS=-I m4

bin_PROGRAMS = rtmpdump rtmpgw rtmpsrv rtmpsuck

rtmpdump_SOURCES = \
  csrc/rtmpdump.c

rtmpdump_LDADD = librtmp.la

rtmpgw_SOURCES = \
 csrc/rtmpgw.c \
 csrc/thread.c

rtmpgw_LDADD = librtmp.la -lpthread

rtmpsrv_SOURCES =  \
  csrc/rtmpsrv.c \
  csrc/thread.c

rtmpsrv_LDADD = librtmp.la -lpthread

rtmpsuck_SOURCES = \
  csrc/rtmpsuck.c \
  csrc/thread.c

rtmpsuck_LDADD = librtmp.la -lpthread

noinst_HEADERS = csrc/thread.h

lib_LTLIBRARIES = librtmp.la

AM_CFLAGS=-DRTMPDUMP_VERSION=\"@VERSION@\"
AM_CFLAGS+=-DUSE_OPENSSL
AM_LDFLAGS=-version-number @LIB_MAJOR_VERSION@:@LIB_MINOR_VERSION@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = librtmp.pc

librtmp_la_LIBADD= -lssl -lcrypto -lz

librtmp_la_SOURCES= \
  csrc/librtmp/amf.c \
  csrc/librtmp/log.c \
  csrc/librtmp/rtmp.c \
  csrc/librtmp/hashswf.c \
  csrc/librtmp/parseurl.c

librtmp_ladir=$(includedir)/librtmp
librtmp_la_HEADERS= \
  csrc/librtmp/amf.h \
  csrc/librtmp/dhgroups.h \
  csrc/librtmp/log.h \
  csrc/librtmp/bytes.h \
  csrc/librtmp/handshake.h \
  csrc/librtmp/rtmp.h \
  csrc/librtmp/dh.h \
  csrc/librtmp/http.h \
  csrc/librtmp/rtmp_sys.h

