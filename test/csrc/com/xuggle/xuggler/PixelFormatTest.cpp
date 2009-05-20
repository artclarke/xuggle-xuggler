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

