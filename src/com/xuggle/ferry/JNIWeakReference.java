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

import java.lang.ref.WeakReference;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.ferry.FerryJNI;

/**
 * Internal Only.
 * <p>
 * This class creates a WeakReference that Ferry classes will use for
 * memory management.  We do this to avoid relying on finalizers() to keep up
 * and instead make every new native allocation first release any unreachable
 * objects.
 * </p><p>
 * The thinking here is that a release of a native object is generally faster than
 * any expected operation you're actually trying to do in native code.  For example,
 * freeing a block of memory should be orders of magnitude faster than decoding
 * a frame of live video.
 * </p><p>
 * Don't use this class outside of the Xuggle libraries; it works, but it's
 * a little complicated
 * </p>
 * 
 */
public class JNIWeakReference extends WeakReference<Object>
{
  @SuppressWarnings("unused")
  private static final Logger log = LoggerFactory.getLogger(JNIWeakReference.class);

  private long mSwigCPtr;
  // This memory manager will outlive the Java object we're referencing; that
  // means this class will sometimes show up as a potential leak, but trust us
  // here; ignore all the refs to hear and see who else is holding the ref to
  // the JNIMemoryAllocator and that's your likely leak culprit (if we didn't
  // do this, then you'd get no indication of who is leaking your native object
  // so stop complaining now).
  private JNIMemoryAllocator mMemAllocator;

  private JNIWeakReference(Object aReferent, long nativeVal)
  {
    super(aReferent, mMgr.getQueue());
    mSwigCPtr = nativeVal;
    if (FerryJNI.RefCounted_getCurrentRefCount(nativeVal, null) == 1)
    {
      // it's only safe to set the allocator if you're the only reference holder.  Otherwise
      // we default to having a null allocator, and allocations are anonymous, but at least
      // won't crash under heavy multi-threaded situations.
      mMemAllocator = new JNIMemoryAllocator();
      // we are the only owner of this object; tell it we're the object it can allocate from
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
  static private JNIMemoryManager mMgr = new JNIMemoryManager();
  static public JNIMemoryManager getMgr()
  {
    return mMgr;
  }
  
  static public JNIWeakReference createReference(Object aReferent, long swigCPtr)
  {
    // Clear out any pending native objects
    mMgr.gc();

    JNIWeakReference ref = new JNIWeakReference(aReferent, swigCPtr);
    mMgr.addReference(ref);
    //log.debug("added   : {}; {}", ref, swigCPtr);
    return ref;
  }
  
  public synchronized void delete()
  {
    if (mSwigCPtr != 0)
    {
      //log.debug("deleting: {}; {}", this, mSwigCPtr);
      FerryJNI.RefCounted_release(mSwigCPtr, null);
      // Free the memory manager we use
      mMemAllocator = null;
      mSwigCPtr = 0;
      mMgr.removeReference(this);
    }
  }
}
