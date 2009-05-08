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
#include <stdexcept>

#include <com/xuggle/ferry/Logger.h>

#include <com/xuggle/xuggler/VideoResampler.h>
#include <com/xuggle/xuggler/VideoPicture.h>
#include <com/xuggle/xuggler/Rational.h>
#include <com/xuggle/xuggler/Property.h>

// This is the only place we include this to limit
// how much it pollutes our code, and make it
// easier to yank later.
extern "C" {
#include <libswscale/swscale.h>
}

VS_LOG_SETUP(VS_CPP_PACKAGE);

using namespace com::xuggle::ferry;

namespace com { namespace xuggle { namespace xuggler
  {

  VideoResampler :: VideoResampler()
  {
    mIHeight = 0;
    mIWidth = 0;
    mOHeight = 0;
    mOWidth = 0;
    mIPixelFmt = IPixelFormat::NONE;
    mOPixelFmt = IPixelFormat::NONE;
    mContext = 0;
  }

  VideoResampler :: ~VideoResampler()
  {
    if (mContext)
      sws_freeContext(mContext);
    mContext = 0;
  }
  
  int32_t
  VideoResampler :: getInputHeight()
  {
    return mIHeight;
  }

  int32_t
  VideoResampler :: getInputWidth()
  {
    return mIWidth;
  }
  
  int32_t
  VideoResampler :: getOutputHeight()
  {
    return mOHeight;
  }
  
  int32_t
  VideoResampler :: getOutputWidth()
  {
    return mOWidth;
  }
  
  IPixelFormat::Type
  VideoResampler :: getInputPixelFormat()
  {
    return mIPixelFmt;
  }

  IPixelFormat::Type
  VideoResampler :: getOutputPixelFormat()
  {
    return mOPixelFmt;
  }
  
  int32_t
  VideoResampler :: resample(IVideoPicture* pOutFrame, IVideoPicture* pInFrame)
  {
    int32_t retval = -1;
    VideoPicture* outFrame = dynamic_cast<VideoPicture*>(pOutFrame);
    VideoPicture* inFrame  = dynamic_cast<VideoPicture*>(pInFrame);
    try
    {
      if (!outFrame)
        throw std::invalid_argument("invalid output frame");
      if (outFrame->getHeight() != mOHeight)
        throw std::runtime_error("output frame height does not match expected value");
      if (outFrame->getWidth() != mOWidth)
        throw std::runtime_error("output frame width does not match expected value");
      if (outFrame->getPixelType() != mOPixelFmt)
        throw std::runtime_error("output frame pixel format does not match expected value");

      
      if (!inFrame)
        throw std::invalid_argument("invalid input frame");
      
      if (inFrame->getHeight() != mIHeight)
        throw std::runtime_error("input frame height does not match expected value");
      if (inFrame->getWidth() != mIWidth)
        throw std::runtime_error("input frame width does not match expected value");
      if (inFrame->getPixelType() != mIPixelFmt)
        throw std::runtime_error("input frame pixel format does not match expected value");
      if (!inFrame->isComplete())
        throw std::runtime_error("incoming frame doesn't have complete data");

      // Allocate our output frame.
      outFrame->setComplete(false, mOPixelFmt, mOWidth, mOHeight,
          inFrame->getPts());
      AVFrame *outAVFrame = outFrame->getAVFrame();

      // Get our input frame; since it's complete() we can pass anything
      // for the argument
      AVFrame *inAVFrame = inFrame->getAVFrame(); 

      /*
      VS_LOG_DEBUG("%dx%d(%d:%d)=>%dx%d(%d:%d)",
          inFrame->getWidth(), inFrame->getHeight(), inFrame->getPixelType(), inAVFrame->linesize[0],
          outFrame->getWidth(), outFrame->getHeight(), outFrame->getPixelType(), outAVFrame->linesize[0]);

      memset(outAVFrame->data[0], 64*3-1, outAVFrame->linesize[0]);
      */
      // And do the conversion
      retval = sws_scale(mContext, inAVFrame->data, inAVFrame->linesize, 0,
          mIHeight, outAVFrame->data, outAVFrame->linesize);
      
      // and go home happy.
      {
        outFrame->setQuality(inFrame->getQuality());
        outFrame->setComplete(retval >= 0,mOPixelFmt, mOWidth, mOHeight,
            inFrame->getPts());
      }
    }
    catch (std::bad_alloc& e)
    {
      throw e;
    }
    catch (std::exception& e)
    {
      VS_LOG_DEBUG("error: %s", e.what());
      retval = -1;
    }
    return retval;
  }
  
  VideoResampler*
  VideoResampler :: make(
          int32_t outputWidth, int32_t outputHeight,
          IPixelFormat::Type outputFmt,
          int32_t inputWidth, int32_t inputHeight,
          IPixelFormat::Type inputFmt)
  {
    VideoResampler* retval = 0;
    try
    {
      if (outputWidth <= 0)
        throw std::invalid_argument("invalid output width");
      if (outputHeight <= 0)
        throw std::invalid_argument("invalid output height");
      if (outputFmt == IPixelFormat::NONE)
        throw std::invalid_argument("cannot set output pixel format to none");
      if (inputWidth <= 0)
        throw std::invalid_argument("invalid input width");
      if (inputHeight <= 0)
        throw std::invalid_argument("invalid input height");
      if (inputFmt == IPixelFormat::NONE)
        throw std::invalid_argument("cannot set input pixel format to none");
      
      retval = VideoResampler::make();
      if (retval)
      {
        retval->mOHeight = outputHeight;
        retval->mOWidth = outputWidth;
        retval->mOPixelFmt = outputFmt;

        retval->mIHeight = inputHeight;
        retval->mIWidth = inputWidth;
        retval->mIPixelFmt = inputFmt;
        
        int32_t flags = 0;
        if (inputWidth < outputWidth)
          // We're upscaling
          flags = SWS_BICUBIC;
        else
          // We're downscaling
          flags = SWS_AREA;
        
        retval->mContext = sws_getContext(
            retval->mIWidth, // src width
            retval->mIHeight, // src height
            (PixelFormat)retval->mIPixelFmt, // src pixel type
            retval->mOWidth, // dst width
            retval->mOHeight, // dst height
            (PixelFormat)retval->mOPixelFmt, // dst pixel type
            flags, // Flags
            0, // Source Filter
            0, // Destination Filter
            0 // An array of parameters for filters
            );
        if (!retval->mContext)
          throw std::runtime_error("could not allocate a image rescaler");
      }
    }
    catch (std::bad_alloc& e)
    {
      VS_REF_RELEASE(retval);
    }
    catch (std::exception& e)
    {
      VS_LOG_DEBUG("error: %s", e.what());
      VS_REF_RELEASE(retval);
    }

    return retval;
  }

  int32_t
  VideoResampler :: getNumProperties()
  {
    return Property::getNumProperties(mContext);
  }

  IProperty*
  VideoResampler :: getPropertyMetaData(int32_t propertyNo)
  {
    return Property::getPropertyMetaData(mContext, propertyNo);
  }

  IProperty*
  VideoResampler :: getPropertyMetaData(const char *name)
  {
    return Property::getPropertyMetaData(mContext, name);
  }

  int32_t
  VideoResampler :: setProperty(const char* aName, const char *aValue)
  {
    return Property::setProperty(mContext, aName, aValue);
  }

  int32_t
  VideoResampler :: setProperty(const char* aName, double aValue)
  {
    return Property::setProperty(mContext, aName, aValue);
  }

  int32_t
  VideoResampler :: setProperty(const char* aName, int64_t aValue)
  {
    return Property::setProperty(mContext, aName, aValue);
  }

  int32_t
  VideoResampler :: setProperty(const char* aName, bool aValue)
  {
    return Property::setProperty(mContext, aName, aValue);
  }


  int32_t
  VideoResampler :: setProperty(const char* aName, IRational *aValue)
  {
    return Property::setProperty(mContext, aName, aValue);
  }


  char*
  VideoResampler :: getPropertyAsString(const char *aName)
  {
    return Property::getPropertyAsString(mContext, aName);
  }

  double
  VideoResampler :: getPropertyAsDouble(const char *aName)
  {
    return Property::getPropertyAsDouble(mContext, aName);
  }

  int64_t
  VideoResampler :: getPropertyAsLong(const char *aName)
  {
    return Property::getPropertyAsLong(mContext, aName);
  }

  IRational*
  VideoResampler :: getPropertyAsRational(const char *aName)
  {
    return Property::getPropertyAsRational(mContext, aName);
  }

  bool
  VideoResampler :: getPropertyAsBoolean(const char *aName)
  {
    return Property::getPropertyAsBoolean(mContext, aName);
  }

  }}}
