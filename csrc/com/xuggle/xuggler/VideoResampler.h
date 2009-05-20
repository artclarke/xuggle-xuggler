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
