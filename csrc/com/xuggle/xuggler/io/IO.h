/*
 * Copyright (c) 2008 by Vlideshow Inc. (a.k.a. The Yard).  All rights reserved.
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
#ifndef IO_H_
#define IO_H_

#include <com/xuggle/Xuggle.h>

namespace com { namespace xuggle { namespace xuggler {
/**
 * This package contains the XUGGLER IO library which is used
 * to register callbacks for FFMPEG to get and send data to
 * Java objects.
 * <p>
 * To get started, check out URLProtocolHandler.h and the IURLProtocolHandler
 * java object object.
 * </p>
 * You can find a (potentially) helpful architecture diagram here:
 * <br/>
 * \image html com/xuggle/xuggler/io/xugglerio_architecture.png
 * And here's how the call flow between native code and another
 * language (in this case Java) goes:
 * <br/>
 * \image html com/xuggle/xuggler/io/xugglerio_callflow.png
 *
 */
namespace io
{
}
}}}

#ifdef VS_OS_WINDOWS
#ifdef VS_API_COMPILING_xuggle_xuggler_io
#define VS_API_XUGGLER_IO VS_API_EXPORT
#else
#define VS_API_XUGGLER_IO VS_API_IMPORT
#endif // VS_API_COMPILING_xuggler_xuggler_io
#else
#define VS_API_XUGGLER
#endif

#endif /* IO_H_ */
