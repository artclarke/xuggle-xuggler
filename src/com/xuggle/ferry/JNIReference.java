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

import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.util.concurrent.atomic.AtomicLong;

import com.xuggle.ferry.FerryJNI;

/**
 * Returned by {@link IBuffer#getByteBuffer(int, int, java.util.concurrent.atomic.AtomicReference)}
 * for users that want to explicitly manage when the returned {@link java.nio.ByteBuffer}
 * is released.
 * <p>
 * This class creates a {@link WeakReference} that Ferry classes will use for
 * memory management. We do this to avoid relying on Java's finalizer thread to
 * keep up and instead make every new native allocation first release any
 * unreachable objects.
 * </p><p>
 * Most times these objects are managed behind the scenes when you
 * call {@link RefCounted#delete()}.  But when we return
 * {@link java.nio.ByteBuffer} objects, there is no equivalent of
 * delete(), so this object can be used if you want to explicitly control
 * when the {@link ByteBuffer}'s underlying native memory is freed.
 * </p>
 * 
 */
public class JNIReference extends WeakReference<Object>
{
  private AtomicLong mSwigCPtr = new AtomicLong(0);

  // This memory manager will outlive the Java object we're referencing; that
  // means this class will sometimes show up as a potential leak, but trust us
  // here; ignore all the refs to here and see who else is holding the ref to
  // the JNIMemoryAllocator and that's your likely leak culprit (if we didn't
  // do this, then you'd get no indication of who is leaking your native object
  // so stop complaining now).
  private volatile JNIMemoryAllocator mMemAllocator;

  private final boolean mIsFerryObject;
  
  private JNIReference(Object aReferent, long nativeVal, boolean isFerry)
  {
    super(aReferent, JNIMemoryManager.getMgr().getQueue());
    mIsFerryObject = isFerry;
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

  static JNIReference createReference(Object aReferent, long swigCPtr,
      boolean isFerry)
  {
    // Clear out any pending native objects
    JNIMemoryManager.getMgr().gc();

    JNIReference ref = new JNIReference(aReferent, swigCPtr, isFerry);
    JNIMemoryManager.getMgr().addReference(ref);
    //System.err.println("added  : "+ref+"; "+swigCPtr+" ("+isFerry+")");
    return ref;
  }
  static JNIReference createReference(Object aReferent, long swigCPtr)
  {
    return createReference(aReferent, swigCPtr, true);
  }
  static JNIReference createNonFerryReference(Object aReferent, long swigCPtr)
  {
    return createReference(aReferent, swigCPtr, false);
  }


  /**
   * Explicitly deletes the underlying native storage used by
   * the object this object references.  The underlying native
   * object is now no long valid, and attempts to use it could
   * cause unspecified behavior.
   * 
   */
  public void delete()
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

  boolean isFerryObject()
  {
    return mIsFerryObject;
  }
}
