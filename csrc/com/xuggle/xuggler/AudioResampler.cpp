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
#include <com/xuggle/xuggler/AudioResampler.h>
#include <com/xuggle/xuggler/AudioSamples.h>
#include <com/xuggle/xuggler/Global.h>
#include <com/xuggle/xuggler/FfmpegIncludes.h>

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
    mOFmt = IAudioSamples::FMT_S16;
    mIFmt = IAudioSamples::FMT_S16;
    mFilterLen = 0;
    mLog2PhaseCount = 0;
    mIsLinear = false;
    mCutoff = 1.0;
    mNextPts = Global::NO_PTS;
  }

  AudioResampler :: ~AudioResampler()
  {
    if (mContext)
      audio_resample_close(mContext);
  }

  AudioResampler*
  AudioResampler :: make(
      int32_t outputChannels, int32_t inputChannels,
      int32_t outputRate, int32_t inputRate
      )
  {
    return make(outputChannels, inputChannels, outputRate, inputRate,
        IAudioSamples::FMT_S16, IAudioSamples::FMT_S16);
  }

  AudioResampler*
  AudioResampler :: make(
      int32_t outputChannels, int32_t inputChannels,
      int32_t outputRate, int32_t inputRate,
      IAudioSamples::Format outputFmt, IAudioSamples::Format inputFmt
      )
  {
    return make(outputChannels, inputChannels, outputRate, inputRate,
        outputFmt, inputFmt,
        16, 10, 0, 0.8);
  }
  
  AudioResampler*
  AudioResampler :: make(
      int32_t outputChannels, int32_t inputChannels,
      int32_t outputRate, int32_t inputRate,
      IAudioSamples::Format outputFmt, IAudioSamples::Format inputFmt,
      int32_t filterLen, int32_t log2PhaseCount,
      bool linear, double cutoff
      )
  {
    AudioResampler* retval = 0;
    try {
      if (outputChannels <= 0)
        throw std::invalid_argument("outputChannels <= 0");

      if (inputChannels <= 0)
        throw std::invalid_argument("inputChannels <= 0");

      if (outputChannels > 2)
        throw std::invalid_argument("outputChannels > 2; unsupported");

      if (inputChannels > 2)
        throw std::invalid_argument("inputChannels > 2; unsupported");

      if (outputRate <= 0)
        throw std::invalid_argument("outputRate <= 0");

      if (inputRate <= 0)
        throw std::invalid_argument("inputRate <= 0");

      if (filterLen <= 0)
        throw std::invalid_argument("filterLen <= 0");

      if (log2PhaseCount < 0)
        throw std::invalid_argument("log2PhaseCount < 0");

      if (cutoff < 0)
        throw std::invalid_argument("cutoffFrequency < 0");

      retval = AudioResampler::make();
      if (retval)
      {
        retval->mContext = av_audio_resample_init(outputChannels, inputChannels,
            outputRate, inputRate,
            (enum SampleFormat) outputFmt, (enum SampleFormat) inputFmt,
            filterLen, log2PhaseCount, (int)linear, cutoff);
        if (retval->mContext)
        {
          retval->mOChannels = outputChannels;
          retval->mOSampleRate = outputRate;
          retval->mIChannels = inputChannels;
          retval->mISampleRate = inputRate;
          retval->mOFmt = outputFmt;
          retval->mIFmt = inputFmt;
          retval->mFilterLen = filterLen;
          retval->mLog2PhaseCount = log2PhaseCount;
          retval->mIsLinear = linear;
          retval->mCutoff = cutoff;
        }
        else
        {
          VS_REF_RELEASE(retval);
        }
      }
    }
    catch (std::bad_alloc & e)
    {
      VS_LOG_ERROR("Error: %s", e.what());
      VS_REF_RELEASE(retval);
      throw e;
    }
    catch (std::exception & e)
    {
      VS_LOG_ERROR("Error: %s", e.what());
      VS_REF_RELEASE(retval);
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

  int32_t
  AudioResampler :: getMinimumNumSamplesRequiredInOutputSamples(
      IAudioSamples *inSamples)
  {
    int32_t retval = -1;
    try {
      int32_t numSamples = 0;
      if (inSamples)
      {
        if (!inSamples->isComplete())
          throw std::invalid_argument("input samples are not complete");
        
        if (inSamples->getSampleRate() != mISampleRate)
          throw std::invalid_argument("unexpected input sample rate");

        if (inSamples->getChannels() != mIChannels)
          throw std::invalid_argument("unexpected # of input channels");
        
        if (inSamples->getFormat() != mIFmt)
          throw std::invalid_argument("unexpected sample format");

        numSamples = inSamples->getNumSamples();
      } else {
        numSamples = 0;
      }
      retval = getMinimumNumSamplesRequiredInOutputSamples(numSamples);
    }
    catch (std::invalid_argument & e)
    {
      VS_LOG_DEBUG("invalid argument: %s", e.what());
      retval = -1;
    }
    catch (std::exception & e)
    {
      VS_LOG_DEBUG("Unknown exception: %s", e.what());
    }
    return retval;
  }
  
  int32_t
  AudioResampler :: getMinimumNumSamplesRequiredInOutputSamples(
      int32_t numSamples)
  {
    int32_t retval = -1;

    try
    {
      if (numSamples < 0)
        throw std::invalid_argument("numSamples < 0 not allowed");
      
      double conversionRatio = 1;
      {
        double top = mOSampleRate;
        VS_ASSERT(top, "should never be zero");
        double bot = mISampleRate;
        VS_ASSERT(bot, "should never be zero");
        conversionRatio = top/bot;
        VS_ASSERT(conversionRatio > 0, "the variables used should have been checked on construction");
      }
      if (conversionRatio <= 0)
        throw std::invalid_argument("programmer error");

      // FFMPEG's re-sample function doesn't let you specify the size of your
      // output buffer, but does use up all the space you might expect
      // plus 16-bytes as a lead-in/lead-out for it to seed the resampler.
      // Hence, the hard-coded 16 here.
      // NOTE: 16 might change IF the value of filters in the audio_resample
      // method in libavcodec/resample.c changes.
#define VS_FFMPEG_AUDIO_RESAMPLER_LEADIN 16
      retval = 
          (int32_t)((numSamples * conversionRatio)+VS_FFMPEG_AUDIO_RESAMPLER_LEADIN+0.5);
    }
    catch (std::invalid_argument & e)
    {
      VS_LOG_DEBUG("invalid argument: %s", e.what());
      retval = -1;
    }
    catch (std::exception & e)
    {
      VS_LOG_DEBUG("Unknown exception: %s", e.what());
    }

    return retval;
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
      
      if (outSamples == inSamples)
        throw std::invalid_argument("resampling into the same IAudioSamples is not allowed");

      // null out the output samples.
      outSamples->setComplete(false, 0, mOSampleRate, mOChannels,
          mOFmt, Global::NO_PTS);

      if (inSamples)
      {
        if (!inSamples->isComplete())
          throw std::invalid_argument("input samples are not complete");
        
        if (inSamples->getSampleRate() != mISampleRate)
          throw std::invalid_argument("unexpected input sample rate");

        if (inSamples->getChannels() != mIChannels)
          throw std::invalid_argument("unexpected # of input channels");
        
        if (inSamples->getFormat() != mIFmt)
          throw std::invalid_argument("unexpected sample format");

        if (numSamples == 0)
          numSamples = inSamples->getNumSamples();
        else
          numSamples = FFMIN(numSamples, inSamples->getNumSamples());
        sampleSize = inSamples->getSampleBitDepth()/8;
      } else {
        numSamples = 0;
        sampleSize = IAudioSamples::findSampleBitDepth(mIFmt)/8;
      }

      int32_t neededSamples = getMinimumNumSamplesRequiredInOutputSamples(numSamples);
      int32_t bytesPerOutputSample = mOChannels*IAudioSamples::findSampleBitDepth(mOFmt)/8;
      int32_t neededBytes = neededSamples * bytesPerOutputSample;
      // This causes a buffer resize to occur if needed
      if (outSamples->ensureCapacity(neededBytes) < 0)
        throw std::runtime_error("attempted to resize output buffer but failed");
      
      uint32_t outBufSize = outSamples->getMaxBufferSize();
      int32_t gap = (neededSamples*bytesPerOutputSample)-outBufSize;
      
      if (gap > 0) {
//        VS_LOG_ERROR("maxBufferSize: %d; neededSampleRoom: %d; sampleSize: %d; numSamples: %d; conversionRatio: %f;",
//            (int32_t)outSamples->getMaxBufferSize(),
//            neededSampleRoom,
//            sampleSize,
//            numSamples,
//            conversionRatio);
        throw std::invalid_argument("not enough room in output buffer");
      }
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
            mOFmt,
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
  
  IAudioSamples::Format
  AudioResampler :: getOutputFormat()
  {
    return mOFmt;
  }
  
  IAudioSamples::Format
  AudioResampler :: getInputFormat()
  {
    return mIFmt;
  }
  
  int32_t 
  AudioResampler :: getFilterLen()
  {
    return mFilterLen;
  }
  
  int32_t
  AudioResampler :: getLog2PhaseCount()
  {
    return mLog2PhaseCount;
  }
  
  bool 
  AudioResampler :: isLinear()
  {
    return mIsLinear;
  }
  
  double
  AudioResampler :: getCutoffFrequency()
  {
    return mCutoff;
  }  

  }}}
