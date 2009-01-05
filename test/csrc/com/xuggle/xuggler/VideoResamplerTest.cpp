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
#include <com/xuggle/xuggler/Global.h>
#include <com/xuggle/xuggler/IVideoResampler.h>
#include "VideoResamplerTest.h"

VS_LOG_SETUP(VS_CPP_PACKAGE);

VideoResamplerTest :: VideoResamplerTest()
{
  h = 0;
  hw = 0;
}
VideoResamplerTest :: ~VideoResamplerTest()
{
  tearDown();
}

void
VideoResamplerTest :: setUp()
{
  h = new Helper();
  hw = new Helper();
  mResampler = 0;
}
void
VideoResamplerTest :: tearDown()
{
  if (h)
    delete h;
  h = 0;

  if (hw)
    delete hw;
  hw = 0;
  mResampler = 0;
}

void
VideoResamplerTest :: frameListener(void* context, IVideoPicture* pOutFrame, IVideoPicture* pInFrame)
{
  VideoResamplerTest* o = (VideoResamplerTest*)context;
  int retval = -1;

  retval = o->mResampler->resample(pOutFrame, pInFrame);
  VS_TUT_ENSURE("could not resample", retval >= 0);
}

void
VideoResamplerTest :: testGetter()
{
  int iHeight=240;
  int iWidth=320;
  int oHeight=iHeight*2;
  int oWidth=iWidth*2;
  IPixelFormat::Type iFmt = IPixelFormat::YUV420P;
  IPixelFormat::Type oFmt = IPixelFormat::RGB24;
  RefPointer<IVideoResampler> resampler = IVideoResampler::make(
      oWidth, oHeight, oFmt,
      iWidth, iHeight, iFmt);

  VS_TUT_ENSURE("! resampler", resampler);

  VS_TUT_ENSURE_EQUALS("unexpected val",
      resampler->getInputWidth(),
      iWidth);
  VS_TUT_ENSURE_EQUALS("unexpected val",
      resampler->getInputHeight(),
      iHeight);
  VS_TUT_ENSURE_EQUALS("unexpected val",
      resampler->getInputPixelFormat(),
      iFmt);
  VS_TUT_ENSURE_EQUALS("unexpected val",
      resampler->getOutputWidth(),
      oWidth);
  VS_TUT_ENSURE_EQUALS("unexpected val",
      resampler->getOutputHeight(),
      oHeight);
  VS_TUT_ENSURE_EQUALS("unexpected val",
      resampler->getOutputPixelFormat(),
      oFmt);
}

void
VideoResamplerTest :: testInvalidConstructors()
{
  int iHeight=240;
  int iWidth=320;
  int oHeight=iHeight*2;
  int oWidth=iWidth*2;
  IPixelFormat::Type iFmt = IPixelFormat::YUV420P;
  IPixelFormat::Type oFmt = IPixelFormat::RGB24;
  RefPointer<IVideoResampler> resampler;

  // test different aspect ratio
  resampler = IVideoResampler::make(
      oWidth, oHeight, oFmt,
      iWidth+1, iHeight+2, oFmt);
  VS_TUT_ENSURE(" resampler", resampler);

  // Temporarily turn down logging
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_DEBUG, false);

  // Now try invalid values.
  resampler = IVideoResampler::make(
      0, oHeight, oFmt,
      iWidth, iHeight, iFmt);
  VS_TUT_ENSURE("! resampler", !resampler);

  resampler = IVideoResampler::make(
      oWidth, 0, oFmt,
      iWidth, iHeight, iFmt);
  VS_TUT_ENSURE("! resampler", !resampler);

  resampler = IVideoResampler::make(
      oWidth, oHeight, IPixelFormat::NONE,
      iWidth, iHeight, iFmt);
  VS_TUT_ENSURE("! resampler", !resampler);

  resampler = IVideoResampler::make(
      oWidth, oHeight, oFmt,
      0, iHeight, iFmt);
  VS_TUT_ENSURE("! resampler", !resampler);

  resampler = IVideoResampler::make(
      oWidth, oHeight, oFmt,
      iWidth, 0, iFmt);
  VS_TUT_ENSURE("! resampler", !resampler);

  resampler = IVideoResampler::make(
      oWidth, oHeight, oFmt,
      iWidth, iHeight, IPixelFormat::NONE);
  VS_TUT_ENSURE("! resampler", !resampler);

}

void
VideoResamplerTest :: testInvalidResamples()
{
  int iHeight=240;
  int iWidth=320;
  int oHeight=iHeight*2;
  int oWidth=iWidth*2;
  IPixelFormat::Type iFmt = IPixelFormat::YUV420P;
  IPixelFormat::Type oFmt = IPixelFormat::RGB24;
  RefPointer<IVideoResampler> resampler = IVideoResampler::make(
      oWidth, oHeight, oFmt,
      iWidth, iHeight, iFmt);
  RefPointer<IVideoPicture> inFrame = IVideoPicture::make(iFmt, iWidth, iHeight);
  RefPointer<IVideoPicture> outFrame = IVideoPicture::make(oFmt, oWidth, oHeight);


  VS_TUT_ENSURE("! resampler", resampler);
  VS_TUT_ENSURE("! inFrame", inFrame);
  VS_TUT_ENSURE("! outFrame", outFrame);

  int retval = -1;

  // Temporarily turn down logging
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_DEBUG, false);

  retval = resampler->resample(0, 0);
  VS_TUT_ENSURE("unexpected success", retval < 0);

  retval = resampler->resample(0, inFrame.value());
  VS_TUT_ENSURE("unexpected success", retval < 0);

  retval = resampler->resample(outFrame.value(), 0);
  VS_TUT_ENSURE("unexpected success", retval < 0);

  retval = resampler->resample(outFrame.value(), inFrame.value());
  VS_TUT_ENSURE("should be < 0 because inFrame is not complete", retval < 0);

}

void
VideoResamplerTest :: testResampleAndOutput()
{
  // Temporarily turn down logging
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_WARN, false);

  RefPointer<IVideoPicture> inFrame = 0;
  RefPointer<IVideoPicture> tmpFrame = 0;
  RefPointer<IVideoPicture> tmpFrame2 = 0;
  RefPointer<IVideoPicture> outFrame = 0;
  RefPointer<IVideoResampler> rgbResampler = 0;
  RefPointer<IVideoResampler> resizeResampler = 0;
  RefPointer<IVideoResampler> yuvResampler = 0;

  int maxFrames = 15*10;

  RefPointer<IRational> num(0);
  RefPointer<IStreamCoder> ic(0);
  RefPointer<IStreamCoder> oc(0);

  double scalingFactor=2.0;
  IPixelFormat::Type oFmt = IPixelFormat::BGR24;

  int numPackets = 0;
  int numFrames = 0;
  int numKeyFrames = 0;

  int retval = -1;

  h->setupReading(h->SAMPLE_FILE);

  RefPointer<IPacket> packet = IPacket::make();

  hw->setupWriting("VideoResamplerTest_4_output.flv", "flv", 0, "flv");

  RefPointer<IPacket> opacket = IPacket::make();
  VS_TUT_ENSURE("! opacket", opacket);

  {
    // now, let's set up video.
    ic = h->coders[h->first_input_video_stream];
    oc = hw->coders[hw->first_output_video_stream];

    oc->setBitRate(ic->getBitRate());
    oc->setPixelType(ic->getPixelType());
    // We're just stretching the height.
    oc->setHeight((int)(ic->getHeight()*scalingFactor));
    oc->setWidth(ic->getWidth());
    num = ic->getFrameRate();
    oc->setFrameRate(num.value());
    num = ic->getTimeBase();
    oc->setTimeBase(num.value());
    oc->setNumPicturesInGroupOfPictures(
        ic->getNumPicturesInGroupOfPictures()
        );

    inFrame = IVideoPicture::make(
        ic->getPixelType(),
        ic->getWidth(),
        ic->getHeight());
    VS_TUT_ENSURE("got no frame", inFrame);

    rgbResampler = IVideoResampler::make(
        ic->getWidth(), ic->getHeight(), oFmt,
        ic->getWidth(), ic->getHeight(), ic->getPixelType());
    VS_TUT_ENSURE("got no resampler", rgbResampler);

    resizeResampler = IVideoResampler::make(
        oc->getWidth(), oc->getHeight(), oFmt,
        ic->getWidth(), ic->getHeight(), oFmt);
    VS_TUT_ENSURE("got no resampler", resizeResampler);

    yuvResampler = IVideoResampler::make(
        oc->getWidth(), oc->getHeight(), oc->getPixelType(),
        oc->getWidth(), oc->getHeight(), oFmt);
    VS_TUT_ENSURE("got no resampler", yuvResampler);

    // tmp frame just converts to RGB
    tmpFrame = IVideoPicture::make(
        oFmt,
        ic->getWidth(),
        ic->getHeight());
    VS_TUT_ENSURE("got no frame", tmpFrame);

    // resizes.
    tmpFrame2 = IVideoPicture::make(
        oFmt,
        oc->getWidth(),
        oc->getHeight());
    VS_TUT_ENSURE("got no frame", tmpFrame);

    // final frame resizes and converts back to original Pixel Type
    outFrame = IVideoPicture::make(
        oc->getPixelType(),
        oc->getWidth(),
        oc->getHeight());
    VS_TUT_ENSURE("got no frame", outFrame);

    retval = ic->open();
    VS_TUT_ENSURE("Could not open input coder", retval >= 0);
    retval = oc->open();
    VS_TUT_ENSURE("Could not open output coder", retval >= 0);
  }

  // write header
  retval = hw->container->writeHeader();
  VS_TUT_ENSURE("could not write header", retval >= 0);

  while (h->container->readNextPacket(packet.value()) == 0
      && numFrames < maxFrames)
  {
    if (packet->getStreamIndex() == h->first_input_video_stream)
    {
      int offset = 0;

      ic = h->coders[packet->getStreamIndex()];
      oc = hw->coders[packet->getStreamIndex()];

      numPackets++;

      while (offset < packet->getSize())
      {

        retval = ic->decodeVideo(
            inFrame.value(),
            packet.value(),
            offset);
        VS_TUT_ENSURE("could not decode any video", retval>0);
        offset += retval;
        if (inFrame->isComplete())
        {
          numFrames++;
          if (inFrame->isKeyFrame())
            numKeyFrames++;

          // Convert into RGB.
          retval = rgbResampler->resample(tmpFrame.value(), inFrame.value());
          VS_TUT_ENSURE("Could not convert to RGB", retval >= 0);
          VS_TUT_ENSURE("not complete", tmpFrame->isComplete());

          // Resize
          retval = resizeResampler->resample(tmpFrame2.value(), tmpFrame.value());
          VS_TUT_ENSURE("Could not convert to RGB", retval >= 0);
          VS_TUT_ENSURE("not complete", tmpFrame2->isComplete());

          // Convert back to YUV
          retval = yuvResampler->resample(outFrame.value(), tmpFrame2.value());
          VS_TUT_ENSURE("could not resize and convert pixels", retval >= 0);
          VS_TUT_ENSURE("not complete", outFrame->isComplete());

          VS_TUT_ENSURE_EQUALS("make sure PTS is copied through",
              inFrame->getPts(), outFrame->getPts());

          retval = oc->encodeVideo(
              opacket.value(),
              outFrame.value(),
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
  retval = hw->coders[hw->first_output_video_stream]->encodeVideo(opacket.value(),
      0, 0);
  VS_TUT_ENSURE("Could not encode any video", retval >= 0);
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

  VS_TUT_ENSURE("could not decode any video", numFrames >0);
  VS_TUT_ENSURE("could not find any key frames", numKeyFrames >0);

}

void
VideoResamplerTest :: testSwitchPixelFormatsAndOutput()
{
  // Temporarily turn down logging
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_WARN, false);

  RefPointer<IVideoPicture> inFrame = 0;
  RefPointer<IVideoPicture> tmpFrame = 0;
  RefPointer<IVideoPicture> outFrame = 0;
  RefPointer<IVideoResampler> rgbResampler = 0;
  RefPointer<IVideoResampler> yuvResampler = 0;

  int maxFrames = 15*10;

  RefPointer<IRational> num(0);
  RefPointer<IStreamCoder> ic(0);
  RefPointer<IStreamCoder> oc(0);

  IPixelFormat::Type oFmt = IPixelFormat::BGR24;

  int numPackets = 0;
  int numFrames = 0;
  int numKeyFrames = 0;

  int retval = -1;

  h->setupReading(h->SAMPLE_FILE);

  RefPointer<IPacket> packet = IPacket::make();

  hw->setupWriting("VideoResamplerTest_5_output.flv", "flv", 0, "flv");

  RefPointer<IPacket> opacket = IPacket::make();
  VS_TUT_ENSURE("! opacket", opacket);

  {
    // now, let's set up video.
    ic = h->coders[h->first_input_video_stream];
    oc = hw->coders[hw->first_output_video_stream];

    oc->setBitRate(ic->getBitRate());
    oc->setPixelType(ic->getPixelType());
    oc->setHeight(ic->getHeight());
    oc->setWidth(ic->getWidth());
    num = ic->getFrameRate();
    oc->setFrameRate(num.value());
    num = ic->getTimeBase();
    oc->setTimeBase(num.value());
    oc->setNumPicturesInGroupOfPictures(
        ic->getNumPicturesInGroupOfPictures()
        );

    inFrame = IVideoPicture::make(
        ic->getPixelType(),
        ic->getWidth(),
        ic->getHeight());
    VS_TUT_ENSURE("got no frame", inFrame);

    rgbResampler = IVideoResampler::make(
        oc->getWidth(), oc->getHeight(), oFmt,
        ic->getWidth(), ic->getHeight(), ic->getPixelType());
    VS_TUT_ENSURE("got no resampler", rgbResampler);

    yuvResampler = IVideoResampler::make(
        oc->getWidth(), oc->getHeight(), oc->getPixelType(),
        oc->getWidth(), oc->getHeight(), oFmt);
    VS_TUT_ENSURE("got no resampler", yuvResampler);

    // tmp frame just converts to RGB
    tmpFrame = IVideoPicture::make(
        oFmt,
        ic->getWidth(),
        ic->getHeight());
    VS_TUT_ENSURE("got no frame", tmpFrame);

    // final frame resizes and converts back to original Pixel Type
    outFrame = IVideoPicture::make(
        oc->getPixelType(),
        oc->getWidth(),
        oc->getHeight());
    VS_TUT_ENSURE("got no frame", outFrame);

    retval = ic->open();
    VS_TUT_ENSURE("Could not open input coder", retval >= 0);
    retval = oc->open();
    VS_TUT_ENSURE("Could not open output coder", retval >= 0);
  }

  // write header
  retval = hw->container->writeHeader();
  VS_TUT_ENSURE("could not write header", retval >= 0);

  while (h->container->readNextPacket(packet.value()) == 0
      && numFrames < maxFrames)
  {
    if (packet->getStreamIndex() == h->first_input_video_stream)
    {
      int offset = 0;

      ic = h->coders[packet->getStreamIndex()];
      oc = hw->coders[packet->getStreamIndex()];

      numPackets++;

      while (offset < packet->getSize())
      {

        retval = ic->decodeVideo(
            inFrame.value(),
            packet.value(),
            offset);
        VS_TUT_ENSURE("could not decode any video", retval>0);
        offset += retval;
        if (inFrame->isComplete())
        {
          numFrames++;
          if (inFrame->isKeyFrame())
            numKeyFrames++;

          // Convert into RGB.
          retval = rgbResampler->resample(tmpFrame.value(), inFrame.value());
          VS_TUT_ENSURE("Could not convert to RGB", retval >= 0);
          VS_TUT_ENSURE("not complete", tmpFrame->isComplete());

          // Convert back to YUV
          retval = yuvResampler->resample(outFrame.value(), tmpFrame.value());
          VS_TUT_ENSURE("could not resize and convert pixels", retval >= 0);
          VS_TUT_ENSURE("not complete", outFrame->isComplete());

          VS_TUT_ENSURE_EQUALS("make sure PTS is copied through",
              inFrame->getPts(), outFrame->getPts());

          retval = oc->encodeVideo(
              opacket.value(),
              outFrame.value(),
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
  retval = hw->coders[hw->first_output_video_stream]->encodeVideo(opacket.value(),
      0, 0);
  VS_TUT_ENSURE("Could not encode any video", retval >= 0);
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

  VS_TUT_ENSURE("could not decode any video", numFrames >0);
  VS_TUT_ENSURE("could not find any key frames", numKeyFrames >0);

}

void
VideoResamplerTest :: testRescaleUpInYUV()
{ 
  h->setupReading(h->SAMPLE_FILE);
  hw->setupWriting("VideoResamplerTest_6_output.flv", "flv", "libmp3lame", "flv",
      hw->expected_pixel_format,
      hw->expected_width*2,
      hw->expected_height*2,
      false,
      hw->expected_sample_rate,
      hw->expected_channels,
      false);

  // Set up my renderer.
  mResampler = IVideoResampler::make(
      hw->expected_width*2, hw->expected_height*2, hw->expected_pixel_format,
      h->expected_width, h->expected_height, h->expected_pixel_format);
  VS_TUT_ENSURE("got no resampler", mResampler);

  // do three seconds of a blue background, but copying frames.
  Helper::decodeAndEncode(*h, *hw, 10, frameListener, this, 0, 0);

}

void
VideoResamplerTest :: testRescaleDownInYUV()
{  
  h->setupReading(h->SAMPLE_FILE);
  hw->setupWriting("VideoResamplerTest_7_output.flv", "flv", "libmp3lame", "flv",
      hw->expected_pixel_format,
      hw->expected_width/2,
      hw->expected_height/2,
      false,
      hw->expected_sample_rate,
      hw->expected_channels,
      false);

  // Set up my renderer.
  mResampler = IVideoResampler::make(
      hw->expected_width/2, hw->expected_height/2, hw->expected_pixel_format,
      h->expected_width, h->expected_height, h->expected_pixel_format);
  VS_TUT_ENSURE("got no resampler", mResampler);

  // do three seconds of a blue background, but copying frames.
  Helper::decodeAndEncode(*h, *hw, 10, frameListener, this, 0, 0);

}

