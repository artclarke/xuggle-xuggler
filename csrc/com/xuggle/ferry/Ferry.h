/*
 * This file is part of Xuggler.
 * 
 * Xuggler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public
 * License along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __FERRY_H__
#define __FERRY_H__

#include <com/xuggle/Xuggle.h>

namespace com
{
namespace xuggle
{
/**
 * This library contains routines used by Xuggle libraries for
 * "ferry"ing Java objects to and from native code.
 *
 * Of particular importance is the RefCounted interface.
 */
namespace ferry
{

}
}
}
#ifdef VS_OS_WINDOWS
#ifdef VS_API_COMPILING_xuggle_ferry
#define VS_API_FERRY VS_API_EXPORT
#else
#define VS_API_FERRY VS_API_IMPORT
#endif // VS_API_COMPILING_xuggle_ferry
#else
#define VS_API_FERRY
#endif
#endif // ! __FERRY_H__
