/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated.  All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any
 * later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.xuggle.xuggler.io;

import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.io.IURLProtocolHandler;

/**
 * The NullProtocolHandler implements {@link IURLProtocolHandler}, but discards
 * any data written and always returns 0 for reading.
 * <p>
 * This is useful when you want to set up an {@link IContainer} instance to
 * encode data, but don't want to actually write the data out after encoding (because
 * all you really want are muxed packets)
 * </p>
 * @author aclarke
 *
 */
public class NullProtocolHandler implements IURLProtocolHandler
{

  // package level so other folks can't create it.
  NullProtocolHandler()
  {
  }
  
  public int close()
  {
    // Always succeed
    return 0;
  }

  public boolean isStreamed(String aUrl, int aFlags)
  {
    // We're not streamed because, well, we do nothing.
    return false;
  }

  public int open(String aUrl, int aFlags)
  {
    // Always succeed
    return 0;
  }

  public int read(byte[] aBuf, int aSize)
  {
    // always read zero bytes
    return 0;
  }

  public long seek(long aOffset, int aWhence)
  {
    // always seek to where we're asked to seek to
    return aOffset;
  }

  public int write(byte[] aBuf, int aSize)
  {
    // always write all bytes
    return aSize;
  }

}
