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
#include "AudioFrameBufferTest.h"
#include <com/xuggle/ferry/RefPointer.h>
#include <com/xuggle/xuggler/Global.h>
#include <com/xuggle/xuggler/AudioSamples.h>
#include <com/xuggle/ferry/Logger.h>
#include <stdexcept>

VS_LOG_SETUP(VS_CPP_PACKAGE);

AudioFrameBufferTest :: AudioFrameBufferTest()
{
}

AudioFrameBufferTest :: ~AudioFrameBufferTest()
{
}

void
AudioFrameBufferTest :: setUp()
{
}

void
AudioFrameBufferTest :: tearDown()
{
}

void
AudioFrameBufferTest :: testSuccess()
{
  AudioFrameBuffer afb(0,
      1, 1, 22050, 16);
  VS_TUT_ENSURE("should be empty", !afb.isFrameAvailable());
}

void AudioFrameBufferTest::testAddSamples()
{
  const int32_t frameSize=15;
  const int32_t channels = 1;
  const int32_t sampleRate = 22050;
  const int32_t bitDepth=16;
  const int32_t numSamples=1024;
  
  RefPointer<IAudioSamples> samples = IAudioSamples::make(numSamples, channels);
  samples->setComplete(true,
      numSamples, sampleRate, channels, IAudioSamples::FMT_S16,
      0);

  VS_TUT_ENSURE_EQUALS("we should be the only person with a ref",
      samples->getCurrentRefCount(), 1);
  {
    AudioFrameBuffer afb(0,
        frameSize, channels, sampleRate, bitDepth);
    VS_TUT_ENSURE("should be empty", !afb.isFrameAvailable());

    int32_t retval = afb.addSamples(samples.value());
    VS_TUT_ENSURE("should succeed", retval >= 0);

    VS_TUT_ENSURE("the AFB should get a ref",
        samples->getCurrentRefCount() > 1);

    VS_TUT_ENSURE("should have frame now", afb.isFrameAvailable());
  }
  // All references should be released with AFB is destroyed
  VS_TUT_ENSURE_EQUALS("we should be the only person with a ref",
      samples->getCurrentRefCount(), 1);
  
}

void
AudioFrameBufferTest :: testAddSamplesInvalidArguments()
{
  const int32_t frameSize=15;
  const int32_t channels = 1;
  const int32_t sampleRate = 22050;
  const int32_t bitDepth=16;
  const int32_t numSamples=1024;
  
  RefPointer<IAudioSamples> samples = IAudioSamples::make(numSamples, channels);
  samples->setComplete(true,
      numSamples, sampleRate, channels, IAudioSamples::FMT_S16,
      0);

  // invalid channels
  {
    LoggerStack stack;
    stack.setGlobalLevel(Logger::LEVEL_DEBUG, false);

    AudioFrameBuffer afb(0,
        frameSize, channels+1, sampleRate, bitDepth);

    int32_t retval = afb.addSamples(samples.value());
    VS_TUT_ENSURE("should fail", retval < 0);
   }
   
  // invalid channels
  {
    LoggerStack stack;
    stack.setGlobalLevel(Logger::LEVEL_DEBUG, false);

    AudioFrameBuffer afb(0,
        frameSize, channels+1, sampleRate, bitDepth);

    int32_t retval = afb.addSamples(samples.value());
    VS_TUT_ENSURE("should fail", retval < 0);
   }

  // invalid sample rate
  {
    LoggerStack stack;
    stack.setGlobalLevel(Logger::LEVEL_DEBUG, false);

    AudioFrameBuffer afb(0,
        frameSize, channels, sampleRate*2, bitDepth);

    int32_t retval = afb.addSamples(samples.value());
    VS_TUT_ENSURE("should fail", retval < 0);
  }

  // invalid bit depth
  {
    LoggerStack stack;
    stack.setGlobalLevel(Logger::LEVEL_DEBUG, false);

    AudioFrameBuffer afb(0,
        frameSize, channels, sampleRate, bitDepth*2);

    int32_t retval = afb.addSamples(samples.value());
    VS_TUT_ENSURE("should fail", retval < 0);
  }
}

void
AudioFrameBufferTest :: testInvalidConstructors()
{
  // bad frame size
  {
    try
    {
      AudioFrameBuffer afb(0,
          0, 1, 22050, 15);
      // if we get here we failed
      VS_TUT_ENSURE("was able to construct corrupt object",
          0);

    }
    catch (std::invalid_argument & e)
    {
      // if we get here, we succeeded
    }
  }

  // bad channels
  {
    try
    {
      AudioFrameBuffer afb(0,
          1, 0, 22050, 15);
      // if we get here we failed
      VS_TUT_ENSURE("was able to construct corrupt object",
          0);

    }
    catch (std::invalid_argument & e)
    {
      // if we get here, we succeeded
    }
  }
  
  // bad sample rate
  {
    try
    {
      AudioFrameBuffer afb(0,
          1, 1, 0, 15);
      // if we get here we failed
      VS_TUT_ENSURE("was able to construct corrupt object",
          0);

    }
    catch (std::invalid_argument & e)
    {
      // if we get here, we succeeded
    }
  }

  // bad bit depth
  {
    try
    {
      AudioFrameBuffer afb(0,
          1, 1, 22050, 0);
      // if we get here we failed
      VS_TUT_ENSURE("was able to construct corrupt object",
          0);

    }
    catch (std::invalid_argument & e)
    {
      // if we get here, we succeeded
    }
  }
}

void
AudioFrameBufferTest :: testGetters()
{
  AudioFrameBuffer afb(0,
      1, 2, 22050, 16);
  VS_TUT_ENSURE_EQUALS("should be equal",
      afb.getFrameSize(),
      1);
  
  VS_TUT_ENSURE_EQUALS("should be equal",
        afb.getChannels(),
        2);

  VS_TUT_ENSURE_EQUALS("should be equal",
        afb.getSampleRate(),
        22050);

  VS_TUT_ENSURE_EQUALS("should be equal",
        afb.getSampleBitDepth(),
        16);
  
  VS_TUT_ENSURE_EQUALS("should be equal",
      afb.getBytesPerSample(),
      4
      );
}

void
AudioFrameBufferTest :: testGetNextFrameNoCopy()
{
  const int32_t frameSize=15;
  const int32_t channels = 1;
  const int32_t sampleRate = 22050;
  const int32_t bitDepth=16;
  const int32_t numSamples=frameSize;
  const int64_t timestamp=15538;
  
  RefPointer<AudioSamples> samples = AudioSamples::make(numSamples, channels);
  samples->setComplete(true,
      numSamples, sampleRate, channels, IAudioSamples::FMT_S16,
      timestamp);

  VS_TUT_ENSURE_EQUALS("we should be the only person with a ref",
      samples->getCurrentRefCount(), 1);
  {
    AudioFrameBuffer afb(0,
        frameSize, channels, sampleRate, bitDepth);
    VS_TUT_ENSURE("should be empty", !afb.isFrameAvailable());

    int32_t retval = afb.addSamples(samples.value());
    VS_TUT_ENSURE("should succeed", retval >= 0);

    VS_TUT_ENSURE("the AFB should get a ref",
        samples->getCurrentRefCount() > 1);

    VS_TUT_ENSURE("should have frame now", afb.isFrameAvailable());
    
    void *srcSamples = samples->getRawSamples(0);
    void *dstSamples = 0;
    int32_t samplesWritten=0;
    int64_t timestampWritten=0;
    retval = afb.getNextFrame(&dstSamples,
        &samplesWritten,
        &timestampWritten);
    VS_TUT_ENSURE_EQUALS("should succeed", retval, frameSize);

    VS_TUT_ENSURE_EQUALS("should be same buffer",
        srcSamples, dstSamples);
    VS_TUT_ENSURE_EQUALS("should be same # of samples",
        samplesWritten,
        frameSize);
    VS_TUT_ENSURE_EQUALS("should be same timestamp",
        timestampWritten,
        timestamp);

    // The AFB should be holding onto the samples we're
    // using until the next call to getNextFrame
    VS_TUT_ENSURE("the AFB should get a ref",
        samples->getCurrentRefCount() > 1);

    VS_TUT_ENSURE("should be no more frames available",
        !afb.isFrameAvailable());
    
    retval = afb.getNextFrame(&dstSamples,
        &samplesWritten,
        &timestampWritten);
    VS_TUT_ENSURE_EQUALS("should get no bytes back",
        retval,
        0);
    // and now because getNextFrame was called again,
    // the frame buffer should have released it's hold on
    // the audio samples from the first call
    VS_TUT_ENSURE_EQUALS("the AFB should not have a ref",
        samples->getCurrentRefCount(),1);
  }
  
  // All references should be released with AFB is destroyed
  VS_TUT_ENSURE_EQUALS("we should be the only person with a ref",
      samples->getCurrentRefCount(), 1);
}

void
AudioFrameBufferTest :: testGetNextFrameWithCopy()
{
  const int32_t frameSize=15;
  const int32_t channels = 1;
  const int32_t sampleRate = 22050;
  const int32_t bitDepth=16;
  
  // this is deliberately rounding down to a prime number
  // to make sure we don't have input # of samples EXACTLY fit an output
  // frame.
  const int32_t numSamples=frameSize/2;
  const int64_t timestamp=15538;

  AudioFrameBuffer afb(0,
      frameSize, channels, sampleRate, bitDepth);
  VS_TUT_ENSURE("should be empty", !afb.isFrameAvailable());

  // We're going to create and add sample until we've filled
  // a frame
  int samplesAdded = 0;
  int nextTimestamp = timestamp;
  for(int i = 0; samplesAdded < frameSize; i++)
  {
    RefPointer<AudioSamples> samples = AudioSamples::make(numSamples, channels);
    int16_t* rawSamples = samples->getRawSamples(0);
    for(int j = 0; j < numSamples*channels; j++)
    {
//      VS_LOG_DEBUG("samples %d[%d]=%d", i, j, i);
      rawSamples[j] = i;
    }
    samples->setComplete(true,
        numSamples, sampleRate, channels, IAudioSamples::FMT_S16,
        nextTimestamp);
    afb.addSamples(samples.value());
    samplesAdded += numSamples;
    nextTimestamp +=IAudioSamples::samplesToDefaultPts(numSamples, sampleRate);
  }
  VS_TUT_ENSURE("should have a frame available", afb.isFrameAvailable());
  
  int16_t *dstSamples = 0;
  int32_t samplesWritten=0;
  int64_t timestampWritten=0;
  int32_t retval = afb.getNextFrame(
      (void**)&dstSamples,
      &samplesWritten,
      &timestampWritten);
  VS_TUT_ENSURE_EQUALS("should succeed", retval, frameSize);

  VS_TUT_ENSURE_EQUALS("should be same # of samples",
      samplesWritten,
      frameSize);
  VS_TUT_ENSURE_EQUALS("should be same timestamp",
      timestampWritten,
      timestamp);

  // now let's walk through the samples we got and
  // make sure they are correct
  for(int i = 0; i < frameSize*channels; i++)
  {
    int16_t expectedValue = i/numSamples;
//    VS_LOG_DEBUG("checking sample %d: %hd versus %hd",
//        i,
//        expectedValue,
//        dstSamples[i]);
    VS_TUT_ENSURE_EQUALS("should be equal",
        dstSamples[i],
        expectedValue);
  }
  VS_TUT_ENSURE("should be no more frames available",
      !afb.isFrameAvailable());

  retval = afb.getNextFrame(0,0,0);
  VS_TUT_ENSURE_EQUALS("should get no bytes back",
      retval,
      0);
}

void
AudioFrameBufferTest :: testGetNextFrameReturnsCorrectTimestamps()
{
  const int32_t channels = 1;
  const int32_t sampleRate = 22050;
  const int32_t bitDepth=16;
  const int32_t numSamples=1024;
  // make it so that we get nice even
  // samples and frame division  
  const int32_t frameSize=numSamples*4;
  const int64_t timestamp=0;

  AudioFrameBuffer afb(0,
      frameSize, channels, sampleRate, bitDepth);
  VS_TUT_ENSURE("should be empty", !afb.isFrameAvailable());

  // We're going to create and add sample until we've filled
  // 2 frames
  int32_t samplesAdded = 0;
  int64_t nextTimestamp = timestamp;
  int64_t expectedTimestamps[2]= { 0, Global::NO_PTS };
  
  for(int i = 0; i < (frameSize/numSamples)*2; i++)
  {
    RefPointer<AudioSamples> samples = AudioSamples::make(numSamples, channels);
    int16_t* rawSamples = samples->getRawSamples(0);
    for(int j = 0; j < numSamples*channels; j++)
    {
//      VS_LOG_DEBUG("samples %d[%d]=%d", i, j, i);
      rawSamples[j] = i;
    }
    samples->setComplete(true,
        numSamples, sampleRate, channels, IAudioSamples::FMT_S16,
        nextTimestamp);
    if (i == (frameSize/numSamples))
    {
      // this will be the start of the second frame
      expectedTimestamps[1] = nextTimestamp;
    }
    afb.addSamples(samples.value());
    samplesAdded += numSamples;
    nextTimestamp +=IAudioSamples::samplesToDefaultPts(numSamples, sampleRate);
  }
  VS_TUT_ENSURE("should have a frame available", afb.isFrameAvailable());

  int32_t totalSamplesWritten = 0;
  int32_t numFramesRetrieved = 0;
  int32_t retval = -1;
  while(totalSamplesWritten < frameSize * 2)
  {
    int16_t *dstSamples = 0;
    int32_t samplesWritten=0;
    int64_t timestampWritten=0;
    retval = afb.getNextFrame(
        (void**)&dstSamples,
        &samplesWritten,
        &timestampWritten);
    VS_TUT_ENSURE_EQUALS("should succeed", retval, frameSize);
    ++numFramesRetrieved;
    
    VS_TUT_ENSURE_EQUALS("should be same # of samples",
        samplesWritten,
        frameSize);

    VS_TUT_ENSURE_EQUALS("should be same time stamp",
        timestampWritten,
        expectedTimestamps[totalSamplesWritten/frameSize]);

    totalSamplesWritten += samplesWritten;
    
    // now let's walk through the samples we got and
    // make sure they are correct
    for(int i = 0; i < frameSize*channels; i++)
    {
      int16_t expectedValue = (numFramesRetrieved-1)*4+i/(channels*numSamples);
      //    VS_LOG_DEBUG("checking sample %d: %hd versus %hd",
      //        i,
      //        expectedValue,
      //        dstSamples[i]);
      VS_TUT_ENSURE_EQUALS("should be equal",
          dstSamples[i],
          expectedValue);
    }
  }
  VS_TUT_ENSURE("should be no more frames available",
      !afb.isFrameAvailable());

  retval = afb.getNextFrame(0,0,0);
  VS_TUT_ENSURE_EQUALS("should get no bytes back",
      retval,
      0); 
}

void
AudioFrameBufferTest :: testGetNextFrameMultipleTimes()
{
  const int32_t channels = 1;
  const int32_t sampleRate = 22050;
  const int32_t bitDepth=16;
  // This combination of sample size and frame size should
  // line up on every multiple of 12.
  const int32_t numSamples=4;
  const int32_t frameSize=3;
  const int64_t timestamp=0;
  const int32_t numIterations=12*100;
  
  AudioFrameBuffer afb(0,
      frameSize, channels, sampleRate, bitDepth);
  VS_TUT_ENSURE("should be empty", !afb.isFrameAvailable());

  // First we'll add lots of samples and call getNextFrame once for
  // each add.  This should leave frames available when done.
  int64_t nextTimestamp = timestamp;
  int32_t retval = -1;
  int32_t totalSamplesWritten = 0;
  for(int i = 0; i < numIterations; i++)
  {
    RefPointer<AudioSamples> samples = AudioSamples::make(numSamples, channels);
    int16_t* rawSamples = samples->getRawSamples(0);
    for(int j = 0; j < numSamples*channels; j++)
    {
//      VS_LOG_DEBUG("samples %d[%d]=%d", i, numSamples*i+j, i);
      rawSamples[j] = i;
    }
    samples->setComplete(true,
        numSamples, sampleRate, channels, IAudioSamples::FMT_S16,
        nextTimestamp);
    retval = afb.addSamples(samples.value());
    VS_TUT_ENSURE_EQUALS("should be able to add", retval, (int32_t)samples->getNumSamples());
    VS_TUT_ENSURE("should have a frame", afb.isFrameAvailable());
    nextTimestamp +=samples->getNextPts();
  
    // now try getting a frame
    int16_t* outSamples=0;
    int64_t timestampWritten=Global::NO_PTS;
    int32_t samplesWritten = 0;
    retval = afb.getNextFrame((void**)&outSamples,
        &samplesWritten,
        &timestampWritten);
    VS_TUT_ENSURE("should get a frame", retval == frameSize);
    VS_TUT_ENSURE("should get a frame", samplesWritten == frameSize);
    for(int j = 0; j < samplesWritten*channels; j++)
    {
      int16_t expectedSample = (totalSamplesWritten+j)/(numSamples*channels);
//      VS_LOG_DEBUG("checking sample [%d]=%hd (expected %hd)",
//          totalSamplesWritten+j,
//          outSamples[j],
//          expectedSample);
      VS_TUT_ENSURE_EQUALS("should get expected sample",
          outSamples[j],
          expectedSample);
    }
    totalSamplesWritten += samplesWritten;
  }
  // now there should still be frames remaining
  VS_TUT_ENSURE("should have frames", afb.isFrameAvailable());
  while(afb.isFrameAvailable())
  {
    int16_t* outSamples=0;
    int64_t timestampWritten=Global::NO_PTS;
    int32_t samplesWritten = 0;
    retval = afb.getNextFrame((void**)&outSamples,
        &samplesWritten,
        &timestampWritten);
    VS_TUT_ENSURE("should get a frame", retval == frameSize);
    VS_TUT_ENSURE("should get a frame", samplesWritten == frameSize);
    for(int j = 0; j < samplesWritten*channels; j++)
    {
      int16_t expectedSample = (totalSamplesWritten+j)/(numSamples*channels);
      //      VS_LOG_DEBUG("checking sample [%d]=%hd (expected %hd)",
      //          totalSamplesWritten+j,
      //          outSamples[j],
      //          expectedSample);
      VS_TUT_ENSURE_EQUALS("should get expected sample",
          outSamples[j],
          expectedSample);
    }
    totalSamplesWritten += samplesWritten;
  }
  VS_TUT_ENSURE_EQUALS("should get all",
      numSamples*numIterations,
      totalSamplesWritten);
}
