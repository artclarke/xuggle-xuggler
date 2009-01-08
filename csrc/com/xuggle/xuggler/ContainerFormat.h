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
  protected:
    ContainerFormat();
    virtual ~ContainerFormat();

  private:
    AVInputFormat *mInputFormat;
    AVOutputFormat *mOutputFormat;
  };

}}}

#endif /*CONTAINERFORMAT_H_*/
