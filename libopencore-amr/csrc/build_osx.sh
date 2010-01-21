#!/bin/sh

export CFLAGS="-isysroot /Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 -arch ppc -arch i386"
export CXXFLAGS="$CFLAGS"
export LDFLAGS="-Wl,-syslibroot,/Developer/SDKs/MacOSX10.4u.sdk -mmacosx-version-min=10.4 -arch ppc -arch i386"
export BUILD_AS_C=1

make -f Makefile.alt -C amrnb "$@" || exit 1
make -f Makefile.alt -C amrwb "$@" || exit 1

