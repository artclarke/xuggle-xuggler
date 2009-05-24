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

import static org.junit.Assert.assertEquals;

import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import com.xuggle.xuggler.io.FfmpegIO;
import com.xuggle.xuggler.io.FfmpegIOHandle;
import com.xuggle.xuggler.io.IURLProtocolHandler;
import com.xuggle.xuggler.io.IURLProtocolHandlerFactory;
import com.xuggle.xuggler.io.URLProtocolManager;

public class URLProtocolHandlerTest
{
  private static final URLProtocolManager mMgr = URLProtocolManager.getManager();
  private static final IURLProtocolHandler mHandler = new IURLProtocolHandler()
  {
    public int close()
    {
      throw new IllegalStateException("fails on purpose");
    }

    public boolean isStreamed(String aUrl, int aFlags)
    {
      return true;
    }

    public int open(String aUrl, int aFlags)
    {
      if (aUrl.equals("test:succeed"))
        return 0;
      else
        throw new IllegalStateException("fails on purpose");
    }

    public int read(byte[] aBuf, int aSize)
    {
      throw new IllegalStateException("fails on purpose");
    }

    public long seek(long aOffset, int aWhence)
    {
      throw new IllegalStateException("fails on purpose");
    }

    public int write(byte[] aBuf, int aSize)
    {
      throw new IllegalStateException("fails on purpose");
    }          
  };
  private static final IURLProtocolHandlerFactory mFactory = new IURLProtocolHandlerFactory()
  {
    public IURLProtocolHandler getHandler(String aProtocol, String aUrl,
        int aFlags)
    {
      return mHandler;
    }
  };
  private final byte[] mBuffer = new byte[10];
  private FfmpegIOHandle mHandle;
  
  @BeforeClass
  public static void beforeClass()
  {
    mMgr.registerFactory("test", mFactory);
  }
  
  @Before
  public void setUp()
  {
    mHandle = new FfmpegIOHandle();
  }

  
  @Test
  public void testOpenException()
  {
    assertEquals("should fail", -1, FfmpegIO.url_open(mHandle, "test:fail", IURLProtocolHandler.URL_RDWR));
  }
  
  @Test
  public void testReadException()
  {
    int retval = -1;
    retval = FfmpegIO.url_open(mHandle, "test:succeed", IURLProtocolHandler.URL_RDWR);
    assertEquals("should succeed", 0, retval);
    retval = FfmpegIO.url_read(mHandle, mBuffer, mBuffer.length);
    assertEquals("should fail", -1, retval);
  }
  
  @Test
  public void testWriteException()
  {
    int retval = -1;
    retval = FfmpegIO.url_open(mHandle, "test:succeed", IURLProtocolHandler.URL_RDWR);
    assertEquals("should succeed", 0, retval);
    retval = FfmpegIO.url_write(mHandle, mBuffer, mBuffer.length);
    assertEquals("should fail", -1, retval);  }
  
  @Test
  public void testSeekException()
  {
    int retval = -1;
    retval = FfmpegIO.url_open(mHandle, "test:succeed", IURLProtocolHandler.URL_RDWR);
    assertEquals("should succeed", 0, retval);
    retval = (int)FfmpegIO.url_seek(mHandle, 0, IURLProtocolHandler.SEEK_END);
    assertEquals("should fail", -1, retval);
  }
  
  @Test
  public void testCloseException()
  {
    int retval = -1;
    retval = FfmpegIO.url_open(mHandle, "test:succeed", IURLProtocolHandler.URL_RDWR);
    assertEquals("should succeed", 0, retval);
    retval = FfmpegIO.url_close(mHandle);
    assertEquals("should fail", -1, retval);
  }

  @Test
  public void testURLProtocolManagerGetResource()
  {
    String url;
    String resource;
    String protocol;
    url = "http://www.xuggle.com/xuggler";
    resource = URLProtocolManager.getResourceFromURL(url);
    protocol = URLProtocolManager.getProtocolFromURL(url);
    assertEquals("http", protocol);
    assertEquals("www.xuggle.com/xuggler", resource);
    url = "file:/foo/bar";
    resource = URLProtocolManager.getResourceFromURL(url);
    protocol = URLProtocolManager.getProtocolFromURL(url);
    assertEquals("file", protocol);
    assertEquals("/foo/bar", resource);
  }
}
