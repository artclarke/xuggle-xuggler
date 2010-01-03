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
package com.xuggle.xuggler.io;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.RandomAccessFile;
import java.util.Collection;
import java.util.LinkedList;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;
import static org.junit.Assert.*;

@RunWith(Parameterized.class)
public class XugglerIOExhaustiveTest
{

  private final static String TEST_FILE = "fixtures/testfile.flv";

  @Parameters
  public static Collection<Object[]> getHandlersToTest() throws FileNotFoundException
  {
    Collection<Object[]> retval = new LinkedList<Object[]>();

    retval.add(new Object[]
    {
        new InputOutputStreamHandler(new FileInputStream(TEST_FILE),
            null, true
        ),
        null,
        true
    });
    
    retval.add(new Object[]
    {
        null,
        new InputOutputStreamHandler(null, new FileOutputStream(
            XugglerIOExhaustiveTest.class.getName() + "_"
                + "InputOutputStreamHandlerTest.flv"), true
        ),
        true
    });
    
    retval.add(new Object[]
    {
        new DataInputOutputHandler(new DataInputStream(new FileInputStream(
            TEST_FILE)), null, true
        ),
        null,
        true
    });
    
    retval.add(new Object[]
    {
        null,
        new DataInputOutputHandler(null, 
            new DataOutputStream(new FileOutputStream(
            XugglerIOExhaustiveTest.class.getName() + "_"
                + "DataInputOutputHandlerTest.flv")), true
        ),
        true
    });
    
    retval.add(new Object[]
    {
        new DataInputOutputHandler(new RandomAccessFile(TEST_FILE, "r"),
            null, true
        ),
        null,
        false
    });
    
    retval.add(new Object[]
    {
        null,
        new DataInputOutputHandler(null,
            new RandomAccessFile(XugglerIOExhaustiveTest.class.getName() + "_"
                + "InputOutputStreamHandlerTest.flv", "rw"), true
        ),
        false
    });
    
    retval.add(new Object[]
    {
        new ReadableWritableChannelHandler(new FileInputStream(TEST_FILE)
            .getChannel(),null, true),
        null,
        true
    });
    
    retval.add(new Object[]
    {
        null,
        new ReadableWritableChannelHandler(null,
            new FileOutputStream(XugglerIOExhaustiveTest.class.getName()
            + "_" + "DataInputOutputHandlerTest.flv").getChannel(), true
        ),
        true
    });

    return retval;
  }

  private final IURLProtocolHandler mReadHandler;
  private final IURLProtocolHandler mWriteHandler;
  private final boolean mStreamable;

  public XugglerIOExhaustiveTest(
      IURLProtocolHandler readHandler,
      IURLProtocolHandler writeHandler,
      boolean expectStreamable)
  {
    assertTrue(readHandler != null || writeHandler != null);
    mReadHandler = readHandler;
    mWriteHandler = null;
    mStreamable = expectStreamable;
  }
  
  @Test
  public void testIsStremable()
  {
    if (mReadHandler != null)
      assertEquals("unexpected streamable flag on: " + mReadHandler,
          mStreamable, mReadHandler.isStreamed(null, 0));
    if (mWriteHandler != null)
      assertEquals("unexpected streamable flag on: " + mWriteHandler,
          mStreamable, mWriteHandler.isStreamed(null, 0));
  }
  
  @Test
  public void testReading()
  {
    int retval = 0;
    if (mReadHandler == null)
      return;
    
    retval = mReadHandler.open(null, IURLProtocolHandler.URL_RDONLY_MODE);
    assertTrue(retval >= 0);

    long bytesRead = 0;

    byte[] buffer = new byte[1024];
    while ((retval = mReadHandler.read(buffer, buffer.length)) > 0)
    {
      bytesRead += retval;
    }
    // and close
    retval = mReadHandler.close();
    assertTrue(retval >= 0);
    assertEquals("unexpected byte count on: " + mReadHandler,
        4546420, bytesRead);
    
  }
  
  @Test
  public void testWriting()
  {
    int retval = 0;
    if (mWriteHandler == null)
      return;
    
    retval = mWriteHandler.open(null, IURLProtocolHandler.URL_WRONLY_MODE);
    assertTrue(retval >= 0);

    IURLProtocolHandler readHandler = new FileProtocolHandler(TEST_FILE);
    assertTrue(retval >= 0);

    long bytesWritten = 0;
    long totalBytes = 0;

    byte[] buffer = new byte[1024];
    while ((retval = readHandler.read(buffer, buffer.length)) > 0)
    {
      totalBytes += retval;
      retval = mWriteHandler.write(buffer, retval);
      assertTrue(retval >= 0);
      bytesWritten += retval;
    }
    assertEquals(totalBytes, bytesWritten);
    assertEquals(4546420, totalBytes);

    retval = mWriteHandler.close();
    assertTrue(retval >= 0);

    retval = readHandler.close();
    assertTrue(retval >= 0);
  }
}
