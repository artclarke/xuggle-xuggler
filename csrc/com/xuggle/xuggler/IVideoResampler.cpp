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

#include <com/xuggle/xuggler/IVideoResampler.h>
#include <com/xuggle/xuggler/Global.h>
#include <com/xuggle/xuggler/VideoResampler.h>
#include <com/xuggle/xuggler/config.h>

namespace com { namespace xuggle { namespace xuggler
  {

  IVideoResampler :: IVideoResampler()
  {
  }

  IVideoResampler :: ~IVideoResampler()
  {
  }

  IVideoResampler*
  IVideoResampler :: make(
      int32_t outputWidth, int32_t outputHeight,
      IPixelFormat::Type outputFmt,
      int32_t inputWidth, int32_t inputHeight,
      IPixelFormat::Type inputFmt)
  {
    Global::init();
#ifdef VS_ENABLE_GPL
    return VideoResampler::make(outputWidth, outputHeight, outputFmt,
        inputWidth, inputHeight, inputFmt);
#else
    // Avoid compiler warnings about unused parameters
    (void) outputWidth;
    (void) outputHeight;
    (void) outputFmt;
    (void) inputWidth;
    (void) inputHeight;
    (void) inputFmt;
    throw std::invalid_argument("IVideoResampler not supported in this build"); 
#endif
  }

  bool
  IVideoResampler :: isSupported(Feature aFeature)
  {
    (void)aFeature; // ignored for now, but might change
#ifdef VS_ENABLE_GPL
    return true;
#else
    return false;
#endif
  }
  }}}
