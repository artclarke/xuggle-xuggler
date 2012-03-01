/*******************************************************************************
 * Copyright (c) 2012 Xuggle Inc.  All rights reserved.
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

#ifndef JAVAURLPROTOCOLHANDLER_H_
#define JAVAURLPROTOCOLHANDLER_H_

#include <com/xuggle/xuggler/io/URLProtocolHandler.h>
#include <jni.h>

namespace com { namespace xuggle { namespace xuggler { namespace io
  {
  class JavaURLProtocolManager;
  class VS_API_XUGGLER_IO JavaURLProtocolHandler : public URLProtocolHandler
  {
  public:
    JavaURLProtocolHandler(
        JavaURLProtocolManager* mgr,
        jobject aJavaProtocolHandler);
    virtual ~JavaURLProtocolHandler();

    // Now, let's have our forwarding functions
    int url_open(const char *url, int flags);
    int url_close();
    int url_read(unsigned char* buf, int size);
    int url_write(const unsigned char* buf, int size);
    int64_t url_seek(int64_t position, int whence);
    SeekableFlags url_seekflags(const char* url, int flags);

  private:
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
#endif /*JAVAURLPROTOCOLHANDLER_H_*/
