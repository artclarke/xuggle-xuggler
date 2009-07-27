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

#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/xuggler/AudioSamples.h>
#include <com/xuggle/xuggler/Global.h>

// for memset
#include <com/xuggle/xuggler/FfmpegIncludes.h>
#include <cstring>
#include <stdexcept>

VS_LOG_SETUP(VS_CPP_PACKAGE);

namespace com { namespace xuggle { namespace xuggler
{
  using namespace com::xuggle::ferry;
  
  AudioSamples :: AudioSamples()
  {
    mSamples = 0;
    mNumSamples = 0;
    mSampleRate = 0;
    mChannels = 1;
    mIsComplete = false;
    mSampleFmt = FMT_S16;
    mPts = Global::NO_PTS;
    mRequestedSamples = 0;
    mTimeBase = IRational::make(1, 1000000);
  }

  AudioSamples :: ~AudioSamples()
  {
    mSamples = 0;
    mNumSamples = 0;
    mSampleRate = 0;
    mChannels = 1;
    mIsComplete = false;
    mSampleFmt = FMT_S16;
    mPts = Global::NO_PTS;
    mRequestedSamples = 0;
  }

  void
  AudioSamples :: allocInternalSamples()
  {
    if (!mSamples)
    {
      int32_t bufSize = FFMAX(AVCODEC_MAX_AUDIO_FRAME_SIZE,
          mRequestedSamples * getSampleSize()) +
          FF_INPUT_BUFFER_PADDING_SIZE;

      mSamples = IBuffer::make(this, bufSize);
      if (!mSamples)
        throw std::bad_alloc();
      setBufferType(mSampleFmt, mSamples.value());

      if (mSamples)
      {
        //void * buf = retval->mSamples->getBytes(0, bufSize);
        //don't set to zero; this means the memory is uninitialized
        //which helps find bugs with valgrind
        //memset(buf, 0, bufSize);
        mNumSamples = 0;
        VS_LOG_TRACE("Created AudioSamples(%d bytes)", bufSize);
      }
      
    }
  }
  short*
  AudioSamples :: getRawSamples(uint32_t startingSample)
  {
    short *retval = 0;
    allocInternalSamples();
    if (mSamples)
    {
      uint32_t startingOffset = startingSample*getSampleSize();
      uint32_t bufLen = (mNumSamples*getSampleSize())-startingOffset;
      retval = (short*)mSamples->getBytes(startingOffset, bufLen);
    }
    return retval;
  }

  AudioSamples*
  AudioSamples :: make(uint32_t numSamples,
      uint32_t numChannels)
  {
    return make(numSamples, numChannels, IAudioSamples::FMT_S16);
  }
  
  AudioSamples*
  AudioSamples :: make(uint32_t numSamples,
      uint32_t numChannels,
      IAudioSamples::Format format)
  {
    AudioSamples *retval=0;
    if (numSamples > 0 && numChannels > 0)
    {
      retval = AudioSamples::make();
      if (retval)
      {
        // FFMPEG actually requires a minimum buffer size, so we
        // make sure we're always at least that large.
        retval->mChannels = numChannels;
        retval->mSampleFmt = format;
        retval->mRequestedSamples = numSamples;
      }
    }
    return retval;
  }
  
  AudioSamples*
  AudioSamples :: make(IBuffer* buffer, int channels,
      IAudioSamples::Format format)
  {
    if (!buffer)
      return 0;
    if (format == IAudioSamples::FMT_NONE)
      return 0;
    if (channels < 0)
      return 0;
    if (buffer->getBufferSize()<= 0)
      return 0;
    
    int bytesPerSample = IAudioSamples::findSampleBitDepth(format)/8*channels;
    int samplesRequested = buffer->getBufferSize()/bytesPerSample;
    AudioSamples* retval = 0;
    try
    {
      retval = make(samplesRequested, channels, format);
      if (!retval)
        return 0;
      retval->setData(buffer);
    }
    catch (std::bad_alloc &e)
    {
      VS_REF_RELEASE(retval);
      throw e;
    }
    catch (std::exception& e)
    {
      VS_LOG_DEBUG("error: %s", e.what());
      VS_REF_RELEASE(retval);
    }

    return retval;
  }

  void
  AudioSamples :: setData(com::xuggle::ferry::IBuffer* buffer)
  {
    if (!buffer) return;
    mSamples.reset(buffer, true);
    setBufferType(mSampleFmt, buffer);
  }
  
  bool
  AudioSamples :: isComplete()
  {
    return mIsComplete;
  }

  IAudioSamples::Format
  AudioSamples :: getFormat()
  {
    return mSampleFmt;
  }

  int32_t
  AudioSamples :: getSampleRate()
  {
    return mSampleRate;
  }

  int32_t
  AudioSamples :: getChannels()
  {
    return mChannels;
  }

  uint32_t
  AudioSamples :: getNumSamples()
  {
    return mNumSamples;
  }

  uint32_t
  AudioSamples :: getMaxBufferSize()
  {
    allocInternalSamples();
    return mSamples->getBufferSize()-FF_INPUT_BUFFER_PADDING_SIZE;
  }

  uint32_t
  AudioSamples :: getSampleBitDepth()
  {
    return IAudioSamples::findSampleBitDepth(mSampleFmt);
  }

  uint32_t
  AudioSamples :: getSampleSize()
  {
    uint32_t bits = getSampleBitDepth();
    if (bits < 8)
      bits = 8;

    return bits/8 * getChannels();
  }

  com::xuggle::ferry::IBuffer*
  AudioSamples :: getData()
  {
    allocInternalSamples();
    IBuffer* retval = mSamples.get();
    if (!retval)
      throw std::bad_alloc();
    return retval;
  }

  uint32_t
  AudioSamples :: getMaxSamples()
  {
    return getMaxBufferSize() / getSampleSize();
  }

  void
  AudioSamples :: setComplete(bool complete, uint32_t numSamples,
      int32_t sampleRate, int32_t channels, Format format,
      int64_t pts)
  {
    mIsComplete = complete;
    if (channels <= 0)
      channels = 1;

    mChannels = channels;
    mSampleRate = sampleRate;
    mSampleFmt = format;
    if (mSamples)
      // if we've allocated a buffer, reset the type
      setBufferType(mSampleFmt, mSamples.value());

    if (mIsComplete)
    {
      mNumSamples = FFMIN(numSamples,
          getMaxBufferSize()/(getSampleSize()));
#if 0
      {
        short* samps = this->getRawSamples(0);
        for(uint32_t i = 0; i < mNumSamples;i++)
        {
          int32_t samp = samps[i];
          VS_LOG_DEBUG("i: %d; samp: %d", i, samp);
        }
      }
#endif // VS_DEBUG
    } else {
      mNumSamples = 0;
    }
    setPts(pts);
  }

  int64_t
  AudioSamples :: getPts()
  {
    return mPts;
  }
  
  void
  AudioSamples :: setPts(int64_t aValue)
  {
    mPts = aValue;
  }

  int64_t
  AudioSamples :: getNextPts()
  {
    int64_t retval = Global::NO_PTS;
    if (mPts != Global::NO_PTS)
      retval = mPts + IAudioSamples::samplesToDefaultPts(this->getNumSamples(), this->getSampleRate());

    return retval;
  }

  int32_t
  AudioSamples :: setSample(uint32_t sampleIndex, int32_t channel, Format format, int32_t sample)
  {
    int32_t retval = -1;
    try {
      if (channel < 0 || channel >= mChannels)
        throw std::invalid_argument("cannot setSample for given channel");
      if (format != FMT_S16)
        throw std::invalid_argument("only support format: FMT_S16");
      if (sampleIndex >= this->getMaxSamples())
        throw std::invalid_argument("sampleIndex out of bounds");

      short *rawSamples = this->getRawSamples(0);
      if (!rawSamples)
        throw std::runtime_error("no samples buffer set in AudioSamples");

      rawSamples[sampleIndex*mChannels + channel] = (short)sample;
      retval = 0;
    }
    catch (std::exception & e)
    {
      VS_LOG_DEBUG("Error: %s", e.what());
      retval = -1;
    }
    return retval;
  }

  int32_t
  AudioSamples :: getSample(uint32_t sampleIndex, int32_t channel, Format format)
  {
    int32_t retval = 0;
    try
    {
      if (channel < 0 || channel >= mChannels)
        throw std::invalid_argument("cannot getSample for given channel");
      if (format != FMT_S16)
        throw std::invalid_argument("only support format: FMT_S16");
      if (sampleIndex >= this->getNumSamples())
        throw std::invalid_argument("sampleIndex out of bounds");

      short *rawSamples = this->getRawSamples(0);
      if (!rawSamples)
        throw std::runtime_error("no samples buffer set in AudioSamples");

      retval = rawSamples[sampleIndex*mChannels + channel];
    }
    catch(std::exception & e)
    {
      VS_LOG_DEBUG("Error: %s", e.what());
      retval = 0;
    }
    return retval;
  }

  void
  AudioSamples :: setBufferType(IAudioSamples::Format format,
      IBuffer* buffer)
  {
    if (!buffer)
      return;
    switch(format)
    {
      case FMT_FLT:
        buffer->setType(IBuffer::IBUFFER_FLT32);
        break;
      case FMT_S16:
        buffer->setType(IBuffer::IBUFFER_SINT16);
        break;
      case FMT_S24:
        // Dear god.. don't use this type of audio
        buffer->setType(IBuffer::IBUFFER_UINT8);
        break;
      case FMT_S32:
        buffer->setType(IBuffer::IBUFFER_SINT32);
        break;
      case FMT_U8:
        buffer->setType(IBuffer::IBUFFER_UINT8);
        break;
      default:
        break;
    }
  }
}}}
