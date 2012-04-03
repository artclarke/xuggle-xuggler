VS_PRODUCT=xuggle_xuggler_io
VS_CPP_PATH=com/xuggle/xuggler/io
VS_CPP_PACKAGE=com.xuggle.xuggler.io
VS_CPP_NAMESPACE=com::xuggle::xuggler:IO
include @top_builddir@/mk/Makefile.global

# This is a convenience library
noinst_LTLIBRARIES = libxuggle-xuggler-io.la

libxuggle_xuggler_io_la_LIBADD=

libxuggle_xuggler_io_la_SOURCES= \
  FfmpegIO.cpp \
  StdioURLProtocolHandler.cpp \
  StdioURLProtocolManager.cpp \
  JavaURLProtocolHandler.cpp \
  JavaURLProtocolManager.cpp \
  URLProtocolHandler.cpp \
  URLProtocolManager.cpp

libxuggle_xuggler_io_ladir=$(includedir)/$(VS_CPP_PATH)
libxuggle_xuggler_io_la_HEADERS= \
  FfmpegIO.h \
  config.h \
  IO.h \
  StdioURLProtocolHandler.h \
  StdioURLProtocolManager.h \
  JavaURLProtocolHandler.h \
  JavaURLProtocolManager.h \
  URLProtocolHandler.h \
  URLProtocolManager.h 
