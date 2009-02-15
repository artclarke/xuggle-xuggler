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

import com.xuggle.xuggler.io.FileProtocolHandler;
import com.xuggle.xuggler.io.IURLProtocolHandler;
import com.xuggle.xuggler.io.IURLProtocolHandlerFactory;


/**
 * Implementation of {@link IURLProtocolHandlerFactory} that demonstrates how
 * you can have Java act just like FFMPEG's internal "file:" protocol.
 *  
 * This just duplicates all the functionality in the default "file:" protocol
 * that FFMPEG implemements, but demonstrates how you can have FFMPEG
 * call back into Java.
 * 
 */
public class FileProtocolHandlerFactory implements
    IURLProtocolHandlerFactory
{
  public IURLProtocolHandler getHandler(String protocol, String url,
      int flags)
  {
    return new FileProtocolHandler(url);
  }

}
