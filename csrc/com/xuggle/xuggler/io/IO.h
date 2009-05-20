/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated.  All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any
 * later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
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
