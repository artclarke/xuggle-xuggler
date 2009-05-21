/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated. All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler. If not, see <http://www.gnu.org/licenses/>.
 */

package com.xuggle.ferry;

import java.lang.ref.PhantomReference;
import java.util.concurrent.atomic.AtomicLong;

import com.xuggle.ferry.FerryJNI;

/**
 * Internal Only.
 * <p>
 * This class creates a {@link PhantomReference} that Ferry classes will use for
 * memory management. We do this to avoid relying on finalizers() to keep up and
 * instead make every new native allocation first release any unreachable
 * objects.
 * </p>
 * <p>
 * The thinking here is that a release of a native object is generally faster
 * than any expected operation you're actually trying to do in native code. For
 * example, freeing a block of memory should be orders of magnitude faster than
 * decoding a frame of live video.
 * </p>
 * <p>
 * Don't use this class outside of the Xuggle libraries; it works, but it's a
 * little complicated
 * </p>
 * 
 */
public class JNIReference extends PhantomReference<Object>
{
  private AtomicLong mSwigCPtr = new AtomicLong(0);

  // This memory manager will outlive the Java object we're referencing; that
  // means this class will sometimes show up as a potential leak, but trust us
  // here; ignore all the refs to here and see who else is holding the ref to
  // the JNIMemoryAllocator and that's your likely leak culprit (if we didn't
  // do this, then you'd get no indication of who is leaking your native object
  // so stop complaining now).
  private volatile JNIMemoryAllocator mMemAllocator;

  private JNIReference(Object aReferent, long nativeVal)
  {
    super(aReferent, JNIMemoryManager.getMgr().getQueue());
    mSwigCPtr.set(nativeVal);
    if (FerryJNI.RefCounted_getCurrentRefCount(nativeVal, null) == 1)
    {
      // it's only safe to set the allocator if you're the only reference
      // holder. Otherwise
      // we default to having a null allocator, and allocations are anonymous,
      // but at least
      // won't crash under heavy multi-threaded situations.
      mMemAllocator = new JNIMemoryAllocator();
      // we are the only owner of this object; tell it we're the object it can
      // allocate from
      JNIMemoryAllocator.setAllocator(nativeVal, mMemAllocator);
    }
    else
    {

      // This creates a Strong reference to the allocator currently being used
      // so that if the Java proxy object the native object depended upon goes
      // away, the memory manager it was using stays around until this reference
      // is killed
      mMemAllocator = JNIMemoryAllocator.getAllocator(nativeVal);
    }
  }

  /**
   * Returns the {@link JNIMemoryManager} we're using.
   * 
   * @return the manager
   */
  static JNIMemoryManager getMgr()
  {
    return JNIMemoryManager.getMgr();
  }

  static JNIReference createReference(Object aReferent, long swigCPtr)
  {
    // Clear out any pending native objects
    JNIMemoryManager.getMgr().gc();

    JNIReference ref = new JNIReference(aReferent, swigCPtr);
    JNIMemoryManager.getMgr().addReference(ref);
    // log.debug("added   : {}; {}", ref, swigCPtr);
    return ref;
  }

  void delete()
  {
    long swigPtr = 0;
    // acquire lock for minimum time
    swigPtr = mSwigCPtr.getAndSet(0);
    if (swigPtr != 0)
    {
      // log.debug("deleting: {}; {}", this, mSwigCPtr);
      FerryJNI.RefCounted_release(swigPtr, null);
      // Free the memory manager we use
      mMemAllocator = null;
      JNIMemoryManager.getMgr().removeReference(this);
    }
  }
}
