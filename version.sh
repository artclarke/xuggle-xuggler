#!/bin/sh
VER=`svnversion .`
if [ "x$VER" != x -a "$VER" != exported ]
then
  echo "#define X264_VERSION \" svn-$VER\"" > config.h
else
  echo "#define X264_VERSION \"\"" > config.h
fi
