/*******************************************************************************
 * Copyright (c) 2008, 2010 Xuggle Inc.  All rights reserved.
 *  
 * This file is part of Xuggle-Xuggler-Main.
 *
 * Xuggle-Xuggler-Main is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xuggle-Xuggler-Main is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Xuggle-Xuggler-Main.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/

#include <exception>
#include <stdexcept>

#include <string.h>

#include <com/xuggle/ferry/JNIHelper.h>

#include <com/xuggle/xuggler/io/JavaURLProtocolManager.h>

namespace com { namespace xuggle { namespace xuggler { namespace io
{
using namespace std;
using namespace com::xuggle::ferry;

JavaURLProtocolManager*
JavaURLProtocolManager :: registerProtocol(const char *aProtocolName, jobject aJavaProtoMgr)
{
  JavaURLProtocolManager* mgr = new JavaURLProtocolManager(aProtocolName, aJavaProtoMgr);
  return dynamic_cast<JavaURLProtocolManager*>(URLProtocolManager::registerProtocol(mgr));
}


JavaURLProtocolManager :: JavaURLProtocolManager(
    const char * aProtocolName,
    jobject aJavaProtoMgr) : URLProtocolManager(aProtocolName)
{
  this->cacheJavaMethods(aJavaProtoMgr);
}

JavaURLProtocolManager :: ~JavaURLProtocolManager()
{
  if (mJavaURLProtocolManager_class)
  {
    JNIHelper::sDeleteGlobalRef(mJavaURLProtocolManager_class);
    mJavaURLProtocolManager_class = NULL;
  }
  if (mJavaProtoMgr)
  {
    JNIHelper::sDeleteGlobalRef((jobject)mJavaProtoMgr);
    mJavaProtoMgr = NULL;
  }
}

void
JavaURLProtocolManager :: cacheJavaMethods(jobject aProtoMgr)
{
  // Now, let's cache the commonly used classes.
  JNIEnv *env=JNIHelper::sGetEnv();

  mJavaProtoMgr = env->NewGlobalRef(aProtoMgr);

  // I don't check for NULL here because... well I don.t
  jclass cls=env->GetObjectClass(aProtoMgr);

  // Keep a reference around
  mJavaURLProtocolManager_class=(jclass)env->NewGlobalRef(cls);

  // Now, look for our get and set mehods.
  mJavaURLProtocolManager_getHandler_mid
      = env->GetMethodID(
          cls,
          "getHandler",
          "(Ljava/lang/String;I)Lcom/xuggle/xuggler/io/IURLProtocolHandler;");

}

JavaURLProtocolHandler *
JavaURLProtocolManager :: getHandler(const char *url, int flags)
{
  jstring jUrl= NULL;
  jobject jHandler= NULL;
  JavaURLProtocolHandler* retval = NULL;

  JNIEnv * env = JNIHelper::sGetEnv();

  try
  {
    // we need to use our manager to get a URLProtocolHandler object to pass
    // into the native URLProtocolHandler
    // now we need to allocate a new ProtocolHandler and attach it to this
    // context
    jUrl = env->NewStringUTF(url);
    if (!jUrl) throw invalid_argument("should throw bad alloc here...");
    jHandler = env->CallObjectMethod(mJavaProtoMgr,
        mJavaURLProtocolManager_getHandler_mid,
        jUrl,
        flags);
    if (!jHandler) throw invalid_argument("couldn't find handler for protocol");

    retval = new JavaURLProtocolHandler(this,
        jHandler);

  }
  catch (std::exception & e)
  {

  }
  // delete your local refs, as this is a running inside another library that likely
  // is not returning to Java soon.
  if (jUrl)
    env->DeleteLocalRef(jUrl);
  if (jHandler)
    env->DeleteLocalRef(jHandler);
  jUrl = NULL;
  jHandler = NULL;

  return retval;
}
}}}}
