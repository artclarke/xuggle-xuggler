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

#include <stdexcept>
// For EINTR
#include <errno.h>

#include <com/xuggle/ferry/JNIHelper.h>
#include <com/xuggle/ferry/Logger.h>

#include <com/xuggle/xuggler/io/JavaURLProtocolHandler.h>
#include <com/xuggle/xuggler/io/JavaURLProtocolManager.h>

using namespace com::xuggle::ferry;

VS_LOG_SETUP(VS_CPP_PACKAGE);

#define XUGGLER_CHECK_INTERRUPT(retval, __COND__) do { \
    if (__COND__) { \
      JNIHelper* helper = JNIHelper::getHelper(); \
      if (helper && helper->isInterrupted()) \
        (retval) = EINTR > 0 ? -EINTR : EINTR; \
        } \
} while(0)

static void
JavaURLProtocolHandler_CheckException(JNIEnv *env)
{
  if (env) {
    jthrowable exception = env->ExceptionOccurred();
    if (exception)
    {
      JNIHelper *helper = JNIHelper::getHelper();
      if (helper &&
          !helper->isInterrupted() &&
          helper->isInterruptedException(exception))
        // preserve the interrupt
        helper->interrupt();
      
      //env->ExceptionDescribe();
      
      // Clear the pending exception.  This is necessary because
      // other JNI methods may need to be called afterwards to 
      // clean up the IO condition.  This means our URL
      // protocolhandler is eating an exception, but them's the breaks.
      env->ExceptionClear();
      
      // free the local reference
      env->DeleteLocalRef(exception);
      
      // and throw back to caller
      throw std::runtime_error("got java exception");
    }
  }
}
namespace com { namespace xuggle{ namespace xuggler { namespace io
{

JavaURLProtocolHandler :: JavaURLProtocolHandler(
    JavaURLProtocolManager* mgr,
    jobject aJavaProtocolHandler) : URLProtocolHandler(mgr)
{
  cacheJavaMethods(aJavaProtocolHandler);
}

JavaURLProtocolHandler :: ~JavaURLProtocolHandler()
{
  if (mJavaProtoHandler)
  {
    JNIHelper::sDeleteGlobalRef(mJavaProtoHandler);
    mJavaProtoHandler = NULL;
    // now all of our other Java refers are invalid as well.
  }
}
void
JavaURLProtocolHandler :: cacheJavaMethods(jobject aProtoHandler)
{
  JNIEnv *env = JNIHelper::sGetEnv();
  if (!env)
    return;

  mJavaProtoHandler = env->NewGlobalRef(aProtoHandler);

  // I don't make GlobalRefs to all of this because
  // (as I understand it) these method referenes will remain
  // valid as long as the mJavaProtoHandler lives.
  // it should also be good across threads, but to be honest,
  // this class does not support a protocol handler being
  // used on multiple threads.
  jclass cls = env->GetObjectClass(aProtoHandler);

  mJavaUrlOpen_mid
      = env->GetMethodID(cls, "open", "(Ljava/lang/String;I)I");
  mJavaUrlClose_mid = env->GetMethodID(cls, "close", "()I");
  mJavaUrlRead_mid = env->GetMethodID(cls, "read", "([BI)I");
  mJavaUrlWrite_mid = env->GetMethodID(cls, "write", "([BI)I");
  mJavaUrlSeek_mid = env->GetMethodID(cls, "seek", "(JI)J");
  mJavaUrlIsStreamed_mid = env->GetMethodID(cls, "isStreamed",
      "(Ljava/lang/String;I)Z");

}

int
JavaURLProtocolHandler :: url_open(const char *url, int flags)
{
  JNIEnv *env = JNIHelper::sGetEnv();
  if (!env)
    return -1;
  int retval = -1;
  jstring jUrl = 0;
  try
  {
    JavaURLProtocolHandler_CheckException(env);
    jUrl = env->NewStringUTF(url);
    JavaURLProtocolHandler_CheckException(env);
    retval = env->CallIntMethod(mJavaProtoHandler, mJavaUrlOpen_mid, jUrl,
        flags);
    JavaURLProtocolHandler_CheckException(env);
    // delete your local refs, as this is a running inside another library that likely
    // is not returning to Java soon.
  }
  catch (std::exception & e)
  {
    VS_LOG_TRACE("%s", e.what());
    retval = -1;
  }
  catch (...)
  {
    VS_LOG_DEBUG("Got unknown exception");
    retval = -1;
  }
  if (jUrl)
    env->DeleteLocalRef(jUrl);

  XUGGLER_CHECK_INTERRUPT(retval, 1);
  return retval;
}

int
JavaURLProtocolHandler :: url_close()
{
  JNIEnv *env = JNIHelper::sGetEnv();
  if (!env)
    return -1;

  int retval = -1;
  try
  {
    JavaURLProtocolHandler_CheckException(env);
    retval = env->CallIntMethod(mJavaProtoHandler, mJavaUrlClose_mid);
    JavaURLProtocolHandler_CheckException(env);
  }
  catch (std::exception & e)
  {
    VS_LOG_TRACE("%s", e.what());
    retval = -1;
  }
  catch (...)
  {
    VS_LOG_DEBUG("Got unknown exception");
    retval = -1;
  }
  XUGGLER_CHECK_INTERRUPT(retval, 1);

  return retval;
}

int64_t
JavaURLProtocolHandler :: url_seek(int64_t position,
    int whence)
{
  JNIEnv *env = JNIHelper::sGetEnv();
  if (!env)
    return -1;

  int64_t retval = -1;

  try
  {
    JavaURLProtocolHandler_CheckException(env);
    retval = env->CallLongMethod(mJavaProtoHandler, mJavaUrlSeek_mid, position,
      whence);
    JavaURLProtocolHandler_CheckException(env);
  }
  catch (std::exception & e)
  {
    VS_LOG_TRACE("%s", e.what());
    retval = -1;
  }
  catch (...)
  {
    VS_LOG_DEBUG("Got unknown exception");
    retval = -1;
  }
  XUGGLER_CHECK_INTERRUPT(retval, 1);
  return retval;
}

int
JavaURLProtocolHandler :: url_read(unsigned char* buf, int size)
{
  JNIEnv *env = JNIHelper::sGetEnv();
  if (!env)
    return -1;

  int retval = -1;
  jbyteArray byteArray = 0;
  
  try
  {
    JavaURLProtocolHandler_CheckException(env);
    byteArray = env->NewByteArray(size);
    JavaURLProtocolHandler_CheckException(env);
    // read into the Java byte array
    if (byteArray)
    {
      retval = env->CallIntMethod(mJavaProtoHandler, mJavaUrlRead_mid,
          byteArray, size);
      JavaURLProtocolHandler_CheckException(env);
    }
    // now, copy into the C array, but only up to retval.
    if (retval > 0)
    {
      env->GetByteArrayRegion(byteArray, 0, retval, (jbyte*)buf);
      JavaURLProtocolHandler_CheckException(env);
    }
  }
  catch (std::exception& e)
  {
    VS_LOG_TRACE("%s", e.what());
    retval = -1;
  }
  catch (...)
  {
    VS_LOG_DEBUG("Got unknown exception");
    retval = -1;
  }
  // delete your local refs, as this is a running inside another library that likely
  // is not returning to Java soon.
  if (byteArray)
    env->DeleteLocalRef(byteArray);
  XUGGLER_CHECK_INTERRUPT(retval, retval < 0 || retval != size);
  return retval;
}

int
JavaURLProtocolHandler :: url_write(const unsigned char* buf, int size)
{
  JNIEnv *env = JNIHelper::sGetEnv();
  if (!env)
    return -1;

  int retval = -1;
  jbyteArray byteArray = 0;

  try
  {
    JavaURLProtocolHandler_CheckException(env);
    byteArray = env->NewByteArray(size);
    JavaURLProtocolHandler_CheckException(env);

    // copy the data passed into the new java byteArray
    if (byteArray)
    {
      env->SetByteArrayRegion(byteArray, 0, size, (jbyte*)buf);
      JavaURLProtocolHandler_CheckException(env);

      // write from the Java byte array
      retval = env->CallIntMethod(mJavaProtoHandler, mJavaUrlWrite_mid,
          byteArray, size);
      JavaURLProtocolHandler_CheckException(env);
    }
  }
  catch (std::exception & e)
  {
    VS_LOG_TRACE("%s", e.what());
    retval = -1;
  }
  catch (...)
  {
    VS_LOG_DEBUG("Got unknown exception");
    retval = -1;
  }

  // delete your local refs, as this is a running inside another library that likely
  // is not returning to Java soon.
  if (byteArray)
    env->DeleteLocalRef(byteArray);
  XUGGLER_CHECK_INTERRUPT(retval, retval < 0 || retval != size);

  return retval;
}

URLProtocolHandler::SeekableFlags
JavaURLProtocolHandler :: url_seekflags( const char* url, int flags)
{
  URLProtocolHandler::SeekableFlags retFlags = SK_NOT_SEEKABLE;
  JNIEnv *env = JNIHelper::sGetEnv();
  if (!env)
    return SK_NOT_SEEKABLE;
  jboolean url_streaming;
  jstring jUrl = 0;
  try
  {
    JavaURLProtocolHandler_CheckException(env);
    jUrl = env->NewStringUTF(url);
    JavaURLProtocolHandler_CheckException(env);
    // delete your local refs, as this is a running inside another library that likely
    // is not returning to Java soon.
    url_streaming = env->CallBooleanMethod(mJavaProtoHandler,
        mJavaUrlIsStreamed_mid, jUrl, flags);
    JavaURLProtocolHandler_CheckException(env);
    if (!url_streaming)
      retFlags = SK_SEEKABLE_NORMAL;

  }
  catch (std::exception & e)
  {
    VS_LOG_TRACE("%s", e.what());
  }
  catch (...)
  {
    VS_LOG_DEBUG("Got unknown exception");
  }
  if (jUrl)
    env->DeleteLocalRef(jUrl);

  return retFlags;
}

}}}}
