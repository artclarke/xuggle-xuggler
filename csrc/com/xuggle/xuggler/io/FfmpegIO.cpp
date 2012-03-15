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
#include <com/xuggle/xuggler/io/FfmpegIO.h>
#include <com/xuggle/xuggler/io/JavaURLProtocolManager.h>

using namespace com::xuggle::ferry;
using namespace com::xuggle::xuggler::io;

#define XUGGLER_CHECK_INTERRUPT(retval, __COND__) do { \
    if (__COND__) { \
      JNIHelper* helper = JNIHelper::getHelper(); \
      if (helper && helper->isInterrupted()) \
        (retval) = EINTR > 0 ? -EINTR : EINTR; \
        } \
} while(0)

VS_API_XUGGLER_IO void VS_API_CALL Java_com_xuggle_xuggler_io_FfmpegIO_init(JNIEnv *env, jclass)
{
  JavaVM* vm=0;
  if (!com::xuggle::ferry::JNIHelper::sGetVM()) {
    env->GetJavaVM(&vm);
    com::xuggle::ferry::JNIHelper::sSetVM(vm);
  }
}

VS_API_XUGGLER_IO jint VS_API_CALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1registerProtocolHandler(
    JNIEnv *jenv, jclass, jstring aProtoName, jobject aProtoMgr)
{
  int retval = -1;
  const char *protoName= NULL;
  try {
    protoName = jenv->GetStringUTFChars(aProtoName, NULL);
    if (protoName != NULL)
    {
      // Yes, we deliberately leak the URLProtocolManager...
      JavaURLProtocolManager::registerProtocol(protoName, aProtoMgr);
      retval = 0;
    }
  }
  catch(std::exception & e)
  {
    // we don't let a native exception override a java exception
    if (!jenv->ExceptionCheck())
    {
      jclass cls=jenv->FindClass("java/lang/RuntimeException");
      if (cls)
        jenv->ThrowNew(cls, e.what());
    }
    retval = -1;
  }
  catch(...)
  {
    // we don't let a native exception override a java exception
    if (!jenv->ExceptionCheck())
    {
      jclass cls=jenv->FindClass("java/lang/RuntimeException");
      if (cls)
        jenv->ThrowNew(cls, "Unhandled and unknown native exception");
    }
    retval = -1;
  }
  if (protoName != NULL) {
    jenv->ReleaseStringUTFChars(aProtoName, protoName);
    protoName = NULL;
  }
  return retval;
}

VS_API_XUGGLER_IO jint VS_API_CALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1url_1open(
    JNIEnv * jenv, jclass, jobject handle, jstring url, jint flags)
{
  URLProtocolHandler*handleVal= NULL;
  const char *nativeURL= NULL;
  jint retval = -1;

  nativeURL = jenv->GetStringUTFChars(url, NULL);
  if (nativeURL != NULL)
  {
    try
    {
      handleVal = URLProtocolManager::findHandler(nativeURL, flags, 0);
      if (!handleVal)
        throw std::runtime_error("could not find protocol manager for url");
      retval = handleVal->url_open(nativeURL, flags);
      XUGGLER_CHECK_INTERRUPT(retval, 1);
    }
    catch(std::exception & e)
    {
      // we don't let a native exception override a java exception
      if (!jenv->ExceptionCheck())
      {
        jclass cls=jenv->FindClass("java/lang/RuntimeException");
        jenv->ThrowNew(cls, e.what());
      }
      retval = -1;
    }
    catch(...)
    {
      // we don't let a native exception override a java exception
      if (!jenv->ExceptionCheck())
      {
        jclass cls=jenv->FindClass("java/lang/RuntimeException");
        jenv->ThrowNew(cls, "Unhandled and unknown native exception");
      }
      retval = -1;
    }

    // regardless of what the function returns, set the pointer
    JNIHelper::sSetPointer(handle, handleVal);
  }

  // free the string
  if (nativeURL != NULL)
  {
    jenv->ReleaseStringUTFChars(url, nativeURL);
    nativeURL = NULL;
  }
  return retval;
}

VS_API_XUGGLER_IO jint VS_API_CALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1url_1read(
    JNIEnv *jenv, jclass, jobject handle, jbyteArray javaBuf, jint buflen)
{
  URLProtocolHandler* handleVal= NULL;

  jint retval = -1;

  handleVal = (URLProtocolHandler*)JNIHelper::sGetPointer(handle);

  jbyte *byteArray = jenv->GetByteArrayElements(javaBuf, NULL);
  try
  {
    if (handleVal)
      retval = handleVal->url_read((uint8_t*)byteArray, buflen);
    XUGGLER_CHECK_INTERRUPT(retval, retval < 0 || retval < buflen);
  }
  catch(std::exception & e)
  {
    // we don't let a native exception override a java exception
    if (!jenv->ExceptionCheck())
    {
      jclass cls=jenv->FindClass("java/lang/RuntimeException");
      jenv->ThrowNew(cls, e.what());
    }
    retval = -1;
  }
  catch(...)
  {
    // we don't let a native exception override a java exception
    if (!jenv->ExceptionCheck())
    {
      jclass cls=jenv->FindClass("java/lang/RuntimeException");
      jenv->ThrowNew(cls, "Unhandled and unknown native exception");
    }
    retval = -1;
  }
  jenv->ReleaseByteArrayElements(javaBuf, byteArray, 0);
  return retval;
}

VS_API_XUGGLER_IO jint VS_API_CALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1url_1write(
    JNIEnv *jenv, jclass, jobject handle, jbyteArray javaBuf, jint buflen)
{
  URLProtocolHandler* handleVal= NULL;

  jint retval = -1;

  handleVal = (URLProtocolHandler*)JNIHelper::sGetPointer(handle);

  jbyte *byteArray = jenv->GetByteArrayElements(javaBuf, NULL);
  try
  {
    if (handleVal)
      retval = handleVal->url_write((uint8_t*)byteArray, buflen);
    XUGGLER_CHECK_INTERRUPT(retval, retval < 0 || retval != buflen);
  }
  catch(std::exception & e)
  {
    // we don't let a native exception override a java exception
    if (!jenv->ExceptionCheck())
    {
      jclass cls=jenv->FindClass("java/lang/RuntimeException");
      jenv->ThrowNew(cls, e.what());
    }
    retval = -1;
  }
  catch(...)
  {
    // we don't let a native exception override a java exception
    if (!jenv->ExceptionCheck())
    {
      jclass cls=jenv->FindClass("java/lang/RuntimeException");
      jenv->ThrowNew(cls, "Unhandled and unknown native exception");
    }
    retval = -1;
  }
  jenv->ReleaseByteArrayElements(javaBuf, byteArray, 0);
  return retval;
}

VS_API_XUGGLER_IO jlong VS_API_CALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1url_1seek(
    JNIEnv *jenv, jclass, jobject handle, jlong position, jint whence)
{
  URLProtocolHandler* handleVal= NULL;

  jint retval = -1;

  handleVal = (URLProtocolHandler*)JNIHelper::sGetPointer(handle);
  try
  {
    if (handleVal)
      retval = handleVal->url_seek(position, whence);
    XUGGLER_CHECK_INTERRUPT(retval, 1);
  }
  catch(std::exception & e)
  {
    // we don't let a native exception override a java exception
    if (!jenv->ExceptionCheck())
    {
      jclass cls=jenv->FindClass("java/lang/RuntimeException");
      jenv->ThrowNew(cls, e.what());
    }
    retval = -1;
  }
  catch(...)
  {
    // we don't let a native exception override a java exception
    if (!jenv->ExceptionCheck())
    {
      jclass cls=jenv->FindClass("java/lang/RuntimeException");
      jenv->ThrowNew(cls, "Unhandled and unknown native exception");
    }
    retval = -1;
  }
  return retval;
}

VS_API_XUGGLER_IO jint VS_API_CALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1url_1close(
    JNIEnv *jenv, jclass, jobject handle)
{
  URLProtocolHandler* handleVal= NULL;

  jint retval = -1;

  handleVal = (URLProtocolHandler*)JNIHelper::sGetPointer(handle);
  try
  {
    if (handleVal)
      retval = handleVal->url_close();
    XUGGLER_CHECK_INTERRUPT(retval, 1);
  }
  catch(std::exception & e)
  {
    // we don't let a native exception override a java exception
    if (!jenv->ExceptionCheck())
    {
      jclass cls=jenv->FindClass("java/lang/RuntimeException");
      jenv->ThrowNew(cls, e.what());
    }
    retval = -1;
  }
  catch(...)
  {
    // we don't let a native exception override a java exception
    if (!jenv->ExceptionCheck())
    {
      jclass cls=jenv->FindClass("java/lang/RuntimeException");
      jenv->ThrowNew(cls, "Unhandled and unknown native exception");
    }
    retval = -1;
  }
  return retval;
}
