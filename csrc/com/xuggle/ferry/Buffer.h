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
#ifndef BUFFER_H_
#define BUFFER_H_

#include <com/xuggle/ferry/IBuffer.h>

namespace com { namespace xuggle { namespace ferry
{

  class Buffer : public IBuffer
  {
    VS_JNIUTILS_REFCOUNTED_OBJECT_PRIVATE_MAKE(Buffer);
  public:
    typedef void (*FreeFunc)(void * mem, void *closure);
    
    virtual void* getBytes(int32_t offset, int32_t length);
    virtual int32_t getBufferSize();

    /**
     * Allocate a new buffer of at least bufferSize.
     */
    static VS_API_FERRY Buffer* make(RefCounted* requestor, int32_t bufferSize);
    
    /**
     * Create an iBuffer that wraps the given buffer, and calls
     * FreeFunc(buffer, closure) on it when we believe it's safe
     * to destruct it.
     */
    static VS_API_FERRY Buffer* make(RefCounted* requestor, void * bufToWrap, int32_t bufferSize,
     FreeFunc freeFunc, void * closure);
    
  protected:
    Buffer();
    virtual ~Buffer();
  private:
    void* mBuffer;
    FreeFunc mFreeFunc;
    void* mClosure;
    int32_t mBufferSize;
    bool mInternallyAllocated;
  };

}}}

#endif /*BUFFER_H_*/
