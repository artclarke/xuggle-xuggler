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
#ifndef URLPROTOCOLHANDLER_H_
#define URLPROTOCOLHANDLER_H_

#include <jni.h>
#include <com/xuggle/xuggler/io/FfmpegIncludes.h>

namespace com { namespace xuggle { namespace xuggler { namespace io
{
class URLProtocolHandler
{
public:
  URLProtocolHandler(const char * protoName, jobject aJavaProtocolHandler);
  virtual ~URLProtocolHandler();

  // Now, let's have our forwarding functions
  int url_open(URLContext *h, const char *url, int flags);
  int url_close(URLContext *h);
  int url_read(URLContext *h, unsigned char* buf, int size);
  int url_write(URLContext *h, unsigned char* buf, int size);
  int64_t url_seek(URLContext *h, int64_t position, int whence);

private:
  const char * mProtoName;

  void cacheJavaMethods(jobject aProtoHandler);
  jobject mJavaProtoHandler;
  jmethodID mJavaUrlOpen_mid;
  jmethodID mJavaUrlClose_mid;
  jmethodID mJavaUrlRead_mid;
  jmethodID mJavaUrlWrite_mid;
  jmethodID mJavaUrlSeek_mid;
  jmethodID mJavaUrlIsStreamed_mid;

};
}}}}
#endif /*URLPROTOCOLHANDLER_H_*/
