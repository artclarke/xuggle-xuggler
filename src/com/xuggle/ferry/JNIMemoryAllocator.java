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

import java.util.HashSet;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Internal Only.
 * 
 * This object allocates large memory chunks and returns them to native code.  The
 * native memory then pins raw bytes based on the byte[]s returned here.  The net
 * effect is that Java ends up thinking it actually allocated the memory.
 * 
 * This function is called DIRECTLY from native code; names of methods MUST NOT CHANGE.
 * @author aclarke
 *
 */
public class JNIMemoryAllocator
{
  private static final Logger log = LoggerFactory.getLogger(JNIMemoryAllocator.class);

  // don't allocate this unless we malloc; most classes don't even
  // touch this memory manager with a 100 foot pole
  private Set<byte[]> mBuffers;
  private int mMaxAllocationAttempts=5;
  private double mFallbackTimeDecay=1.5;
  private boolean mRetryFailedAllocs = false;
  
  /**
   * Allocate a new block of bytes.  Called from native code.
   * 
   * Will retry many times if it can't get memory, backing off
   * in timeouts to get there.
   * 
   * @param size # of bytes requested
   * @return An array of size or more bytes long, or null on failure.
   */
  public synchronized byte[] malloc(int size)
  {
    byte[] retval = null;
    if (mBuffers == null)
    {
      mBuffers = new HashSet<byte[]>();
    }
    if (mRetryFailedAllocs)
    {
      int allocationAttempts=0;
      int backoffTimeout=10; // start at 10 milliseconds
      while(true)
      {
        try
        {
          retval = new byte[size];
          // we succeed, so break out
          break;
        }
        catch (OutOfMemoryError e)
        {
          // try clearing our queue now and do it again.  Why?
          // because the first failure may have allowed us to 
          // catch a RefCounted no longer in use, and the second
          // attempt may have freed that memory.

          // do a JNI collect before the alloc
          ++allocationAttempts;
          if (allocationAttempts >= mMaxAllocationAttempts)
            throw e;

          log.debug("retrying ({}) allocation of {} bytes", allocationAttempts, size);
          System.gc();
          try
          {
            // give the finalizer a chance
            if (allocationAttempts <= 1)
            {
              // first just yield
              Thread.yield();
            } else {
              Thread.sleep(backoffTimeout);
              // and slowly get longer...
              backoffTimeout = (int)(backoffTimeout*mFallbackTimeDecay);
            }
          }
          catch (InterruptedException e1)
          {
            throw new OutOfMemoryError();
          }
          // do a JNI collect before the alloc
          JNIWeakReference.getMgr().gc();
        }
      }
    }
    else
    {
      retval = new byte[size];      
    }
    if (!mBuffers.add(retval))
    {
      assert false : "buffers already added";
    }
    retval[retval.length-1] = 0;
    /*
    log.debug("malloc: {}({}:{})", new Object[]{
        retval.hashCode(),
        retval.length,
        size
      });
      */
    return retval;
  }
  
  /**
   * Free memory allocated by the malloc(int) method.
   * Called from Native code.
   * 
   * @param mem the byes to be freed.
   */
  public synchronized void free(byte[] mem)
  {
    if (!mBuffers.remove(mem))
    {
      assert false : "buffer not in memory";
    }
    //log.debug("free:   {}({})", mem.hashCode(), mem.length);
  }
 
  /**
   * Native method that tells a native objects (represented by the nativeObj
   * long pointer val) that this JNIMemoryAllocator is being used to
   * allocate it's large blocks of memory.
   * 
   * Follow that?  No.  That's OK... you really don't want to know.
   * @param nativeObj A C pointer (a la swig).
   * @param mgr  Us.
   */
  public native static void setAllocator(long nativeObj, JNIMemoryAllocator mgr);
  
  /**
   * Get the allocate for the underlying native pointer.
   * @param nativeObj The native pointer.
   * @return The allocator to use, or null.
   */
  public native static JNIMemoryAllocator getAllocator(long nativeObj);
}
