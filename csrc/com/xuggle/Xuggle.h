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
