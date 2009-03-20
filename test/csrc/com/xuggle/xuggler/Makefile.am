VS_PRODUCT=xuggle_xuggler
VS_CPP_PATH=com/xuggle/xuggler
VS_CPP_PACKAGE=com.xuggle.xuggler
VS_CPP_NAMESPACE=com::xuggle::xuggler
VS_TEST=1
include @top_builddir@/mk/Makefile.global

EXTRA_LIBS=

check_PROGRAMS=xugglerTestAudioResampler \
  xugglerTestAudioSamples \
  xugglerTestAudioResampler \
  xugglerTestCodec \
  xugglerTestContainerFormat \
  xugglerTestContainer \
  xugglerTestVideoPicture \
  xugglerTestHelper \
  xugglerTestMediaDataWrapper \
  xugglerTestPacket \
  xugglerTestPixelFormat \
  xugglerTestRational \
  xugglerTestStreamCoder \
  xugglerTestStreamCoderProperty \
  xugglerTestStreamCoderX264 \
  xugglerTestStream \
  xugglerTestTimeValue \
  xugglerTestError

if VS_ENABLE_GPL
check_PROGRAMS+=xugglerTestAudioResampler \
  xugglerTestVideoResampler
endif

inst_check=$(check_PROGRAMS)
inst_checkdir=$(bindir)

# Make sure we set the abs_top_srcdir environment variable so the 
# tests can auto-find their fixtures
TESTS=$(check_PROGRAMS)

xugglerTestAudioSamples_SOURCES= \
  AudioSamplesTest.cpp \
  Main.cpp \
  Helper.cpp

nodist_xugglerTestAudioSamples_SOURCES= \
  AudioSamplesTest_CXXRunner.cpp

xugglerTestAudioSamples_LDADD= \
  $(top_builddir)/csrc/$(VS_CPP_PATH)/libxuggle-xuggler.la $(EXTRA_LIBS)

xugglerTestAudioResampler_SOURCES=\
  AudioResamplerTest.cpp \
  Main.cpp \
  Helper.cpp

nodist_xugglerTestAudioResampler_SOURCES=\
  AudioResamplerTest_CXXRunner.cpp

xugglerTestAudioResampler_LDADD=\
  $(top_builddir)/csrc/$(VS_CPP_PATH)/libxuggle-xuggler.la $(EXTRA_LIBS)

xugglerTestCodec_SOURCES=\
  CodecTest.cpp \
  Main.cpp \
  Helper.cpp

nodist_xugglerTestCodec_SOURCES=\
  CodecTest_CXXRunner.cpp

xugglerTestCodec_LDADD=\
  $(top_builddir)/csrc/$(VS_CPP_PATH)/libxuggle-xuggler.la $(EXTRA_LIBS)

xugglerTestContainerFormat_SOURCES=\
  ContainerFormatTest.cpp \
  Main.cpp \
  Helper.cpp

nodist_xugglerTestContainerFormat_SOURCES=\
  ContainerFormatTest_CXXRunner.cpp

xugglerTestContainerFormat_LDADD=\
  $(top_builddir)/csrc/$(VS_CPP_PATH)/libxuggle-xuggler.la $(EXTRA_LIBS)

xugglerTestContainer_SOURCES=\
  ContainerTest.cpp \
  Main.cpp \
  Helper.cpp

nodist_xugglerTestContainer_SOURCES=\
  ContainerTest_CXXRunner.cpp

xugglerTestContainer_LDADD=\
  $(top_builddir)/csrc/$(VS_CPP_PATH)/libxuggle-xuggler.la $(EXTRA_LIBS)

xugglerTestVideoPicture_SOURCES=\
  VideoPictureTest.cpp \
  Main.cpp \
  Helper.cpp

nodist_xugglerTestVideoPicture_SOURCES=\
  VideoPictureTest_CXXRunner.cpp

xugglerTestVideoPicture_LDADD=\
  $(top_builddir)/csrc/$(VS_CPP_PATH)/libxuggle-xuggler.la $(EXTRA_LIBS)

xugglerTestHelper_SOURCES=\
  HelperTest.cpp \
  Main.cpp \
  Helper.cpp

nodist_xugglerTestHelper_SOURCES=\
  HelperTest_CXXRunner.cpp

xugglerTestHelper_LDADD=\
  $(top_builddir)/csrc/$(VS_CPP_PATH)/libxuggle-xuggler.la $(EXTRA_LIBS)

xugglerTestMediaDataWrapper_SOURCES=\
  MediaDataWrapperTest.cpp \
  Main.cpp \
  Helper.cpp

nodist_xugglerTestMediaDataWrapper_SOURCES=\
  MediaDataWrapperTest_CXXRunner.cpp

xugglerTestMediaDataWrapper_LDADD=\
  $(top_builddir)/csrc/$(VS_CPP_PATH)/libxuggle-xuggler.la $(EXTRA_LIBS)

xugglerTestPacket_SOURCES=\
  PacketTest.cpp \
  Main.cpp \
  Helper.cpp

nodist_xugglerTestPacket_SOURCES=\
  PacketTest_CXXRunner.cpp

xugglerTestPacket_LDADD=\
  $(top_builddir)/csrc/$(VS_CPP_PATH)/libxuggle-xuggler.la $(EXTRA_LIBS)

xugglerTestPixelFormat_SOURCES=\
  PixelFormatTest.cpp \
  Main.cpp \
  Helper.cpp

nodist_xugglerTestPixelFormat_SOURCES=\
  PixelFormatTest_CXXRunner.cpp

xugglerTestPixelFormat_LDADD=\
  $(top_builddir)/csrc/$(VS_CPP_PATH)/libxuggle-xuggler.la $(EXTRA_LIBS)

xugglerTestRational_SOURCES=\
  RationalTest.cpp \
  Main.cpp \
  Helper.cpp

nodist_xugglerTestRational_SOURCES=\
  RationalTest_CXXRunner.cpp 

xugglerTestRational_LDADD=\
  $(top_builddir)/csrc/$(VS_CPP_PATH)/libxuggle-xuggler.la $(EXTRA_LIBS)

xugglerTestStreamCoder_SOURCES=\
  StreamCoderTest.cpp \
  Main.cpp \
  Helper.cpp

nodist_xugglerTestStreamCoder_SOURCES=\
  StreamCoderTest_CXXRunner.cpp

xugglerTestStreamCoder_LDADD=\
  $(top_builddir)/csrc/$(VS_CPP_PATH)/libxuggle-xuggler.la $(EXTRA_LIBS)

xugglerTestStreamCoderX264_SOURCES=\
  StreamCoderX264Test.cpp \
  Main.cpp \
  Helper.cpp

nodist_xugglerTestStreamCoderX264_SOURCES=\
  StreamCoderX264Test_CXXRunner.cpp

xugglerTestStreamCoderX264_LDADD=\
  $(top_builddir)/csrc/$(VS_CPP_PATH)/libxuggle-xuggler.la $(EXTRA_LIBS)

xugglerTestStreamCoderProperty_SOURCES=\
  StreamCoderPropertyTest.cpp \
  Main.cpp \
  Helper.cpp

nodist_xugglerTestStreamCoderProperty_SOURCES=\
  StreamCoderPropertyTest_CXXRunner.cpp

xugglerTestStreamCoderProperty_LDADD=\
  $(top_builddir)/csrc/$(VS_CPP_PATH)/libxuggle-xuggler.la $(EXTRA_LIBS)


xugglerTestStream_SOURCES=\
  StreamTest.cpp \
  Main.cpp \
  Helper.cpp

nodist_xugglerTestStream_SOURCES=\
  StreamTest_CXXRunner.cpp

xugglerTestStream_LDADD=\
  $(top_builddir)/csrc/$(VS_CPP_PATH)/libxuggle-xuggler.la $(EXTRA_LIBS)

xugglerTestTimeValue_SOURCES=\
  TimeValueTest.cpp \
  Main.cpp \
  Helper.cpp

nodist_xugglerTestTimeValue_SOURCES=\
  TimeValueTest_CXXRunner.cpp

xugglerTestTimeValue_LDADD=\
  $(top_builddir)/csrc/$(VS_CPP_PATH)/libxuggle-xuggler.la $(EXTRA_LIBS)

xugglerTestError_SOURCES=\
  ErrorTest.cpp \
  Main.cpp \
  Helper.cpp

nodist_xugglerTestError_SOURCES=\
  ErrorTest_CXXRunner.cpp

xugglerTestError_LDADD=\
  $(top_builddir)/csrc/$(VS_CPP_PATH)/libxuggle-xuggler.la $(EXTRA_LIBS)

xugglerTestVideoResampler_SOURCES=\
  VideoResamplerTest.cpp \
  Main.cpp \
  Helper.cpp

nodist_xugglerTestVideoResampler_SOURCES=\
  VideoResamplerTest_CXXRunner.cpp

xugglerTestVideoResampler_LDADD=\
  $(top_builddir)/csrc/$(VS_CPP_PATH)/libxuggle-xuggler.la $(EXTRA_LIBS)

BUILT_SOURCES= \
  AudioSamplesTest_CXXRunner.cpp \
  AudioResamplerTest_CXXRunner.cpp \
  CodecTest_CXXRunner.cpp \
  ContainerFormatTest_CXXRunner.cpp \
  ContainerTest_CXXRunner.cpp \
  VideoPictureTest_CXXRunner.cpp \
  HelperTest_CXXRunner.cpp \
  MediaDataWrapperTest_CXXRunner.cpp \
  PacketTest_CXXRunner.cpp \
  PixelFormatTest_CXXRunner.cpp \
  RationalTest_CXXRunner.cpp \
  StreamTest_CXXRunner.cpp \
  TimeValueTest_CXXRunner.cpp \
  ErrorTest_CXXRunner.cpp \
  VideoResamplerTest_CXXRunner.cpp \
  StreamCoderTest_CXXRunner.cpp \
  StreamCoderPropertyTest_CXXRunner.cpp \
  StreamCoderX264Test_CXXRunner.cpp

noinst_HEADERS = \
  AudioResamplerTest.h \
  AudioSamplesTest.h \
  CodecTest.h \
  ContainerFormatTest.h \
  ContainerTest.h \
  VideoPictureTest.h \
  Helper.h \
  HelperTest.h \
  MediaDataWrapperTest.h \
  PacketTest.h \
  PixelFormatTest.h \
  RationalTest.h \
  StreamCoderTest.h \
  StreamCoderPropertyTest.h \
  StreamCoderX264Test.h \
  StreamTest.h \
  TimeValueTest.h \
  ErrorTest.h \
  VideoResamplerTest.h

clean-local:
	rm -rf $(BUILT_SOURCES)
	rm -rf *.flv
	rm -rf *.mov
	rm -rf *.mp4
	rm -rf *.mp3
	rm -rf memcheck*.log
