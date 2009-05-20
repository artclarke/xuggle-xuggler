/*
 * This file is part of Xuggler.
 * 
 * Xuggler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public
 * License along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
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
