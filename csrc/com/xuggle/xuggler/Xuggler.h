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
