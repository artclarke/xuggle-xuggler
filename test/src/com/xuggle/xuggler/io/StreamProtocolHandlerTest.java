/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated. All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler. If not, see <http://www.gnu.org/licenses/>.
 */

package com.xuggle.xuggler.io;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;

import junit.framework.TestCase;

import org.junit.*;

public class StreamProtocolHandlerTest extends TestCase
{

  private StreamProtocolHandlerFactory mFactory = null;
  private IURLProtocolHandler mHandler = null;
  private final String mSampleFile = "fixtures/testfile.flv";
  private final String mProtocolString = StreamProtocolHandlerFactory.DEFAULT_PROTOCOL;

  @Before
  public void setUp()
  {
    mFactory = new StreamProtocolHandlerFactory();
    URLProtocolManager.getManager().registerFactory(mProtocolString, mFactory);
    mHandler = null;
  }

  @After
  public void tearDown()
  {
    mHandler = null;
  }

  @Test
  public void testValidFFMPEGURLFileOpenForReading()
      throws FileNotFoundException
  {
    int flags = IURLProtocolHandler.URL_RDONLY_MODE;

    FileInputStream stream = new FileInputStream(mSampleFile);
    final String url = mFactory.mapIO("1", stream, null, false, true);
    int retval = -1;

    // Test all the different ways to open a valid file.
    mHandler = mFactory.getHandler(
        StreamProtocolHandlerFactory.DEFAULT_PROTOCOL, url, flags);
    assertTrue("could not find a mHandler using the mFactory", mHandler != null);

    // the mFactory should pass the URL to the mHandler
    retval = mHandler.open(null, flags);
    assertEquals(0, retval);

    retval = mHandler.close();
    assertEquals(0, retval);

    retval = mHandler.open(url, flags);
    assertTrue(retval == 0);

    retval = mHandler.close();
    assertEquals(0, retval);

    // now, try opening using FFMPEG
    FfmpegIOHandle handle = new FfmpegIOHandle();

    retval = FfmpegIO.url_open(handle, url, flags);
    assertEquals(0, retval);

    retval = FfmpegIO.url_close(handle);
    assertTrue(retval == 0);

    assertNotNull(mFactory.unmapIO(url));
  }
  
  @Test
  public void testAutoUnmapping() throws FileNotFoundException
  {
    int flags = IURLProtocolHandler.URL_RDONLY_MODE;

    FileInputStream stream = new FileInputStream(mSampleFile);
    
    // Register the URL, telling the factory to deregister it
    // when closed
    final String url = mFactory.mapIO("1", stream, null);
    int retval = -1;

    // now, try opening using FFMPEG
    FfmpegIOHandle handle = new FfmpegIOHandle();

    retval = FfmpegIO.url_open(handle, url, flags);
    assertEquals(0, retval);

    retval = FfmpegIO.url_close(handle);
    assertEquals(0, retval);

    assertNull("found handler when we expected it to be unmapped",
        mFactory.unmapIO(url));
    
  }
  @Test
  public void testFileRead() throws FileNotFoundException
  {
    // open our file
    FileInputStream stream = new FileInputStream(mSampleFile);
    mHandler = new StreamProtocolHandler(
        mFactory.new RegistrationInformation(mSampleFile, stream, null,
            false, true));

    int retval = 0;
    retval = mHandler.open(mSampleFile, IURLProtocolHandler.URL_RDONLY_MODE);
    assertTrue(retval >= 0);

    long bytesRead = 0;

    byte[] buffer = new byte[1024];
    while ((retval = mHandler.read(buffer, buffer.length)) > 0)
    {
      bytesRead += retval;
    }
    // and close
    retval = mHandler.close();
    assertTrue(retval >= 0);
    assertEquals(4546420, bytesRead);
  }

  @Test
  public void testFileWrite() throws FileNotFoundException
  {

    String copyFile = this.getClass().getName() + "_" + this.getName() + ".flv";

    FileInputStream inStream = new FileInputStream(mSampleFile);
    FileOutputStream outStream = new FileOutputStream(copyFile);
    mHandler = new StreamProtocolHandler(
        mFactory.new RegistrationInformation(copyFile, null, outStream,
            false, true));
    int retval = 0;

    // First, open the write mHandler.
    retval = mHandler.open(copyFile, IURLProtocolHandler.URL_WRONLY_MODE);
    assertTrue(retval >= 0);

    // Now, create and open a read mHandler.
    // note that without a protocol string, should default to file:
    IURLProtocolHandler reader = new StreamProtocolHandler(
        mFactory.new RegistrationInformation(mSampleFile, inStream, null,
            false, true));
    retval = reader.open(null, IURLProtocolHandler.URL_RDONLY_MODE);

    long bytesWritten = 0;
    long totalBytes = 0;

    byte[] buffer = new byte[1024];
    while ((retval = reader.read(buffer, buffer.length)) > 0)
    {
      totalBytes += retval;
      // Write the output.
      retval = mHandler.write(buffer, retval);
      assertTrue(retval >= 0);
      bytesWritten += retval;
    }
    assertEquals(totalBytes, bytesWritten);
    assertEquals(4546420, totalBytes);

    // close each file
    retval = reader.close();
    assertTrue(retval >= 0);

    retval = mHandler.close();
    assertTrue(retval >= 0);

  }

  @Test
  public void testFFMPEGUrlRead()
  {
    testFFMPEGUrlReadTestFile(mSampleFile);
  }

  @Test
  public void testFFMPEGIOUrlRead() throws FileNotFoundException
  {
    FileInputStream stream = new FileInputStream(mSampleFile);

    testFFMPEGUrlReadTestFile(mFactory.mapIO(mSampleFile, stream, null));
  }

  private void testFFMPEGUrlReadTestFile(String filename)
  {

    long retval = 0;
    // Call url_open wrapper
    FfmpegIOHandle handle = new FfmpegIOHandle();

    retval = FfmpegIO.url_open(handle, filename,
        IURLProtocolHandler.URL_RDONLY_MODE);
    assertTrue("url_open(" + filename + ") failed: " + retval, retval >= 0);

    // call url_read wrapper
    long bytesRead = 0;
    byte[] buffer = new byte[1024];
    while ((retval = FfmpegIO.url_read(handle, buffer, buffer.length)) > 0)
    {
      bytesRead += retval;
    }
    assertEquals(4546420, bytesRead);

    // call url_close wrapper
    retval = FfmpegIO.url_close(handle);
    assertTrue("url_close failed: " + retval, retval >= 0);
  }

  @Test
  public void testFfmpegIoUrlWrite() throws FileNotFoundException
  {
    String outFile = this.getClass().getName() + "_" + this.getName() + ".flv";
    FileOutputStream stream = new FileOutputStream(outFile);
    testFFMPEGUrlWriteTestFile(mFactory.mapIO(outFile, null, stream));
  }

  private void testFFMPEGUrlWriteTestFile(String filename)
  {
    int retval = 0;
    // Call url_open wrapper
    FfmpegIOHandle handle = new FfmpegIOHandle();

    retval = FfmpegIO.url_open(handle, filename,
        IURLProtocolHandler.URL_WRONLY_MODE);
    assertTrue("url_open failed: " + retval, retval >= 0);

    // call url_read wrapper
    byte[] buffer = new byte[4];
    buffer[0] = 'F';
    buffer[1] = 'L';
    buffer[2] = 'V';
    buffer[3] = 0;

    retval = FfmpegIO.url_write(handle, buffer, buffer.length);
    assertTrue("url_write failed: " + retval, retval == buffer.length);

    // call url_close wrapper
    retval = FfmpegIO.url_close(handle);
    assertTrue("url_close failed: " + retval, retval >= 0);
  }

}
