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

#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/ferry/IBuffer.h>
#include <com/xuggle/ferry/Buffer.h>

#include <stdexcept>

using namespace com::xuggle::ferry;

VS_LOG_SETUP(VS_CPP_PACKAGE);

namespace com { namespace xuggle { namespace ferry
{

  IBuffer::IBuffer()
  {
  }

  IBuffer::~IBuffer()
  {
  }
  
  IBuffer*
  IBuffer :: make(com::xuggle::ferry::RefCounted* requestor, int32_t bufferSize)
  {
    return Buffer::make(requestor, bufferSize);
  }
  
  IBuffer*
  IBuffer :: make(RefCounted* requestor, void * bufToWrap,
      int32_t bufferSize,
      FreeFunc freeFunc,
      void * closure)
  {
    return Buffer::make(requestor, bufToWrap, bufferSize, freeFunc, closure);
  }
  

  

}}}
