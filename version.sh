#!/bin/sh
VER=`svnversion .`
if [ "x$VER" != x -a "$VER" != exported ]
then
  echo "#define X264_VERSION \" svn-$VER\"" > config.h
  API=`grep '#define X264_BUILD' < x264.h | sed -e 's/.* \([1-9][0-9]*\).*/\1/'`
  VER=`echo $VER | sed -e 's/[^0-9].*//'`
  echo "#define X264_POINTVER \"0.$API.$VER\"" >> config.h
else
  echo "#define X264_VERSION \"\"" > config.h
  echo "#define X264_POINTVER \"\"" >> config.h
fi
