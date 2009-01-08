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
#include <com/xuggle/xuggler/IStream.h>
#include <com/xuggle/xuggler/IStreamCoder.h>
#include <com/xuggle/xuggler/ICodec.h>
#include <com/xuggle/xuggler/Global.h>
#include "Helper.h"
#include "AudioSamplesTest.h"

using namespace VS_CPP_NAMESPACE;

VS_LOG_SETUP(VS_CPP_PACKAGE);

AudioSamplesTest :: AudioSamplesTest()
{
  h = 0;
  hw = 0;
}

AudioSamplesTest :: ~AudioSamplesTest()
{
  tearDown();
}

void
AudioSamplesTest :: setUp()
{
  if (h)
    delete h;
  h = new Helper();
  if (hw)
    delete hw;
  hw = new Helper();
}

void
AudioSamplesTest :: tearDown()
{
  if (h)
    delete h;
  h = 0;
  if (hw)
    delete hw;
  hw = 0;
}

void
AudioSamplesTest :: testCreationAndDestruction()
{
  RefPointer<IAudioSamples> samples = 0;

  samples = IAudioSamples::make(1024, 1);
  VS_TUT_ENSURE("got no samples", samples);
  VS_TUT_ENSURE("sample buffer not big enough",
      samples->getMaxBufferSize() >= 1024);
  VS_TUT_ENSURE_EQUALS("there are samples in the buffer?",
      samples->getNumSamples(), (unsigned int)0);
  VS_TUT_ENSURE("unexpected sample size",
      samples->getSampleBitDepth() == sizeof(short)*8);
  samples = IAudioSamples::make(0, 1);
  VS_TUT_ENSURE("got some samples where none expected", !samples);
}

void
AudioSamplesTest :: testDecodingToBuffer()
{
  RefPointer<IAudioSamples> samples = 0;
  int numSamples = 0;
  int numPackets = 0;
  int audioStream = -1;
  int retval = -1;

  samples = IAudioSamples::make(1024, h->expected_channels);
  VS_TUT_ENSURE("got no samples", samples);
  VS_TUT_ENSURE("should not be complete", !samples->isComplete());

  h->setupReading(h->SAMPLE_FILE);
  RefPointer<IPacket> packet = IPacket::make();
  for (int i = 0; i < h->num_streams; i++)
  {
    if (h->codecs[i]->getType() == ICodec::CODEC_TYPE_AUDIO)
    {
      // got audio
      audioStream = i;
      retval = h->coders[i]->open();
      VS_TUT_ENSURE("! open codec", retval >= 0);
      break;
    }
  }

  VS_TUT_ENSURE("couldn't find an audio stream", audioStream >= 0);
  int maxSamples = 10 * h->coders[audioStream]->getSampleRate(); // 10 seconds

  while (h->container->readNextPacket(packet.value()) == 0 &&
      numSamples < maxSamples)
  {
    if (packet->getStreamIndex() == audioStream)
    {
      int offset = 0;

      numPackets++;

      while (offset < packet->getSize())
      {
        retval = h->coders[audioStream]->decodeAudio(
            samples.value(),
            packet.value(),
            offset);
        VS_TUT_ENSURE("could not decode any audio",
            retval > 0);
        offset += retval;
        VS_TUT_ENSURE("could not write any samples",
            samples->getNumSamples() > 0);
        numSamples += samples->getNumSamples();

        VS_TUT_ENSURE("did not finish", samples->isComplete());
        VS_TUT_ENSURE_EQUALS("wrong sample rate", samples->getSampleRate(),
            h->expected_sample_rate);
        VS_TUT_ENSURE_EQUALS("wrong channels", samples->getChannels(),
            h->expected_channels);
        VS_TUT_ENSURE_EQUALS("wrong format", samples->getFormat(),
            IAudioSamples::FMT_S16);
      }
    }
  }
  retval = h->coders[audioStream]->close();

  VS_TUT_ENSURE("could not get any audio packets", numPackets > 0);
  VS_TUT_ENSURE("could not decode any audio", numSamples > 0);

}

void
AudioSamplesTest :: testEncodingToBuffer()
{
  RefPointer<IAudioSamples> samples = 0;
  int numSamples = 0;
  int numPackets = 0;
  int retval = -1;

  samples = IAudioSamples::make(1024*128*4, h->expected_channels);
  VS_TUT_ENSURE("got no samples", samples);

  h->setupReading(h->SAMPLE_FILE);

  RefPointer<IPacket> packet = IPacket::make();

  hw->setupWriting("AudioSamplesTest_3_output.flv", 0, "libmp3lame", "flv");
  int outStream = hw->first_output_audio_stream;
  VS_TUT_ENSURE("Could not find an audio stream in the output", outStream >= 0);
  int inStream = h->first_input_audio_stream;
  VS_TUT_ENSURE("Could not find an audio stream in the input", inStream >= 0);

  RefPointer<IStreamCoder> ic = h->coders[inStream];
  RefPointer<IStreamCoder> oc = hw->coders[outStream];
  RefPointer<IPacket> opacket = IPacket::make();
  VS_TUT_ENSURE("! opacket", opacket);

  // Set the output coder correctly.
  oc->setSampleRate(ic->getSampleRate());
  oc->setChannels(ic->getChannels());
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

        // now, write out the packets.
        unsigned int numSamplesConsumed = 0;
        do {
          retval = oc->encodeAudio(opacket.value(), samples.value(),
              numSamplesConsumed);
          VS_TUT_ENSURE("Could not encode any audio", retval >= 0);
          numSamplesConsumed += retval;

          //VS_TUT_ENSURE("could not encode audio", opacket->getSize() > 0);

          RefPointer<IBuffer> encodedBuffer = opacket->getData();
          VS_TUT_ENSURE("no encoded data", encodedBuffer);
          VS_TUT_ENSURE("less data than there should be",
              encodedBuffer->getBufferSize() >=
              opacket->getSize());

          // now, write the packet to disk.
          if (opacket->isComplete())
          {
            retval = hw->container->writePacket(opacket.value());
            VS_TUT_ENSURE("could not write packet", retval >= 0);
          }
        } while (numSamplesConsumed < samples->getNumSamples());
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
AudioSamplesTest :: testSetSampleEdgeCases()
{
  RefPointer<IAudioSamples> samples = 0;

  // turn down logging for this test
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_WARN, false);

  samples = IAudioSamples::make(1024, 1);
  VS_TUT_ENSURE("got no samples", samples);
  int retval = 0;

  retval = samples->setSample(samples->getMaxBufferSize(), 1, IAudioSamples::FMT_S16, 0);
  VS_TUT_ENSURE("sampleIndex should be too large", retval < 0);

  retval = samples->setSample(3, 2, IAudioSamples::FMT_S16, 0);
  VS_TUT_ENSURE("channels is too large", retval < 0);

  retval = samples->setSample(3, -1, IAudioSamples::FMT_S16, 0);
  VS_TUT_ENSURE("channels is too small", retval < 0);

  retval = samples->setSample(3, 1, IAudioSamples::FMT_NONE, 0);
  VS_TUT_ENSURE("format is not supported", retval < 0);
}

void
AudioSamplesTest :: testSetSampleSunnyDayScenarios()
{
  RefPointer<IAudioSamples> samples = 0;

  samples = IAudioSamples::make(1024, 2);
  VS_TUT_ENSURE("got no samples", samples);
  int retval = 0;

  samples->setComplete(true, 512, 22050, 2, IAudioSamples::FMT_S16, 0);

  unsigned int sampleIndex = 3;
  short channel0Sample = 8;
  short channel1Sample = 16;
  retval = samples->setSample(sampleIndex, 0, IAudioSamples::FMT_S16, channel0Sample);
  VS_TUT_ENSURE("should set sample without error", retval >= 0);
  retval = samples->setSample(sampleIndex, 1, IAudioSamples::FMT_S16, channel1Sample);
  VS_TUT_ENSURE("should set sample without error", retval >= 0);

  VS_TUT_ENSURE("should be equal to set value",
      samples->getSample(sampleIndex, 0, IAudioSamples::FMT_S16) == channel0Sample);
  VS_TUT_ENSURE("should be equal to set value",
      samples->getSample(sampleIndex, 1, IAudioSamples::FMT_S16) == channel1Sample);
}

void
AudioSamplesTest :: testGetSampleRainyDayScenarios()
{
  // turn down logging for this test
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_WARN, false);

  RefPointer<IAudioSamples> samples = 0;

  samples = IAudioSamples::make(1024, 1);
  VS_TUT_ENSURE("got no samples", samples);
  int retval = 0;

  short channel0Sample = 8;
  samples->setSample(0, 0, IAudioSamples::FMT_S16, channel0Sample);
  samples->setComplete(true, 1, 22050, 1, IAudioSamples::FMT_S16, 0);

  retval = samples->getSample(0, 0, IAudioSamples::FMT_S16);
  VS_TUT_ENSURE_EQUALS("should be set value", channel0Sample, retval);

  retval = samples->getSample(samples->getNumSamples(), 0, IAudioSamples::FMT_S16);
  VS_TUT_ENSURE_EQUALS("should be too large a sampleIndex", 0, retval);

  retval = samples->getSample(0, 1, IAudioSamples::FMT_S16);
  VS_TUT_ENSURE_EQUALS("should be too large a channel", 0, retval);

  retval = samples->getSample(0, -1, IAudioSamples::FMT_S16);
  VS_TUT_ENSURE_EQUALS("should be too small a channel", 0, retval);
}
