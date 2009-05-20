/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated.  All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any
 * later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.xuggle.ferry;

import java.util.HashSet;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Internal Only.
 * 
 * This object allocates large memory chunks and returns them to native code.
 * The native memory then pins raw bytes based on the byte[]s returned here. The
 * net effect is that Java ends up thinking it actually allocated the memory,
 * but since a {@link RefCounted} object will also maintain a reference to this
 * allocator, you can use this to detect instances of 'leaked' references in
 * your Java code.
 * <p>
 * This function is called DIRECTLY from native code; names of methods MUST NOT
 * CHANGE.
 * </p>
 * 
 * @author aclarke
 * 
 */

public class JNIMemoryAllocator
{
  private static final Logger log = LoggerFactory
      .getLogger(JNIMemoryAllocator.class);

  // don't allocate this unless we malloc; most classes don't even
  // touch this memory manager with a 100 foot pole
  final private Set<byte[]> mBuffers = new HashSet<byte[]>();
  final static private int MAX_ALLOCATION_ATTEMPTS = 5;
  final static private double FALLBACK_TIME_DECAY = 1.5;
  final static private boolean SHOULD_RETRY_FAILED_ALLOCS = true;

  /**
   * Allocate a new block of bytes. Called from native code.
   * 
   * Will retry many times if it can't get memory, backing off in timeouts to
   * get there.
   * 
   * @param size
   *          # of bytes requested
   * @return An array of size or more bytes long, or null on failure.
   */

  public byte[] malloc(int size)
  {
    byte[] retval = null;
    // first check the parachute
    JNIMemoryParachute.getParachute().packChute();
    try
    {
      if (SHOULD_RETRY_FAILED_ALLOCS)
      {
        int allocationAttempts = 0;
        int backoffTimeout = 10; // start at 10 milliseconds
        while (true)
        {
          try
          {
            // log.debug("attempting malloc of size: {}", size);
            retval = new byte[size];
            // log.debug("malloced block of size: {}", size);
            // we succeed, so break out
            break;
          }
          catch (final OutOfMemoryError e)
          {
            // try clearing our queue now and do it again. Why?
            // because the first failure may have allowed us to
            // catch a RefCounted no longer in use, and the second
            // attempt may have freed that memory.

            // do a JNI collect before the alloc
            ++allocationAttempts;
            if (allocationAttempts >= MAX_ALLOCATION_ATTEMPTS)
            {
              // try pulling our rip cord
              JNIMemoryParachute.getParachute().pullCord();
              // do one last "hope gc" to free our own memory
              JNIReference.getMgr().gc();
              // and throw the error back to the native code
              throw e;
            }

            log.debug("retrying ({}) allocation of {} bytes",
                allocationAttempts, size);
            try
            {
              // give the finalizer a chance
              if (allocationAttempts <= 1)
              {
                // first just yield
                Thread.yield();
              }
              else
              {
                Thread.sleep(backoffTimeout);
                // and slowly get longer...
                backoffTimeout = (int) (backoffTimeout * FALLBACK_TIME_DECAY);
              }
            }
            catch (InterruptedException e1)
            {
              throw e;
            }
            // do a JNI collect before the alloc
            JNIReference.getMgr().gc();
          }
        }
      }
      else
      {
        retval = new byte[size];
      }

      synchronized (mBuffers)
      {
        if (!mBuffers.add(retval))
        {
          assert false : "buffers already added";
        }
      }
      retval[retval.length - 1] = 0;
      /*
       * log.debug("malloc: {}({}:{})", new Object[]{ retval.hashCode(),
       * retval.length, size });
       */
    }
    catch (Throwable t)
    {
      // do not let an exception leak out since we go back to native code.
      retval = null;
    }
    return retval;
  }

  /**
   * Free memory allocated by the malloc(int) method. Called from Native code.
   * 
   * @param mem
   *          the byes to be freed.
   */

  public void free(byte[] mem)
  {
    try
    {
      synchronized (mBuffers)
      {
        if (!mBuffers.remove(mem))
        {
          assert false : "buffer not in memory";
        }
        // log.debug("free:   {}({})", mem.hashCode(), mem.length);
      }
    }
    catch (Throwable t)
    {
      // don't let an exception leak out.
    }
  }

  /**
   * Native method that tells a native objects (represented by the nativeObj
   * long pointer val) that this JNIMemoryAllocator is being used to allocate
   * it's large blocks of memory.
   * <p>
   * Follow that? No. That's OK... you really don't want to know.
   * </p>
   * 
   * @param nativeObj
   *          A C pointer (a la swig).
   * @param mgr
   *          Us.
   */

  public native static void setAllocator(long nativeObj, JNIMemoryAllocator mgr);

  /**
   * Get the allocate for the underlying native pointer.
   * 
   * @param nativeObj
   *          The native pointer.
   * @return The allocator to use, or null.
   */

  public native static JNIMemoryAllocator getAllocator(long nativeObj);

  /**
   * Are we mirroring all memory allocations from native code in the JVM?
   * 
   * <p>
   * By default if this support is compiled into the Ferry native libraries,
   * large Ferry-using objects will be allocated inside the JVM instead of the
   * C++ heap. This aids in debugging memory leaks, but for high-performance
   * applications it may introduce a potentially significant overhead. So you
   * can turn if off here.
   * </p>
   * <p>
   * That said, we <strong>strongly</strong> recommend you leave it on; by
   * mirroring native memory inside the JVM we get the advantage of having
   * Java's garbage collector know really how much memory it is using, and
   * therefore it does a better job of auto-releasing Ferry objects.
   * </p>
   * <p>
   * If you set this to false, the Java garbage collector won't know that
   * objects allocating large chunks of memory in C++ are being pinned by
   * objects in Java. Since the corresponding Java objects look small to the
   * Garbage Collector, it may not aggressively collect those small objects. As
   * a result you may start seeing OutOfMemoryErrors coming from the native
   * calls.
   * </p>
   * <p>
   * Pretty much THE ONLY REASON to turn this off is if you've decided you're
   * going to explicitly call {@link RefCounted#delete()} when you're done a
   * Ferried Java object and as a result want more control yourself.
   * </p>
   * 
   * @return true if we're mirroring native memory in the JVM. Otherwise false.
   */

  public static boolean isMirroringNativeMemoryInJVM()
  {
    return FerryJNI.isMirroringNativeMemoryInJVM();
  }

  /**
   * Set whether or not we mirror native memory allocations of large objects
   * inside the JVM.
   * 
   * @param value
   *          true for yes; false for no.
   * 
   * @see #isMirroringNativeMemoryInJVM()
   */

  public static void setMirroringNativeMemoryInJVM(boolean value)
  {
    FerryJNI.setMirroringNativeMemoryInJVM(value);
  }
}
