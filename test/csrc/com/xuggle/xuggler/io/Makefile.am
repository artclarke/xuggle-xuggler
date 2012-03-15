VS_PRODUCT=xuggle_xuggler_io
VS_CPP_PATH=com/xuggle/xuggler/io
VS_CPP_PACKAGE=com.xuggle.xuggler.io
VS_CPP_NAMESPACE=com::xuggle::xuggler::io
VS_TEST=1
include @top_builddir@/mk/Makefile.global

check_PROGRAMS=\
  xugglerioTestStdioURLProtocolHandler

inst_check=$(check_PROGRAMS)
inst_checkdir=$(bindir)

# Make sure we set the abs_top_srcdir environment variable so the 
# tests can auto-find their fixtures
TESTS=$(check_PROGRAMS)

xugglerioTestStdioURLProtocolHandler_SOURCES= \
  StdioURLProtocolHandlerTest.cpp \
  Main.cpp

nodist_xugglerioTestStdioURLProtocolHandler_SOURCES= \
  StdioURLProtocolHandlerTest_CXXRunner.cpp

xugglerioTestStdioURLProtocolHandler_LDADD= \
  $(top_builddir)/csrc/com/xuggle/libxuggle.la

BUILT_SOURCES= \
  StdioURLProtocolHandlerTest_CXXRunner.cpp

noinst_HEADERS = \
  StdioURLProtocolHandlerTest.h

clean-local:
	rm -rf $(BUILT_SOURCES)
	rm -rf *.flv
	rm -rf *.mov
	rm -rf *.mp4
	rm -rf *.mp3
	rm -rf *.ogg
	rm -rf memcheck*.log
