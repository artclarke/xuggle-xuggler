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
  IVideoPicture :: make(
      com::xuggle::ferry::IBuffer* buffer,
      IPixelFormat::Type format, int width, int height)
  {
    Global::init();
    return VideoPicture::make(buffer, format, width, height);
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
    catch (std::bad_alloc &e)
    {
      VS_REF_RELEASE(retval);
      throw e;
    }
    catch (std::exception & e)
    {
      VS_LOG_DEBUG("got error: %s", e.what());
      VS_REF_RELEASE(retval);
    }
    return retval;
  }
}}}
