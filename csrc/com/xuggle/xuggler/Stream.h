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
#ifndef STREAM_H_
#define STREAM_H_

#include <com/xuggle/xuggler/IStream.h>
#include <com/xuggle/xuggler/FfmpegIncludes.h>
#include <com/xuggle/xuggler/IRational.h>

namespace com { namespace xuggle { namespace xuggler
{

  class StreamCoder;
  class Container;

  class Stream : public IStream
  {
    VS_JNIUTILS_REFCOUNTED_OBJECT_PRIVATE_MAKE(Stream)
  public:

    // IStream
    virtual Direction getDirection() { return mDirection; }
    virtual int getIndex();
    virtual int getId();
    virtual IStreamCoder * getStreamCoder();
    virtual IRational * getFrameRate();
    virtual IRational * getTimeBase();
    virtual int64_t getStartTime();
    virtual int64_t getDuration();
    virtual int64_t getCurrentDts();
    virtual int getNumIndexEntries();
    virtual int64_t getNumFrames();

    // Not for calling from Java
    static Stream * make(Container* container, AVStream *, Direction direction,
        IStreamCoder *sourceCoder);

    // The StreamCoder will call this if it needs to
    virtual void setTimeBase(IRational *);
    virtual void setFrameRate(IRational *);

    // Called by the managing container when it is closed
    // at this point this stream is no longer valid.
    virtual int containerClosed(Container* container);

    virtual int32_t acquire();
    virtual int32_t release();

    virtual IRational* getSampleAspectRatio();
    virtual void setSampleAspectRatio(IRational* newRatio);
    virtual const char* getLanguage();
    virtual void setLanguage(const char* language);

    virtual IContainer* getContainer();
    
  protected:
    Stream();
    virtual ~Stream();

  private:
    void reset();
    AVStream *mStream;
    Direction mDirection;
    StreamCoder* mCoder;
    Container* mContainer;
    /**
     * We mirror the language setting that FFMPEG
     * has, but add one character so we can insert
     * a null if needed.
     */
    char mLanguage[5];
  };

}}}

#endif /*STREAM_H_*/
