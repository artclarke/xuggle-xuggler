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
#ifndef CONTAINER_H_
#define CONTAINER_H_

#include <com/xuggle/ferry/RefCounted.h>
#include <com/xuggle/ferry/RefPointer.h>
#include <com/xuggle/xuggler/IContainer.h>
#include <com/xuggle/xuggler/FfmpegIncludes.h>
#include <com/xuggle/xuggler/Stream.h>
#include <com/xuggle/xuggler/StreamCoder.h>

#include <vector>
#include <list>

namespace com { namespace xuggle { namespace xuggler
{
  class Container : public IContainer
  {
    VS_JNIUTILS_REFCOUNTED_OBJECT(Container)
  public:
    virtual int32_t setInputBufferLength(uint32_t size);
    virtual uint32_t getInputBufferLength();
    virtual bool isOpened();
    virtual bool isHeaderWritten();

    virtual int32_t open(const char *url, Type type,
        IContainerFormat* pContainerFormat);
    virtual int32_t open(const char *url, Type type,
        IContainerFormat* pContainerFormat, bool, bool);
    virtual IContainerFormat *getContainerFormat();

    virtual Type getType();
    virtual int32_t close();
    virtual int32_t getNumStreams();

    virtual IStream* getStream(uint32_t position);

    virtual IStream* addNewStream(int32_t id);

    virtual int32_t readNextPacket(IPacket *packet);
    virtual int32_t writePacket(IPacket *packet, bool forceInterleave);
    virtual int32_t writePacket(IPacket *packet);

    virtual int32_t writeHeader();
    virtual int32_t writeTrailer();
    AVFormatContext *getFormatContext();

    /*
     * Added as of 1.17
     */
    virtual int32_t queryStreamMetaData();
    virtual int32_t seekKeyFrame(int streamIndex, int64_t timestamp, int32_t flags);
    virtual int64_t getDuration();
    virtual int64_t getStartTime();
    virtual int64_t getFileSize();
    virtual int32_t getBitRate();

    /*
     * Added for 1.19
     */
    virtual int32_t getNumProperties();
    virtual IProperty* getPropertyMetaData(int32_t propertyNo);
    virtual IProperty* getPropertyMetaData(const char *name);

    virtual int32_t setProperty(const char* name, const char* value);
    virtual int32_t setProperty(const char* name, double value);
    virtual int32_t setProperty(const char* name, int64_t value);
    virtual int32_t setProperty(const char* name, bool value);
    virtual int32_t setProperty(const char* name, IRational *value);
    
    virtual char * getPropertyAsString(const char* name);
    virtual double getPropertyAsDouble(const char* name);
    virtual int64_t getPropertyAsLong(const char* name);
    virtual  IRational *getPropertyAsRational(const char* name);
    virtual bool getPropertyAsBoolean(const char* name);
    
    virtual int32_t getFlags();
    virtual void setFlags(int32_t newFlags);
    virtual bool getFlag(Flags flag);
    virtual void setFlag(Flags flag, bool value);
    
    virtual const char * getURL();
    virtual int32_t flushPackets();

    virtual IStream* addCopiedStream(int32_t id, IStreamCoder *sourceCoder);

  protected:
    virtual ~Container();
    Container();

  private:
    int32_t openInputURL(const char*url, IContainerFormat*, bool, bool);
    int32_t openOutputURL(const char*url, IContainerFormat*);
    int32_t setupAllInputStreams();
    IStream *addStream(int id, IStreamCoder *sourceCoder);
    // This is the object we wrap
    AVFormatContext *mFormatContext;
    void reset();
    // We do pointer to RefPointers to avoid too many
    // acquire() / release() cycles as the vector manages
    // itself.
    std::vector<
      com::xuggle::ferry::RefPointer<Stream>*
      > mStreams;
    uint32_t mNumStreams;
    bool mIsOpened;
    bool mNeedTrailerWrite;
    std::list<
      com::xuggle::ferry::RefPointer<IStreamCoder>
      > mOpenCoders;

    bool mIsMetaDataQueried;
    uint32_t mInputBufferLength;
  };
}}}

#endif /*CONTAINER_H_*/
