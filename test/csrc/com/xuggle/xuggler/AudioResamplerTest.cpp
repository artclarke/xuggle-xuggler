/*
 * Copyright (c) 2008 by Vlideshow Inc. (a.k.a. The Yard).  All rights reserved.
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
#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/xuggler/IAudioResampler.h>
#include <com/xuggle/xuggler/Global.h>
#include "Helper.h"
#include "AudioResamplerTest.h"

// For Random
#include <stdlib.h>

using namespace VS_CPP_NAMESPACE;

VS_LOG_SETUP(VS_CPP_PACKAGE);

AudioResamplerTest :: AudioResamplerTest()
{
  h = 0;
  hw = 0;
}

AudioResamplerTest :: ~AudioResamplerTest()
{
  tearDown();
}

void
AudioResamplerTest :: setUp()
{
  if (h)
    delete h;
  h = new Helper();
  if (hw)
    delete hw;
  hw = new Helper();
}

void
AudioResamplerTest :: tearDown()
{
  if (h)
    delete h;
  h = 0;
  if (hw)
    delete hw;
  hw = 0;
}

void
AudioResamplerTest :: testGetters()
{
  RefPointer<IAudioResampler> sampler = IAudioResampler::make(1, 2,
      22050, 44100);
  VS_TUT_ENSURE("!sampler", sampler);
  VS_TUT_ENSURE_EQUALS("", sampler->getOutputChannels(), 1);
  VS_TUT_ENSURE_EQUALS("", sampler->getInputChannels(), 2);
  VS_TUT_ENSURE_EQUALS("", sampler->getOutputRate(), 22050);
  VS_TUT_ENSURE_EQUALS("", sampler->getInputRate(), 44100);

  sampler = IAudioResampler::make(3, 2,
      22050, 44100);
  VS_TUT_ENSURE("sampler", !sampler);
  sampler = IAudioResampler::make(-1, -1,
      -1, -1);
  VS_TUT_ENSURE("sampler", !sampler);
}

void
AudioResamplerTest :: testInvalidArguments()
{
  int retval = -1;

  // Turn down logging for this test as it should be noisy
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_DEBUG, false);

  RefPointer<IAudioResampler> sampler;
  sampler = IAudioResampler::make(3, 2,
      22050, 44100);
  VS_TUT_ENSURE("sampler", !sampler);
  sampler = IAudioResampler::make(-1, -1,
      -1, -1);
  VS_TUT_ENSURE("sampler", !sampler);

  sampler = IAudioResampler::make(1, 2, 22050, 44100);
  VS_TUT_ENSURE("! sampler", sampler);

  retval = sampler->resample(0, 0, 0);
  VS_TUT_ENSURE("no crash", retval < 0);

  RefPointer<IAudioSamples> inSamples = IAudioSamples::make(1024, 1);
  RefPointer<IAudioSamples> outSamples = IAudioSamples::make(1024, 1);
  // Set to the wrong sample rate.
  inSamples->setComplete(true, 2, 22050, 2, IAudioSamples::FMT_S16, 0);
  retval = sampler->resample(outSamples.value(),
      inSamples.value(),
      4);
  VS_TUT_ENSURE("should fail because of sample rate", retval < 0);
  inSamples->setComplete(true, 4, 44100, 1, IAudioSamples::FMT_S16, 0);
  retval = sampler->resample(outSamples.value(),
      inSamples.value(),
      4);
  VS_TUT_ENSURE("should fail because of channels", retval < 0);
}

void
AudioResamplerTest :: testResamplingAudio()
{
  RefPointer<IAudioResampler> resampler = IAudioResampler::make(2, 1,
      44100, 22050);
  RefPointer<IAudioSamples> samples = 0;
  RefPointer<IAudioSamples> resamples = 0;
  int numSamples = 0;
  int numPackets = 0;
  int retval = -1;
  samples = IAudioSamples::make(1024, 1);
  VS_TUT_ENSURE("got no samples", samples);
  resamples = IAudioSamples::make(1024, 2);
  VS_TUT_ENSURE("got no samples", samples);

  h->setupReading(h->SAMPLE_FILE);

  RefPointer<IPacket> packet = IPacket::make();

  hw->setupWriting("AudioResamplerTest_3_output.flv", 0, "libmp3lame", "flv");
  int outStream = hw->first_output_audio_stream;
  VS_TUT_ENSURE("Could not find an audio stream in the output", outStream >= 0);
  int inStream = h->first_input_audio_stream;
  VS_TUT_ENSURE("Could not find an audio stream in the input", inStream >= 0);

  RefPointer<IStreamCoder> ic = h->coders[inStream];
  RefPointer<IStreamCoder> oc = hw->coders[outStream];
  RefPointer<IPacket> opacket = IPacket::make();
  VS_TUT_ENSURE("! opacket", opacket);

  // Set the output coder correctly.
  int outChannels = 2;
  int outRate = 44100;
  resampler = IAudioResampler::make(outChannels, ic->getChannels(),
      outRate, ic->getSampleRate());
  oc->setSampleRate(outRate);
  oc->setChannels(outChannels);
  oc->setBitRate(ic->getBitRate());

  int maxSamples = 10 * ic->getSampleRate(); // 10 seconds

  retval = ic->open();
  VS_TUT_ENSURE("Could not open input coder", retval >= 0);
  retval = oc->open();
  VS_TUT_ENSURE("Could not open output coder", retval >= 0);

  // write header
  retval = hw->container->writeHeader();
  VS_TUT_ENSURE("could not write header", retval >= 0);

  while (h->container->readNextPacket(packet.value()) == 0
      && numSamples < maxSamples)
  {
    if (packet->getStreamIndex() == inStream)
    {
      int offset = 0;

      numPackets++;

      while (offset < packet->getSize())
      {
        retval = ic->decodeAudio(
            samples.value(),
            packet.value(),
            offset);
        VS_TUT_ENSURE("could not decode any audio",
            retval > 0);
        offset += retval;
        VS_TUT_ENSURE("could not write any samples",
            samples->getNumSamples() > 0);
        numSamples += samples->getNumSamples();

        // now, resample the audio
        retval = resampler->resample(resamples.value(), samples.value(), 0);
        VS_TUT_ENSURE("could not resample", retval > 0);
        VS_TUT_ENSURE("no resamples", resamples->getNumSamples() > 0);
        VS_TUT_ENSURE_EQUALS("wrong sample rate", resamples->getSampleRate(),
            outRate);
        VS_TUT_ENSURE_EQUALS("wrong channels", resamples->getChannels(),
            outChannels);
        // now, write out the packets.
        unsigned int samplesConsumed = 0;
        do {
          retval = oc->encodeAudio(opacket.value(), resamples.value(),
              samplesConsumed);
          VS_TUT_ENSURE("Could not encode any audio", retval >= 0);
          samplesConsumed += (unsigned int)retval;
          VS_LOG_TRACE("packet: %d; is: %d; os: %d",
              numPackets, numSamples, samplesConsumed);

          if (opacket->isComplete())
          {
            VS_TUT_ENSURE("could not encode audio", opacket->getSize() > 0);
            RefPointer<IBuffer> encodedBuffer = opacket->getData();
            VS_TUT_ENSURE("no encoded data", encodedBuffer);
            VS_TUT_ENSURE("less data than there should be",
                encodedBuffer->getBufferSize() >=
                opacket->getSize());
            retval = hw->container->writePacket(opacket.value());
            VS_TUT_ENSURE("could not write packet", retval >= 0);
          }
          // keep going until we've encoded all samples in this buffer.
        } while (samplesConsumed < resamples->getNumSamples());
      }
    }
  }
  // sigh; it turns out that to flush the encoding buffers you need to
  // ask the encoder to encode a NULL set of samples.  So, let's do that.
  retval = oc->encodeAudio(opacket.value(), 0, 0);
  VS_TUT_ENSURE("Could not encode any audio", retval >= 0);
  if (retval > 0)
  {
    retval = hw->container->writePacket(opacket.value());
    VS_TUT_ENSURE("could not write packet", retval >= 0);
  }

  retval = ic->close();
  VS_TUT_ENSURE("! close", retval >= 0);
  retval = oc->close();
  VS_TUT_ENSURE("! close", retval >= 0);
  retval = hw->container->writeTrailer();
  VS_TUT_ENSURE("! writeTrailer", retval >= 0);

  VS_TUT_ENSURE("could not get any audio packets", numPackets > 0);
  VS_TUT_ENSURE("could not decode any audio", numSamples > 0);
}

void
AudioResamplerTest :: testDifferentResampleRates()
{
  int retval = -1;
  RefPointer<IAudioResampler> sampler;

  RefPointer<IAudioSamples> inSamples = IAudioSamples::make(4096, 1);
  RefPointer<IAudioSamples> outSamples = IAudioSamples::make(4096*4, 1);

  // initialize our starting samples
  RefPointer<IBuffer> inBuffer = inSamples->getData();

  int16_t * inputBuf = (int16_t*)inBuffer->getBytes(0,
      inSamples->getNumSamples());
  VS_TUT_ENSURE("!samples", inputBuf);

  unsigned int tests[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
      11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
      32, 64, 128,
      256, 512, 1024, 2048, 3072, 4096 };
  unsigned int j = 0;
  // New sample every time
  sampler = IAudioResampler::make(2, 1, 44100, 22050);
  VS_TUT_ENSURE("! sampler", sampler);

  unsigned int totalInput = 0;
  unsigned int totalOutput = 0;
  for (j = 0; j < sizeof(tests)/sizeof(unsigned int); j++)
  {
    unsigned int i=0;

    for (i = 0; i < tests[j]; i++)
    {
      // This is a hack and doesn't need real randomness; just tesing.
      bool negative= rand() % 2;
      inputBuf[i] = (int16_t)(32767.0*((double)rand()/(double)(RAND_MAX))) * (negative ? -1 : 1);
    }
    totalInput += tests[j];
    inSamples->setComplete(true, i, 22050, 1,
        IAudioSamples::FMT_S16, 0);
    retval = sampler->resample(outSamples.value(),
        inSamples.value(),
        inSamples->getNumSamples());
    VS_TUT_ENSURE("should succeed", retval >= 0);
    totalOutput += retval;
    VS_LOG_TRACE("i: %d; o: %d; ti: %d; to: %d",
        inSamples->getNumSamples(),
        outSamples->getNumSamples(),
        totalInput,
        totalOutput);
  }
  // That was all so we could test this.  audio_resample hard codes the filter
  // coefficient in the resample to 16 samples, so we should always hold back
  // at least 16.
  VS_TUT_ENSURE_EQUALS("more than I thought", totalOutput, (totalInput)*2-16);

}

void
AudioResamplerTest :: testTimeStampIsAdjustedWhenResamplerEatsBytesUpsampling()
{
  int retval = -1;
  int numSamples = 10000;
  int iSampleRate = numSamples;
  int oSampleRate = iSampleRate*2;
  RefPointer<IAudioResampler> sampler;

  RefPointer<IAudioSamples> inSamples = IAudioSamples::make(numSamples, 1);
  RefPointer<IAudioSamples> outSamples = IAudioSamples::make(numSamples*oSampleRate/iSampleRate, 2);

  // initialize our starting samples
  RefPointer<IBuffer> inBuffer = inSamples->getData();
  int16_t * inputBuf = (int16_t*)inBuffer->getBytes(0,
      inSamples->getNumSamples());
  VS_TUT_ENSURE("!samples", inputBuf);

  sampler = IAudioResampler::make(2, 1, oSampleRate, iSampleRate);
  VS_TUT_ENSURE("! sampler", sampler);

  for (int i = 0; i < numSamples; i++)
  {
    // This is a hack and doesn't need real randomness; just tesing.
    bool negative= rand() % 2;
    inputBuf[i] = (int16_t)(32767.0*((double)rand()/(double)(RAND_MAX))) * (negative ? -1 : 1);
  }
  // Do it once...
  inSamples->setComplete(true, numSamples, iSampleRate, 1,
      IAudioSamples::FMT_S16, 0);
  retval = sampler->resample(outSamples.value(),
      inSamples.value(),
      inSamples->getNumSamples());
  VS_TUT_ENSURE("should succeed", retval >= 0);
  VS_LOG_TRACE("r: %d; i: %d; o: %d; ipts: %lld; opts: %lld",
      retval,
      inSamples->getNumSamples(),
      outSamples->getNumSamples(),
      inSamples->getPts(),
      outSamples->getPts());
  VS_TUT_ENSURE_EQUALS("should eat exactly 16 samples",
      outSamples->getNumSamples(),
      inSamples->getNumSamples()*oSampleRate/iSampleRate - 16);

  // Do it twice
  inSamples->setComplete(true, numSamples, iSampleRate, 1,
      IAudioSamples::FMT_S16, 1*Global::DEFAULT_PTS_PER_SECOND);
  retval = sampler->resample(outSamples.value(),
      inSamples.value(),
      inSamples->getNumSamples());
  VS_TUT_ENSURE("should succeed", retval >= 0);
  VS_LOG_TRACE("r: %d; i: %d; o: %d; ipts: %lld; opts: %lld",
      retval,
      inSamples->getNumSamples(),
      outSamples->getNumSamples(),
      inSamples->getPts(),
      outSamples->getPts());
  VS_TUT_ENSURE_EQUALS("should eat no samples",
      outSamples->getNumSamples(),
      inSamples->getNumSamples()*oSampleRate/iSampleRate);
  VS_TUT_ENSURE_EQUALS("timestamp should be back 16 samples worth",
      outSamples->getPts(),
      inSamples->getPts() - IAudioSamples::samplesToDefaultPts(16, oSampleRate));

  // Do it thrice
  inSamples->setComplete(true, numSamples, iSampleRate, 1,
      IAudioSamples::FMT_S16, 2*Global::DEFAULT_PTS_PER_SECOND);
  retval = sampler->resample(outSamples.value(),
      inSamples.value(),
      inSamples->getNumSamples());
  VS_TUT_ENSURE("should succeed", retval >= 0);
  VS_LOG_TRACE("r: %d; i: %d; o: %d; ipts: %lld; opts: %lld",
      retval,
      inSamples->getNumSamples(),
      outSamples->getNumSamples(),
      inSamples->getPts(),
      outSamples->getPts());
  VS_TUT_ENSURE_EQUALS("should eat no samples",
      outSamples->getNumSamples(),
      inSamples->getNumSamples()*oSampleRate/iSampleRate);
  VS_TUT_ENSURE_EQUALS("timestamp should be back 16 samples worth",
        outSamples->getPts(),
        inSamples->getPts() - IAudioSamples::samplesToDefaultPts(16, oSampleRate));

  // get the trailing bytes
  retval = sampler->resample(outSamples.value(),
      0,
      0);
  VS_TUT_ENSURE("should succeed", retval >= 0);
  VS_LOG_TRACE("r: %d; i: %d; o: %d; ipts: %lld; opts: %lld",
      retval,
      0,
      outSamples->getNumSamples(),
      0LL,
      outSamples->getPts());
  VS_TUT_ENSURE_EQUALS("timestamp should be back 16 samples worth from end of last samples given",
        outSamples->getPts(),
        inSamples->getNextPts() - IAudioSamples::samplesToDefaultPts(16, oSampleRate));

}
