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

%typemap (javacode) com::xuggle::xuggler::IContainerFormat,com::xuggle::xuggler::IContainerFormat*,com::xuggle::xuggler::IContainerFormat& %{

  /**
   * info about this format object
   * @return information about this object
   */
   
  @Override
  public String toString()
  {
    StringBuilder result = new StringBuilder();
    
    result.append(this.getClass().getName()+"@"+hashCode()+"[");
    if (isInput())
    {
      result.append("iname:"+getInputFormatShortName()+";");
      result.append("ilongname:"+getInputFormatLongName()+";");
    }
    if (isOutput())
    {
      result.append("oname:"+getOutputFormatShortName()+";");
      result.append("olongname:"+getOutputFormatLongName()+";");
      result.append("omimetype:"+getOutputFormatMimeType()+";");
      result.append("oextensions:"+getOutputExtensions()+";");
    }
    result.append("]");
    return result.toString();
  }
  
  /**
   * Returns a list of all codecs supported for this Object.
   *
   * <p>
   *
   * If this object hasn't been set up for output, then
   * we return null.
   *
   * </p>
   *
   * @return A list of supported codecs, in decreasing
   *   order of the IContainerFormat Object's preference.
   *
   */
   
  public java.util.List<ICodec.ID> getOutputCodecsSupported()
  {
    java.util.List<ICodec.ID> retval =
      new java.util.LinkedList<ICodec.ID>();
    
    int numCodecs = getOutputNumCodecsSupported();
    for(int i = 0; i < numCodecs; i++)
    {
      ICodec.ID id = getOutputCodecID(i);
      if (id != ICodec.ID.CODEC_ID_NONE)
        retval.add(id);
    }    
    return retval;
  }
  
    /**
   * Return a collection of all input formats installed on this system.
   * @return the list.
   */
  public static java.util.Collection<IContainerFormat>
  getInstalledInputFormats()
  {
    java.util.Collection<IContainerFormat> retval =
      new java.util.HashSet<IContainerFormat>();
    int count = getNumInstalledInputFormats();
    for(int i = 0; i< count;++i)
    {
      IContainerFormat fmt = getInstalledInputFormat(i);
      if (fmt != null)
        retval.add(fmt);
    }
    return retval;
  }
  
  /**
   * Return a collection of all output formats installed on this system.
   * @return the list.
   */
  public static java.util.Collection<IContainerFormat>
  getInstalledOutputFormats()
  {
    java.util.Collection<IContainerFormat> retval =
      new java.util.HashSet<IContainerFormat>();
    int count = getNumInstalledOutputFormats();
    for(int i = 0; i< count;++i)
    {
      IContainerFormat fmt = getInstalledOutputFormat(i);
      if (fmt != null)
        retval.add(fmt);
    }
    return retval;
  }
  
    /**
   * For a given input codec guess an ID supported by this
   * IContainerFormat that might be good for encoding.
   * 
   * <p>
   * For example, if the input codec ID was
   * {@link ICodec.ID#CODEC_ID_FLAC} and you were trying
   * to encoding into an "FLV" file that doesn't support that
   * codec for outputting, this method will instead suggest
   * {@link ICodec.ID#CODEC_ID_MP3} instead.
   * </p>
   * 
   * <p>
   * Make the best effort to establish the ouput codec id for a given
   * input codec and output container format.  This method relies on
   * FFMPEGs internal database of codec IDs to identify the correct
   * output codec IDs that can fit in this container.
   * </p>
   * 
   * @param inputCodec the input codec
   *
   * @return the best guess output codec ID, or null if none found
   * @throws IllegalArgumentException if inputCodec is null
   *   or {@link #isOutput()} is false.
   */

  public ICodec.ID establishOutputCodecId(ICodec inputCodec)
  {
    if (inputCodec == null)
      throw new IllegalArgumentException();
    return establishOutputCodecId(inputCodec.getID());
  }

  /**
   * For a given input codec guess an ID supported by this
   * IContainerFormat that might be good for encoding.
   * 
   * <p>
   * For example, if the input codec ID was
   * {@link ICodec.ID#CODEC_ID_FLAC} and you were trying
   * to encoding into an "FLV" file that doesn't support that
   * codec for outputting, this method will instead suggest
   * {@link ICodec.ID#CODEC_ID_MP3} instead.
   * </p>
   * 
   * <p>
   * Make the best effort to establish the ouput codec id for a given
   * input codec and output container format.  This method relies on
   * FFMPEGs internal database of codec IDs to identify the correct
   * output codec IDs that can fit in this container.
   * </p>
   * 
   * @param inputCodecId the input codec id
   *
   * @return the best guess output codec ID, or null if none found
   * @throws IllegalArgumentException if inputCodecId is null
   *   or equal to {@link ICodec.ID#CODEC_ID_NONE}, or 
   *   {@link #isOutput()} is false.
   */

  public ICodec.ID establishOutputCodecId(ICodec.ID inputCodecId)
  {
    // test parameteres

    if (null == inputCodecId)
      throw new IllegalArgumentException("null input codec id");
    if (ICodec.ID.CODEC_ID_NONE == inputCodecId)
      throw new IllegalArgumentException("input codec id is \"NONE\"");
    if (!this.isOutput())
      throw new IllegalArgumentException(
        "passed output container format, actally an input container format");

    // if the output container supports in teh input codec, and can
    // encode, return the input codec

    if (this.isCodecSupportedForOutput(inputCodecId) &&
      ICodec.findEncodingCodec(inputCodecId).canEncode())
    {
      return inputCodecId;
    }

    // establish the input codec type

    ICodec.Type inputCodecType = ICodec.findDecodingCodec(inputCodecId)
      .getType();

    // the would be output codec 

    ICodec.ID outputCodecId = null;

    // find the default codec for the output container by input codec type
    
    switch (inputCodecType)
    {
    case CODEC_TYPE_AUDIO:
      outputCodecId = this.getOutputDefaultAudioCodec();
      break;
    case CODEC_TYPE_VIDEO:
      outputCodecId = this.getOutputDefaultVideoCodec();
      break;
    case CODEC_TYPE_SUBTITLE:
      outputCodecId = this.getOutputDefaultSubtitleCodec();
      break;
    }

    // if there still isn't a valid codec, hunt through all the codecs
    // for the output format and see if ANY match the input codec type

    if (null == outputCodecId || ICodec.ID.CODEC_ID_NONE == outputCodecId ||
      !ICodec.findEncodingCodec(inputCodecId).canEncode())
    {
      for(ICodec.ID codecId: this.getOutputCodecsSupported())
      {
        ICodec codec = ICodec.findEncodingCodec(codecId);
        if (codec.getType() == inputCodecType)
        {
          // if it is a valid codec break out of the search

          outputCodecId = codec.getID();
          if (null != outputCodecId && 
            ICodec.ID.CODEC_ID_NONE != outputCodecId || 
            ICodec.findEncodingCodec(inputCodecId).canEncode())
          {
            break;
          }
        }
      }
    }

    // return found ouput codec id, or null if not found

    return outputCodecId;
  }
  
%}

%include <com/xuggle/xuggler/IContainerFormat.h>
