/*
 * Copyright (c) 2008 by Vlideshow Inc. (a.k.a. The Yard).  All rights reserved.
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

#include <com/xuggle/ferry/JNIHelper.h>
#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/ferry/IBuffer.h>
#include <com/xuggle/ferry/Buffer.h>

#include <stdexcept>

using namespace com::xuggle::ferry;

VS_LOG_SETUP(VS_CPP_PACKAGE);

namespace com { namespace xuggle { namespace ferry
{

  IBuffer::IBuffer()
  {
  }

  IBuffer::~IBuffer()
  {
  }
  
  jobject
  IBuffer :: getByteBuffer(int32_t offset, int32_t length)
  {
    void * buffer = 0;
    jobject retval = 0;
    
    buffer = this->getBytes(offset, length);
    if (buffer)
    {
      JNIEnv *env = JNIHelper::sGetEnv();
      if (env)
      {
        retval = env->NewDirectByteBuffer(buffer, length);
      }
    }
    return retval;
  }

  jbyteArray
  IBuffer :: getByteArray(int32_t offset, int32_t length)
  {
    int8_t * buffer = 0;
    jbyteArray retval = 0;
    
    buffer = static_cast<int8_t*>(this->getBytes(offset, length));
    if (buffer)
    {
      JNIEnv *env = JNIHelper::sGetEnv();
      if (env)
      {
        retval = env->NewByteArray(length);
        if (env->ExceptionCheck())
        {
          if (retval) env->DeleteLocalRef(retval);
          retval = 0;
        }
        if (retval)
        {
          // copy the data into the byte array
          env->SetByteArrayRegion(retval, 0, length, buffer);
          if (env->ExceptionCheck())
          {
            // an error occurred; release our byte array
            // reference and return.
            env->DeleteLocalRef(retval);
            retval = 0;
          }
        }
      }
    }
    return retval;
  }

  IBuffer*
  IBuffer :: make(com::xuggle::ferry::RefCounted* requestor, int32_t bufferSize)
  {
    return Buffer::make(requestor, bufferSize);
  }

  IBuffer*
  IBuffer :: make(com::xuggle::ferry::RefCounted* requestor, jbyteArray buffer, int32_t offset, int32_t length)
  {
    IBuffer* retval = 0;
    try
    {
      JNIEnv* env = JNIHelper::sGetEnv();
      if (!env)
        throw std::runtime_error("could not get java environment");
      
      if (env->ExceptionCheck())
        throw std::runtime_error("pending Java exception");

      if (!buffer)
        throw std::invalid_argument("no byte buffer passed in");
      
      jsize bufSize = env->GetArrayLength(buffer);
      if (env->ExceptionCheck())
        throw std::runtime_error("could not get java byteArray size");

      if (bufSize < offset + length)
        throw std::out_of_range("invalid offset and length");
      
      retval = Buffer::make(requestor, length);
      if (!retval)
        throw std::runtime_error("could not allocate IBuffer");

      jbyte* bytes = static_cast<jbyte*>(retval->getBytes(0, length));
      if (!bytes)
        throw std::bad_alloc();
      
      // now try the copy
      env->GetByteArrayRegion(buffer, offset, length, bytes);
      if (!env)
        throw std::runtime_error("could not copy data into native IBuffer memory");
    }
    catch(std::exception & c)
    {
      VS_REF_RELEASE(retval);
      throw c;
    }
    return retval;

  }

  /**
   * This method is passed as a freefunc to the Buffer object.  Once
   * the IBuffer has no more references to it, this method will be called,
   * and will release the backing java.nio.ByteBuffer object that we got
   * data from.
   */
  static void
  IBuffer_javaDirectFreeFunc(void *, void * closure)
  {
    jobject globalRef = static_cast<jobject>(closure);
    JNIEnv* env = JNIHelper::sGetEnv();
    VS_LOG_TRACE("Freeing %p with Java Env: %p", globalRef, closure);
    if (env && globalRef)
    {
      env->DeleteGlobalRef(globalRef);
    }
  }
  
  IBuffer*
  IBuffer :: make(com::xuggle::ferry::RefCounted* requestor, jNioByteArray directByteBuffer, int32_t offset, int32_t length)
  {
    IBuffer * retval = 0;
    jobject globalRef = 0;
    JNIEnv* env = JNIHelper::sGetEnv();
    try
    {
      if (!env)
        throw std::runtime_error("could not get java environment");
      VS_LOG_TRACE("got env");
      
      if (env->ExceptionCheck())
        throw std::runtime_error("pending Java exception");

      if (!directByteBuffer)
        throw std::invalid_argument("no byte buffer passed in");
      
      jclass byteBufferClass = env->FindClass("java/nio/ByteBuffer");
      if (env->ExceptionCheck() || !byteBufferClass)
        throw std::runtime_error("could not get find java/nio/ByteBuffer class");
      jboolean rightClass = env->IsInstanceOf(directByteBuffer,byteBufferClass);
      env->DeleteLocalRef(byteBufferClass);
      if (env->ExceptionCheck())
        throw std::runtime_error("could not get instanceof passed in object");
      if (!rightClass)
      {
        jclass cls=env->FindClass("java/lang/IllegalArgumentException");
        if (cls)
          env->ThrowNew(cls, "object passed in is not instance of java.nio.ByteBuffer");
        throw std::runtime_error("object not instanceof java.nio.ByteBuffer");
      }
      VS_LOG_TRACE("right instance");      
      // let's figure out if this is a direct buffer
      int32_t availableLength = env->GetDirectBufferCapacity(directByteBuffer);
      VS_LOG_TRACE("Got length: %d", availableLength);      
      if (env->ExceptionCheck())
        throw std::runtime_error("could not get java byteArray size");
      int8_t* javaBuffer = static_cast<int8_t*>(env->GetDirectBufferAddress(directByteBuffer));
      VS_LOG_TRACE("Got data buffer: %p", javaBuffer);
      if (env->ExceptionCheck())
        throw std::runtime_error("could not get java direct byte buffer");
      
      if (availableLength == -1 || !javaBuffer)
      {
        jclass cls=env->FindClass("java/lang/IllegalArgumentException");
        if (cls)
          env->ThrowNew(cls, "object passed in is not instance of java.nio.ByteBuffer or this JVM doesn't allow native code to access direct buffers");
        throw std::runtime_error("object not instanceof java.nio.ByteBuffer");
      }


      if (availableLength < length + offset)
        throw std::runtime_error("not enough data in byte buffer");
      
      // Let's try creating a wrapper around this object.
      // Now, let's get a global reference to remember 
      globalRef = env->NewGlobalRef(directByteBuffer);
      if (env->ExceptionCheck())
        throw std::runtime_error("could not get global reference to passed in byte array");
      VS_LOG_TRACE("Got global ref: %p", globalRef);
            
      retval = Buffer::make(requestor, javaBuffer+offset, length,
          IBuffer_javaDirectFreeFunc, globalRef);
      if (!retval)
        throw std::runtime_error("could not wrap java byte array");
      globalRef = 0;
      VS_LOG_TRACE("Made ibuffer: %p", retval);

    }
    catch (std::exception & c)
    {
      if (env && globalRef)
        env->DeleteGlobalRef(globalRef);
      globalRef = 0;
      VS_REF_RELEASE(retval);
      throw c;
    }
    
    return retval;
  }


}}}
