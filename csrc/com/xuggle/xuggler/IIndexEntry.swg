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

 %typemap (javacode) com::xuggle::xuggler::IIndexEntry,com::xuggle::xuggler::IIndexEntry*,com::xuggle::xuggler::IIndexEntry& %{
   /**
   * info about this entry.
   * @return information about this entry.
   */
   
  @Override
  public String toString()
  {
    StringBuilder result = new StringBuilder();
    
    result.append(this.getClass().getName()+"@"+hashCode()+"[");
    result.append("position:"+getPosition()+";");
    result.append("timestamp:"+getTimeStamp()+";");
    result.append("flags:"+getFlags()+";");
    result.append("size:"+getSize()+";");
    result.append("min-distance:"+getMinDistance()+";");
    result.append("]");
    return result.toString();
  }

 
%}
%include <com/xuggle/xuggler/IIndexEntry.h>
 
