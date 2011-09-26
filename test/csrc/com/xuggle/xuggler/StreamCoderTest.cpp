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
#include <com/xuggle/xuggler/IStreamCoder.h>
#include <com/xuggle/xuggler/Global.h>
#include "Helper.h"
#include "StreamCoderTest.h"

#include <cstring>

using namespace VS_CPP_NAMESPACE;

VS_LOG_SETUP(VS_CPP_PACKAGE);

StreamCoderTest :: StreamCoderTest()
{
  h = 0;
  hw = 0;
}

StreamCoderTest :: ~StreamCoderTest()
{
  tearDown();
}

void
StreamCoderTest :: setUp()
{
  coder = 0;
  h = new Helper();
  hw = new Helper();
}

void
StreamCoderTest :: tearDown()
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
StreamCoderTest :: testGetters()
{
  int refcount=0;
  h->setupReading(h->SAMPLE_FILE);

  for(int i = 0; i< h->num_streams; i++)
  {
    RefPointer<IStream> stream;
    RefPointer<ICodec> codec;
    RefPointer<IRational> rational;
    stream = h->streams[i];
    coder = h->coders[i];

    VS_TUT_ENSURE_EQUALS("invalid direction", coder->getDirection(),
        IStreamCoder::DECODING);

    codec = coder->getCodec();
    refcount = codec->getCurrentRefCount();
    coder->setCodec(codec.value());
    // ensure that one release and one acquire happens
    VS_TUT_ENSURE_EQUALS("invalid releasing or acquiring of codec",
        codec->getCurrentRefCount(),
        refcount);

    VS_TUT_ENSURE_EQUALS("wrong codec type",
        codec->getType(),
        h->expected_codec_types[i]);
    VS_TUT_ENSURE_EQUALS("wrong codec id",
        codec->getID(),
        h->expected_codec_ids[i]);

    if (codec->getType() == ICodec::CODEC_TYPE_AUDIO)
    {
      if (h->expected_sample_rate)
        VS_TUT_ENSURE_EQUALS("unexpected sample rate",
            coder->getSampleRate(),
            h->expected_sample_rate
        );
      if (h->expected_channels)
        VS_TUT_ENSURE_EQUALS("unexpected sample rate",
            coder->getChannels(),
            h->expected_channels
        );

    } else if (codec->getType() == ICodec::CODEC_TYPE_VIDEO)
    {
      if (h->expected_width)
        VS_TUT_ENSURE_EQUALS("unexpected width",
            coder->getWidth(),
            h->expected_width
        );
      if (h->expected_height)
              VS_TUT_ENSURE_EQUALS("unexpected height",
                  coder->getHeight(),
                  h->expected_height
              );
      if (h->expected_gops)
        VS_TUT_ENSURE_EQUALS("unexpected group of pictures",
            coder->getNumPicturesInGroupOfPictures(),
            h->expected_gops
        );
      if (h->expected_pixel_format != IPixelFormat::NONE)
        VS_TUT_ENSURE_EQUALS("unexpected group of pictures",
            coder->getPixelType(),
            h->expected_pixel_format
        );
      if (h->expected_time_base)
      {
        rational = coder->getTimeBase();
        VS_TUT_ENSURE_DISTANCE("unexpected time base",
            rational->getDouble(),
            h->expected_time_base,
            0.0001
        );
        rational = stream->getTimeBase();
        VS_TUT_ENSURE_DISTANCE("unexpected time base",
            rational->getDouble(),
            h->expected_time_base,
            0.0001
        );

      }
    }
    else
    {
      VS_LOG_ERROR("Unexpected type of codec");
      TS_ASSERT(false);
    }
  }
}

void
StreamCoderTest :: testSetCodec()
{
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_WARN, false);

  h->setupReading(h->SAMPLE_FILE);

  coder = h->coders[0];
  RefPointer<ICodec> codec = h->codecs[0];

  // The helper, the coder, and me
  VS_TUT_ENSURE_EQUALS("wrong codec ref count",
      codec->getCurrentRefCount(),
      3);

  coder->setCodec(0);
  // The helper, and me
  VS_TUT_ENSURE_EQUALS("wrong codec ref count",
      codec->getCurrentRefCount(),
      2);

  coder->setCodec(ICodec::CODEC_ID_NELLYMOSER);
  codec = coder->getCodec();
  VS_TUT_ENSURE("could not find NELLYMOSER codec", codec);

  // The coder, and me
  VS_TUT_ENSURE_EQUALS("codec refcount wrong",
      codec->getCurrentRefCount(),
      2);
  // Just the helper
  VS_TUT_ENSURE_EQUALS("codec refcount wrong",
      h->codecs[0]->getCurrentRefCount(),
      1);

  codec = ICodec::findDecodingCodecByName("aac");
  VS_TUT_ENSURE("could not find ac3 decoder", codec);
  VS_TUT_ENSURE_EQUALS("ac3 codec refcount wrong",
      codec->getCurrentRefCount(),
      1);
  coder->setCodec(codec.value());
  VS_TUT_ENSURE_EQUALS("ac3 codec refcount wrong after setting",
      codec->getCurrentRefCount(),
      2);

  // This should fail because it's an Encoder, not a Decoder
  RefPointer<ICodec> encodec = h->codecs[0];

  encodec = ICodec::findEncodingCodecByName("libmp3lame");

  VS_TUT_ENSURE("could not find libmp3lame encoder", codec);
  coder->setSampleRate(22050);
  coder->setCodec(encodec.value());
  codec = coder->getCodec();
  VS_TUT_ENSURE_EQUALS("should allow wrong direction", codec.value(), encodec.value());
  int retval = coder->open();
  VS_TUT_ENSURE("should succeed even though wrong direction", retval >= 0);
  (void) retval; // in release compiles VS_TUT_ENSURE is removed and so -Werror produces errors about unused retvals
  coder->close();
}

void
StreamCoderTest :: testOpenAndClose()
{
  int retval = 0;
  h->setupReading(h->SAMPLE_FILE);
  for(int i = 0; i < h->num_streams; i++)
  {
    retval = h->coders[i]->open();
    VS_TUT_ENSURE("could not open coder", retval >= 0);
    retval = h->coders[i]->close();
    VS_TUT_ENSURE("could not close coder", retval >= 0);
  }
}

void
StreamCoderTest :: testOpenButNoClose()
{
  int retval = 0;

  // Temporarily turn down logging
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_WARN, false);
  {
    Helper lh;
    lh.setupReading(lh.SAMPLE_FILE);
    for(int i = 0; i < lh.num_streams; i++)
    {
      retval = lh.coders[i]->open();
      VS_TUT_ENSURE("could not open coder", retval >= 0);
    }
  }
}

void
StreamCoderTest :: testCloseButNoOpen()
{
  int retval = 0;
  h->setupReading(h->SAMPLE_FILE);
  for(int i = 0; i < h->num_streams; i++)
  {
    retval = h->coders[i]->close();
    VS_TUT_ENSURE("could close coder?", retval < 0);
  }
}

void
StreamCoderTest :: testDecodingAndEncodingFullyInterleavedFile()
{
  RefPointer<IAudioSamples> samples = 0;
  RefPointer<IVideoPicture> frame = 0;
  RefPointer<IRational> num(0);
  RefPointer<IStreamCoder> ic(0);
  RefPointer<IStreamCoder> oc(0);

  int numSamples = 0;
  int numPackets = 0;
  int numFrames = 0;
  int numKeyFrames = 0;

  int retval = -1;

  h->setupReading(h->SAMPLE_FILE);

  RefPointer<IPacket> packet = IPacket::make();

  hw->setupWriting("StreamCoderTest_6_output.mov", "mpeg4", "pcm_mulaw", "mov");

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
            VS_TUT_ENSURE("audio duration not set", opacket->getDuration() > 0);
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

void
StreamCoderTest :: testTimestamps()
{
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_WARN, false);
  Helper hr;
  hr.setupReading("testfile_h264_mp4a_tmcd.mov");
  RefPointer<IStreamCoder> ac;
  RefPointer<IStreamCoder> vc;
  if (hr.first_input_audio_stream >= 0)
    ac = hr.coders[hr.first_input_audio_stream];

  if (hr.first_input_video_stream >= 0)
    vc = hr.coders[hr.first_input_video_stream];

  hw->setupWriting("StreamCoderTest_7_output.flv",
      vc ? "flv" : 0,
      ac ? "libmp3lame" : 0,
      "flv",
      vc ? vc->getPixelType() : IPixelFormat::YUV420P,
      vc ? vc->getWidth() : 0,
      vc ? vc->getHeight() : 0,
      false,
      ac ? ac->getSampleRate() : 0,
      ac ? ac->getChannels() : 0,
      true);

  RefPointer<IVideoPicture> frame;

  if (vc) {
    frame = IVideoPicture::make(
      vc->getPixelType(),
      vc->getWidth(),
      vc->getHeight());
  }
  RefPointer<IAudioSamples> samples;
  if (ac) {
    samples = IAudioSamples::make(10000,
        ac->getChannels());
  }
  int retval = -1;
  int numReads = 0;
  int maxReads = 10000;

  while ((retval = hr.readNextDecodedObject(samples.value(), frame.value())) > 0
      && numReads < maxReads)
  {
    int result = retval;
    numReads++;
    VS_TUT_ENSURE("should only return 1 or 2", retval == 1 || retval == 2);

    if (retval == 1)
    {
      VS_TUT_ENSURE("something should be complete...", samples->isComplete());
      retval = hw->writeSamples(samples.value());
      VS_TUT_ENSURE("could not write audio", retval >= 0);
    }

    if (retval == 2)
    {
      VS_TUT_ENSURE("something should be complete...", frame->isComplete());
      retval = hw->writeFrame(frame.value());
      VS_TUT_ENSURE("could not write video", retval >= 0);
    }

    {
      RefPointer<IRational> timebase = hr.coders[hr.packet->getStreamIndex()]->getTimeBase();
      ICodec::Type type = hr.coders[hr.packet->getStreamIndex()]->getCodecType();
      (void) type;
      (void) result;
      VS_LOG_TRACE("Packet stream: %d.%d; pts: %lld; dts: %lld; spts: %lld; fpts: %lld; tb: %d/%d; ret: %d,"
          "samps: %d",
          hr.packet->getStreamIndex(),
          type,
          hr.packet->getPts(),
          hr.packet->getDts(),
          samples ? samples->getPts() : -1,
          frame ? frame->getPts() : -1,
          timebase->getNumerator(), timebase->getDenominator(),
          result,
          samples ? samples->getNumSamples() : 0);
    }


  }
  VS_TUT_ENSURE("should return some data",
      numReads > 0);
}

void
StreamCoderTest :: testGetFrameSize()
{
  int retval = 0;
  h->setupReading(h->SAMPLE_FILE);
  for(int i = 0; i < h->num_streams; i++)
  {
    retval = h->coders[i]->open();
    VS_TUT_ENSURE("could not open coder", retval >= 0);

    int frameSize = h->coders[i]->getAudioFrameSize();
    VS_TUT_ENSURE("framesize not set", frameSize >= 0);

    ICodec::Type type = h->coders[i]->getCodecType();
    if (type == ICodec::CODEC_TYPE_AUDIO)
    {
      VS_TUT_ENSURE_EQUALS("unexpected audio frame size",
          frameSize, 576);
    }
    else
    {
      // video
      VS_TUT_ENSURE_EQUALS("unexpected video frame size",
          frameSize, 0);
    }
    retval = h->coders[i]->close();
    VS_TUT_ENSURE("could not close codec", retval >= 0);
  }
}

void
StreamCoderTest :: disabled_testDecodingAndEncodingNellymoserAudio()
{
  RefPointer<IAudioSamples> samples = 0;
  RefPointer<IVideoPicture> frame = 0;
  RefPointer<IRational> num(0);
  RefPointer<IStreamCoder> ic(0);
  RefPointer<IStreamCoder> oc(0);

  int numSamples = 0;
  int numPackets = 0;
  int numFrames = 0;
  int numKeyFrames = 0;

  int retval = -1;

  h->setupReading(h->SAMPLE_FILE);

  RefPointer<IPacket> packet = IPacket::make();

  hw->setupWriting("StreamCoderTest_8_output.flv", "flv", "nellymoser", "flv");

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

void
StreamCoderTest :: testGetSetExtraData()
{
  int retval = 0;
  h->setupReading("testfile_h264_mp4a_tmcd.mov");
  for(int i = 0; i < h->num_streams; i++)
  {
    if (h->coders[i]->getCodecType() != ICodec::CODEC_TYPE_VIDEO)
      continue;
    retval = h->coders[i]->open();
    VS_TUT_ENSURE(0, retval >= 0);
    int32_t extraDataSize = h->coders[i]->getExtraDataSize();
    VS_TUT_ENSURE(0, extraDataSize > 0);
    uint8_t* bytes;
    if (extraDataSize > 0)
    {
      RefPointer<IBuffer> buffer = IBuffer::make(0, extraDataSize);
      retval = h->coders[i]->getExtraData(buffer.value(), 0, extraDataSize);
      VS_TUT_ENSURE_EQUALS(0, retval, extraDataSize);
      bytes = (uint8_t*)buffer->getBytes(0, extraDataSize);
      for(int j = 0; j < extraDataSize; j++)
      {
//        VS_LOG_DEBUG("Byte %d: %hhd", j, bytes[j]);
        bytes[j] = 5;
      }
      retval = h->coders[i]->setExtraData(buffer.value(), 0, extraDataSize, false);
      VS_TUT_ENSURE_EQUALS(0, retval, extraDataSize);
      // reset the buffer
      buffer = IBuffer::make(0, extraDataSize+1);
      retval = h->coders[i]->getExtraData(buffer.value(), 0, extraDataSize);
      VS_TUT_ENSURE_EQUALS(0, retval, extraDataSize);
      bytes = (uint8_t*)buffer->getBytes(0, extraDataSize);
      for(int j = 0; j < extraDataSize; j++)
      {
        VS_TUT_ENSURE_EQUALS(0, bytes[j], 5);
      }
      bytes[extraDataSize] = 5;
      // test the path where we re-alloc buffers
      retval = h->coders[i]->setExtraData(buffer.value(), 0, extraDataSize+1, true);
      VS_TUT_ENSURE_EQUALS(0, retval, extraDataSize+1);
      // reset the buffer
      buffer = IBuffer::make(0, extraDataSize+1);
      retval = h->coders[i]->getExtraData(buffer.value(), 0, extraDataSize+1);
      VS_TUT_ENSURE_EQUALS(0, retval, extraDataSize+1);
      bytes = (uint8_t*)buffer->getBytes(0, extraDataSize+1);
      for(int j = 0; j < extraDataSize+1; j++)
      {
        VS_TUT_ENSURE_EQUALS(0, bytes[j], 5);
      }
      h->coders[i]->close();
    }
  }
}

