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
#include "VideoPictureTest.h"

using namespace VS_CPP_NAMESPACE;

VS_LOG_SETUP(VS_CPP_PACKAGE);

VideoPictureTest :: VideoPictureTest()
{
  hr = 0;
  hw = 0;
}

VideoPictureTest :: ~VideoPictureTest()
{
  tearDown();
}

void
VideoPictureTest :: setUp()
{
  hr = new Helper();
  hw = new Helper();
}

void
VideoPictureTest :: tearDown()
{
  if (hr)
    delete hr;
  hr = 0;
  if (hw)
    delete hw;
  hw = 0;
}

void
VideoPictureTest :: testCreationAndDestruction()
{
  RefPointer<IVideoPicture> frame = 0;

  frame = IVideoPicture::make(IPixelFormat::YUV420P, 320, 240);
  VS_TUT_ENSURE("got no frame", frame);
  VS_TUT_ENSURE("not a key frame", frame->isKeyFrame());
  VS_TUT_ENSURE("not complete", !frame->isComplete());
  VS_TUT_ENSURE_EQUALS("wrong width", frame->getWidth(), 320);
  VS_TUT_ENSURE_EQUALS("wrong height", frame->getHeight(), 240);
  VS_TUT_ENSURE_EQUALS("wrong pixel format", frame->getPixelType(),
      IPixelFormat::YUV420P);
}

void
VideoPictureTest :: testDecodingIntoReusedFrame()
{
  int numFrames = 0;
  int numKeyFrames = 0;
  int numPackets = 0;
  int videoStream = -1;
  int retval = -1;
  RefPointer<IVideoPicture> frame = 0;

  int maxFrames = 15 * 10; // 10 seconds of 15fps video
  hr->setupReading(hr->SAMPLE_FILE);
  RefPointer<IPacket> packet = IPacket::make();
  for (int i = 0; i < hr->num_streams; i++)
  {
    if (hr->codecs[i]->getType() == ICodec::CODEC_TYPE_VIDEO)
    {
      // got audio
      videoStream = i;

      frame = IVideoPicture::make(
          hr->coders[i]->getPixelType(),
          hr->coders[i]->getWidth(),
          hr->coders[i]->getHeight());
      VS_TUT_ENSURE("got no frame", frame);


      retval = hr->coders[i]->open();
      VS_TUT_ENSURE("! open coder", retval >= 0);
      break;
    }
  }
  VS_TUT_ENSURE("couldn't find a video stream", videoStream >= 0);
  while (hr->container->readNextPacket(packet.value()) == 0
      && numFrames < maxFrames)
  {
    if (packet->getStreamIndex() == videoStream)
    {
      int offset = 0;

      numPackets++;

      VS_LOG_TRACE("packet: pts: %ld; dts: %ld", packet->getPts(),
          packet->getDts());

      while (offset < packet->getSize())
      {
        retval = hr->coders[videoStream]->decodeVideo(
            frame.value(),
            packet.value(),
            offset);
        VS_TUT_ENSURE("could not decode any video", retval>0);
        VS_LOG_TRACE("Presentation time stamp: %ld", frame->getPts());
        offset += retval;
        if (frame->isComplete())
        {
          numFrames++;
          if (frame->isKeyFrame())
            numKeyFrames++;
          if (hr->expected_pixel_format != IPixelFormat::NONE)
            VS_TUT_ENSURE_EQUALS("unexpected pixel type",
                frame->getPixelType(),
                hr->expected_pixel_format);
          if (hr->expected_width)
            VS_TUT_ENSURE_EQUALS("unexpected width",
                frame->getWidth(),
                hr->expected_width);
          if (hr->expected_height)
            VS_TUT_ENSURE_EQUALS("unexpected height",
                frame->getHeight(),
                hr->expected_height);
        }
      }
    }
  }
  retval = hr->coders[videoStream]->close();
  VS_TUT_ENSURE("could not close coder", retval >= 0);
  VS_TUT_ENSURE("got partial last frame", frame->isComplete());
  VS_TUT_ENSURE("could not find video packets", numPackets >0);
  VS_TUT_ENSURE("could not decode any video", numFrames >0);
  VS_TUT_ENSURE("could not find any key frames", numKeyFrames >0);
}

void
VideoPictureTest :: testDecodingAndEncodingIntoFrame()
{
  int numFrames = 0;
  int numKeyFrames = 0;
  int numPackets = 0;
  int videoStream = -1;
  int retval = -1;
  RefPointer<IVideoPicture> frame = 0;
  RefPointer<IRational> num(0);

  int maxFrames = 15 * 10; // 10 seconds of 15fps video

  RefPointer<IPacket> packet = IPacket::make();
  VS_TUT_ENSURE("got no packet", packet);

  hw->setupWriting("FrameTest_3_output.flv", "flv", 0, "flv");
  int outStream = hw->first_output_video_stream;
  VS_TUT_ENSURE("couldn't find an outbound video stream", outStream >= 0);

  hr->setupReading(hr->SAMPLE_FILE);
  videoStream = hr->first_input_video_stream;
  VS_TUT_ENSURE("couldn't find a video stream", videoStream >= 0);

  retval = hr->coders[videoStream]->open();
  VS_TUT_ENSURE("! open coder", retval >= 0);

  // Set up the outbound coder with the right
  // type information
  {
    RefPointer<IStreamCoder> ic = hr->coders[videoStream];
    RefPointer<IStreamCoder> oc = hw->coders[outStream];
    oc->setBitRate(ic->getBitRate());
    oc->setPixelType(ic->getPixelType());
    oc->setHeight(ic->getHeight());
    oc->setWidth(ic->getWidth());
    num = IRational::make(15,1);
    oc->setFrameRate(num.value());
    num = IRational::make(1,1000);
    oc->setTimeBase(num.value());
    oc->setNumPicturesInGroupOfPictures(
        ic->getNumPicturesInGroupOfPictures()
        );

    frame = IVideoPicture::make(
        ic->getPixelType(),
        ic->getWidth(),
        ic->getHeight());
    VS_TUT_ENSURE("got no frame", frame);

    // write a header to our file.
    retval = hw->container->writeHeader();
    VS_TUT_ENSURE("! writeHeader", retval >= 0);
    // open the outbound coder
    retval = oc->open();
    VS_TUT_ENSURE("! open coder", retval >= 0);
  }

  RefPointer<IPacket> encodingPacket = IPacket::make();
  VS_TUT_ENSURE("! encodingPacket", encodingPacket);

  retval = encodingPacket->allocateNewPayload(200000);
  VS_TUT_ENSURE("! allocateNewPayload", retval >= 0);

  while (hr->container->readNextPacket(packet.value()) == 0
      && numFrames < maxFrames)
  {
    VS_LOG_TRACE("packet: pts: %lld dts: %lld size: %d index: %d flags: %d duration: %d position: %lld",
        packet->getPts(),
        packet->getDts(),
        packet->getSize(),
        packet->getStreamIndex(),
        packet->getFlags(),
        packet->getDuration(),
        packet->getPosition());
    if (packet->getStreamIndex() == videoStream)
    {
      int offset = 0;

      numPackets++;

      VS_LOG_TRACE("packet: pts: %ld; dts: %ld", packet->getPts(),
          packet->getDts());
      while (offset < packet->getSize())
      {

        retval = hr->coders[videoStream]->decodeVideo(
            frame.value(),
            packet.value(),
            offset);
        VS_TUT_ENSURE("could not decode any video", retval>0);
        num = hr->coders[videoStream]->getTimeBase();
        VS_TUT_ENSURE_EQUALS("time base changed", num->getDouble(), hr->expected_time_base);
        offset += retval;
        if (frame->isComplete())
        {
          numFrames++;
          if (frame->isKeyFrame())
            numKeyFrames++;
          VS_LOG_TRACE("Encoding frame %d; pts: %lld; cur_dts: %lld",
              numFrames, frame->getPts(),
              hr->streams[videoStream]->getCurrentDts());

          retval = hw->coders[outStream]->encodeVideo(
              encodingPacket.value(),
              frame.value(),
              -1);

          VS_TUT_ENSURE("could not encode video", retval >= 0);
          VS_TUT_ENSURE("could not encode video", encodingPacket->getSize() > 0);

          RefPointer<IBuffer> encodedBuffer = encodingPacket->getData();
          VS_TUT_ENSURE("no encoded data", encodedBuffer);
          VS_TUT_ENSURE("less data than there should be",
              encodedBuffer->getBufferSize() >=
              encodingPacket->getSize());

          // now, write the packet to disk.
          retval = hw->container->writePacket(encodingPacket.value());
          VS_TUT_ENSURE("could not write packet", retval >= 0);

        }
      }
    }
  }
  // sigh; it turns out that to flush the encoding buffers you need to
  // ask the encoder to encode a NULL set of samples.  So, let's do that.
  retval = hw->coders[outStream]->encodeVideo(encodingPacket.value(), 0, 0);
  VS_TUT_ENSURE("Could not encode any video", retval >= 0);
  if (retval > 0)
  {
    retval = hw->container->writePacket(encodingPacket.value());
    VS_TUT_ENSURE("could not write packet", retval >= 0);
  }


  retval = hr->coders[videoStream]->close();
  VS_TUT_ENSURE("could not close coder", retval >= 0);
  // open the outbound coder
  retval = hw->coders[outStream]->close();
  VS_TUT_ENSURE("! close coder", retval >= 0);
  VS_TUT_ENSURE("got partial last frame", frame->isComplete());
  VS_TUT_ENSURE("could not find video packets", numPackets >0);
  VS_TUT_ENSURE("could not decode any video", numFrames >0);
  VS_TUT_ENSURE("could not find any key frames", numKeyFrames >0);
  retval = hw->container->writeTrailer();
  VS_TUT_ENSURE("! writeTrailer", retval >= 0);
}

void
VideoPictureTest :: testDecodingAndEncodingIntoFrameByCopyingData()
{
  int numFrames = 0;
  int numKeyFrames = 0;
  int numPackets = 0;
  int videoStream = -1;
  int retval = -1;
  RefPointer<IVideoPicture> frame = 0;
  RefPointer<IRational> num(0);

  int maxFrames = 15 * 10; // 10 seconds of 15fps video

  RefPointer<IPacket> packet = IPacket::make();
  VS_TUT_ENSURE("got no packet", packet);

  hw->setupWriting("FrameTest_4_output.flv", "flv", 0, "flv");
  int outStream = hw->first_output_video_stream;
  VS_TUT_ENSURE("couldn't find an outbound video stream", outStream >= 0);

  hr->setupReading(hr->SAMPLE_FILE);
  videoStream = hr->first_input_video_stream;
  VS_TUT_ENSURE("couldn't find a video stream", videoStream >= 0);

  retval = hr->coders[videoStream]->open();
  VS_TUT_ENSURE("! open coder", retval >= 0);

  // Set up the outbound coder with the right
  // type information
  {
    RefPointer<IStreamCoder> ic = hr->coders[videoStream];
    RefPointer<IStreamCoder> oc = hw->coders[outStream];
    oc->setBitRate(ic->getBitRate());
    oc->setPixelType(ic->getPixelType());
    oc->setHeight(ic->getHeight());
    oc->setWidth(ic->getWidth());
    num = IRational::make(15,1);
    oc->setFrameRate(num.value());
    num = IRational::make(1,1000);
    oc->setTimeBase(num.value());
    oc->setNumPicturesInGroupOfPictures(
        ic->getNumPicturesInGroupOfPictures()
        );

    frame = IVideoPicture::make(
        ic->getPixelType(),
        ic->getWidth(),
        ic->getHeight());
    VS_TUT_ENSURE("got no frame", frame);

    // write a header to our file.
    retval = hw->container->writeHeader();
    VS_TUT_ENSURE("! writeHeader", retval >= 0);
    // open the outbound coder
    retval = oc->open();
    VS_TUT_ENSURE("! open coder", retval >= 0);
  }

  RefPointer<IPacket> encodingPacket = IPacket::make();
  VS_TUT_ENSURE("! encodingPacket", encodingPacket);

  retval = encodingPacket->allocateNewPayload(200000);
  VS_TUT_ENSURE("! allocateNewPayload", retval >= 0);

  while (hr->container->readNextPacket(packet.value()) == 0
      && numFrames < maxFrames)
  {
    VS_LOG_TRACE("packet: pts: %lld dts: %lld size: %d index: %d flags: %d duration: %d position: %lld",
        packet->getPts(),
        packet->getDts(),
        packet->getSize(),
        packet->getStreamIndex(),
        packet->getFlags(),
        packet->getDuration(),
        packet->getPosition());
    if (packet->getStreamIndex() == videoStream)
    {
      int offset = 0;

      numPackets++;

      VS_LOG_TRACE("packet: pts: %ld; dts: %ld", packet->getPts(),
          packet->getDts());
      while (offset < packet->getSize())
      {

        retval = hr->coders[videoStream]->decodeVideo(
            frame.value(),
            packet.value(),
            offset);
        VS_TUT_ENSURE("could not decode any video", retval>0);
        num = hr->coders[videoStream]->getTimeBase();
        VS_TUT_ENSURE_EQUALS("time base changed", num->getDouble(), hr->expected_time_base);
        offset += retval;
        if (frame->isComplete())
        {
          numFrames++;
          if (frame->isKeyFrame())
            numKeyFrames++;

          // This is the key part of the test; we're going to try copying
          // the frame into a new frame;
          RefPointer<IVideoPicture> frameCopy = IVideoPicture::make(
              frame->getPixelType(),
              frame->getWidth(),
              frame->getHeight());
          VS_TUT_ENSURE("Could not make new frame", frameCopy);
          VS_TUT_ENSURE("could not copy frame", frameCopy->copy(frame.value()));

          
          VS_LOG_TRACE("Encoding frame %d; pts: %lld; cur_dts: %lld",
              numFrames, frameCopy->getPts(),
              hr->streams[videoStream]->getCurrentDts());

          retval = hw->coders[outStream]->encodeVideo(
              encodingPacket.value(),
              frameCopy.value(),
              -1);

          VS_TUT_ENSURE("could not encode video", retval >= 0);
          VS_TUT_ENSURE("could not encode video", encodingPacket->getSize() > 0);

          RefPointer<IBuffer> encodedBuffer = encodingPacket->getData();
          VS_TUT_ENSURE("no encoded data", encodedBuffer);
          VS_TUT_ENSURE("less data than there should be",
              encodedBuffer->getBufferSize() >=
              encodingPacket->getSize());

          // now, write the packet to disk.
          retval = hw->container->writePacket(encodingPacket.value());
          VS_TUT_ENSURE("could not write packet", retval >= 0);

        }
      }
    }
  }
  // sigh; it turns out that to flush the encoding buffers you need to
  // ask the encoder to encode a NULL set of samples.  So, let's do that.
  retval = hw->coders[outStream]->encodeVideo(encodingPacket.value(), 0, 0);
  VS_TUT_ENSURE("Could not encode any video", retval >= 0);
  if (retval > 0)
  {
    retval = hw->container->writePacket(encodingPacket.value());
    VS_TUT_ENSURE("could not write packet", retval >= 0);
  }


  retval = hr->coders[videoStream]->close();
  VS_TUT_ENSURE("could not close coder", retval >= 0);
  // open the outbound coder
  retval = hw->coders[outStream]->close();
  VS_TUT_ENSURE("! close coder", retval >= 0);
  VS_TUT_ENSURE("got partial last frame", frame->isComplete());
  VS_TUT_ENSURE("could not find video packets", numPackets >0);
  VS_TUT_ENSURE("could not decode any video", numFrames >0);
  VS_TUT_ENSURE("could not find any key frames", numKeyFrames >0);
  retval = hw->container->writeTrailer();
  VS_TUT_ENSURE("! writeTrailer", retval >= 0);
}

void
VideoPictureTest :: testDecodingAndEncodingIntoAFrameByCopyingDataInPlace()
{
  int numFrames = 0;
  int numKeyFrames = 0;
  int numPackets = 0;
  int videoStream = -1;
  int retval = -1;
  RefPointer<IVideoPicture> frame = 0;
  RefPointer<IVideoPicture> frameCopy = 0;

  RefPointer<IRational> num(0);

  int maxFrames = 15 * 10; // 10 seconds of 15fps video

  RefPointer<IPacket> packet = IPacket::make();
  VS_TUT_ENSURE("got no packet", packet);

  hw->setupWriting("FrameTest_5_output.flv", "flv", 0, "flv");
  int outStream = hw->first_output_video_stream;
  VS_TUT_ENSURE("couldn't find an outbound video stream", outStream >= 0);

  hr->setupReading(hr->SAMPLE_FILE);
  videoStream = hr->first_input_video_stream;
  VS_TUT_ENSURE("couldn't find a video stream", videoStream >= 0);

  retval = hr->coders[videoStream]->open();
  VS_TUT_ENSURE("! open coder", retval >= 0);

  // Set up the outbound coder with the right
  // type information
  {
    RefPointer<IStreamCoder> ic = hr->coders[videoStream];
    RefPointer<IStreamCoder> oc = hw->coders[outStream];
    oc->setBitRate(ic->getBitRate());
    oc->setPixelType(ic->getPixelType());
    oc->setHeight(ic->getHeight());
    oc->setWidth(ic->getWidth());
    num = IRational::make(15,1);
    oc->setFrameRate(num.value());
    num = IRational::make(1,1000);
    oc->setTimeBase(num.value());
    oc->setNumPicturesInGroupOfPictures(
        ic->getNumPicturesInGroupOfPictures()
        );

    frame = IVideoPicture::make(
        ic->getPixelType(),
        ic->getWidth(),
        ic->getHeight());
    VS_TUT_ENSURE("got no frame", frame);

    frameCopy = IVideoPicture::make(
        ic->getPixelType(),
        ic->getWidth(),
        ic->getHeight());
    VS_TUT_ENSURE("got no frame", frameCopy);


    // write a header to our file.
    retval = hw->container->writeHeader();
    VS_TUT_ENSURE("! writeHeader", retval >= 0);
    // open the outbound coder
    retval = oc->open();
    VS_TUT_ENSURE("! open coder", retval >= 0);
  }

  RefPointer<IPacket> encodingPacket = IPacket::make();
  VS_TUT_ENSURE("! encodingPacket", encodingPacket);

  retval = encodingPacket->allocateNewPayload(200000);
  VS_TUT_ENSURE("! allocateNewPayload", retval >= 0);

  while (hr->container->readNextPacket(packet.value()) == 0
      && numFrames < maxFrames)
  {
    VS_LOG_TRACE("packet: pts: %lld dts: %lld size: %d index: %d flags: %d duration: %d position: %lld",
        packet->getPts(),
        packet->getDts(),
        packet->getSize(),
        packet->getStreamIndex(),
        packet->getFlags(),
        packet->getDuration(),
        packet->getPosition());
    if (packet->getStreamIndex() == videoStream)
    {
      int offset = 0;

      numPackets++;

      VS_LOG_TRACE("packet: pts: %ld; dts: %ld", packet->getPts(),
          packet->getDts());
      while (offset < packet->getSize())
      {

        retval = hr->coders[videoStream]->decodeVideo(
            frame.value(),
            packet.value(),
            offset);
        VS_TUT_ENSURE("could not decode any video", retval>0);
        num = hr->coders[videoStream]->getTimeBase();
        VS_TUT_ENSURE_EQUALS("time base changed", num->getDouble(), hr->expected_time_base);
        offset += retval;
        if (frame->isComplete())
        {
          numFrames++;
          if (frame->isKeyFrame())
            numKeyFrames++;

          // This is the key part of the test; we're going to try copying
          // the frame into a new frame;
          bool copy = frameCopy->copy(frame.value());
          VS_TUT_ENSURE("could not copy frame", copy);

          VS_LOG_TRACE("Encoding frame %d; pts: %lld; cur_dts: %lld",
              numFrames, frameCopy->getPts(),
              hr->streams[videoStream]->getCurrentDts());

          retval = hw->coders[outStream]->encodeVideo(
              encodingPacket.value(),
              frameCopy.value(),
              -1);

          VS_TUT_ENSURE("could not encode video", retval >= 0);
          VS_TUT_ENSURE("could not encode video", encodingPacket->getSize() > 0);

          RefPointer<IBuffer> encodedBuffer = encodingPacket->getData();
          VS_TUT_ENSURE("no encoded data", encodedBuffer);
          VS_TUT_ENSURE("less data than there should be",
              encodedBuffer->getBufferSize() >=
              encodingPacket->getSize());

          // now, write the packet to disk.
          retval = hw->container->writePacket(encodingPacket.value());
          VS_TUT_ENSURE("could not write packet", retval >= 0);

        }
      }
    }
  }
  // sigh; it turns out that to flush the encoding buffers you need to
  // ask the encoder to encode a NULL set of samples.  So, let's do that.
  retval = hw->coders[outStream]->encodeVideo(encodingPacket.value(), 0, 0);
  VS_TUT_ENSURE("Could not encode any video", retval >= 0);
  if (retval > 0)
  {
    retval = hw->container->writePacket(encodingPacket.value());
    VS_TUT_ENSURE("could not write packet", retval >= 0);
  }


  retval = hr->coders[videoStream]->close();
  VS_TUT_ENSURE("could not close coder", retval >= 0);
  // open the outbound coder
  retval = hw->coders[outStream]->close();
  VS_TUT_ENSURE("! close coder", retval >= 0);
  VS_TUT_ENSURE("got partial last frame", frame->isComplete());
  VS_TUT_ENSURE("could not find video packets", numPackets >0);
  VS_TUT_ENSURE("could not decode any video", numFrames >0);
  VS_TUT_ENSURE("could not find any key frames", numKeyFrames >0);
  retval = hw->container->writeTrailer();
  VS_TUT_ENSURE("! writeTrailer", retval >= 0);
}

void
VideoPictureTest :: testGetAndSetPts()
{
  RefPointer<IVideoPicture> frame = 0;

  frame = IVideoPicture::make(IPixelFormat::YUV420P, 320, 240);
  VS_TUT_ENSURE("got no frame", frame);
  VS_TUT_ENSURE("not a key frame", frame->isKeyFrame());
  VS_TUT_ENSURE("not complete", !frame->isComplete());
  VS_TUT_ENSURE_EQUALS("wrong width", frame->getWidth(), 320);
  VS_TUT_ENSURE_EQUALS("wrong height", frame->getHeight(), 240);
  VS_TUT_ENSURE_EQUALS("wrong pixel format", frame->getPixelType(),
      IPixelFormat::YUV420P);

  frame->setComplete(true, IPixelFormat::YUV420P, 320, 240, 1);
  VS_TUT_ENSURE_EQUALS("unexpected pts", frame->getPts(), 1);
  frame->setPts(2);
  VS_TUT_ENSURE_EQUALS("unexpected pts", frame->getPts(), 2);

}
