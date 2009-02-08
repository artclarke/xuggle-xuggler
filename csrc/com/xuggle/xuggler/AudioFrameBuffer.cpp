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
#include <com/xuggle/xuggler/AudioFrameBuffer.h>
#include <com/xuggle/xuggler/Global.h>
#include <com/xuggle/ferry/Logger.h>
#include <stdexcept>
#include <cstring>

VS_LOG_SETUP(VS_CPP_PACKAGE);

using namespace com::xuggle::ferry;

namespace com { namespace xuggle { namespace xuggler {

  AudioFrameBuffer :: AudioFrameBuffer(
      RefCounted *aRequestor,
      int32_t aFrameSize,
      int32_t aAudioChannels,
      int32_t aSampleRate,
      int32_t aBitsPerSample)
  {
    if (aFrameSize <= 0)
      throw std::invalid_argument("must pass in frame size > 0");
    
    if (aAudioChannels <= 0)
      throw std::invalid_argument("must have at least one channel");
    
    if (aSampleRate <= 0)
      throw std::invalid_argument("must pass in sample size > 0");
    
    if (aBitsPerSample <= 0)
      throw std::invalid_argument("must pass in sample bit depth > 0");
       
    mOffsetInCurrentSamples = 0;
    mChannels = aAudioChannels;
    mSampleRate = aSampleRate;
    mBitDepth = aBitsPerSample;
    mFrameSize = aFrameSize;
    mFrame = IBuffer::make(aRequestor, mFrameSize*getBytesPerSample());
  }

  AudioFrameBuffer :: ~AudioFrameBuffer()
  {
  }

  int32_t
  AudioFrameBuffer :: addSamples(IAudioSamples *aInputSamples)
  {
    int32_t retval = -1;
    try
    {
      if (!aInputSamples)
        throw std::invalid_argument("null input samples");
      
      if (aInputSamples->getChannels() != mChannels)
        throw std::invalid_argument("wrong number of channels in input");
      
      if (aInputSamples->getSampleRate() != mSampleRate)
        throw std::invalid_argument("wrong sample rate in input");
      
      if ((int32_t)aInputSamples->getSampleBitDepth() != mBitDepth)
        throw std::invalid_argument("wrong bit depth in input");
      
      AudioSamples* inputSamples = dynamic_cast<AudioSamples*>(aInputSamples);
      if (!inputSamples)
        throw std::invalid_argument("wrong underlying C++ type");
      
      // Just add these samples to our queue for processing later
      RefPointer<AudioSamples> p;
      p.reset(inputSamples, true);
      mQueue.push_back(p);
      
      // and register success
      retval = (int32_t) inputSamples->getNumSamples();
    }
    catch (std::exception & e)
    {
      VS_LOG_DEBUG("caught exception: %s", e.what());
      retval = -1;
    }
    return retval;    
  }

  int32_t
  AudioFrameBuffer :: getNextFrame(
      void ** aOutputBuf,
      int32_t *aSamplesWritten,
      int64_t *aStartingTimestamp)
  {
    int32_t retval = -1;
    try
    {
      int32_t samplesWritten = 0;
      int64_t startingTimestamp = Global::NO_PTS;
      bool firstSampleWritten = false;

      bool frameAvailable = isFrameAvailable();
      
      // Make sure we free up references to any
      // no longer used current samples because
      // this method was called.  This deals with the
      // case where getNextFrame() is called without
      // any more data available
      if (mCurrentSamples && 
          mOffsetInCurrentSamples >= (int32_t)mCurrentSamples->getNumSamples())
      {
        // reset the current
        mCurrentSamples = 0;
      }
      
      while(frameAvailable && samplesWritten < mFrameSize)
      {
        // Check if we're already processing samples
        if (mCurrentSamples && 
            mOffsetInCurrentSamples >= (int32_t)mCurrentSamples->getNumSamples())
        {
          // reset the current
          mCurrentSamples = 0;
        }
        if (!mCurrentSamples)
        {
          if (mQueue.empty()) {
            // we've written as much data as we have
            break;
          }
          mCurrentSamples = mQueue.front();
          mQueue.pop_front();
          VS_ASSERT("should never be null", mCurrentSamples);
          mOffsetInCurrentSamples = 0;
        }
        int32_t availableSamples = mCurrentSamples->getNumSamples()-mOffsetInCurrentSamples;
        if (!firstSampleWritten)
        {
          // Get this timestamp as it'll be the one returned
          startingTimestamp = mCurrentSamples->getTimeStamp() +
            IAudioSamples::samplesToDefaultPts(mOffsetInCurrentSamples,
                mSampleRate);
          firstSampleWritten=true;
        }
        
        int32_t neededSamples = mFrameSize - samplesWritten;
        int32_t samplesToWrite = neededSamples < availableSamples ? neededSamples : availableSamples;
 
        // write the samples
        if (aOutputBuf)
        {
          // First an optimization; if we haven't written any samples
          // yet on this call, and available samples are more than the
          // requested samples, we can just use the buffer of the current
          // samples
          if (samplesWritten == 0 && availableSamples >= neededSamples)
          {
            *aOutputBuf = mCurrentSamples->getRawSamples(mOffsetInCurrentSamples);
          } else {
            void * inputBytes = mCurrentSamples->getRawSamples(mOffsetInCurrentSamples);
            void * outputBytes = ((char*)mFrame->getBytes(0, mFrame->getBufferSize()));
            void * copyDst = (char*)outputBytes + (samplesWritten * getBytesPerSample());
            int32_t bytesToWrite = samplesToWrite * getBytesPerSample();
            memcpy(copyDst, inputBytes, bytesToWrite);
            *aOutputBuf = outputBytes;
          }
        }
        samplesWritten += samplesToWrite;
        // update the offset
        mOffsetInCurrentSamples += samplesToWrite;
      }
  
      if (aSamplesWritten)
        *aSamplesWritten = samplesWritten;
      if (aStartingTimestamp)
        *aStartingTimestamp = startingTimestamp;
      
      // and call this success
      retval = samplesWritten;
    }
    catch (std::exception & e)
    {
      VS_LOG_DEBUG("caught exception: %s", e.what());
      retval = -1;
    }
    return retval;
  }

  bool
  AudioFrameBuffer :: isFrameAvailable()
  {
    int samplesAvailable = 0;
    // first compute samples available in current samples
    if (mCurrentSamples)
    {
      samplesAvailable += mCurrentSamples->getNumSamples()-mOffsetInCurrentSamples;
    }
    // and keep counting queued audio until we've counted everything
    // or confirmed we have a frame available
    AudioSamplesQueue::iterator iter=mQueue.begin();
    AudioSamplesQueue::iterator end=mQueue.end();
    for( ; samplesAvailable < mFrameSize && iter != end; ++iter)
    {
      samplesAvailable += (*iter)->getNumSamples();
    }
    
    return (samplesAvailable >= mFrameSize);
  }
}}}
