#!/bin/sh
# Copyright (c) 2008 by Xuggle Inc. All rights reserved.
#
# It is REQUESTED BUT NOT REQUIRED if you use this library, that you let 
# us know by sending e-mail to info\@xuggle.com telling us briefly how you're
# using the library and what you like or don't like about it.
#
# This library is free software; you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free Software
# Foundation; either version 2.1 of the License, or (at your option) any later
# version.
#
# This library is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this library; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

# Usage; $0 "directory_to_check" "file_to_read_last_revision" ["optional file to write last to" ["optional ant properties file to write"]
# First try the best way; using 'svn info'
revision=`cd "$1" && LC_ALL=C svn info 2> /dev/null | grep Revision | cut -d' ' -f2`
# Fine... if that didn't work, see if there is a .svn/entries revision entry
test $revision || revision=`cd "$1" && grep revision .svn/entries 2>/dev/null | cut -d '"' -f2`
# OK, now we're getting crazy.  look for a .svn/entries, the the 'dir' line
# and get the text after it
test $revision || revision=`cd "$1" && sed -n -e '/^dir$/{n;p;q}' .svn/entries 2>/dev/null`

if test -f "$2" ; then
  test $revision || revision=`cat "$2"`
fi
# no version number found
test $revision || revision=0

if test ! "x$3" = "x"; then
 echo $revision > "$3"
fi
# And spit out to the command line so the tool that called this
# can do it's magic
if test -z "$4"; then
  # just spit out the revision to the command line
  echo $revision
else
  echo "library.revision: $revision" > "$4"
fi
