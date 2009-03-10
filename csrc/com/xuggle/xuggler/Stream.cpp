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
#include <cstring>
#include <stdexcept>

#include <com/xuggle/ferry/Logger.h>

#include <com/xuggle/xuggler/Global.h>
#include <com/xuggle/xuggler/Stream.h>
#include <com/xuggle/xuggler/Rational.h>
#include <com/xuggle/xuggler/StreamCoder.h>
#include <com/xuggle/xuggler/Container.h>

VS_LOG_SETUP(VS_CPP_PACKAGE);

namespace com { namespace xuggle { namespace xuggler
{

  Stream :: Stream()
  {
    mStream = 0;
    mDirection = INBOUND;
    mCoder = 0;
    mContainer = 0;
    memset(mLanguage, 0, sizeof(mLanguage));
  }

  Stream :: ~Stream()
  {
    reset();
  }

  void
  Stream :: reset()
  {
    VS_ASSERT("should be a valid object", getCurrentRefCount() >= 1);
    if (mStream && mDirection == OUTBOUND)
    {
      // when outbound we manage our AVStream context;
      // when inbound, FFMPEG cleans up after itself.
      av_free(mStream);
    }
    mStream = 0;
    VS_REF_RELEASE(mCoder);
    
    // We don't keep a reference to the container to avoid a ref-count loop
    // and so we don't release.
    mContainer = 0;
  }
  Stream*
  Stream :: make(Container *container, AVStream * aStream, Direction direction)
  {
    // note: make will acquire this for us.
    Stream *newStream = 0;
    if (aStream) {
      newStream = make();
      if (newStream)
      {
        newStream->mStream = aStream;
        newStream->mDirection = direction;

        newStream->mCoder = StreamCoder::make(
              direction == INBOUND?IStreamCoder::DECODING :
              IStreamCoder::ENCODING,
              aStream->codec,
              newStream);
        VS_ASSERT(newStream->mCoder, "Could not allocate a coder!");
        newStream->mContainer = container;
      }
    }
    return newStream;
  }

  int
  Stream :: getIndex()
  {
    return (mStream ? mStream->index : -1);
  }

  int
  Stream :: getId()
  {
    return (mStream ? mStream->id : -1);
  }

  IStreamCoder *
  Stream :: getStreamCoder()
  {
    StreamCoder *retval = 0;

    // acquire a reference for the caller
    retval = mCoder;
    VS_REF_ACQUIRE(retval);

    return retval;
  }

  IRational *
  Stream :: getFrameRate()
  {
    IRational * result = 0;
    if (mStream)
    {
      result = Rational::make(&mStream->r_frame_rate);
    }
    return result;
  }

  IRational *
  Stream :: getTimeBase()
  {
    IRational * result = 0;
    if (mStream)
    {
      result = Rational::make(&mStream->time_base);
    }
    return result;
  }
  void
  Stream :: setTimeBase(IRational* src)
  {
    if (mStream && src)
    {
      mStream->time_base.den = src->getDenominator();
      mStream->time_base.num = src->getNumerator();
    }
  }
  void
  Stream :: setFrameRate(IRational* src)
  {
    if (mStream && src)
    {
      mStream->r_frame_rate.den = src->getDenominator();
      mStream->r_frame_rate.num = src->getNumerator();
    }
  }

  int64_t
  Stream :: getStartTime()
  {
    return (mStream ? mStream->start_time : Global::NO_PTS);
  }

  int64_t
  Stream :: getDuration()
  {
    return (mStream ? mStream->duration : Global::NO_PTS);
  }

  int64_t
  Stream :: getCurrentDts()
  {
    return (mStream ? mStream->cur_dts : Global::NO_PTS);
  }

  int64_t
  Stream :: getNumFrames()
  {
    return (mStream ? mStream->nb_frames : 0);
  }
  int
  Stream :: getNumIndexEntries()
  {
    return (mStream ? mStream->nb_index_entries : 0);
  }
  int
  Stream :: containerClosed(Container *)
  {
    // let the coder know we're closed.
    if (mCoder)
      mCoder->streamClosed(this);
    reset();
    return 0;
  }

  int32_t
  Stream :: acquire()
  {
    int retval = 0;
    retval = RefCounted::acquire();
    VS_LOG_TRACE("Acquired %p: %d", this, retval);
    return retval;
  }

  int32_t
  Stream :: release()
  {
    int retval = 0;
    retval = RefCounted::release();
    VS_LOG_TRACE("Released %p: %d", this, retval);
    return retval;
  }

  IRational*
  Stream :: getSampleAspectRatio()
  {
    IRational* retval = 0;
    if (mStream)
    {
      retval = IRational::make(
          mStream->sample_aspect_ratio.num,
          mStream->sample_aspect_ratio.den);
    }
    return retval;
  }

  void
  Stream :: setSampleAspectRatio(IRational* aNewValue)
  {
    if (aNewValue && mStream)
    {
      mStream->sample_aspect_ratio.num =
        aNewValue->getNumerator();
      mStream->sample_aspect_ratio.den =
        aNewValue->getDenominator();
    }
    return;
  }

  const char*
  Stream :: getLanguage()
  {
    const char*retval = 0;

    if (mStream && mStream->language && *mStream->language)
    {
      int i = 0;
      for(; i < 4; i++)
      {
        if (!mStream->language[i])
          break;
        mLanguage[i]=mStream->language[i];
      }
      mLanguage[i]=0;
      retval = mLanguage;
    }

    return retval;
  }

  void
  Stream :: setLanguage(const char* aNewValue)
  {
    if (mStream)
    {
      if (aNewValue)
      {
        for(int i = 0; i < 4; i++)
        {
          mStream->language[i]=aNewValue[i];
          if(!aNewValue[i])
            break;
        }
      }
      else
      {
        mStream->language[0] = 0;
      }
    }
    return;
  }

  IContainer*
  Stream :: getContainer()
  {
    // add ref for caller
    VS_REF_ACQUIRE(mContainer);
    return mContainer;
  }

  int32_t
  Stream :: setStreamCoder(IStreamCoder *aCoder)
  {
    int32_t retval = -1;
    try
    {
      if (mCoder && mCoder->isOpen())
        throw std::runtime_error("cannot call setStreamCoder when current coder is open");
      
      if (!aCoder)
        throw std::runtime_error("cannot set to a null stream coder");

      StreamCoder *coder = dynamic_cast<StreamCoder*>(aCoder);
      if (!coder)
        throw std::runtime_error("IStreamCoder is not of expected underlying C++ type");

      // Close the old stream coder
      if (mCoder)
      {
        mCoder->streamClosed(this);
      }

      if (coder->setStream(this) < 0)
        throw std::runtime_error("IStreamCoder doesn't like this stream");

      VS_REF_RELEASE(mCoder);
      mCoder = coder;
      VS_REF_ACQUIRE(mCoder);
      retval = 0;
    }
    catch (std::exception & e)
    {
      VS_LOG_ERROR("Error: %s", e.what());
      retval = -1;
    }
    return retval;
  }
}}}
