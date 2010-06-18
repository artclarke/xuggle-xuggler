ACLOCAL_AMFLAGS=-I m4

bin_PROGRAMS = rtmpdump rtmpgw rtmpsrv rtmpsuck

rtmpdump_SOURCES = \
  rtmpdump/rtmpdump.c

rtmpdump_LDADD = librtmp.la

rtmpgw_SOURCES = \
 rtmpdump/rtmpgw.c \
 rtmpdump/thread.c

rtmpgw_LDADD = librtmp.la -lpthread

rtmpsrv_SOURCES =  \
  rtmpdump/rtmpsrv.c \
  rtmpdump/thread.c

rtmpsrv_LDADD = librtmp.la -lpthread

rtmpsuck_SOURCES = \
  rtmpdump/rtmpsuck.c \
  rtmpdump/thread.c

rtmpsuck_LDADD = librtmp.la -lpthread

noinst_HEADERS = rtmpdump/thread.h

lib_LTLIBRARIES = librtmp.la

AM_CFLAGS=-DRTMPDUMP_VERSION=\"@VERSION@\"
AM_CFLAGS+=-DUSE_OPENSSL
AM_LDFLAGS=-version-number @LIB_MAJOR_VERSION@:@LIB_MINOR_VERSION@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = librtmp.pc

librtmp_la_LIBADD= -lssl -lcrypto -lz

librtmp_la_SOURCES= \
  rtmpdump/librtmp/amf.c \
  rtmpdump/librtmp/log.c \
  rtmpdump/librtmp/rtmp.c \
  rtmpdump/librtmp/hashswf.c \
  rtmpdump/librtmp/parseurl.c

librtmp_ladir=$(includedir)/librtmp
librtmp_la_HEADERS= \
  rtmpdump/librtmp/amf.h \
  rtmpdump/librtmp/dhgroups.h \
  rtmpdump/librtmp/log.h \
  rtmpdump/librtmp/bytes.h \
  rtmpdump/librtmp/handshake.h \
  rtmpdump/librtmp/rtmp.h \
  rtmpdump/librtmp/dh.h \
  rtmpdump/librtmp/http.h \
  rtmpdump/librtmp/rtmp_sys.h

