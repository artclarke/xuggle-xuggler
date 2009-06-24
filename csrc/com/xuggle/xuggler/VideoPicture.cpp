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

#include <stdexcept>
// for memset
#include <cstring>
#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/ferry/RefPointer.h>
#include <com/xuggle/xuggler/Global.h>
#include "com/xuggle/xuggler/VideoPicture.h"

VS_LOG_SETUP(VS_CPP_PACKAGE);

namespace com { namespace xuggle { namespace xuggler
{

  VideoPicture :: VideoPicture()
  {
    mIsComplete = false;
    mFrame = avcodec_alloc_frame();
    if (!mFrame)
      throw std::bad_alloc();
    // Set the private data pointer to point to me.
    mFrame->opaque = this;
    mWidth = -1;
    mHeight = -1;
    mPixelFormat = IPixelFormat::NONE;
    mTimeBase = IRational::make(1, 1000000);
  }

  VideoPicture :: ~VideoPicture()
  {
    if (mFrame)
      av_free(mFrame);
    mFrame = 0;
  }

  VideoPicture*
  VideoPicture :: make(IPixelFormat::Type format, int width, int height)
  {
    VideoPicture * retval=0;
    try {
      retval = VideoPicture::make();
      retval->mPixelFormat = format;
      retval->mWidth = width;
      retval->mHeight = height;
      // default new frames to be key frames
      retval->setKeyFrame(true);
    }
    catch (std::bad_alloc &e)
    {
      VS_REF_RELEASE(retval);
      throw e;
    }
    catch (std::exception& e)
    {
      VS_LOG_DEBUG("error: %s", e.what());
      VS_REF_RELEASE(retval);
    }

    return retval;
  }

  bool
  VideoPicture :: copy(IVideoPicture * srcFrame)
  {
    bool result = false;
    try
    {
      if (!srcFrame)
        throw std::runtime_error("empty source frame to copy");

      if (!srcFrame->isComplete())
        throw std::runtime_error("source frame is not complete");

      VideoPicture* src = dynamic_cast<VideoPicture*>(srcFrame);
      if (!src)
        throw std::runtime_error("src frame is not of right subtype");

      // now copy the data
      allocInternalFrameBuffer();

      // get the raw buffers
      unsigned char* srcBuffer = (unsigned char*)src->mBuffer->getBytes(0, src->getSize());
      unsigned char* dstBuffer = (unsigned char*)mBuffer->getBytes(0, getSize());
      if (!srcBuffer || !dstBuffer)
        throw std::runtime_error("could not get buffer to copy");
      memcpy(dstBuffer, srcBuffer, getSize());

      this->setComplete(true,
          srcFrame->getPixelType(),
          srcFrame->getWidth(),
          srcFrame->getHeight(),
          srcFrame->getPts());
      result = true;
    }
    catch (std::exception & e)
    {
      VS_LOG_DEBUG("error: %s", e.what());
      result = false;
    }
    return result;
  }

  com::xuggle::ferry::IBuffer*
  VideoPicture :: getData()
  {
    com::xuggle::ferry::IBuffer *retval = 0;
    try {
      if (getSize() > 0) {
        if (!mBuffer || mBuffer->getBufferSize() < getSize())
        {
          allocInternalFrameBuffer();
        }
        retval = mBuffer.get();
        if (!retval) {
          throw std::bad_alloc();
        }
      }
    } catch (std::bad_alloc &e) {
      VS_REF_RELEASE(retval);
      throw e;
    } catch (std::exception & e)
    {
      VS_LOG_DEBUG("Error: %s", e.what());
      VS_REF_RELEASE(retval);
    }
    return retval;
  }

  void
  VideoPicture :: fillAVFrame(AVFrame *frame)
  {
    if (!mBuffer || mBuffer->getBufferSize() < getSize())
      allocInternalFrameBuffer();
    unsigned char* buffer = (unsigned char*)mBuffer->getBytes(0, getSize());
    *frame = *mFrame;
    avpicture_fill((AVPicture*)frame, buffer, mPixelFormat, mWidth, mHeight);
    frame->quality = getQuality();
    frame->type = FF_BUFFER_TYPE_USER;
  }

  void
  VideoPicture :: copyAVFrame(AVFrame* frame, int32_t width, int32_t height)
  {
    try
    {
      // Need to copy the contents of frame->data to our
      // internal buffer.
      VS_ASSERT(frame, "no frame?");
      VS_ASSERT(frame->data[0], "no data in frame");
      // resize the frame to the AVFrame
      mWidth = width;
      mHeight = height;

      int bufSize = getSize();
      if (bufSize <= 0)
        throw std::runtime_error("invalid size for frame");

      if (!mBuffer || mBuffer->getBufferSize() < bufSize)
        // reuse buffers if we can.
        allocInternalFrameBuffer();

      uint8_t* buffer = (uint8_t*)mBuffer->getBytes(0, bufSize);
      if (!buffer)
        throw std::runtime_error("really?  no buffer");

      if (frame->data[0])
      {
        // Make sure the frame isn't already using our buffer
        if(buffer != frame->data[0])
        {
          avpicture_fill((AVPicture*)mFrame, buffer, mPixelFormat, mWidth, mHeight);
          av_picture_copy((AVPicture*)mFrame, (AVPicture*)frame,
              (PixelFormat)mPixelFormat, mWidth, mHeight);
        }
        mFrame->key_frame = frame->key_frame;
      }
      else
      {
        throw std::runtime_error("no data in frame to copy");
      }
    }
    catch (std::exception & e)
    {
      VS_LOG_DEBUG("error: %s", e.what());
    }
  }

  AVFrame*
  VideoPicture :: getAVFrame()
  {
    if (!mBuffer || mBuffer->getBufferSize() < getSize())
    {
      // reuse buffers if we can.
      allocInternalFrameBuffer();
    }
    return mFrame;
  }
  
  int
  VideoPicture :: getDataLineSize(int lineNo)
  {
    int retval = -1;
    if (mFrame
        && lineNo >= 0
        && (unsigned int) lineNo < (sizeof(mFrame->linesize)/sizeof(mFrame->linesize[0])))
      retval = mFrame->linesize[lineNo];
    return retval;
  }

  bool
  VideoPicture :: isKeyFrame()
  {
    return (mFrame ? mFrame->key_frame : false);
  }

  void
  VideoPicture :: setKeyFrame(bool aIsKey)
  {
    if (mFrame)
      mFrame->key_frame = aIsKey;
  }
  
  int64_t
  VideoPicture :: getPts()
  {
    return (mFrame ? mFrame->pts : -1);
  }

  void
  VideoPicture :: setPts(int64_t value)
  {
    if (mFrame)
      mFrame->pts = value;
  }

  int
  VideoPicture :: getQuality()
  {
    return (mFrame ? mFrame->quality : FF_LAMBDA_MAX);
  }

  void
  VideoPicture :: setQuality(int newQuality)
  {
    if (newQuality < 0 || newQuality > FF_LAMBDA_MAX)
      newQuality = FF_LAMBDA_MAX;
    if (mFrame)
      mFrame->quality = newQuality;
  }

  void
  VideoPicture :: setComplete(
      bool aIsComplete,
      IPixelFormat::Type format,
      int width,
      int height,
      int64_t pts
  )
  {
    try {

      if (format != mPixelFormat)
        throw std::runtime_error("pixel formats don't match");
      if (width != -1 && mWidth != -1 && width != mWidth)
        throw std::runtime_error("width does not match");
      if (height != -1 && mHeight != -1 && height != mHeight)
        throw std::runtime_error("height does not match");
      if (!mFrame)
        throw std::runtime_error("no AVFrame allocated");

      mIsComplete = aIsComplete;

      if (mIsComplete)
      {
        setPts(pts);
      }
    }
    catch (std::exception& e)
    {
      VS_LOG_DEBUG("error: %s", e.what());
    }
  }

  int32_t
  VideoPicture :: getSize()
  {
    int retval = -1;
    if (mWidth > 0 && mHeight > 0)
      retval = avpicture_get_size((PixelFormat)mPixelFormat, mWidth, mHeight);
    return retval;
  }

  void
  VideoPicture :: allocInternalFrameBuffer()
  {
    int bufSize = getSize();
    if (bufSize <= 0)
      throw std::runtime_error("invalid size for frame");

    // Now, it turns out some accelerated assembly functions will
    // read at least a word past the end of an image buffer, so
    // we make space for that to happen.
    // I arbitrarily choose the sizeof a long-long (64 bit).
    int extraBytes=8;
    bufSize += extraBytes;

    // reuse buffers if we can.
    if (!mBuffer || mBuffer->getBufferSize() < bufSize)
    {
      // Make our copy buffer.
      mBuffer = com::xuggle::ferry::IBuffer::make(this, bufSize);
      if (!mBuffer) {
        throw std::bad_alloc();
      }

      // Now, to further work around issues, I added the extra 8-bytes,
      // and now I'm going to zero just those 8-bytes out.  I don't
      // zero-out the whole buffer because I want Valgrind to detect
      // if it's not written to first.  But I know this overrun
      // issue exists in the MMX conversions in SWScale for some libraries,
      // so I'm going to fake it out here.
      {
        unsigned char * buf =
          ((unsigned char*)mBuffer->getBytes(0, bufSize));

        memset(buf+bufSize-extraBytes, 0, extraBytes);
      }
    }
    uint8_t* buffer = (uint8_t*)mBuffer->getBytes(0, bufSize);
    if (!buffer)
      throw std::bad_alloc();

    int imageSize = avpicture_fill((AVPicture*)mFrame,
        buffer,
        mPixelFormat,
        mWidth,
        mHeight);
    if (imageSize != bufSize-extraBytes)
      throw std::runtime_error("could not fill picture");

    mFrame->type = FF_BUFFER_TYPE_USER;
    VS_ASSERT(mFrame->data[0] != 0, "Empty buffer");
  }

  IVideoPicture::PictType
  VideoPicture :: getPictureType()
  {
    IVideoPicture::PictType retval = IVideoPicture::DEFAULT_TYPE;
    if (mFrame)
      retval = (PictType) mFrame->pict_type;
    return retval;
  }
  
  void
  VideoPicture :: setPictureType(IVideoPicture::PictType type)
  {
    if (mFrame)
      mFrame->pict_type = (int) type;
  }
}}}
