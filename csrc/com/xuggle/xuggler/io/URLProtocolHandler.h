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
  int url_write(URLContext *h, const unsigned char* buf, int size);
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
