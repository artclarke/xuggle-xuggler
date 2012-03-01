#!/bin/sh

# Copyright (c) 2012 by Art Clarke
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# It is REQUESTED BUT NOT REQUIRED if you use this library, that you let 
# us know by sending e-mail to info\@xuggle.com telling us briefly how you're
# using the library and what you like or don't like about it.

# First some canonical names we use
VS_WIN32=i686-w64-mingw32
VS_WIN64=x86_64-w64-mingw32
VS_LIN32=i686-pc-linux-gnu
VS_LIN64=x86_64-pc-linux-gnu
VS_MAC32=i386-xuggle-darwin
VS_MAC64=x86_64-xuggle-darwin

XUGGLE_HOME="${XUGGLE_HOME:-/usr/local}"
DIR=$( dirname $0 )
if [ -f "${DIR}/config.guess" ]; then
  HOST=$( "${DIR}/config.guess" )
fi

darwin_lipo()
{
  STAGE_DIR="/tmp/xuggle-${RANDOM}"
  echo "LIPO to ${STAGE_DIR}"
  (cd "${DIR}/build/native/${VS_MAC32}" && make DESTDIR="${STAGE_DIR}/stage32" install)
  (cd "${DIR}/build/native/${VS_MAC64}" && make DESTDIR="${STAGE_DIR}/stage64" install)

  "${DIR}/mk/buildtools/darwin-universal.sh" \
     "${DIR}/dist/stage${XUGGLE_HOME}" \
     "${STAGE_DIR}/stage32${XUGGLE_HOME}" \
     "${STAGE_DIR}/stage64${XUGGLE_HOME}" \
  rm -rf "${STAGE_DIR}"
  echo "Complete: darwin_lipo"
}

case $HOST in
  *darwin*)
    darwin_lipo && \
    true
  ;;
  *linux*)
    ant -Dbuild.configure.os=${VS_LIN32} stage-native && \
    ant -Dbuild.configure.os=${VS_LIN64} stage-native && \
    ant -Dbuild.configure.os=${VS_WIN32} stage-native && \
    ant -Dbuild.configure.os=${VS_WIN64} stage-native && \
    true
  ;;
  *mingw*)
    case $HOST in
      *x86_64*)
        ant -Dbuild.configure.os=${VS_WIN64} stage-native && \
        true
      ;;
      *)
        ant -Dbuild.configure.os=${VS_WIN32} stage-native && \
        true
      ;;
    esac
  ;;
esac

