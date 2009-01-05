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
package com.xuggle.ferry;

/**
 * 
 * Internal Only.
 * 
 * An Opaque handle that holds a Native pointer/handle.
 * 
 * This class holds a opaque long value
 * that can be used to pass into JNI C
 * function that expect a ** in C.
 * <p>
 * For example, a function like this in native code:
 * </p>
 * <code>
 * <pre>
 *   int url_open(URLContext **handlePtr, char *name, int flags);
 *   int url_read(URLContext *handle, void* buf, int len);
 * </pre>
 * </code>
 * might map to the following Java method:
 * <code>
 * <pre>
 * public class Example {
 *   public native int url_open(JNIPointerReference p, String name, int flags);
 *   public native int url_read(JNIPointerReference p, byte[] buf, int len);
 *   public void example() {
 *      int retval = 0;
 *      JNIPointerReference p;
 *      // p.setPointer is called by the native function 
 *      retval = url_open(p, "foo", 0);
 *      
 *      // p.getPointer is called by the native function
 *      byte[] buf = new bytes[1024];
 *      retval = url_read(p, buf, buf.length);
 *   }
 * </pre>
 * </code>
 * 
 * <p>
 * <b>IMPORTANT: DO NOT RENAME THIS CLASS OR METHODS IN IT.  NATIVE CODE
 * DEPENDS ON THE NAMES AND SIGNATURES.</b>
 * </p>
 * 
 */
public class JNIPointerReference
{
  private long pointer = -1;

  @SuppressWarnings("unused")
  // This method is "private" but we assume it'll be called from
  // native code (that can override protections).
  private synchronized long getPointer()
  {
    return pointer;
  }

  // This method is "private" but we assume it'll be called from
  // native code (that can override protections).
  @SuppressWarnings("unused")
  private synchronized long setPointer(long newVal)
  {
    long oldVal = pointer;
    pointer = newVal;
    return oldVal;
  }

  public String toString()
  {
    return "native:" + pointer;
  }
}
