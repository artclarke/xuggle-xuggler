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
#include "IAudioResampler.h"
#include "Global.h"
#include "AudioResampler.h"

namespace com { namespace xuggle { namespace xuggler
  {

  IAudioResampler :: IAudioResampler()
  {
  }

  IAudioResampler :: ~IAudioResampler()
  {
  }

  IAudioResampler*
  IAudioResampler:: make(int32_t outputChannels, int32_t inputChannels,
          int32_t outputRate, int32_t inputRate)
  {
    Global::init();
    return AudioResampler::make(outputChannels, inputChannels,
        outputRate, inputRate);
  }

  IAudioResampler*
  IAudioResampler :: make(int32_t outputChannels, int32_t inputChannels,
      int32_t outputRate, int32_t inputRate,
      IAudioSamples::Format outputFmt, IAudioSamples::Format inputFmt)
  {
    Global::init();
    return AudioResampler::make(outputChannels, inputChannels,
        outputRate, inputRate,
        outputFmt, inputFmt);
  }


  IAudioResampler*
  IAudioResampler :: make(int32_t outputChannels, int32_t inputChannels,
      int32_t outputRate, int32_t inputRate,
      IAudioSamples::Format outputFmt, IAudioSamples::Format inputFmt,
      int32_t filterLen, int32_t log2PhaseCount,
      bool isLinear, double cutoff)
  {
    Global::init();
    return AudioResampler::make(outputChannels, inputChannels,
        outputRate, inputRate,
        outputFmt, inputFmt,
        filterLen, log2PhaseCount,
        isLinear, cutoff);
  }

  }}}
