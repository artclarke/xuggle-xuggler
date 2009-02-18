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
#include "StreamCoderX264Test.h"
// For getenv()
#include <stdlib.h>

using namespace VS_CPP_NAMESPACE;

VS_LOG_SETUP(VS_CPP_PACKAGE);

StreamCoderX264Test :: StreamCoderX264Test()
{
  h = 0;
  hw = 0;
}

StreamCoderX264Test :: ~StreamCoderX264Test()
{
  tearDown();
}

void
StreamCoderX264Test :: setUp()
{
  coder = 0;
  h = new Helper();
  hw = new Helper();
}

void
StreamCoderX264Test :: tearDown()
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
StreamCoderX264Test :: testSuccess()
{
  return;
}

void
StreamCoderX264Test :: testDecodingAndEncodingH264Video()
{
  // This test doesn't pass memcheck but we're disabling for now
  char *memcheck = getenv("VS_TEST_MEMCHECK");
  if (memcheck)
    return;
  
  RefPointer<IAudioSamples> samples = 0;
  RefPointer<IVideoPicture> frame = 0;
  RefPointer<IRational> num(0);
  RefPointer<IStreamCoder> ic(0);
  RefPointer<IStreamCoder> oc(0);

  RefPointer<ICodec> newcodec;
  newcodec = ICodec::findEncodingCodecByName("libx264");
  if (!newcodec)
    // we're probably in a LGPL build, and so we shouldn't run this test
    return;

  int numSamples = 0;
  int numPackets = 0;
  int numFrames = 0;
  int numKeyFrames = 0;

  int retval = -1;

  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_WARN, false);

  // use the 20 second file to make memcheck faster
  h->setupReading("testfile_videoonly_20sec.flv");

  RefPointer<IPacket> packet = IPacket::make();

  hw->setupWriting("StreamCoderX264Test_testDecodingAndEncodingH264Video.mp4", "libx264", "libmp3lame", "mp4");

  RefPointer<IPacket> opacket = IPacket::make();
  VS_TUT_ENSURE("! opacket", opacket);

  {
    // Let's set up audio first.
    ic = h->coders[h->first_input_audio_stream];
    oc = hw->coders[hw->first_output_audio_stream];

    // Set the output coder correctly.
    oc->setSampleRate(ic->getSampleRate());
    oc->setChannels(ic->getChannels());
    oc->setBitRate(ic->getBitRate());

    samples = IAudioSamples::make(1024, ic->getChannels());
    VS_TUT_ENSURE("got no samples", samples);


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

        // now, write out the packets.
        unsigned int numSamplesConsumed = 0;
        do {
          retval = oc->encodeAudio(opacket.value(), samples.value(),
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
        } while (numSamplesConsumed < samples->getNumSamples());
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
               numPackets, 1062);
  if (h->expected_audio_samples)
    VS_TUT_ENSURE_EQUALS("unexpected number of audio samples",
               numSamples, 438912);
  if (h->expected_video_frames)
    VS_TUT_ENSURE_EQUALS("unexpected number of video frames",
               numFrames, 300);
  if (h->expected_video_key_frames)
    VS_TUT_ENSURE_EQUALS("unexpected number of video key frames",
               numKeyFrames, 25);
}


