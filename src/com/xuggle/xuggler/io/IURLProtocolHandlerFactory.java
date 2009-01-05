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
package com.xuggle.xuggler.io;

import com.xuggle.xuggler.io.IURLProtocolHandler;

/**
 * Used by URLProtocolManager to get a factory for a given protocol.
 * 
 * Implement this interface on any factories that make your
 * specific implementation of IURLProtocolHandler, and then
 * register the factory with your URLProtocolManager
 */
public interface IURLProtocolHandlerFactory
{
  /**
   * Called by FFMPEG in order to get a handler to use for a given file.
   * 
   * @param protocol The protocol without a ':'.  For example, "file", "http", or "yourcustomprotocol"
   * @param url The URL that FFMPEG is trying to open.
   * @param flags The flags that FFMPEG will pass to {@link IURLProtocolHandler#open(String, int)}
   * @return A {@link IURLProtocolHandler} to use, or null.  If null, a file not found error will be passed back
   *   to FFMPEG.
   */
  public IURLProtocolHandler getHandler(String protocol, String url,
      int flags);
}
