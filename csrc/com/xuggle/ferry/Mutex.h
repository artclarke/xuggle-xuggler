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
#ifndef MUTEX_H_
#define MUTEX_H_

#include <jni.h>
#include <com/xuggle/ferry/RefCounted.h>

namespace com { namespace xuggle { namespace ferry {

  /**
   * Internal Only.
   * <p>
   * This object exists so that Native code can get access to 
   * thread safe locking objects if they need it.
   * </p><p>
   * Implements a blocking Mutually-Exclusive lock
   * by wrapping a Java lock.
   * </p><p>
   * If not running inside Java, lock() and unlock()
   * are NO-OPs.
   * </p>
   */
  class VS_API_FERRY Mutex : public RefCounted
  {
  public:
    static Mutex * make();

    void lock();
    void unlock();
  protected:
    Mutex();
    virtual ~Mutex();
  private:
    jobject mLock;
    volatile int64_t mSpinCount;

    static bool init();
    static void initJavaBindings(JavaVM* vm, void* closure);
    static bool mInitialized;
    static jclass mClass;
    static jmethodID mConstructorMethod;


  };

}}}

#endif /*MUTEX_H_*/
