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

%typemap (javacode) com::xuggle::xuggler::IStreamCoder,com::xuggle::xuggler::IStreamCoder*,com::xuggle::xuggler::IStreamCoder& %{

  /**
   * Returns the fourcc tag, in order of least significant byte
   * to most significant byte.
   *
   * @return a 4 char array of the fourcc
   */
  
  public char[] getCodecTagArray()
  {
    char[] retval = new char[4];
    int fourcc = this.getCodecTag();
    
    retval[0] = (char)(fourcc & 0xFF);
    retval[1] = (char)((fourcc >> 8) & 0xFF);
    retval[2] = (char)((fourcc >> 16) & 0xFF);
    retval[3] = (char)((fourcc >> 24) & 0xFF);
    return retval;
  }
  /**
   * Set the fourcc tag.
   *
   * @param fourcc A four char array, in order of least significant byte
   *  to most significant byte.
   *
   * @throws IllegalArgumentException if the array passed in is not exactly 4 bytes.
   */
   
   public void setCodecTag(char[] fourcc)
   {
     if (fourcc == null || fourcc.length != 4)
      throw new IllegalArgumentException();
     int tag = 0;
     tag = (fourcc[3]<<24)+(fourcc[2]<<16)+(fourcc[1]<<8)+fourcc[0];
     this.setCodecTag(tag);
   }
   
   /**
    * Prints  details on this IStreamCoder
    *
    * @return key details for this IStreamCoder
    */
    
  @Override
  public String toString()
  {
    StringBuilder result = new StringBuilder();
    ICodec.Type type = getCodecType();
    
    result.append(this.getClass().getName()+"@"+hashCode()+"[");
    result.append("codec="+getCodec()+";");
    result.append("time base="+getTimeBase()+";");
    result.append("frame rate="+getFrameRate()+";");
    switch(type)
    {
      case CODEC_TYPE_VIDEO:
        result.append("pixel type="+getPixelType()+";");
        result.append("width="+getWidth()+";");
        result.append("height="+getHeight()+";");
        break;
      case CODEC_TYPE_AUDIO:
        result.append("sample rate="+getSampleRate()+";");
        result.append("channels="+getChannels()+";");
        break;
      default:
        break;
    }
    result.append("]");
    return result.toString();
  }
  
  /**
   * {@inheritDoc}
   */
  public java.util.Collection<String> getPropertyNames()
  {
    java.util.Collection<String> retval = new java.util.LinkedList<String>();
    int numProperties = this.getNumProperties();
    for(int i = 0; i < numProperties; i++)
    {
      IProperty property = this.getPropertyMetaData(i);
      String name = property.getName();
      retval.add(name);
    }
    return retval;
  }

  /**
   * Returns <strong>a read-only copy</strong> of the extra data in this stream coder as a new {@link IBuffer}.
   * @return the extra data, or null if none or error.
   * @since 3.3
   */
  public IBuffer getExtraData()
  {
    int size = getExtraDataSize();
    if (size <= 0)
      return null;
    
    IBuffer retval = IBuffer.make(this, size);
    if (retval == null)
      return null;
    if (getExtraData(retval, 0, size) != size) {
      retval.delete();
      retval = null;
    }
    return retval;
  }

  
%}

%typemap(javainterfaces) com::xuggle::xuggler::IStreamCoder, com::xuggle::xuggler::IStreamCoder*, com::xuggle::xuggler::IStreamCoder& "com.xuggle.xuggler.IConfigurable";
%include <com/xuggle/xuggler/IStreamCoder.h>

