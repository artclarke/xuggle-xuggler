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
#include <com/xuggle/xuggler/IVideoPicture.h>
#include <com/xuggle/xuggler/IPixelFormat.h>
#include <com/xuggle/xuggler/Global.h>
#include "Helper.h"
#include "PixelFormatTest.h"

using namespace VS_CPP_NAMESPACE;

void
PixelFormatTest :: testGetAndSetPixel()
{
  int width = 37; // choose prime number
  int height = 23; // choose prime number
  RefPointer<IVideoPicture> frame = IVideoPicture::make(IPixelFormat::YUV420P, width, height);

  unsigned char y = 5;
  unsigned char u = 10;
  unsigned char v = 15;
  int xcoord = width-1;
  int ycoord = height-1;
  IPixelFormat::setYUV420PPixel(frame.value(), xcoord, ycoord, IPixelFormat::YUV_Y, y);
  IPixelFormat::setYUV420PPixel(frame.value(), xcoord, ycoord, IPixelFormat::YUV_U, u);
  IPixelFormat::setYUV420PPixel(frame.value(), xcoord, ycoord, IPixelFormat::YUV_V, v);

  VS_TUT_ENSURE_EQUALS("unexpected pixel value",
      (int)y, (int)IPixelFormat::getYUV420PPixel(frame.value(), xcoord, ycoord, IPixelFormat::YUV_Y));
  VS_TUT_ENSURE_EQUALS("unexpected pixel value",
      (int)u, (int)IPixelFormat::getYUV420PPixel(frame.value(), xcoord, ycoord, IPixelFormat::YUV_U));
  VS_TUT_ENSURE_EQUALS("unexpected pixel value",
      (int)v, (int)IPixelFormat::getYUV420PPixel(frame.value(), xcoord, ycoord, IPixelFormat::YUV_V));
}

