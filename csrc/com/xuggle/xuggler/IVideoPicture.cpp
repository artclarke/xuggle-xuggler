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
#include <com/xuggle/ferry/Logger.h>

#include "IVideoPicture.h"
#include "Global.h"
#include "VideoPicture.h"
#include <stdexcept>

VS_LOG_SETUP(VS_CPP_PACKAGE);

namespace com { namespace xuggle { namespace xuggler
{

  IVideoPicture :: IVideoPicture()
  {
  }

  IVideoPicture :: ~IVideoPicture()
  {
  }

  IVideoPicture*
  IVideoPicture :: make(IPixelFormat::Type format, int width, int height)
  {
    Global::init();
    return VideoPicture::make(format, width, height);
  }
  
  IVideoPicture*
  IVideoPicture :: make(IVideoPicture *srcFrame)
  {
    IVideoPicture* retval = 0;
    Global::init();
    try
    {
      if (!srcFrame)
        throw std::runtime_error("no source data to copy");

      retval = IVideoPicture::make(srcFrame->getPixelType(), srcFrame->getWidth(), srcFrame->getHeight());
      if (!retval)
        throw std::runtime_error("could not allocate new frame");
      
      if (!retval->copy(srcFrame))
        throw std::runtime_error("could not copy source frame");
    }
    catch (std::exception & e)
    {
      VS_LOG_DEBUG("got error: %s", e.what());
      VS_REF_RELEASE(retval);
    }
    return retval;
  }
}}}
