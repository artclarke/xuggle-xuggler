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

#ifndef CONTAINERFORMAT_H_
#define CONTAINERFORMAT_H_

#include <com/xuggle/xuggler/IContainerFormat.h>
#include <com/xuggle/xuggler/FfmpegIncludes.h>

namespace com { namespace xuggle { namespace xuggler
{

  class ContainerFormat : public IContainerFormat
  {
    VS_JNIUTILS_REFCOUNTED_OBJECT(ContainerFormat)
  public:

    // IContainerFormat implementation
    virtual int32_t setInputFormat(const char *shortName);
    virtual int32_t setOutputFormat(const char*shortName,
        const char*url,
        const char* mimeType);
    
    virtual const char* getInputFormatShortName();
    virtual const char* getInputFormatLongName();

    virtual const char* getOutputFormatShortName();
    virtual const char* getOutputFormatLongName();
    virtual const char* getOutputFormatMimeType();

    
    AVInputFormat* getInputFormat();
    AVOutputFormat* getOutputFormat();
    void setInputFormat(AVInputFormat*);
    void setOutputFormat(AVOutputFormat*);
    
    virtual int32_t getInputFlags();
    virtual void setInputFlags(int32_t newFlags);
    virtual bool getInputFlag(Flags flag);
    virtual void setInputFlag(Flags flag, bool value);

    virtual int32_t getOutputFlags();
    virtual void setOutputFlags(int32_t newFlags);
    virtual bool getOutputFlag(Flags flag);
    virtual void setOutputFlag(Flags flag, bool value);


    virtual bool isOutput();
    virtual bool isInput();

    virtual const char *getOutputExtensions();
    virtual ICodec::ID getOutputDefaultAudioCodec();
    virtual ICodec::ID getOutputDefaultVideoCodec();
    virtual ICodec::ID getOutputDefaultSubtitleCodec();
    virtual int32_t getOutputNumCodecsSupported();
    virtual ICodec::ID getOutputCodecID(int32_t index);
    virtual int32_t getOutputCodecTag(int32_t index);
    virtual int32_t getOutputCodecTag(ICodec::ID id);
    virtual bool isCodecSupportedForOutput(ICodec::ID id);
  protected:
    ContainerFormat();
    virtual ~ContainerFormat();

  private:
    AVInputFormat *mInputFormat;
    AVOutputFormat *mOutputFormat;
  };

}}}

#endif /*CONTAINERFORMAT_H_*/
