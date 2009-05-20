/*
 * This file is part of Xuggler.
 * 
 * Xuggler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public
 * License along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
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
