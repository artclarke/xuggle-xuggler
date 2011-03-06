VS_PRODUCT=theyard_xuggler_io
VS_CPP_PATH=com/xuggle/xuggler/io
VS_CPP_PACKAGE=com.xuggle.xuggler.io
VS_CPP_NAMESPACE=com::xuggle::xuggler:IO
include @top_builddir@/mk/Makefile.global

lib_LTLIBRARIES = libxuggle-xuggler-io.la

libxuggle_xuggler_io_la_LIBADD=$(top_builddir)/csrc/com/xuggle/ferry/libxuggle-ferry.la -lavformat -lavcodec -lavutil

libxuggle_xuggler_io_la_SOURCES= \
  FfmpegIO.cpp \
  URLProtocolHandler.cpp \
  URLProtocolManager.cpp

libxuggle_xuggler_io_ladir=$(includedir)/$(VS_CPP_PATH)
libxuggle_xuggler_io_la_HEADERS= \
  FfmpegIncludes.h \
  FfmpegIO.h \
  config.h \
  IO.h \
  URLProtocolHandler.h \
  URLProtocolManager.h \
  URLProtocolWrapper.h 
