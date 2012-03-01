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

#ifndef JAVAURLPROTOCOLMANAGER_H_
#define JAVAURLPROTOCOLMANAGER_H_

#include <com/xuggle/xuggler/io/URLProtocolManager.h>
#include <com/xuggle/xuggler/io/JavaURLProtocolHandler.h>

namespace com { namespace xuggle { namespace xuggler { namespace io
{
  /**
   * A class for managing custom io protocols.
   */
  class VS_API_XUGGLER_IO JavaURLProtocolManager : public URLProtocolManager
  {
  public:
    /**
     * Returns a URLProtocol handler for the given url and flags
     *
     * @return a {@link URLProtocolHandler} or NULL if none can be created.
     */
    JavaURLProtocolHandler* getHandler(const char* url, int flags);

    /**
     * Convenience method that creats a JavaURLProtocolManager and registers with the
     * URLProtocolManager global methods.
     */
    static JavaURLProtocolManager* registerProtocol(const char *aProtocolName, jobject aJavaProtoMgr);

  protected:
    JavaURLProtocolManager(const char *aProtocolName, jobject aJavaProtoMgr);
    virtual ~JavaURLProtocolManager();

  private:
    void cacheJavaMethods(jobject aProtoMgr);
    jobject mJavaProtoMgr;
    jmethodID mJavaURLProtocolManager_getHandler_mid;
    jclass mJavaURLProtocolManager_class;
  };
}}}}
#endif /*JAVAURLPROTOCOLMANAGER_H_*/
