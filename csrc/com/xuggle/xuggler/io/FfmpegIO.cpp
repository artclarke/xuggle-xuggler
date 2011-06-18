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

#include <com/xuggle/ferry/JNIHelper.h>
#include <com/xuggle/xuggler/io/FfmpegIO.h>
#include <com/xuggle/xuggler/io/FfmpegIncludes.h>
#include <com/xuggle/xuggler/io/URLProtocolManager.h>
extern "C"
{
  // FFmpeg put these into the private API in libavformat/url.h but
  // we still need it.  Very dangerous this.  Should replace with right API when it returns.
    int ffurl_alloc(URLContext **h, const char *url, int flags);
    int ffurl_connect(URLContext *h);
    int ffurl_open(URLContext **h, const char *url, int flags);
    int ffurl_read(URLContext *h, unsigned char *buf, int size);
    int ffurl_read_complete(URLContext *h, unsigned char *buf, int size);
    int ffurl_write(URLContext *h, const unsigned char *buf, int size);
    int64_t ffurl_seek(URLContext *h, int64_t pos, int whence);
    int ffurl_close(URLContext *h);
    int64_t ffurl_size(URLContext *h);
    int ffurl_get_file_handle(URLContext *h);
    int ffurl_register_protocol(URLProtocol *protocol, int size);
}

using namespace com::xuggle::ferry;
using namespace com::xuggle::xuggler::io;

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *, void *)
{
    /* Because of static initialize in Mac OS, the only safe thing
     * to do here is return the version */
    return com::xuggle::ferry::JNIHelper::sGetJNIVersion();
}


JNIEXPORT void JNICALL Java_com_xuggle_xuggler_io_FfmpegIO_init(JNIEnv *env, jclass)
{
  JavaVM* vm=0;
  if (!com::xuggle::ferry::JNIHelper::sGetVM()) {
    env->GetJavaVM(&vm);
    com::xuggle::ferry::JNIHelper::sSetVM(vm);
    av_register_all();
  }
}

JNIEXPORT jint JNICALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1registerProtocolHandler(
    JNIEnv *jenv, jclass, jstring aProtoName, jobject aProtoMgr)
{
  int retval = -1;
  const char *protoName= NULL;
  protoName = jenv->GetStringUTFChars(aProtoName, NULL);
  if (protoName != NULL)
  {
    URLProtocolManager* mgr = new URLProtocolManager(protoName, aProtoMgr);

    URLProtocol* proto = mgr->getURLProtocol();
    retval = ffurl_register_protocol(proto, sizeof(URLProtocolWrapper));

    jenv->ReleaseStringUTFChars(aProtoName, protoName);
    protoName = NULL;

    // Yes, we deliberately leak the URLProtocolManager...
  }

  return retval;
}

JNIEXPORT jint JNICALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1url_1open(
    JNIEnv * jenv, jclass, jobject handle, jstring url, jint flags)
{
  URLContext*handleVal= NULL;
  const char *nativeURL= NULL;
  jint retval = -1;

  nativeURL = jenv->GetStringUTFChars(url, NULL);
  if (nativeURL != NULL)
  {
    try
    {
      retval = ffurl_open(&handleVal, nativeURL, flags);
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

JNIEXPORT jint JNICALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1url_1read(
    JNIEnv *jenv, jclass, jobject handle, jbyteArray javaBuf, jint buflen)
{
  URLContext*handleVal= NULL;

  jint retval = -1;

  handleVal = (URLContext*)JNIHelper::sGetPointer(handle);

  jbyte *byteArray = jenv->GetByteArrayElements(javaBuf, NULL);
  try
  {
    retval = ffurl_read(handleVal, (unsigned char*)byteArray, buflen);
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

JNIEXPORT jint JNICALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1url_1write(
    JNIEnv *jenv, jclass, jobject handle, jbyteArray javaBuf, jint buflen)
{
  URLContext*handleVal= NULL;

  jint retval = -1;

  handleVal = (URLContext*)JNIHelper::sGetPointer(handle);

  jbyte *byteArray = jenv->GetByteArrayElements(javaBuf, NULL);
  try
  {
    retval = ffurl_write(handleVal, (unsigned char*)byteArray, buflen);
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

JNIEXPORT jlong JNICALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1url_1seek(
    JNIEnv *jenv, jclass, jobject handle, jlong position, jint whence)
{
  URLContext*handleVal= NULL;

  jint retval = -1;

  handleVal = (URLContext*)JNIHelper::sGetPointer(handle);
  try
  {
    retval = ffurl_seek(handleVal, position, whence);
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

JNIEXPORT jint JNICALL Java_com_xuggle_xuggler_io_FfmpegIO_native_1url_1close(
    JNIEnv *jenv, jclass, jobject handle)
{
  URLContext*handleVal= NULL;

  jint retval = -1;

  handleVal = (URLContext*)JNIHelper::sGetPointer(handle);
  try
  {
    retval = ffurl_close(handleVal);
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
