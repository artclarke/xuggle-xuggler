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
#ifndef ICONTAINERFORMAT_H_
#define ICONTAINERFORMAT_H_

#include <com/xuggle/ferry/RefCounted.h>
#include <com/xuggle/xuggler/Xuggler.h>

namespace com { namespace xuggle { namespace xuggler
{
  /**
   * Specifies format information than can be used to configure
   * an {@link IContainer} for input or output.
   * <p>
   * Often times XUGGLER can guess the correct formats to put into
   * a given IContainer object, but sometimes it needs help.  You
   * can allocate an IContainerFormat object and specify information about
   * input or output containers, and then pass this to IContainer.open(...)
   * to help us guess.
   * </p>
   */
  class VS_API_XUGGLER IContainerFormat : public com::xuggle::ferry::RefCounted
  {
  public:
    /**
     * Sets the input format for this container.
     * 
     * @param shortName The short name for this container (using FFMPEG's
     *   short name).
     * @return >= 0 on success.  < 0 if shortName cannot be found.
     */
    virtual int32_t setInputFormat(const char *shortName)=0;

    /**
     * Sets the output format for this container.
     * 
     * We'll look at the shortName, url and mimeType and try to guess
     * a valid output container format.
     * 
     * @param shortName The short name for this container (using FFMPEG's
     *   short name).
     * @param url The URL for this container.
     * @param mimeType The mime type for this container.
     * @return >= 0 on success.  < 0 if we cannot find a good container.
     */
    virtual int32_t setOutputFormat(const char*shortName,
        const char*url,
        const char* mimeType)=0;

    /**
     * @return The short name for the input format, or 0 (null) if none.
     */
    virtual const char* getInputFormatShortName()=0;

    /**
     * @return The long name for the input format, or 0 (null) if none.
     */
    virtual const char* getInputFormatLongName()=0;

    /**
     * @return The short name for the output format, or 0 (null) if none.
     */
    virtual const char* getOutputFormatShortName()=0;

    /**
     * @return The long name for the output format, or 0 (null) if none.
     */
    virtual const char* getOutputFormatLongName()=0;
    
    /**
     * @return The mime type for the output format, or 0 (null) if none.
     */
    virtual const char* getOutputFormatMimeType()=0;
    
    /**
     * Create a new IContainerFormat object.
     * 
     * @return a new object, or null on error.
     */
    static IContainerFormat* make();
  protected:
    virtual ~IContainerFormat()=0;
    
  };
}}}
#endif /*ICONTAINERFORMAT_H_*/
