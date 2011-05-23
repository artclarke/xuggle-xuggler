/*******************************************************************************
 * Copyright (c) 2008, 2010 Xuggle Inc.  All rights reserved.
 *  
 * This file is part of Xuggle-Xuggler-Main.
 *
 * Xuggle-Xuggler-Main is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xuggle-Xuggler-Main is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Xuggle-Xuggler-Main.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/

#ifndef FFMPEGINCLUDES_H_
#define FFMPEGINCLUDES_H_

extern "C"
{
// WARNING: This is GCC specific and is to fix a build issue
// in FFmpeg where UINT64_C is not always defined.  The
// __WORDSIZE value is a GCC constant
#define __STDC_CONSTANT_MACROS 1
#include <stdint.h>
#ifndef UINT64_C
# if __WORDSIZE == 64
#  define UINT64_C(c)	c ## UL
# else
#  define UINT64_C(c)	c ## ULL
# endif
#endif

// Sigh; the latest FFmpeg puts in a deprecated statement
// in an enum which just will not compile with most GCC
// versions.  So we unset it here.
#define attribute_deprecated
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
}
#endif /*FFMPEGINCLUDES_H_*/
