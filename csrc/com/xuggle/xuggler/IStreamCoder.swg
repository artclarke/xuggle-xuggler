/*
 * This file is part of Xuggler.
 * 
 * Xuggler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public
 * License along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

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
%}

%typemap(javainterfaces) com::xuggle::xuggler::IStreamCoder, com::xuggle::xuggler::IStreamCoder*, com::xuggle::xuggler::IStreamCoder& "com.xuggle.xuggler.IConfigurable";
%include <com/xuggle/xuggler/IStreamCoder.h>

