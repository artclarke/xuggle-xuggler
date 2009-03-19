VS_PRODUCT=xuggle_xuggler
VS_CPP_PATH=com/xuggle/xuggler
VS_CPP_PACKAGE=com.xuggle.xuggler
VS_CPP_NAMESPACE=com::xuggle::xuggler
include @top_builddir@/mk/Makefile.global

SUBDIRS=io
SUFFIXES=.i
lib_LTLIBRARIES = libxuggle-xuggler.la

libxuggle_xuggler_la_LIBADD= -lavformat -lavcodec -lavutil 
if VS_ENABLE_GPL
libxuggle_xuggler_la_LIBADD+= -lswscale
endif
libxuggle_xuggler_la_LIBADD+= $(top_builddir)/csrc/com/xuggle/ferry/libxuggle-ferry.la 

libxuggle_xuggler_la_SOURCES= \
  AudioResampler.cpp \
  AudioSamples.cpp \
  Codec.cpp \
  Container.cpp \
  ContainerFormat.cpp \
  VideoPicture.cpp \
  Global.cpp \
  IAudioResampler.cpp \
  IAudioSamples.cpp \
  ICodec.cpp \
  IContainer.cpp \
  IContainerFormat.cpp \
  IVideoPicture.cpp \
  IMediaData.cpp \
  IMediaDataWrapper.cpp \
  IPacket.cpp \
  IPixelFormat.cpp \
  IProperty.cpp \
  IRational.cpp \
  IStreamCoder.cpp \
  IStream.cpp \
  ITimeValue.cpp \
  IVideoResampler.cpp \
  MediaDataWrapper.cpp \
  Packet.cpp \
  Property.cpp \
  Rational.cpp \
  StreamCoder.cpp \
  Stream.cpp \
  TimeValue.cpp

nodist_libxuggle_xuggler_la_SOURCES= \
  Xuggler.cpp

if VS_ENABLE_GPL
libxuggle_xuggler_la_SOURCES+= \
  VideoResampler.cpp 
endif

libxuggle_xuggler_ladir=$(includedir)/$(VS_CPP_PATH)
libxuggle_xuggler_la_HEADERS= \
  Xuggler.h \
  config.h \
  FfmpegIncludes.h \
  Global.h \
  IAudioResampler.h \
  IAudioSamples.h \
  ICodec.h \
  IContainerFormat.h \
  IContainer.h \
  IVideoPicture.h \
  IMediaData.h \
  IMediaDataWrapper.h \
  IMediaDataWrapper.swg \
  IPacket.h \
  IPixelFormat.h \
  IProperty.h \
  IRational.h \
  IStreamCoder.h \
  IStreamCoder.swg \
  IStream.h \
  ITimeValue.h \
  IVideoResampler.h \
  Xuggler.i \
  AudioResampler.h \
  AudioSamples.h \
  Codec.h \
  ContainerFormat.h \
  Container.h \
  VideoPicture.h \
  MediaDataWrapper.h \
  Packet.h \
  PixelFormat.h \
  Property.h \
  Rational.h \
  StreamCoder.h \
  Stream.h \
  TimeValue.h \
  VideoResampler.h 

BUILT_SOURCES= \
  Xuggler.cpp

Xuggler.cpp : $(libxuggle_xuggler_la_HEADERS)

clean-local:
	rm -f $(BUILT_SOURCES)
	if test -w "$(SWIGOUTTMPDIR)"; then rm -rf $(SWIGOUTTMPDIR); fi

maintainer-clean-local:
	rm -rf $(SWIGOUTDIR)/$(VS_CPP_PATH)

