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
#ifndef URLPROTOCOLMANAGER_H_
#define URLPROTOCOLMANAGER_H_

#include <jni.h>
#include <com/xuggle/xuggler/io/URLProtocolWrapper.h>
#include <com/xuggle/xuggler/io/URLProtocolHandler.h>

namespace com { namespace xuggle { namespace xuggler { namespace io
{
class URLProtocolManager
{
public:
  URLProtocolManager(const char *aProtocolName, jobject aJavaProtoMgr);
  virtual ~URLProtocolManager();

  URLProtocol* getURLProtocol(void);
private:
  URLProtocolWrapper *mWrapper;
  jobject mJavaProtoMgr;

  // Convenience function used by following url_* functions
  static URLProtocolHandler* url_getHandle(URLContext *h);
  // Wrapper functions that are used by FFMPEG
  static int url_open(URLContext* h, const char* url, int flags);
  static int url_close(URLContext *h);
  static int url_read(URLContext *h, unsigned char* buf, int size);
  static int url_write(URLContext *h, unsigned char* buf, int size);
  static int64_t url_seek(URLContext *h, int64_t position, int whence);

  void cacheJavaMethods(jobject aProtoMgr);
  jmethodID mJavaURLProtocolManager_getHandler_mid;
  jclass mJavaURLProtocolManager_class;

};
}}}}
#endif /*URLPROTOCOLMANAGER_H_*/
