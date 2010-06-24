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

#include <com/xuggle/xuggler/io/URLProtocolManager.h>

namespace com { namespace xuggle { namespace xuggler { namespace io
{
using namespace std;
using namespace com::xuggle::ferry;

URLProtocolManager::URLProtocolManager(const char * aProtocolName,
    jobject aJavaProtoMgr)
{
  if (aProtocolName == NULL || aJavaProtoMgr == NULL || *aProtocolName == 0)
    throw invalid_argument("bad argument to protocol manager");
  // I hate C++; I need to make a copy of the protocol name.
  int len = strlen(aProtocolName);
  char * protoName = new char[len+1];
  strcpy(protoName, aProtocolName);

  this->cacheJavaMethods(aJavaProtoMgr);

  // Now, find the getHandler() factory function method and keep it around
  // for later use.

  mWrapper = new URLProtocolWrapper;

  mWrapper->proto.name = protoName;
  protoName = NULL;
  mWrapper->proto.url_open = URLProtocolManager::url_open;
  mWrapper->proto.url_read = URLProtocolManager::url_read;
  mWrapper->proto.url_write = URLProtocolManager::url_write;
  mWrapper->proto.url_seek = URLProtocolManager::url_seek;
  mWrapper->proto.url_close = URLProtocolManager::url_close;
  // Unsupported...
  mWrapper->proto.url_read_pause = 0;
  mWrapper->proto.url_read_seek = 0;
  mWrapper->proto.url_get_file_handle=0;
  mWrapper->proto.priv_data_size = 0;
  mWrapper->proto.priv_data_class = 0;
  // and remember ourselves...
  mWrapper->protocolMgr = this;
}

URLProtocolManager::~URLProtocolManager()
{
  if (mWrapper == NULL)
  {
    if (mWrapper->proto.name != NULL)
    {
      delete [] mWrapper->proto.name;
      mWrapper->proto.name = NULL;
    }

    delete mWrapper;
    mWrapper = NULL;
  }
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

void URLProtocolManager::cacheJavaMethods(jobject aProtoMgr)
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

URLProtocol* URLProtocolManager::getURLProtocol(void)
{
  return (URLProtocol*)mWrapper;
}

/*
 * Here lie static functions that forward to the right class function in the protocol manager
 */
int URLProtocolManager::url_open(URLContext* h, const char* url, int flags)
{
  int retval = -1;
  jstring jUrl= NULL;
  jobject jHandler= NULL;

  JNIEnv * env = JNIHelper::sGetEnv();

  try
  {
    if (!h) throw invalid_argument("missing handle");
    URLProtocolWrapper* proto = (URLProtocolWrapper*)h->prot;
    if (!proto) throw invalid_argument ("missing proto wrapper");
    URLProtocolManager* mgr = (URLProtocolManager*)proto->protocolMgr;
    if (!mgr) throw invalid_argument("missing proto mgr");

    // we need to use our manager to get a URLProtocolHandler object to pass
    // into the native URLProtocolHandler
    // now we need to allocate a new ProtocolHandler and attach it to this
    // URLContext
    jUrl = env->NewStringUTF(url);
    if (!jUrl) throw invalid_argument("should throw bad alloc here...");
    jHandler = env->CallObjectMethod(mgr->mJavaProtoMgr,
        mgr->mJavaURLProtocolManager_getHandler_mid,
        jUrl,
        flags);
    if (!jHandler) throw invalid_argument("couldn't find handler for protocol");

    URLProtocolHandler* handler = new URLProtocolHandler(proto->proto.name,
        jHandler);
    h->priv_data = (void*)handler;
    retval = handler->url_open(h, url, flags);
  }
  catch (...)
  {
    retval = -1;
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

URLProtocolHandler * URLProtocolManager::url_getHandle(URLContext *h)
{
  if (!h)
    throw invalid_argument("missing handle");
  URLProtocolHandler* handler = (URLProtocolHandler*)h->priv_data;
  if (!handler)
    throw invalid_argument("no URLProtocolHandler set");
  return handler;
}

int URLProtocolManager::url_close(URLContext *h)
{
  int retval = -1;
  try
  {
    URLProtocolHandler* handler = URLProtocolManager::url_getHandle(h);
    retval = handler->url_close(h);
    // when we close that handler is no longer valid; you must re-open to re-use.
    delete handler;
    h->priv_data = 0;
  }
  catch (...)
  {
    retval = -1;
  }
  return retval;
}

int URLProtocolManager::url_read(URLContext *h, unsigned char* buf, int size)
{
  int retval = -1;
  try
  {
    URLProtocolHandler* handler = URLProtocolManager::url_getHandle(h);
    retval = handler->url_read(h, buf, size);
  }
  catch(...)
  {
    retval = -1;
  }
  return retval;
}

int URLProtocolManager::url_write(URLContext *h, const unsigned char* buf,
    int size)
{
  int retval = -1;
  try
  {
    URLProtocolHandler* handler = URLProtocolManager::url_getHandle(h);
    retval = handler->url_write(h, buf, size);
  }
  catch(...)
  {
    retval = -1;
  }
  return retval;
}

int64_t URLProtocolManager::url_seek(URLContext *h, int64_t position,
    int whence)
{
  int64_t retval = -1;
  try
  {
    URLProtocolHandler* handler = URLProtocolManager::url_getHandle(h);
    retval = handler->url_seek(h, position, whence);
  }
  catch(...)
  {
    retval = -1;
  }
  return retval;
}
}}}}
