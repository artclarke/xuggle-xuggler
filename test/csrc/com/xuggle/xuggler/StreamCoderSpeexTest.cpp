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
#include <com/xuggle/xuggler/IStreamCoder.h>
#include <com/xuggle/xuggler/Global.h>
#include "Helper.h"
#include "StreamCoderSpeexTest.h"

#include <cstring>

using namespace VS_CPP_NAMESPACE;

VS_LOG_SETUP(VS_CPP_PACKAGE);

StreamCoderSpeexTest :: StreamCoderSpeexTest()
{
  h = 0;
  hw = 0;
}

StreamCoderSpeexTest :: ~StreamCoderSpeexTest()
{
  tearDown();
}

void
StreamCoderSpeexTest :: setUp()
{
  coder = 0;
  h = new Helper();
  hw = new Helper();
}

void
StreamCoderSpeexTest :: tearDown()
{
  coder = 0;
  if (h)
    delete h;
  h = 0;
  if (hw)
    delete hw;
  hw = 0;
}

void
StreamCoderSpeexTest :: testEncodingSpeexAudio()
{
  // disable because broken on Windows
  return;
  RefPointer<IAudioSamples> samples = 0;
  RefPointer<IAudioSamples> outSamples = 0;
  RefPointer<IVideoPicture> frame = 0;
  RefPointer<IRational> num(0);
  RefPointer<IStreamCoder> ic(0);
  RefPointer<IStreamCoder> oc(0);

  RefPointer<IAudioResampler> resampler;
  
  int numSamples = 0;
  int numPackets = 0;
  int numFrames = 0;
  int numKeyFrames = 0;

  int retval = -1;

  h->setupReading(h->SAMPLE_FILE);

  RefPointer<IPacket> packet = IPacket::make();

  hw->setupWriting("StreamCoderSpeexTest_output.ogg",
      "libtheora", "libspeex", "ogg");

  RefPointer<IPacket> opacket = IPacket::make();
  VS_TUT_ENSURE("! opacket", opacket);

  {
    // Let's set up audio first.
    ic = h->coders[h->first_input_audio_stream];
    oc = hw->coders[hw->first_output_audio_stream];

    // Set the output coder correctly.
    oc->setSampleRate(16000);
    oc->setChannels(1);
    oc->setBitRate(ic->getBitRate());

    samples = IAudioSamples::make(1024, ic->getChannels());
    VS_TUT_ENSURE("got no samples", samples);
    
    outSamples = IAudioSamples::make(1024, 1);
    VS_TUT_ENSURE("got no samples", outSamples);

    resampler = IAudioResampler::make(1, ic->getChannels(),
        16000, ic->getSampleRate());

    retval = ic->open();
    VS_TUT_ENSURE("Could not open input coder", retval >= 0);
    retval = oc->open();
    VS_TUT_ENSURE("Could not open output coder", retval >= 0);
  }
  {
    // now, let's set up video.
    ic = h->coders[h->first_input_video_stream];
    oc = hw->coders[hw->first_output_video_stream];

    // We're going to set up high quality video
    oc->setBitRate(720000);
    oc->setGlobalQuality(0);
    oc->setFlags(oc->getFlags() | IStreamCoder::FLAG_QSCALE);

    oc->setPixelType(ic->getPixelType());
    oc->setHeight(ic->getHeight());
    oc->setWidth(ic->getWidth());
    num = IRational::make(1,15);
    oc->setFrameRate(num.value());
    num = ic->getTimeBase();
    oc->setTimeBase(num.value());
    oc->setNumPicturesInGroupOfPictures(
        ic->getNumPicturesInGroupOfPictures()
    );

    // Ask the StreamCoder to guess width and height for us
    frame = IVideoPicture::make(
        ic->getPixelType(),
        -1, -1);
    VS_TUT_ENSURE("got no frame", frame);
    VS_TUT_ENSURE("should not have width", frame->getWidth() <= 0);
    VS_TUT_ENSURE("should not have height", frame->getHeight() <= 0);

    retval = ic->open();
    VS_TUT_ENSURE("Could not open input coder", retval >= 0);
    retval = oc->open();
    VS_TUT_ENSURE("Could not open output coder", retval >= 0);
  }

  // write header
  retval = hw->container->writeHeader();
  VS_TUT_ENSURE("could not write header", retval >= 0);

  while (h->container->readNextPacket(packet.value()) == 0)
  {
    ic = h->coders[packet->getStreamIndex()];
    oc = hw->coders[packet->getStreamIndex()];

    if (packet->getStreamIndex() == h->first_input_audio_stream)
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
        retval = resampler->resample(outSamples.value(),
            samples.value(),
            samples->getNumSamples());
        VS_TUT_ENSURE("Could not resample audio", retval > 0);
        

        // now, write out the packets.
        unsigned int numSamplesConsumed = 0;
        do {
          retval = oc->encodeAudio(opacket.value(),
              outSamples.value(),
              numSamplesConsumed);
          VS_TUT_ENSURE("Could not encode any audio", retval > 0);
          numSamplesConsumed += retval;

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
        } while (numSamplesConsumed < outSamples->getNumSamples());
      }
    } else if (packet->getStreamIndex() == h->first_input_video_stream)
    {
      int offset = 0;

      numPackets++;

      VS_LOG_TRACE("packet: pts: %ld; dts: %ld", packet->getPts(),
          packet->getDts());
      while (offset < packet->getSize())
      {

        retval = ic->decodeVideo(
            frame.value(),
            packet.value(),
            offset);
        VS_TUT_ENSURE("could not decode any video", retval>0);
        num = ic->getTimeBase();
        VS_TUT_ENSURE_EQUALS("time base changed", num->getDouble(), h->expected_time_base);
        offset += retval;
        if (frame->isComplete())
        {
          VS_TUT_ENSURE("should now have a frame width", frame->getWidth() > 0);
          VS_TUT_ENSURE("should now have a frame height", frame->getHeight() > 0);
                    
          numFrames++;
          if (frame->isKeyFrame())
            numKeyFrames++;

          frame->setQuality(0);
          retval = oc->encodeVideo(
              opacket.value(),
              frame.value(),
              -1);

          VS_TUT_ENSURE("could not encode video", retval >= 0);
          VS_TUT_ENSURE("could not encode video", opacket->isComplete());

          VS_TUT_ENSURE("could not encode video", opacket->getSize() > 0);

          RefPointer<IBuffer> encodedBuffer = opacket->getData();
          VS_TUT_ENSURE("no encoded data", encodedBuffer);
          VS_TUT_ENSURE("less data than there should be",
              encodedBuffer->getBufferSize() >=
              opacket->getSize());

          // now, write the packet to disk.
          retval = hw->container->writePacket(opacket.value());
          VS_TUT_ENSURE("could not write packet", retval >= 0);


        }
      }
    }
  }
  // sigh; it turns out that to flush the encoding buffers you need to
  // ask the encoder to encode a NULL set of samples.  So, let's do that.
  retval = hw->coders[hw->first_output_audio_stream]->encodeAudio(opacket.value(), 0, 0);
  VS_TUT_ENSURE("Could not encode any audio", retval >= 0);
  if (opacket->isComplete())
  {
    retval = hw->container->writePacket(opacket.value());
    VS_TUT_ENSURE("could not write packet", retval >= 0);
  }
  retval = hw->coders[hw->first_output_video_stream]->encodeVideo(opacket.value(), 0, 0);
  VS_TUT_ENSURE("Could not encode any video", retval >= 0);
  if (opacket->isComplete())
  {
    retval = hw->container->writePacket(opacket.value());
    VS_TUT_ENSURE("could not write packet", retval >= 0);
  }

  retval = hw->container->writeTrailer();
  VS_TUT_ENSURE("! writeTrailer", retval >= 0);

  retval = h->coders[h->first_input_audio_stream]->close();
  VS_TUT_ENSURE("! close", retval >= 0);

  retval = h->coders[h->first_input_video_stream]->close();
  VS_TUT_ENSURE("! close", retval >= 0);

  retval = hw->coders[hw->first_output_audio_stream]->close();
  VS_TUT_ENSURE("! close", retval >= 0);

  retval = hw->coders[hw->first_output_video_stream]->close();
  VS_TUT_ENSURE("! close", retval >= 0);

  VS_TUT_ENSURE("could not get any audio packets", numPackets > 0);
  VS_TUT_ENSURE("could not decode any audio", numSamples > 0);
  VS_TUT_ENSURE("could not decode any video", numFrames >0);
  VS_TUT_ENSURE("could not find any key frames", numKeyFrames >0);

  if (h->expected_packets)
    VS_TUT_ENSURE_EQUALS("unexpected number of packets",
        numPackets, h->expected_packets);
  if (h->expected_audio_samples)
    VS_TUT_ENSURE_EQUALS("unexpected number of audio samples",
        numSamples, h->expected_audio_samples);
  if (h->expected_video_frames)
    VS_TUT_ENSURE_EQUALS("unexpected number of video frames",
        numFrames, h->expected_video_frames);
  if (h->expected_video_key_frames)
    VS_TUT_ENSURE_EQUALS("unexpected number of video key frames",
        numKeyFrames, h->expected_video_key_frames);
}
