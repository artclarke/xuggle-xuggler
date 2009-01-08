/*
 * Copyright (c) 2008-2009 by Xuggle Inc. All rights reserved.
 *
 * It is REQUESTED BUT NOT REQUIRED if you use this library, that you let 
 * us know by sending e-mail to info@xuggle.com telling us briefly how you're
 * using the library and what you like or don't like about it.
 *
 * This library is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any later
 * version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#ifndef __XUGGLER_H__
#define __XUGGLER_H__

#include <com/xuggle/Xuggle.h>
#include <inttypes.h>

namespace com {
namespace xuggle {
/**
 * This package contains the core XUGGLER library routines
 * that deal with the manipulation of media files.
 *
 * To get started, check out Global.h and the Global object.
 */
namespace xuggler {
}
}
}
#ifdef VS_OS_WINDOWS
#ifdef VS_API_COMPILING_xuggle_xuggler
#define VS_API_XUGGLER VS_API_EXPORT
#else
#define VS_API_XUGGLER VS_API_IMPORT
#endif // VS_API_COMPILING_xuggle_xuggler
#else
#define VS_API_XUGGLER
#endif
#endif // ! __XUGGLER_H__
