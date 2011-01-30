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
   * @return A list of supported codecs, in decreasing preferred order.
   * @since 3.1
   */
  public java.util.List<ICodec.ID> getOutputCodecsSupported()
  {
    final java.util.List<ICodec.ID> retval =
      new java.util.LinkedList<ICodec.ID>();
    final java.util.Set<ICodec.ID> uniqueSet =
      new java.util.HashSet<ICodec.ID>();
    
    int numCodecs = getOutputNumCodecsSupported();
    for(int i = 0; i < numCodecs; i++)
    {
      ICodec.ID id = getOutputCodecID(i);
      // remove duplicate IDs
      if (id != ICodec.ID.CODEC_ID_NONE && !uniqueSet.contains(id))
        retval.add(id);
      uniqueSet.add(id);
    }    
    return retval;
  }

  /**
   * Returns a list of all codec tags supported for this container.
   *
   * <p>
   *
   * If this object hasn't been set up for output, then
   * we return null.
   *
   * </p>
   *
   * @return A list of supported codec tags, in decreasing preferred order.
   * @since 3.3
   */
  public java.util.List<Integer> getOutputTagsSupported()
  {
    final java.util.List<Integer> retval =
      new java.util.LinkedList<Integer>();
    final java.util.Set<Integer> uniqueSet =
      new java.util.HashSet<Integer>();
    
    int numCodecs = getOutputNumCodecsSupported();
    for(int i = 0; i < numCodecs; i++)
    {
      int tag = getOutputCodecTag(i);
      ICodec.ID id = getOutputCodecID(i);
      // remove duplicate tags
      if (id != ICodec.ID.CODEC_ID_NONE && !uniqueSet.contains(tag))
        retval.add(tag);
      uniqueSet.add(tag);
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
   * For a given output codec type guess the best codec
   * for encoding into this container.
   * 
   * @param type the codec type
   *
   * @return the best guess output codec ID
   * 
   * @see #establishOutputCodecId(com.xuggle.xuggler.ICodec.Type, com.xuggle.xuggler.ICodec.ID)
   * @throws IllegalArgumentException if type is null
   *   or {@link #isOutput()} is false.
   * @throws UnsupportedOperationException if we cannot establish
   *   a codec.
   */

  public ICodec.ID establishOutputCodecId(ICodec.Type type)
  {
    return establishOutputCodecId(type, null);
  }
  
  /**
   * For a given input codec id guess an ID supported by this
   * IContainerFormat that might be good for encoding.
   * 
   * @param inputCodecId the input codec id
   *
   * @return the best guess output codec ID
   * 
   * @see #establishOutputCodecId(com.xuggle.xuggler.ICodec.Type, com.xuggle.xuggler.ICodec.ID)
   * @throws IllegalArgumentException if inputCodec is null
   *   or {@link #isOutput()} is false.
   * @throws UnsupportedOperationException if we cannot establish
   *   a codec.
   */

  public ICodec.ID establishOutputCodecId(ICodec.ID inputCodecId)
  {
    ICodec codec = null;
    try
    {
      if (inputCodecId == null || inputCodecId == ICodec.ID.CODEC_ID_NONE)
        throw new IllegalArgumentException("null inputCodecId");
      
      codec = ICodec.findDecodingCodec(inputCodecId);
      if (codec == null)
        throw new UnsupportedOperationException("could not find decoding codec");
      return establishOutputCodecId(codec);
    }
    finally
    {
      if (codec != null)
        codec.delete();
    }
  }
  /**
   * For a given input codec guess an ID supported by this
   * IContainerFormat that might be good for encoding.
   * 
   * @param inputCodec the input codec
   *
   * @see #establishOutputCodecId(com.xuggle.xuggler.ICodec.Type, com.xuggle.xuggler.ICodec.ID)
   * @return the best guess output codec ID
   * 
   * @throws IllegalArgumentException if inputCodec is null
   *   or {@link #isOutput()} is false.
   * @throws UnsupportedOperationException if we cannot establish
   *   a codec.
   */

  public ICodec.ID establishOutputCodecId(ICodec inputCodec)
  {
    if (inputCodec == null)
      throw new IllegalArgumentException();
    return establishOutputCodecId(inputCodec.getType(), inputCodec.getID());
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
   * <p>
   * The algorithm followed is:
   * </p>
   * <ul>
   *   <li>If the container has a preferred codec for the given
   *   codec type, use that; else</li>
   *   <li>If the input codec can be used for encoding into this
   *   container, and is installed on this system, use that; else</li>
   *   <li>Query the container format to see what other formats it
   *   can support, and are installed on this system, and choose the
   *   first one that will work; else</li>
   *   <li>Throw an {@link UnsupportedOperationException}.</li>
   * </ul>
   * 
   * <p>
   * This method relies on
   * FFMPEGs internal database of codec IDs to identify the correct
   * output codec IDs that can fit in this container.
   * </p>
   * 
   * @param type the type of codec desired
   * @param inputCodecId an input codec id to attempt to match, or null
   *   if none.
   *
   * @return the best guess output codec ID
   * 
   * @throws IllegalArgumentException if inputCodecId is null
   *   or equal to {@link ICodec.ID#CODEC_ID_NONE}, or 
   *   {@link #isOutput()} is false.
   * @throws UnsupportedOperationException if we cannot establish
   *   a codec.
   */

  public ICodec.ID establishOutputCodecId(
      ICodec.Type type,
      ICodec.ID inputCodecId)
  {
    ICodec codec = null;
    ICodec inputCodec = null;
    try
    {
      if (type == null)
        throw new IllegalArgumentException("null codec type");

      if (!this.isOutput())
        throw new IllegalArgumentException(
            "passed output container format, actally an input container format");

      if (inputCodecId != null && inputCodecId != ICodec.ID.CODEC_ID_NONE)
      {
        inputCodec = ICodec.findDecodingCodec(inputCodecId);
        if (inputCodec == null)
          throw new IllegalArgumentException("could not find input codec id");
        
        if (inputCodec.getType() != type)
          throw new IllegalArgumentException("inputCodecId of different type"+
            " than expected");
      }

      // the would be output codec

      ICodec.ID outputCodecId = null;

      // find the default codec for the output container by input codec type

      switch (type)
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
      if (outputCodecId != null && outputCodecId != ICodec.ID.CODEC_ID_NONE)
      {
        codec = ICodec.findEncodingCodec(outputCodecId);
      }
      if (codec != null)
        return outputCodecId;

      // ok, otherwise that didn't work, so try the input codec
      if (inputCodec != null)
      {
        codec = ICodec.findEncodingCodec(inputCodecId);
        if (codec != null &&
            this.isCodecSupportedForOutput(codec.getID()))
          return codec.getID();
      }

      // if there still isn't a valid codec, hunt through all the codecs
      // for the output format and see if ANY match the input codec type

      for (ICodec.ID codecId : this.getOutputCodecsSupported())
      {
        if (codec != null)
          codec.delete();
        codec = ICodec.findEncodingCodec(codecId);
        if (codec != null && codec.getType() == type)
        {
          // if it is a valid codec break out of the search

          outputCodecId = codec.getID();
          if (codec.canEncode())
          {
            break;
          }
        }
      }
      if (outputCodecId == null || outputCodecId == ICodec.ID.CODEC_ID_NONE)
        throw new UnsupportedOperationException("could not guess codec");
      return outputCodecId;
    }
    finally
    {
      if (codec != null)
        codec.delete();
      if (inputCodec != null)
        inputCodec.delete();
    }
  }
   
%}

%include <com/xuggle/xuggler/IContainerFormat.h>
