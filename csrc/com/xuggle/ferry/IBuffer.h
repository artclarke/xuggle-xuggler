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

#ifndef IBUFFER_H_
#define IBUFFER_H_

#include <com/xuggle/ferry/Ferry.h>
#include <com/xuggle/ferry/RefCounted.h>

namespace com
{
namespace xuggle
{
namespace ferry
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
  typedef void
  (*FreeFunc)(void * mem, void *closure);

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
  virtual void*
  getBytes(int32_t offset, int32_t length)=0;

  /**
   * Allocate a new buffer by wrapping a native buffer.
   * 
   * @param requestor An optional value telling the IBuffer class what object requested it.  This is used for debugging memory leaks; it's a marker for the FERRY object (e.g. IPacket) that actually requested the buffer.  If you're not an FERRY object, pass in null here.
   * @param bufToWrap Buffer to wrap
   * @param bufferSize The minimum buffer size you're requesting in bytes; a buffer with a larger size may be returned.
   * @param freeFunc A function that will be called when we decide to free the buffer
   * @param closure A value that will be passed, along with this, to freeFunc
   * 
   * @return A new buffer, or null on error.
   */
  static IBuffer*
  make(RefCounted* requestor, void * bufToWrap, int32_t bufferSize,
      FreeFunc freeFunc, void * closure);

#endif // ! SWIG
  /**
   * Get the current maximum number of bytes that can
   * be safely placed in this buffer.
   * 
   * @return Maximum number of bytes this buffer can manage.
   */
  virtual int32_t
  getBufferSize()=0;

  /**
   * Allocate a new buffer of at least bufferSize.
   * 
   * @param requestor An optional value telling the IBuffer class what object requested it.  This is used for debugging memory leaks; it's a marker for the FERRY object (e.g. IPacket) that actually requested the buffer.  If you're not an FERRY object, pass in null here.
   * @param bufferSize The minimum buffer size you're requesting in bytes; a buffer with a larger size may be returned.
   * 
   * @return A new buffer, or null on error.
   */
  static IBuffer*
  make(com::xuggle::ferry::RefCounted* requestor, int32_t bufferSize);

protected:
  IBuffer();
  virtual
  ~IBuffer();
};

}
}
}

#endif /*IBUFFER_H_*/
