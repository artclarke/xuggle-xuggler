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
#ifndef VIDEOPICTURE_H_
#define VIDEOPICTURE_H_

#include <com/xuggle/ferry/RefPointer.h>
#include <com/xuggle/xuggler/IVideoPicture.h>
#include <com/xuggle/xuggler/ICodec.h>
#include <com/xuggle/xuggler/FfmpegIncludes.h>
#include <com/xuggle/ferry/IBuffer.h>
#include <com/xuggle/xuggler/IRational.h>

namespace com { namespace xuggle { namespace xuggler
{

  class VideoPicture : public IVideoPicture
  {
    VS_JNIUTILS_REFCOUNTED_OBJECT_PRIVATE_MAKE(VideoPicture);
  public:
    // IMediaData
    virtual int64_t getTimeStamp() { return getPts(); }
    virtual void setTimeStamp(int64_t aTimeStamp) { setPts(aTimeStamp); }
    virtual bool isKey() { return isKeyFrame(); }
    virtual IRational* getTimeBase() { return mTimeBase.get(); }
    virtual void setTimeBase(IRational *aBase) { mTimeBase.reset(aBase, true); }

    // IVideoPicture Implementation
    virtual bool isKeyFrame();
    virtual void setKeyFrame(bool aIsKey);
    virtual bool isComplete() { return mIsComplete; }
    virtual int getWidth() { return mWidth; }
    virtual int getHeight() { return mHeight; }
    virtual IPixelFormat::Type getPixelType() { return mPixelFormat; }
    virtual int64_t getPts();
    virtual void setPts(int64_t);

    virtual int getQuality();
    virtual void setQuality(int newQuality);    
    virtual int32_t getSize();
    virtual com::xuggle::ferry::IBuffer* getData();
    virtual int getDataLineSize(int lineNo);
    virtual void setComplete(bool aIsComplete, IPixelFormat::Type format,
        int width, int height, int64_t pts);
    virtual bool copy(IVideoPicture* srcFrame);
    
    // Not for calling from Java
    /**
     * Called by the StreamCoder before it encodes a picture.
     * 
     * The VideoPicture fills in Ffmpeg's AVFrame structure with the
     * underlying data for the frame we're managing, but we
     * maintain memory management.
     */
    void fillAVFrame(AVFrame *frame);

    /**
     * Called by the StreamCoder once it's done decoding.
     * 
     * We copy data from the buffers that ffmpeg allocated into
     * our own buffers.
     * 
     * @param frame The AVFrame that ffmpeg filled in.
     * @param width The width of the AVFrame
     * @param height The height of the AVFrame
     * 
     */
    void copyAVFrame(AVFrame *frame, int32_t width, int32_t height);
    
    /**
     * Call to get the raw underlying AVFrame we manage; don't
     * pass this to ffmpeg directly as ffmpeg often does weird
     * stuff to these guys.
     *
     * Note: This method is exported out of the DLL because
     * the extras library uses it.
     */
    VS_API_XUGGLER AVFrame *getAVFrame();

    /**
     * The default factory for a frame.  We require callers to always
     * tell us the format, width and height of the image they want to 
     * store in this VideoPicture.
     * 
     * @param format The pixel format
     * @param width The expected width of this image.
     * @param height The expected height of this image.
     * 
     * @return A new frame that can store an image and associated decoding
     *   information.
     */
    static VideoPicture* make(IPixelFormat::Type format, int width, int height);
    
  protected:
    VideoPicture();
    virtual ~VideoPicture();
    
  private:
    void allocInternalFrameBuffer();
    
    // This is where frame information is kept
    // about a decoded frame.
    AVFrame * mFrame;
    bool mIsComplete;
    
    // Meta information about the AVFrame or
    // buffer
    IPixelFormat::Type mPixelFormat;
    int mWidth;
    int mHeight;
    
    com::xuggle::ferry::RefPointer<com::xuggle::ferry::IBuffer> mBuffer;
    com::xuggle::ferry::RefPointer<IRational> mTimeBase;
  };

}}}

#endif /*VIDEOPICTURE_H_*/
