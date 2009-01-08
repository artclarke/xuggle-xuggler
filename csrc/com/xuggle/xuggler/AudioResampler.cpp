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
#include <com/xuggle/ferry/RefPointer.h>
#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/xuggler/AudioResampler.h>
#include <com/xuggle/xuggler/FfmpegIncludes.h>
#include <com/xuggle/xuggler/AudioSamples.h>
#include <com/xuggle/xuggler/Global.h>

#include <stdexcept>

VS_LOG_SETUP(VS_CPP_PACKAGE);

namespace com { namespace xuggle { namespace xuggler
  {
  using namespace com::xuggle::ferry;

  AudioResampler :: AudioResampler()
  {
    mContext = 0;
    mOChannels=0;
    mOSampleRate=0;
    mIChannels=0;
    mISampleRate=0;
    mPtsOffset=0;
    mNextPts = Global::NO_PTS;
  }

  AudioResampler :: ~AudioResampler()
  {
    if (mContext)
      audio_resample_close(mContext);
  }

  AudioResampler*
  AudioResampler :: make(int outputChannels, int inputChannels,
      int outputRate, int inputRate)
  {
    AudioResampler* retval = 0;

    VS_LOG_TRACE("Initalizing audiosampler");
    if (outputChannels <= 0 ||
        inputChannels <= 0 ||
        outputChannels > 2 ||
        inputChannels > 2 ||
        outputRate <= 0 ||
        inputRate <= 0)
    {
      VS_LOG_TRACE("cannot support more than 2 channels, or less than 0 anything");
    } else {
      retval = AudioResampler::make();
      if (retval)
      {
        retval->mContext = audio_resample_init(outputChannels, inputChannels,
            outputRate, inputRate);
        if (retval->mContext)
        {
          retval->mOChannels = outputChannels;
          retval->mOSampleRate = outputRate;
          retval->mIChannels = inputChannels;
          retval->mISampleRate = inputRate;
        }
        else
        {
          VS_REF_RELEASE(retval);
        }
      }
    }
    return retval;
  }

  int
  AudioResampler :: getOutputChannels()
  {
    VS_ASSERT(mContext, "no context");
    return mOChannels;
  }

  int
  AudioResampler :: getOutputRate()
  {
    VS_ASSERT(mContext, "no context");
    return mOSampleRate;
  }

  int
  AudioResampler :: getInputChannels()
  {
    VS_ASSERT(mContext, "no context");
    return mIChannels;
  }

  int
  AudioResampler :: getInputRate()
  {
    VS_ASSERT(mContext, "no context");
    return mISampleRate;
  }

  int
  AudioResampler :: resample(IAudioSamples * pOutSamples,
      IAudioSamples* pInSamples,
      unsigned int numSamples)
  {
    int retval = -1;
    AudioSamples* outSamples = dynamic_cast<AudioSamples*>(pOutSamples);
    AudioSamples* inSamples = dynamic_cast<AudioSamples*>(pInSamples);
    unsigned int sampleSize=0;

    try {
      if (!outSamples)
        throw std::invalid_argument("no output samples");

      // null out the output samples.
      outSamples->setComplete(false, 0, mOSampleRate, mOChannels,
          IAudioSamples::FMT_S16, Global::NO_PTS);

      if (inSamples)
      {
        if (inSamples->getSampleRate() != mISampleRate)
          throw std::invalid_argument("unexpected input sample rate");

        if (inSamples->getChannels() != mIChannels)
          throw std::invalid_argument("unexpected # of input channels");

        if (numSamples == 0)
          numSamples = inSamples->getNumSamples();
        else
          numSamples = FFMIN(numSamples, inSamples->getNumSamples());
        sampleSize = inSamples->getSampleBitDepth()/8;
      } else {
        numSamples = 0;
        sampleSize = sizeof(short);
      }

      double conversionRatio = 1;
      {
        double top = mOSampleRate*mOChannels;
        VS_ASSERT(top, "should never be zero");
        double bot = mISampleRate*mIChannels;
        VS_ASSERT(bot, "should never be zero");
        conversionRatio = top/bot;
        VS_ASSERT(conversionRatio > 0, "the variables used should have been checked on construction");
      }
      if (conversionRatio <= 0)
        throw std::invalid_argument("programmer error");

      // FFMPEG's encode function doesn't let you specify the size of your
      // output buffer, but does use up all the space you might expect
      // plus 16-bytes as a lead-in/lead-out for it to seed the resampler.
      // Hence, the hard-coded 16 here.
      // NOTE: 16 might change IF the value of filters in the audio_resample
      // method in libavcodec/resample.c changes.
      unsigned int neededSampleRoom = (unsigned int)(numSamples * conversionRatio)+16;
      // leave -1 space for rounding
      if (outSamples->getMaxBufferSize() < neededSampleRoom * sampleSize)
        throw std::invalid_argument("not enough room in output buffer");

      short * inBuf = inSamples ? inSamples->getRawSamples(0) : 0;

      short *outBuf = outSamples->getRawSamples(0);
      if (!outBuf)
        throw std::invalid_argument("could not get output bytes");

      VS_ASSERT(mContext, "Should have been set at initialization");
      if (!mContext)
        throw std::invalid_argument("programmer error");

      // Now we should be far enough along that we can safely try a resample.
      retval = audio_resample(mContext, outBuf, inBuf, numSamples);

#if 0
      if (retval >0){
        char string[2048*16+1];
        unsigned int i=0;
        for (i = 0; i < sizeof(string)-1; i++)
          string[i] = 'X';

        bool allZero = true;

        for (i=0; i< FFMIN(numSamples, 2000);i++)
        {
          snprintf(string+(5*i), sizeof(string)-5*i, "%04hX.", inBuf[i]);
          if (inBuf[i] != 0)
            allZero = false;
        }
        VS_LOG_DEBUG("Input Buffer (%d): %s", numSamples, string);

        for (i=0; i< FFMIN((unsigned int)retval, 2000);i++)
        {
          snprintf(string+(9*i), sizeof(string)-9*i, "%04hX%04hX.",
              outBuf[2*i], outBuf[2*i+1]);
          if (outBuf[2*i] != 0 || outBuf[2*i+1] != 0)
            allZero = false;
        }
        VS_LOG_DEBUG("Output Buffer (%d): %s", retval, string);
        if (!allZero)
          VS_LOG_DEBUG("Got an audio buffer with content");
      }
#endif // 0

      if (retval >= 0)
      {
        // copy the Pts
        int64_t pts = Global::NO_PTS;
        if (inSamples)
        {
          pts = inSamples->getPts();
          mNextPts = pts + IAudioSamples::samplesToDefaultPts(retval, mOSampleRate);
        }
        else
        {
          pts = mNextPts;
        }
        if (pts != Global::NO_PTS)
          pts += mPtsOffset;

        outSamples->setComplete(true, retval,
            mOSampleRate, mOChannels,
            inSamples ? inSamples->getFormat() : IAudioSamples::FMT_S16,
                pts);
        int expectedSamples = 0;
        if (inSamples)
        {
          double top = mOSampleRate;
          double bottom = mISampleRate;
          double sampleOnlyConverstionRatio = top / bottom;
          expectedSamples = (int)(numSamples * sampleOnlyConverstionRatio);
        }
        else
        {
          VS_LOG_TRACE("Got null samples; outputted all cached and set pts offset from %lld to 0",
              mPtsOffset);
          expectedSamples = retval;
          // and reset the offset
          mPtsOffset = 0;
        }

        if (retval != expectedSamples)
        {
          // we got a different number of samples than expected; we need to update
          // our pts offset
          int sampleDelta = retval - expectedSamples;
          int64_t ptsDelta = IAudioSamples::samplesToDefaultPts(sampleDelta, mOSampleRate);
          mPtsOffset += ptsDelta;
          VS_LOG_TRACE("Resample ate some samples (cr: %f): %d vs %d (%d); adjust PTS offset by %lld to %lld; pts set to %lld",
              conversionRatio,
              expectedSamples,
              retval,
              sampleDelta,
              ptsDelta,
              mPtsOffset,
              mNextPts);
        }
      }
    }
    catch (std::invalid_argument & e)
    {
      VS_LOG_DEBUG("invalid argument: %s", e.what());
      retval = -1;
    }

    return retval;
  }
  }}}
