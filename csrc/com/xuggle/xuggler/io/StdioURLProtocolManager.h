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

#ifndef STDIOURLPROTOCOLMANAGER_H_
#define STDIOURLPROTOCOLMANAGER_H_

#include <com/xuggle/xuggler/io/URLProtocolManager.h>
#include <com/xuggle/xuggler/io/StdioURLProtocolHandler.h>

namespace com { namespace xuggle { namespace xuggler { namespace io
{
  /**
   * A class for managing custom io protocols.
   */
  class VS_API_XUGGLER_IO StdioURLProtocolManager : public URLProtocolManager
  {
  public:
    /**
     * Returns a URLProtocol handler for the given url and flags
     *
     * @return a {@link URLProtocolHandler} or NULL if none can be created.
     */
    StdioURLProtocolHandler* getHandler(const char* url, int flags);

    /**
     * Convenience method that creats a StdioURLProtocolManager and registers with the
     * URLProtocolManager global methods.
     */
    static StdioURLProtocolManager* registerProtocol(const char *aProtocolName);

  protected:
    StdioURLProtocolManager(const char *aProtocolName);
    virtual ~StdioURLProtocolManager();

  private:
  };
}}}}
#endif /*STDIOURLPROTOCOLMANAGER_H_*/
