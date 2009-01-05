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
