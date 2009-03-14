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
#ifndef IBUFFER_H_
#define IBUFFER_H_

#ifndef SWIG
#include <jni.h>
#endif
typedef jobject jNioByteArray;

#include <com/xuggle/ferry/Ferry.h>
#include <com/xuggle/ferry/RefCounted.h>

namespace com { namespace xuggle { namespace ferry
{


  /**
   * Allows Java code to get data from a native buffers, and optionally modify native memory directly.
   * <p> 
   * When accessing from Java, you can copy in and
   * out ranges of buffers.  You can do this by-copy
   * (which is safer but a little slower) or by-reference
   * where you <b>directly access underlying C++/native
   * memory from Java</b>.  Take special care if you decide
   * that native access is required.
   * </p>
   * <p>
   * When accessing from C++, you get direct access to
   * the underlying buffer.
   * </p>
   */
  class VS_API_FERRY IBuffer : public com::xuggle::ferry::RefCounted
  {
  public:
#ifndef SWIG
    /**
     * Returns up to length bytes, starting at offset in the
     * underlying buffer we're managing.
     * 
     * Note that with this method you are accessing the direct
     * memory, so be careful.
     * 
     * @param offset The offset (in bytes) into the buffer managed by this IBuffer
     * @param length The requested length (in bytes) you want to access.  The buffer returned may
     *   actually be longer than length.
     * 
     * @return A pointer to the direct buffer, or 0 if the offset or length
     *   would not fit within the buffer we're managing.
     */
    virtual void* getBytes(int32_t offset, int32_t length)=0;
#endif // ! SWIG

    /**
     * Returns up to length bytes, starting at offset in the
     * underlying buffer we're managing.
     * <p> 
     * This method COPIES the data into the byte array being
     * returned..
     * </p><p>
     * If you don't NEED the direct access that getByteBuffer
     * offers (and most programs can in fact take the performance
     * hit of the copy), we recommend you use this method.
     * It's much harder to accidentally leave native memory lying
     * around waiting for cleanup then.
     * </p>
     * 
     * @param offset The offset (in bytes) into the buffer managed by this IBuffer
     * @param length The requested length (in bytes) you want to access.  The buffer returned may
     *   actually be longer than length.
     * 
     * @return A copy of the data that is in this IBuffer, or null
     *   if error.
     */
    jbyteArray getByteArray(int32_t offset, int32_t length);

    /**
     * Get the current maximum number of bytes that can
     * be safely placed in this buffer.
     * 
     * @return Maximum number of bytes this buffer can manage.
     */
    virtual int32_t getBufferSize()=0;

    /**
     * Allocate a new buffer of at least bufferSize.
     * 
     * @param requestor An optional value telling the IBuffer class what object requested it.  This is used for debugging memory leaks; it's a marker for the FERRY object (e.g. IPacket) that actually requested the buffer.  If you're not an FERRY object, pass in null here.
     * @param bufferSize The minimum buffer size you're requesting in bytes; a buffer with a larger size may be returned.
     * 
     * @return A new buffer, or null on error.
     */
    static IBuffer* make(com::xuggle::ferry::RefCounted* requestor, int32_t bufferSize);

    /**
     * Allocate a new IBuffer and copy the data in the passed in buffer into the new IBuffer.
     * 
     * @param requestor An option value telling the IBuffer class what object requested it.  Default to null if you don't know what to pass here.
     * @param buffer a java byte[] array passed in from a JNI call
     * @param offset the offset to start copying from.
     * @param length the number of bytes to copy
     * 
     * @return a new IBuffer, or null on error.  
     */
    static IBuffer* make(com::xuggle::ferry::RefCounted* requestor, jbyteArray buffer, int32_t offset, int32_t length);

    /**
     * Allocate a new IBuffer that wraps the java.nio.ByteBuffer that is passed in.  This does
     * not copy data and so is as fast as we can get.
     * 
     * This will fail with an error unless the byte buffer is a direct byte buffer
     * (i.e. {@link java.nio.ByteBuffer#isDirect()} returns true.
     * 
     * @param requestor An option value telling the IBuffer class what object requested it.  Default to null if you don't know what to pass here.
     * @param directByteBuffer an instance of java.nio.ByteBuffer that has been allocated as a Direct buffer.
     * @param offset the offset to start copying from.
     * @param length the number of bytes to copy
     * 
     * @return a new IBuffer, or null on error.  
     */
    static IBuffer* make(com::xuggle::ferry::RefCounted* requestor, jNioByteArray directByteBuffer, int32_t offset, int32_t length);

#ifdef SWIG
    %javamethodmodifiers java_getByteBuffer(int32_t, int32_t) "private"
#endif // SWIG
    jNioByteArray java_getByteBuffer(int32_t offset, int32_t length);

  protected:
    IBuffer();
    virtual ~IBuffer();
  };

}}}

#endif /*IBUFFER_H_*/
