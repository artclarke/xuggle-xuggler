#!/bin/sh
VER=`svnversion .`
if [ "x$VER" != x -a "$VER" != exported ]
then
  echo "#define X264_VERSION \" svn-$VER\"" >> config.h
  VER=`echo $VER | sed -e 's/[^0-9].*//'`
else
  echo "#define X264_VERSION \"\"" >> config.h
  VER="x"
fi
API=`grep '#define X264_BUILD' < x264.h | sed -e 's/.* \([1-9][0-9]*\).*/\1/'`
echo "#define X264_POINTVER \"0.$API.$VER\"" >> config.h
