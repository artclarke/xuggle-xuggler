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

#include "IAudioSamples.h"
#include "Global.h"
#include "AudioSamples.h"

#include "FfmpegIncludes.h"

namespace com { namespace xuggle { namespace xuggler
{

  IAudioSamples :: IAudioSamples()
  {
  }

  IAudioSamples :: ~IAudioSamples()
  {
  }
  
  uint32_t
  IAudioSamples :: findSampleBitDepth(Format format)
  {
    int bits = av_get_bits_per_sample_format((enum SampleFormat) format);
    return bits;
  }

  IAudioSamples*
  IAudioSamples :: make(uint32_t numSamples, uint32_t numChannels)
  {
    Global::init();
    return AudioSamples::make(numSamples, numChannels);
  }
  
  int64_t
  IAudioSamples :: samplesToDefaultPts(int64_t samples, int sampleRate)
  {
    // Note: These need to always round up!  a "partial sample" actually must
    // be whole (and similar with time stamps).
    int64_t retval = Global::NO_PTS;
    Global::init();
    if (sampleRate > 0)
    {
      int64_t num = samples * Global::DEFAULT_PTS_PER_SECOND;
      int64_t den = sampleRate;
      long double calc = ((long double)num)/((long double)den);
      retval = (int64_t)(calc);
    }
    return retval;
  }

  int64_t
  IAudioSamples :: defaultPtsToSamples(int64_t duration, int sampleRate)
  {
    int64_t retval = 0;
    Global::init();
    if (duration != Global::NO_PTS)
    {
      int64_t num = duration * sampleRate;
      int64_t den = Global::DEFAULT_PTS_PER_SECOND;
      long double calc = ((long double)num)/((long double)den);
      retval = (int64_t)(calc);
    }
    return retval;
  }


}}}
