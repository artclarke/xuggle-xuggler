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
#ifndef VIDEORESAMPLER_H_
#define VIDEORESAMPLER_H_

#include <com/xuggle/xuggler/IVideoResampler.h>


/*
 * Do not include anything from the swscale library here.
 * 
 * We're just keeping an opaque pointer here.
 */
struct SwsContext;

namespace com { namespace xuggle { namespace xuggler
  {

  class VideoResampler : public IVideoResampler
  {
  private:
    VS_JNIUTILS_REFCOUNTED_OBJECT_PRIVATE_MAKE(VideoResampler)
  public:
    virtual int32_t getInputWidth();
    virtual int32_t getInputHeight();
    virtual IPixelFormat::Type getInputPixelFormat();
    
    virtual int32_t getOutputWidth();
    virtual int32_t getOutputHeight();
    virtual IPixelFormat::Type getOutputPixelFormat();
    
    virtual int32_t resample(IVideoPicture *pOutFrame, IVideoPicture *pInFrame);
    
    static VideoResampler* make(
        int32_t outputWidth, int32_t outputHeight,
        IPixelFormat::Type outputFmt,
        int32_t inputWidth, int32_t inputHeight,
        IPixelFormat::Type inputFmt);
    
  protected:
    VideoResampler();
    virtual ~VideoResampler();
  private:
    int32_t mIHeight;
    int32_t mIWidth;
    int32_t mOHeight;
    int32_t mOWidth;
    IPixelFormat::Type mIPixelFmt;
    IPixelFormat::Type mOPixelFmt;
    
    SwsContext* mContext;
  };

  }}}

#endif /*VIDEORESAMPLER_H_*/
