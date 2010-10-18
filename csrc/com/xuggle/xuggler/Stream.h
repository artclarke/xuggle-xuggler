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

#ifndef STREAM_H_
#define STREAM_H_

#include <com/xuggle/ferry/RefPointer.h>
#include <com/xuggle/xuggler/IStream.h>
#include <com/xuggle/xuggler/FfmpegIncludes.h>
#include <com/xuggle/xuggler/IRational.h>
#include <com/xuggle/xuggler/IMetaData.h>

namespace com { namespace xuggle { namespace xuggler
{

  class StreamCoder;
  class Container;
  class IPacket;

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
    static Stream * make(Container* container, AVStream *, Direction direction);

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
    virtual IStream::ParseType getParseType();
    virtual void setParseType(ParseType type);

    virtual int32_t setStreamCoder(IStreamCoder *coder);
    
    virtual AVStream* getAVStream() { return mStream; }
    
    virtual IMetaData* getMetaData();
    virtual void setMetaData(IMetaData* metaData);

    virtual int32_t stampOutputPacket(IPacket* packet);
    virtual int32_t setStreamCoder(IStreamCoder *newCoder, bool assumeOnlyStream);
    virtual IIndexEntry* findTimeStampEntryInIndex(
        int64_t wantedTimeStamp, int32_t flags);
    virtual int32_t findTimeStampPositionInIndex(
        int64_t wantedTimeStamp, int32_t flags);
    virtual IIndexEntry* getIndexEntry(int32_t position);
    virtual int32_t addIndexEntry(IIndexEntry* entry);

  protected:
    Stream();
    virtual ~Stream();

  private:
    void reset();
    AVStream *mStream;
    Direction mDirection;
    StreamCoder* mCoder;
    Container* mContainer;
    com::xuggle::ferry::RefPointer<IMetaData> mMetaData;
    
    int64_t mLastDts;
  };

}}}

#endif /*STREAM_H_*/
