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

import java.io.Closeable;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.xuggler.io.IURLProtocolHandler;

/**
 * Implementation of URLProtocolHandler that can read from {@link InputStream}
 * objects or write to {@link OutputStream} objects.
 *
 * <p>
 * 
 * The {@link IURLProtocolHandler#URL_RDWR} mode is not supported.
 * 
 * </p>
 * <p>
 * 
 * {@link IURLProtocolHandler#isStreamed(String, int)} always return true.
 * 
 * </p>
 * 
 * @author aclarke
 *
 */

public class StreamProtocolHandler implements IURLProtocolHandler
{
  private final String mName;
  private final InputStream mIStream;
  private final OutputStream mOStream;
  private Closeable mOpenStream = null;
  
  private final Logger log = LoggerFactory.getLogger(this.getClass());

  /**
   * Only usable by the factory.
   */

  StreamProtocolHandler(String name, InputStream input, OutputStream output)
  {
    log.trace("Initializing handler: {}", name);
    mName = name;
    mIStream = input;
    mOStream = output;
  }

  /**
   * {@inheritDoc}
   */

  public int close()
  {
    log.trace("Closing stream: {}", mName);
    int retval = 0;
    if (mOpenStream != null)
    {
      try
      {
        mOpenStream.close();
      }
      catch (IOException e)
      {
        log.error("could not close stream {}: {}", mName, e);
        retval = -1;
      }
    }
    mOpenStream = null;
    return retval;
  }

  /**
   * {@inheritDoc}
   */
  
  public int open(String url, int flags)
  {
    log.trace("attempting to open {} with flags {}", url == null ? mName
        : url, flags);
    
    if (mOpenStream != null) {
      log.debug("attempting to open already open handler: {}", mName);
      return -1;
    }
    switch (flags)
    {
    case URL_RDWR:
      log.debug("do not support read/write mode for Java IO Handlers");
      return -1;
    case URL_WRONLY_MODE:
      // RandomAccessFile doesn't support write-only
      mOpenStream = mOStream;
      break;
    case URL_RDONLY_MODE:
      mOpenStream = mIStream;
      break;
    default:
      log.error("Invalid flag passed to open: {}", mName);
      return -1;
    }
    if (mOpenStream == null)
    {
      log.error("cannot open stream for that mode: {}", mName);
      return -1;
    }

    log.debug("Opened file: {}", mName);
    return 0;
  }

  /**
   * {@inheritDoc}
   */
    
  public int read(byte[] buf, int size)
  {
    if (mOpenStream == null || !(mOpenStream instanceof InputStream))
      return -1;
    
    InputStream stream = (InputStream)mOpenStream;
    try
    {
      int ret = -1;
      ret = stream.read(buf, 0, size);
      //log.debug("Got result for read: {}", ret);
      return ret;
    }
    catch (IOException e)
    {
      log.error("Got IO exception reading from file: {}; {}", mName, e);
      return -1;
    }
  }

  /**
   * {@inheritDoc}
   * 
   * This method is not supported on this class and always return -1;
   */
  
  public long seek(long offset, int whence)
  {
    // not supported
    return -1;
  }

  /**
   * {@inheritDoc}
   */
  
  public int write(byte[] buf, int size)
  {
    if (mOpenStream == null || !(mOpenStream instanceof OutputStream))
      return -1;
    
    OutputStream stream = (OutputStream)mOpenStream;
    //log.debug("writing {} bytes to: {}", size, file);
    try
    {
      stream.write(buf, 0, size);
      return size;
    }
    catch (IOException e)
    {
      log.error("Got error writing to file: {}; {}", mName, e);
      return -1;
    }
  }

  /**
   * {@inheritDoc}
   * 
   * Always returns true for this class.
   */
  
  public boolean isStreamed(String url, int flags)
  {
    return true;
  }
}
