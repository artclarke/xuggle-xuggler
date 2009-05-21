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
  
%}

%include <com/xuggle/xuggler/IContainerFormat.h>
