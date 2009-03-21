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
/*
 * ContainerParameters.cpp
 *
 *  Created on: Mar 20, 2009
 *      Author: aclarke
 */

#include <com/xuggle/xuggler/ContainerParameters.h>

namespace com { namespace xuggle { namespace xuggler
{

ContainerParameters :: ContainerParameters()
{
  memset(&mParameters, 0, sizeof(mParameters));
  mStandard[sizeof(mStandard)-1] = 0;
  mStandard[0] = 0;
}

ContainerParameters :: ~ContainerParameters()
{
}

IRational *
ContainerParameters :: getTimeBase()
{
  return IRational::make(mParameters.time_base.num, mParameters.time_base.den);
}

void
ContainerParameters :: setTimeBase(IRational* base)
{
  if (base) {
    mParameters.time_base.num = base->getNumerator();
    mParameters.time_base.den = base->getDenominator();
  }
}

int32_t
ContainerParameters :: getAudioSampleRate()
{
  return mParameters.sample_rate;
}

void
ContainerParameters :: setAudioSampleRate(int32_t sampleRate)
{
  if (sampleRate > 0)
    mParameters.sample_rate=sampleRate;
}

int32_t 
ContainerParameters :: getAudioChannels()
{
  return mParameters.channels;
}
void
ContainerParameters :: setAudioChannels(int32_t channels)
{
  if (channels > 0)
    mParameters.channels = channels;
}

int32_t
ContainerParameters :: getVideoWidth()
{
  return mParameters.width;
}

void
ContainerParameters :: setVideoWidth(int32_t width)
{
  if (width > 0)
    mParameters.width = width;
}

int32_t
ContainerParameters :: getVideoHeight()
{
  return mParameters.height;
}

void ContainerParameters :: setVideoHeight(int32_t height)
{
  if (height > 0)
    mParameters.height = height;
}

IPixelFormat::Type
ContainerParameters :: getPixelFormat()
{
  return (IPixelFormat::Type)mParameters.pix_fmt;
}

void ContainerParameters :: setPixelFormat(IPixelFormat::Type type)
{
  mParameters.pix_fmt = (enum PixelFormat) type;
}

int32_t
ContainerParameters :: getTVChannel()
{
  return mParameters.channel;
}
void
ContainerParameters :: setTVChannel(int32_t channel)
{
  if (channel >= 0)
    mParameters.channel = channel;
}

const char*
ContainerParameters :: getTVStandard()
{
  return *mParameters.standard ? mParameters.standard : 0;
}

void
ContainerParameters :: setTVStandard(const char* standard)
{
  if (standard && *standard)
  {
    strncpy(mStandard, standard, sizeof(mStandard)-1);
    mParameters.standard = mStandard;
  } else {
    mParameters.standard = 0;
  }
} 

bool
ContainerParameters :: isMPEG2TSRaw()
{
  return mParameters.mpeg2ts_raw;
}

void
ContainerParameters :: setMPEG2TSRaw(bool setting)
{
  mParameters.mpeg2ts_raw = setting;
}

bool
ContainerParameters :: isMPEG2TSComputePCR()
{
  return mParameters.mpeg2ts_compute_pcr;
}
void
ContainerParameters :: setMPEG2TSComputePCR(bool setting)
{
  mParameters.mpeg2ts_compute_pcr=setting;
}

bool
ContainerParameters :: isInitialPause()
{
  return mParameters.initial_pause;
}

void
ContainerParameters :: setInitialPause(bool setting)
{
  mParameters.initial_pause=setting;
}

AVFormatParameters*
ContainerParameters :: getAVParameters()
{
  return &mParameters;
}

}}}
