#!/bin/sh

# Copyright (c) 2009 by Xuggle Inc.
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

# It requires a bash-compliant shell to be installed in order to work.
# We do this instead of ant's <copy/> task to deal with native code
# on windows; drive-letters (e.g. C:) causes havoc with the native builds,
# but get translated by it into Msys/Cygwin standard drive letters, which
# then won't work with <copy/>

STAGEDIR="$1"
DESTDIR="$2"
echo "Installing $STAGEDIR/* to $DESTDIR"
cp -rf "$STAGEDIR"/* $DESTDIR
