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

#include <com/xuggle/xuggler/io/StdioURLProtocolManager.h>

namespace com { namespace xuggle { namespace xuggler { namespace io
{
using namespace std;
using namespace com::xuggle::ferry;

StdioURLProtocolManager*
StdioURLProtocolManager :: registerProtocol(const char *aProtocolName)
{
  StdioURLProtocolManager* mgr = new StdioURLProtocolManager(aProtocolName);
  return dynamic_cast<StdioURLProtocolManager*>(URLProtocolManager::registerProtocol(mgr));
}


StdioURLProtocolManager :: StdioURLProtocolManager(
    const char * aProtocolName) : URLProtocolManager(aProtocolName)
{
}

StdioURLProtocolManager :: ~StdioURLProtocolManager()
{
}

StdioURLProtocolHandler *
StdioURLProtocolManager :: getHandler(const char *, int)
{
  return new StdioURLProtocolHandler(this);
}
}}}}
