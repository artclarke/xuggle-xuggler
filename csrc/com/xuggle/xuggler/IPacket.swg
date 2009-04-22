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
%typemap (javacode) com::xuggle::xuggler::IPacket,com::xuggle::xuggler::IPacket*,com::xuggle::xuggler::IPacket& %{

  /**
   * info about this packet
   * @return information about this packet
   */
   
  @Override
  public String toString()
  {
    StringBuilder result = new StringBuilder();
    
    result.append(this.getClass().getName()+"@"+hashCode()+"[");
    result.append("complete:"+isComplete()+";");
    result.append("dts:"+getDts()+";");
    result.append("pts:"+getPts()+";");
    result.append("size:"+getSize()+";");
    result.append("key:"+isKey()+";");
    result.append("flags:"+getFlags()+";");
    result.append("stream index:"+getStreamIndex()+";");
    result.append("duration:"+getDuration()+";");
    result.append("position:"+getPosition()+";");
    result.append("time base:"+getTimeBase()+";");
    result.append("]");
    return result.toString();
  }

%}

%include <com/xuggle/xuggler/IPacket.h>
