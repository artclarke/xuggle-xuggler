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
#ifndef PACKET_H_
#define PACKET_H_

#include <com/xuggle/ferry/RefPointer.h>
#include <com/xuggle/xuggler/IPacket.h>
#include <com/xuggle/xuggler/FfmpegIncludes.h>
#include <com/xuggle/ferry/IBuffer.h>

namespace com { namespace xuggle { namespace xuggler
{
  class Packet : public IPacket
  {
  public:
    /* The default make() method doesn't add a payload */
    VS_JNIUTILS_REFCOUNTED_OBJECT(Packet);
    /* This make allocates a default payload of size payloadSize */
    static Packet* make(int32_t payloadSize);
    /* This make creates a packet that just wraps a given IBuffer */
    static Packet* make(com::xuggle::ferry::IBuffer* buffer);
  public:
    // IMediaData
    virtual int64_t getTimeStamp() { return getDts(); }
    virtual void setTimeStamp(int64_t aTimeStamp) { setDts(aTimeStamp); }
    virtual bool isKey() { return isKeyPacket(); }
    virtual IRational* getTimeBase() { return mTimeBase.get(); }
    virtual void setTimeBase(IRational *aBase) { mTimeBase.reset(aBase, true); }
    
    // IPacket
    virtual void reset();

    virtual int64_t getPts();
    virtual int64_t getDts();
    virtual int32_t getSize();
    virtual int32_t getMaxSize();

    virtual int32_t getStreamIndex();
    virtual int32_t getFlags();
    virtual bool isKeyPacket();
    virtual int64_t getDuration();
    virtual int64_t getPosition();
    virtual com::xuggle::ferry::IBuffer* getData();
    virtual int32_t allocateNewPayload(int32_t payloadSize);
    virtual bool isComplete();
    
    virtual void setKeyPacket(bool keyPacket);
    virtual void setFlags(int32_t flags);
    virtual void setPts(int64_t pts);
    virtual void setDts(int64_t dts);
    virtual void setComplete(bool complete, int32_t size);
    virtual void setStreamIndex(int32_t streamIndex);
    
    AVPacket *getAVPacket() { return mPacket; }
    /*
     * Unfortunately people can do a getAVPacket() and have
     * FFMPEG update the buffers without us knowing.  When
     * that happens, we need them to tell us so we can update
     * our own buffer state.
     */
    void wrapAVPacket(AVPacket* pkt);
    void wrapBuffer(com::xuggle::ferry::IBuffer *buffer);
    // Used by the IBuffer to free buffers.
    static void freeAVBuffer(void *buf, void *closure);

  protected:
    Packet();
    virtual ~Packet();
  private:
    AVPacket* mPacket;
    com::xuggle::ferry::RefPointer<com::xuggle::ferry::IBuffer> mBuffer;
    com::xuggle::ferry::RefPointer<IRational> mTimeBase;
    bool mIsComplete;
  };

}}}

#endif /*PACKET_H_*/
