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

%}

%include <com/xuggle/xuggler/ICodec.h>
