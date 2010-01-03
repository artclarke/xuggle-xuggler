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
