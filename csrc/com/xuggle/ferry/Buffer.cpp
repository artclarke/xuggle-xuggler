/*
 * Copyright (c) 2008 by Vlideshow Inc. (a.k.a. The Yard).  All rights reserved.
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
#include <cstring>

#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/ferry/JNIMemoryManager.h>

#include <com/xuggle/ferry/Buffer.h>

VS_LOG_SETUP(VS_CPP_PACKAGE);

namespace com { namespace xuggle { namespace ferry
{

  Buffer :: Buffer() : mBuffer(0), mBufferSize(0)
  {
    mFreeFunc = 0;
    mClosure = 0;
    mInternallyAllocated = false;
  }

  Buffer :: ~Buffer()
  {
    if (mBuffer)
    {
      VS_ASSERT(mBufferSize, "had buffer but no size");
      if (mInternallyAllocated)
        JNIMemoryManager::free(mBuffer);
      else if (mFreeFunc)
        mFreeFunc(mBuffer, mClosure);
      mBuffer = 0;
      mBufferSize = 0;
      mFreeFunc = 0;
      mClosure = 0;
    }
  }


  void*
  Buffer :: getBytes(int32_t offset, int32_t length)
  {
    void* retval = 0;

    if (length == 0)
      length = mBufferSize - offset;

    if ((length > 0) && (length + offset <= mBufferSize))
      retval = ((unsigned char*) mBuffer)+offset;

    return retval;
  }

  int32_t
  Buffer :: getBufferSize()
  {
    return mBufferSize;
  }

  Buffer*
  Buffer :: make(com::xuggle::ferry::RefCounted* requestor, int32_t bufferSize)
  {
    Buffer* retval = 0;
    if (bufferSize > 0)
    {
      retval = Buffer::make();
      if (retval)
      {
        void * allocator = requestor ? requestor->getJavaAllocator() : 0;
        
        retval->mBuffer = JNIMemoryManager::malloc(allocator, bufferSize);
        if (retval->mBuffer)
        {
          retval->mBufferSize = bufferSize;
          retval->mInternallyAllocated = true;
        }
        else
        {
          // we couldn't allocate buffer size.  Fail.
          VS_REF_RELEASE(retval);
        }
      }
    }
    return retval;
  }

  Buffer*
  Buffer :: make(com::xuggle::ferry::RefCounted* /*unused*/, void *bufToWrap, int32_t bufferSize,
      FreeFunc freeFunc, void *closure)
  {
    Buffer * retval = 0;
    
    if (bufToWrap && bufferSize>0)
    {
      retval = Buffer::make();
      if (retval)
      {
        retval->mFreeFunc = freeFunc;
        retval->mClosure = closure;
        retval->mBufferSize = bufferSize;
        retval->mBuffer = bufToWrap;
        retval->mInternallyAllocated = false;
      }
    }
    return retval;
  }
  
}}}
