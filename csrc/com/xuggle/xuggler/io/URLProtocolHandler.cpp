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

#include <stdexcept>

#include <com/xuggle/ferry/JNIHelper.h>
#include <com/xuggle/ferry/Logger.h>

#include <com/xuggle/xuggler/io/URLProtocolHandler.h>

using namespace com::xuggle::ferry;

VS_LOG_SETUP(VS_CPP_PACKAGE);

static void URLProtocolHandler_CheckException(JNIEnv *env)
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
      
      // don't clear the exception -- the otherside of the
      // JNI boundary should re-raise.
      // env->ExceptionClear();
      
      // free the local reference
      env->DeleteLocalRef(exception);
      
      // and throw back to caller
      throw std::runtime_error("got java exception");
    }
  }
}
namespace com { namespace xuggle{ namespace xuggler { namespace io
{
URLProtocolHandler::URLProtocolHandler(const char * protoName,
    jobject aJavaProtocolHandler)
{
  // We can assume that protoName will outlive this class, so
  // we don't need to copy it.
  mProtoName = protoName;

  cacheJavaMethods(aJavaProtocolHandler);
}

URLProtocolHandler::~URLProtocolHandler()
{
  if (mJavaProtoHandler)
  {
    JNIHelper::sDeleteGlobalRef(mJavaProtoHandler);
    mJavaProtoHandler = NULL;

    // now all of our other Java refers are invalid as well.
  }
}
void URLProtocolHandler::cacheJavaMethods(jobject aProtoHandler)
{
  JNIEnv *env = JNIHelper::sGetEnv();

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

int URLProtocolHandler::url_open(URLContext *h, const char *url, int flags)
{
  JNIEnv *env = JNIHelper::sGetEnv();
  int retval = -1;
  jboolean url_streaming;
  jstring jUrl = 0;
  try
  {
    URLProtocolHandler_CheckException(env);
    jUrl = env->NewStringUTF(url);
    URLProtocolHandler_CheckException(env);
    retval = env->CallIntMethod(mJavaProtoHandler, mJavaUrlOpen_mid, jUrl,
        flags);
    URLProtocolHandler_CheckException(env);
    // delete your local refs, as this is a running inside another library that likely
    // is not returning to Java soon.
    url_streaming = env->CallBooleanMethod(mJavaProtoHandler,
        mJavaUrlIsStreamed_mid, jUrl, flags);
    URLProtocolHandler_CheckException(env);
    if (url_streaming)
      h->is_streamed = 1;
    else
      h->is_streamed = 0;

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

  return retval;
}

int URLProtocolHandler::url_close(URLContext *)
{
  JNIEnv *env = JNIHelper::sGetEnv();
  int retval = -1;
  try
  {
    URLProtocolHandler_CheckException(env);
    retval = env->CallIntMethod(mJavaProtoHandler, mJavaUrlClose_mid);
    URLProtocolHandler_CheckException(env);
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
  return retval;
}

int64_t URLProtocolHandler::url_seek(URLContext *, int64_t position,
    int whence)
{
  JNIEnv *env = JNIHelper::sGetEnv();
  int retval = -1;

  try
  {
    URLProtocolHandler_CheckException(env);
    retval = env->CallLongMethod(mJavaProtoHandler, mJavaUrlSeek_mid, position,
      whence);
    URLProtocolHandler_CheckException(env);
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
  return retval;
}

int URLProtocolHandler::url_read(URLContext *, unsigned char* buf, int size)
{
  JNIEnv *env = JNIHelper::sGetEnv();
  int retval = -1;
  jbyteArray byteArray = 0;
  
  try
  {
    URLProtocolHandler_CheckException(env);
    byteArray = env->NewByteArray(size);
    URLProtocolHandler_CheckException(env);
    // read into the Java byte array
    if (byteArray)
    {
      retval = env->CallIntMethod(mJavaProtoHandler, mJavaUrlRead_mid,
          byteArray, size);
      URLProtocolHandler_CheckException(env);
    }
    // now, copy into the C array, but only up to retval.
    if (retval > 0)
    {
      env->GetByteArrayRegion(byteArray, 0, retval, (jbyte*)buf);
      URLProtocolHandler_CheckException(env);
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
  return retval;
}

int URLProtocolHandler::url_write(URLContext *, unsigned char* buf, int size)
{
  JNIEnv *env = JNIHelper::sGetEnv();
  int retval = -1;
  jbyteArray byteArray = 0;

  try
  {
    URLProtocolHandler_CheckException(env);
    byteArray = env->NewByteArray(size);
    URLProtocolHandler_CheckException(env);

    // copy the data passed into the new java byteArray
    if (byteArray)
    {
      env->SetByteArrayRegion(byteArray, 0, size, (jbyte*)buf);
      URLProtocolHandler_CheckException(env);

      // write from the Java byte array
      retval = env->CallIntMethod(mJavaProtoHandler, mJavaUrlWrite_mid,
          byteArray, size);
      URLProtocolHandler_CheckException(env);
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
  return retval;
}
}}}}
