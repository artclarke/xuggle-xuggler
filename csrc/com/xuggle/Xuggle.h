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

#ifndef XUGGLE_H_
#define XUGGLE_H_
/**
 * This package is empty and is just a containing namespace
 * for xuggle
 */
namespace com
{

/**
 * This package contains Native code belonging to
 * Xuggle libraries.
 */
namespace xuggle {

}
}

#define VS_STRINGIFY(__arg) #__arg

#ifdef VS_OS_WINDOWS
#define VS_API_EXPORT __declspec(dllexport)
#define VS_API_IMPORT __declspec(dllimport)
#else
#define VS_API_EXPORT
#define VS_API_IMPORT
#endif // VS_OS_WINDOWS

#endif // XUGGLE_H_
