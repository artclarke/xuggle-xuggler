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
#include "Mutex.h"
#include "JNIHelper.h"
#include <cstdio>

namespace com { namespace xuggle { namespace ferry {

  jclass Mutex :: mClass = 0;
  jmethodID Mutex :: mConstructorMethod = 0;

  bool Mutex :: mInitialized = false;

  Mutex :: Mutex()
  {
    mLock = 0;
  }

  Mutex :: ~Mutex()
  {
    JNIEnv *env=JNIHelper::sGetEnv();
    if (env)
    {
      if (mLock)
        env->DeleteGlobalRef(mLock);
    }
    mLock = 0;
  }

  bool
  Mutex :: init()
  {
    if (!mInitialized) {
      JNIHelper::sRegisterInitializationCallback(initJavaBindings,0);
      mInitialized = true;
    }
    return mInitialized;
  }

  void
  Mutex :: initJavaBindings(JavaVM*, void*)
  {
    JNIEnv *env=JNIHelper::sGetEnv();
    if (env && !mClass)
    {
      // We're inside a JVM, let's get to work.
      jclass cls=env->FindClass("java/lang/Object");
      if (cls)
      {
        // and find all our methods

        mConstructorMethod = env->GetMethodID(cls,
            "<init>", "()V");

        // keep a reference around
        mClass = (jclass)env->NewWeakGlobalRef(cls);
      }
    }
  }

  void
  Mutex :: lock()
  {
    if (!mInitialized)
      Mutex::init();

    if (mLock)
    {
      JNIEnv *env=JNIHelper::sGetEnv();
      if (env)
      {
        //fprintf(stderr, " PRE-ENTER: %p\n", mLock);
        if (env->MonitorEnter(mLock) != JNI_OK)
          fprintf(stderr, "Could not enter lock: %p\n", mLock);
        //fprintf(stderr, "POST-ENTER: %p\n", mLock);
      }
    }
  }

  void
  Mutex :: unlock()
  {
    if (!mInitialized)
      Mutex::init();

    if (mLock)
    {
      JNIEnv *env=JNIHelper::sGetEnv();
      if (env)
      {
        //fprintf(stderr, "  PRE-EXIT: %p\n", mLock);
        if (env->MonitorExit(mLock) != JNI_OK)
          fprintf(stderr, "Could not exit lock: %p\n", mLock);
        //fprintf(stderr, " POST-EXIT: %p\n", mLock);
      }
    }
  }

  Mutex*
  Mutex :: make()
  {
    if (!mInitialized)
      Mutex::init();

    Mutex* retval = 0;
    JNIEnv *env=JNIHelper::sGetEnv();

    if (env) {
      if (!mClass) {
        // we're being called BEFORE our initialization
        // callback was called, but during someone
        // elses initialization route, so we can be
        // sure we have a JVM.
        Mutex::initJavaBindings(
            JNIHelper::sGetVM(),
            0);
      }
      if (mClass && mConstructorMethod)
      {
        retval = new Mutex();
        if (retval)
        {
          // we're running within Java
          jobject newValue = env->NewObject(mClass, mConstructorMethod);
          if (newValue)
          {
            retval->mLock = env->NewGlobalRef(newValue);
            env->DeleteLocalRef(newValue);
            newValue = 0;
          } else {
            delete retval;
            retval = 0;
          }
        }
      }
    }
    if (retval)
      retval->acquire();
    return retval;
  }
}}}
