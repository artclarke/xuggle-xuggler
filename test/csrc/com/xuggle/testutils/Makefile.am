VS_PRODUCT=xuggle_testutils
VS_CPP_PATH=com/xuggle/testutils
VS_CPP_PACKAGE=com.xuggle.testutils
VS_CPP_NAMESPACE=com::xuggle::testutils
VS_TEST=1
include @top_builddir@/mk/Makefile.global

check_PROGRAMS=testutilsTestTrue
TESTS=$(check_PROGRAMS)

testutilsTestTrue_SOURCES= \
  TrueTest.cpp \
  Main.cpp

nodist_testutilsTestTrue_SOURCES= \
  TrueTest_CXXRunner.cpp 

testutilsTestTrue_LDADD= \
  $(top_builddir)/csrc/com/xuggle/ferry/libxuggle-ferry.la

BUILT_SOURCES= \
  TrueTest_CXXRunner.cpp

noinst_HEADERS = \
  TestUtils.h \
  TrueTest.h

clean-local:
	rm -rf $(BUILT_SOURCES)
	rm -rf *.flv
	rm -rf *.mov
	rm -rf *.mp3
	rm -rf memcheck*.log
