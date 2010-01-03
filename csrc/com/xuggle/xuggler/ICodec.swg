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

%typemap (javacode) com::xuggle::xuggler::ICodec,com::xuggle::xuggler::ICodec*,com::xuggle::xuggler::ICodec& %{

  /**
   * Prints the type and id of this codec
   * @return the type and id
   */
   
  @Override
  public String toString()
  {
    StringBuilder result = new StringBuilder();
    
    result.append(this.getClass().getName()+"@"+hashCode()+"[");
    result.append("type="+getType()+";");
    result.append("id="+getID()+";");
    result.append("name="+getName()+";");
    result.append("]");
    return result.toString();
  }

  /**
   * Gets a collection of ALL codecs installed on this
   * system. 
   * 
   * @return A collection of all installed codecs.
   */

  public static java.util.Collection<ICodec>
  getInstalledCodecs()
  {
    java.util.Collection<ICodec> retval = new java.util.HashSet<ICodec>();
    int count = getNumInstalledCodecs();
    for(int i=0;i<count;i++)
    {
      ICodec codec = getInstalledCodec(i);
      if (codec != null)
        retval.add(codec);
    }
    return retval;
  }

  /**
   * Returns a list of supported frame-rates this codec can encode
   * video to.
   * 
   * <p>
   * 
   * Not all codecs support reporting this, in which case the returned list
   * will be empty.
   * 
   * </p>
   * 
   * @return a list
   */

  public java.util.List<IRational>
  getSupportedVideoFrameRates()
  {
    java.util.List<IRational> retval =
      new java.util.LinkedList<IRational>();
    int count = getNumSupportedVideoFrameRates();
    for(int i=0;i<count;i++)
    {
      IRational rate = getSupportedVideoFrameRate(i);
      if (rate != null)
        retval.add(rate);
    }
    return retval;
  }

  /**
   * Returns a list of supported pixel formats this codec can encode
   * video in.
   * 
   * <p>
   * 
   * Not all codecs support reporting this, in which case the returned list
   * will be empty.
   * 
   * </p>
   * 
   * @return a list
   */

  public java.util.List<IPixelFormat.Type>
  getSupportedVideoPixelFormats()
  {
    java.util.List<IPixelFormat.Type> retval =
      new java.util.LinkedList<IPixelFormat.Type>();
    int count = getNumSupportedVideoPixelFormats();
    for(int i=0;i<count;i++)
    {
      IPixelFormat.Type type = getSupportedVideoPixelFormat(i);
      if (type != null && type != IPixelFormat.Type.NONE)
        retval.add(type);
    }
    return retval;
  }

  /**
   * Returns a list of supported audio sample rates this codec can encode
   * audio in.
   * 
   * <p>
   * 
   * Not all codecs support reporting this, in which case the returned list
   * will be empty.
   * 
   * </p>
   * 
   * @return a list
   */

  public java.util.List<Integer>
  getSupportedAudioSampleRates()
  {
    java.util.List<Integer> retval =
      new java.util.LinkedList<Integer>();
    int count = getNumSupportedAudioSampleRates();
    for(int i=0;i<count;i++)
    {
      int rate = getSupportedAudioSampleRate(i);
      if (rate != 0)
        retval.add(rate);
    }
    return retval;
  }

  /**
   * Returns a list of supported audio sample formats this codec can encode
   * audio in.
   * 
   * <p>
   * 
   * Not all codecs support reporting this, in which case the returned list
   * will be empty.
   * 
   * </p>
   * 
   * @return a list
   */

  public java.util.List<IAudioSamples.Format>
  getSupportedAudioSampleFormats()
  {
    java.util.List<IAudioSamples.Format> retval =
      new java.util.LinkedList<IAudioSamples.Format>();
    int count = getNumSupportedAudioSampleFormats();
    for(int i=0;i<count;i++)
    {
      IAudioSamples.Format fmt = getSupportedAudioSampleFormat(i);
      if (fmt != null && fmt != IAudioSamples.Format.FMT_NONE)
        retval.add(fmt);
    }
    return retval;
  }

  /**
   * Returns a list of supported audio channel layouts this codec can encode
   * audio in.
   * 
   * <p>
   * 
   * Not all codecs support reporting this, in which case the returned list
   * will be empty.
   * 
   * </p>
   * 
   * @return a list
   */

  public java.util.List<Long>
  getSupportedAudioChannelLayouts()
  {
    java.util.List<Long> retval =
      new java.util.LinkedList<Long>();
    int count = getNumSupportedAudioChannelLayouts();
    for(int i=0;i<count;i++)
    {
      long layout = getSupportedAudioChannelLayout(i);
      if (layout != 0)
        retval.add(layout);
    }
    return retval;
  }

%}

%include <com/xuggle/xuggler/ICodec.h>
