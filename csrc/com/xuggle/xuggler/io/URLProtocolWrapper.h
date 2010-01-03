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

#ifndef URLPROTOCOLWRAPPER_H_
#define URLPROTOCOLWRAPPER_H_

#include <com/xuggle/xuggler/io/FfmpegIncludes.h>

extern "C"
{
  typedef struct URLProtocolWrapper
  {
    // This must be the first element in this wrapper to ensure we can
    // trick FFMPEG into passing it around for us.
    URLProtocol proto;
    // You can add anything you need after this.  This will be passed
    // in the URLContext->proto field in the protocol functions.
    // You should note that the same proto gets passed for all threads, and
    // so you should only have 'global' variables here.
    void * protocolMgr;
  } URLProtocolWrapper;
}
#endif /*URLPROTOCOLWRAPPER_H_*/
