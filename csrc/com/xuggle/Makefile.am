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

bin_PROGRAMS= xuggle-xuggler-main
lib_LTLIBRARIES = 

xuggle_xuggler_main_SOURCES = Main.cpp
xuggle_xuggler_main_LDADD = libxuggle.la

#if VS_IS_GCC
# Finally  this script will be used to build the actual shared library
#
# This is NOT OBVIOUS, but libtool will find any object or libraries
# on the link line and attempt to do smart things.  Unfortunately it
# will overthing things.  But I still want to use their lovely .la
# wrappers.  So we pass "-XCClinker" flags to have items go past
# libtool and not get processed.
#
# I also DELIBERATELY link with the static stdc++ and gcc libraries.
# This means that if Xuggle EVER leaks an exception outside of the
# DLL, we can cause very very strange crashes in the JVM.  So, no
# exceptions cross JNI.
#
# And yes, this code heavily depends on GCC; get over it.
#libxuggle.la: libxuggle_static.la
#	$(LIBTOOL) --mode=link $(CC) -fPIC -DPIC \
#	$(VS_LDFLAGS) \
#	-no-undefined -shared -static-libtool-libs \
#	-XCClinker -static-libgcc \
#	Xuggle.lo \
#	-Wl,--whole-archive \
#	"$(builddir)/xuggler/libxuggle-xuggler.la" \
#	"$(builddir)/xuggler/io/libxuggle-xuggler-io.la" \
#	"$(builddir)/ferry/libxuggle-ferry.la" \
#	-Wl,--no-whole-archive \
#	$(VS_PKG_LIBRARIES) \
#	-version-number @LIB_MAJOR_VERSION@:@LIB_MINOR_VERSION@:0 \
#	-rpath $(prefix) \
#	-o libxuggle.la ;
#else
# if not GCC, then let libtool try its best
lib_LTLIBRARIES+= libxuggle.la
# MUST be a C file; we force C linkage
libxuggle_la_SOURCES = Xuggle.c
libxuggle_la_LIBADD = $(builddir)/xuggler/libxuggle-xuggler.la \
	$(builddir)/xuggler/io/libxuggle-xuggler-io.la \
	$(builddir)/ferry/libxuggle-ferry.la

libxuggle_la_LDFLAGS = \
	-no-undefined -shared \
	-version-number @LIB_MAJOR_VERSION@:@LIB_MINOR_VERSION@:0 \
	$(VS_LDFLAGS) \
	$(VS_PKG_LIBRARIES) \
	@VS_LIBSTDCXX@ 
#	@VS_LIBGCC@ 

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
libxuggle_la_LDFLAGS+= \
	-nostdlib -static-libtool-libs -XCClinker -static-libgcc
endif

#endif # VS_IS_GCC
