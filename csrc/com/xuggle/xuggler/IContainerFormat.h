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

#ifndef ICONTAINERFORMAT_H_
#define ICONTAINERFORMAT_H_

#include <com/xuggle/ferry/RefCounted.h>
#include <com/xuggle/xuggler/Xuggler.h>
#include <com/xuggle/xuggler/ICodec.h>

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
     * Get the short name for the input format.
     * @return The short name for the input format, or null if none.
     */
    virtual const char* getInputFormatShortName()=0;

    /**
     * Get the long name for the input format.
     * @return The long name for the input format, or null if none.
     */
    virtual const char* getInputFormatLongName()=0;

    /**
     * Get the short name for the output format.
     * @return The short name for the output format, or null if none.
     */
    virtual const char* getOutputFormatShortName()=0;

    /**
     * Get the long name for the output format.
     * @return The long name for the output format, or null if none.
     */
    virtual const char* getOutputFormatLongName()=0;
    
    /**
     * Get the mime type for the output format.
     * @return The mime type for the output format, or null if none.
     */
    virtual const char* getOutputFormatMimeType()=0;
    
    /**
     * Create a new IContainerFormat object.
     * 
     * @return a new object, or null on error.
     */
    static IContainerFormat* make();
    
    /*
     * Added for 1.19
     */
    typedef enum {
      FLAG_NOFILE=0x0001,
      FLAG_NEEDNUMBER = 0x0002,
      FLAG_SHOW_IDS=0x0008,
      FLAG_RAWPICTURE=0x0020,
      FLAG_GLOBALHEADER=0x0040,
      FLAG_NOTIMESTAMPS=0x0080,
      FLAG_GENERIC_INDEX=0x0100,
      FLAG_TS_DISCONT=0x0200
    } Flags;
    
    /**
     * Get the input flags associated with this object.
     *
     * @return The (compacted) value of all flags set.
     */
    virtual int32_t getInputFlags()=0;

    /**
     * Set the input flags to use with this object.  All values
     * must be ORed (|) together.
     *
     * @see Flags
     *
     * @param newFlags The new set flags for this codec.
     */
    virtual void setInputFlags(int32_t newFlags) = 0;

    /**
     * Get the input setting for the specified flag
     *
     * @param flag The flag you want to find the setting for
     *
     * @return 0 for false; non-zero for true
     */
    virtual bool getInputFlag(Flags flag) = 0;

    /**
     * Set the input flag.
     *
     * @param flag The flag to set
     * @param value The value to set it to (true or false)
     *
     */
    virtual void setInputFlag(Flags flag, bool value) = 0;

    /**
     * Get the output flags associated with this object.
     *
     * @return The (compacted) value of all flags set.
     */
    virtual int32_t getOutputFlags()=0;

    /**
     * Set the output flags to use with this object.  All values
     * must be ORed (|) together.
     *
     * @see Flags
     *
     * @param newFlags The new set flags for this codec.
     */
    virtual void setOutputFlags(int32_t newFlags) = 0;

    /**
     * Get the output setting for the specified flag
     *
     * @param flag The flag you want to find the setting for
     *
     * @return 0 for false; non-zero for true
     */
    virtual bool getOutputFlag(Flags flag) = 0;

    /**
     * Set the output flag.
     *
     * @param flag The flag to set
     * @param value The value to set it to (true or false)
     *
     */
    virtual void setOutputFlag(Flags flag, bool value) = 0;

    /**
     * Is this an output container format?
     * 
     * @return true if output; false it not
     */
    virtual bool isOutput()=0;
    
    /**
     * Is this an input container format?
     * 
     * @return true if input; false it not
     */
    virtual bool isInput()=0;
    
    /*
     * Added for 2.1
     */

    /**
     * Get the filename extensions that this output format prefers
     * (most common first).
     * 
     * @return a command separated string of output extensions, or
     *   null if none.
     */
    
    virtual const char *getOutputExtensions()=0;
    
    /**
     * Get the default audio codec this container prefers, if known.
     * 
     * @return the default audio codec id, or {@link ICodec.ID#CODEC_ID_NONE}
     *   if unknown.
     */
    
    virtual ICodec::ID getOutputDefaultAudioCodec()=0;

    /**
     * Get the default video codec this container prefers, if known.
     * 
     * @return the default video codec id, or {@link ICodec.ID#CODEC_ID_NONE}
     *   if unknown.
     */
    
    virtual ICodec::ID getOutputDefaultVideoCodec()=0;

    /**
     * Get the default subtitle codec this container prefers, if known.
     * 
     * @return the default subtitle codec id, or {@link ICodec.ID#CODEC_ID_NONE}
     *   if unknown.
     */
    
    virtual ICodec::ID getOutputDefaultSubtitleCodec()=0;

    /**
     * Gets the number of different codecs this container
     * can include for encoding.
     * 
     * This can be used as an upper bound when using the
     * {@link #getOutputCodecID(int)} and
     * {@link #getOutputCodecTag(int)}
     * methods to dynamically query the actual codecs.
     * 
     * @return The total number of different codec types that can
     *   be encoded into this container format.
     */
    
    virtual int32_t getOutputNumCodecsSupported()=0;
    
    /**
     * Queries for a supported codec id from the list of codecs
     * that can be encoded into this ContainerFormat.
     * 
     * @param index The index in our lookup table.  Index >= 0 and
     *   < {@link #getOutputNumCodecsSupported()}.  Index values may
     *   change between releases, so always query.
     *   
     * @return The codec id at this position, or
     *   {@link ICodec.ID#CODEC_ID_NONE}
     *   if there is none.
     *
     */
    
    virtual ICodec::ID getOutputCodecID(int32_t index)=0;
    
    /**
     * Queries for a supported codec tag from the list of codecs
     * that can be encoded into this ContainerFormat.
     * <p>
     * Tags are 4-byte values that are often used as markers
     * in a container format for a codec type.
     * </p>
     * @param index The index in our lookup table.  Index >= 0 and
     *   < {@link #getOutputNumCodecsSupported()}.  Index values may
     *   change between releases, so always query.
     *   
     * @return The codec id tag this position, or 0 if there is none.
     *
     */
    
    virtual int32_t getOutputCodecTag(int32_t index)=0;
    
    
    /**
     * Get the 4-byte tag the container would output for
     * the given codec id.
     * 
     * @param id the codec you are about
     * 
     * @return the 4-byte codec tag, or 0 if that id is not
     *   supported. 
     */
    
    virtual int32_t getOutputCodecTag(ICodec::ID id)=0;
    
    /**
     * Returns true if this container format can output media
     * encoded with the given codec.
     * 
     * @param id the codec you care about.
     * 
     * @return true if the codec can be put in this output container;
     *   false otherwise.
     */
    
    virtual bool isCodecSupportedForOutput(ICodec::ID id)=0;

    /**
     * Get the number of input formats this install can demultiplex (read)
     * from.
     * 
     * @return the number of formats
     */
    static int32_t getNumInstalledInputFormats();
    
    /**
     * Return an object for the input format at the given index.
     * 
     * @param index an index for the input format list we maintain
     * 
     * @return a format object for that input or null if
     *   unknown, index < 0 or index >= {@link #getNumInstalledInputFormats()}
     */
    static IContainerFormat* getInstalledInputFormat(int32_t index);

    /**
     * Get the number of output formats this install can multiplex 
     * (write) to.
     * 
     * @return the number of formats
     */
    static int32_t getNumInstalledOutputFormats();

    /**
     * Return an object for the output format at the given index.
     * 
     * @param index an index for the output format list we maintain
     * 
     * @return a format object for that output or null if
     *   unknown, index < 0 or index >= {@link #getNumInstalledOutputFormats()}
     */
    static IContainerFormat* getInstalledOutputFormat(int32_t index);
    
  protected:
    virtual ~IContainerFormat()=0;
    
  };
}}}
#endif /*ICONTAINERFORMAT_H_*/
