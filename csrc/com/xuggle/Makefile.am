VS_CPP_PATH=com/xuggle
VS_CPP_PACKAGE=com.xuggle
VS_CPP_NAMESPACE=com::xuggle

include @top_builddir@/mk/Makefile.global
# defines VS_PKG_LIBRARIES.  Must be in a file that automake does not
# process at autoreconf time.
include @builddir@/Makefile.dependencies

SUBDIRS=ferry xuggler

xuggledir=$(includedir)/$(VS_CPP_PATH)
xuggle_HEADERS= \
  Xuggle.h

#noinst_PROGRAMS=xuggle-xuggler-main

lib_LTLIBRARIES = 

#xuggle_xuggler_main_SOURCES = Main.cpp
#xuggle_xuggler_main_LDADD = libxuggle.la

lib_LTLIBRARIES+= libxuggle.la
libxuggle_la_SOURCES = Xuggle.cpp
libxuggle_la_LIBADD = $(builddir)/xuggler/libxuggle-xuggler.la \
	$(builddir)/xuggler/io/libxuggle-xuggler-io.la \
	$(builddir)/ferry/libxuggle-ferry.la

libxuggle_la_LDFLAGS = \
	-no-undefined -shared \
	-version-number @LIB_MAJOR_VERSION@:@LIB_MINOR_VERSION@:0 \
	$(VS_LDFLAGS) \
	$(VS_PKG_LIBRARIES)

if VS_OS_WINDOWS
libxuggle_la_LDFLAGS+= \
	-Wl,--add-stdcall-alias \
	-Wl,--enable-runtime-pseudo-reloc \
	-Wl,--enable-auto-image-base \
	-Wl,-Bsymbolic \
	-Wl,--warn-common \
	-Wl,--as-needed \
	-XCClinker -lgcc \
	-XCClinker -lmingw32 \
	-XCClinker -lmoldname \
	-XCClinker -lmingwex \
	-XCClinker -lmsvcrt \
	-XCClinker -ladvapi32 \
	-XCClinker -lshell32 \
	-XCClinker -luser32 \
	-XCClinker -lkernel32
else
if VS_OS_LINUX
libxuggle_la_LDFLAGS+= \
	-nostdlib -static-libtool-libs -XCClinker -static-libgcc
else
libxuggle_la_LDFLAGS+= \
	-nostdlib -static-libtool-libs -XCClinker -static-libgcc
endif
endif
