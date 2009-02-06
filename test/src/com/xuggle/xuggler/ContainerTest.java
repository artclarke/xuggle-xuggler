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
package com.xuggle.xuggler;

import org.junit.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IContainerFormat;
import com.xuggle.xuggler.io.IURLProtocolHandler;

import junit.framework.TestCase;

public class ContainerTest extends TestCase
{
  private final String mSampleFile = "fixtures/testfile.flv";
  private final Logger log = LoggerFactory.getLogger(this.getClass());

  @Test
  public void testContainerOpenAndClose()
  {
    // Try getting a new Container
    IContainer container = null;
    IContainerFormat fmt = null;    
    int retval = -1;

    container = IContainer.make();
    fmt = IContainerFormat.make();
    
    // try opening a container format
    
    retval = fmt.setInputFormat("flv");
    assertTrue("could not set input format", retval >= 0);
    
    retval = fmt.setOutputFormat("flv", null, null);
    assertTrue("could not set output format", retval >= 0);
    
    // force collection of native resources.
    container.delete();
    container = null;
    
    // get a new container; which will deref the old one.
    container = IContainer.make();
    
    // now, try opening a container.
    retval = container.open("file:"+this.getClass().getName()+"_"+this.getName()+".flv",
        IContainer.Type.WRITE, fmt);
    assertTrue("could not open file for writing", retval >= 0);

    retval = container.close();
    assertTrue("could not close file", retval >= 0);
    
    retval = container.open("file:"+this.getClass().getName()+"_"+this.getName()+".flv",
        IContainer.Type.WRITE, null);
    assertTrue("could not open file for writing", retval >= 0);

    retval = container.close();
    assertTrue("could not close file", retval >= 0);
    
    retval = container.open("file:"+mSampleFile,
        IContainer.Type.READ, fmt);
    assertTrue("could not open file for writing", retval >= 0);

    retval = container.close();
    assertTrue("could not close file", retval >= 0);
    
    retval = container.open("file:"+mSampleFile,
        IContainer.Type.READ, null);
    assertTrue("could not open file for writing", retval >= 0);

    retval = container.close();
    assertTrue("could not close file", retval >= 0);
    
  }
  
  @Test
  public void testCorrectNumberOfStreams()
  {
    IContainer container = IContainer.make();
    int retval = -1;
    
    retval = container.open(mSampleFile, IContainer.Type.READ, null);
    assertTrue("could not open file", retval >= 0);
    
    int numStreams = container.getNumStreams();
    assertTrue("incorrect number of streams: " + numStreams, numStreams == 2);
    
    retval = container.close();
    assertTrue("could not close the file", retval >= 0);
  }
  
  @Test
  public void testWriteHeaderAndTrailer()
  {
    IContainer container = IContainer.make();
    int retval = -1;
    
    retval = container.open(this.getClass().getName()+"_"+this.getName()+".flv",
        IContainer.Type.WRITE, null);
    assertTrue("could not open file", retval >= 0);
    
    retval = container.writeHeader();
    assertTrue("could not write header", retval >= 0);
    
    retval = container.writeTrailer();
    assertTrue("could not write header", retval >= 0);
    
    retval = container.close();
    assertTrue("could not close the file", retval >= 0);
    
  }

  @Test
  public void testWriteTrailerWithNoWriteHeaderDoesNotCrashJVM()
  {
    IContainer container = IContainer.make();
    int retval = -1;
    
    retval = container.open(this.getClass().getName()+"_"+this.getName()+".flv",
        IContainer.Type.WRITE, null);
    assertTrue("could not open file", retval >= 0);

    // Should give a warning and fail, but should NOT crash the JVM like it used to.
    retval = container.writeTrailer();
    assertTrue("could not write header", retval < 0);
    
    retval = container.close();
    assertTrue("could not close the file", retval >= 0);
    
  }
  
  @Test
  public void testOpenFileSetsInputContainer()
  {
    IContainer container = null;
    IContainerFormat fmt = null;    
    int retval = -1;

    container = IContainer.make();
    
    // now, try opening a container.
    retval = container.open("file:"+this.getClass().getName()+"_"+this.getName()+".flv",
        IContainer.Type.WRITE, null);
    assertTrue("could not open file for writing", retval >= 0);
    
    fmt = container.getContainerFormat();
    assertNotNull(fmt);
    assertEquals("flv", fmt.getOutputFormatShortName());

    retval = container.close();
    assertTrue("could not close file", retval >= 0);
    
    retval = container.open("file:"+mSampleFile,
        IContainer.Type.READ, fmt);
    assertTrue("could not open file for writing", retval >= 0);

    fmt = container.getContainerFormat();
    assertNotNull(fmt);
    assertEquals("flv", fmt.getInputFormatShortName());


    retval = container.close();
    assertTrue("could not close file", retval >= 0);
    
  }
  
  @Test
  public void testQueryStreamMetaData()
  {
    IContainer container = IContainer.make();
    
    int retval = -1;
    
    retval = container.open(mSampleFile, IContainer.Type.READ, null);
    assertTrue("could not open file", retval >= 0);
    
    retval = container.queryStreamMetaData();
    assertTrue("could not query stream meta data", retval >= 0);
    
  }

  @Test
  public void testQueryStreamMetaDataFailsIfFileNotOpen()
  {
    IContainer container = IContainer.make();
    
    int retval = -1;
    
    retval = container.queryStreamMetaData();
    assertTrue("could query stream meta data", retval < 0);
    
  }


  @Test
  public void testGetDuration()
  {
    IContainer container = IContainer.make();
    
    int retval = -1;
    
    retval = container.open(mSampleFile, IContainer.Type.READ, null);
    assertTrue("could not open file", retval >= 0);
    
    assertEquals("unexpected duration", 149264000, container.getDuration());
    
  }

  @Test
  public void testGetStartTime()
  {
    IContainer container = IContainer.make();
    
    int retval = -1;
    
    retval = container.open(mSampleFile, IContainer.Type.READ, null);
    assertTrue("could not open file", retval >= 0);
    
    assertEquals("unexpected start time", 0, container.getStartTime());
    
  }

  @Test
  public void testGetFileSize()
  {
    IContainer container = IContainer.make();
    
    int retval = -1;
    
    retval = container.open(mSampleFile, IContainer.Type.READ, null);
    assertTrue("could not open file", retval >= 0);
    
    assertEquals("unexpected filesize", 4546420, container.getFileSize());
    
  }

  @Test
  public void testGetBitRate()
  {
    IContainer container = IContainer.make();
    
    int retval = -1;
    
    retval = container.open(mSampleFile, IContainer.Type.READ, null);
    assertTrue("could not open file", retval >= 0);
    
    assertEquals("unexpected bit rate", 64000, container.getBitRate());
    
  }
  
  @Test
  public void testReadPackets()
  {
    IContainer container = IContainer.make();
    
    int retval = -1;
    retval = container.open(mSampleFile, IContainer.Type.READ, null);
    assertTrue("could not open file", retval >= 0);

    long packetsRead = 0;
    IPacket packet = IPacket.make();
    while(container.readNextPacket(packet) >= 0)
    {
      if (packet.isComplete())
        ++packetsRead;
    }
    assertEquals("got unexpected number of packets in file", 7950, packetsRead);
  }

  /**
   * Seeks 20 seconds into the test file for a key frame, and then counts packets
   * from there.  Should be less than the results of {@link #testReadPackets()}.
   */
  @Test
  public void testSeekKeyFrame()
  {
    IContainer container = IContainer.make();
    
    int retval = -1;
    retval = container.open(mSampleFile, IContainer.Type.READ, null);
    assertTrue("could not open file", retval >= 0);

    // annoyingly we need to make sure that the timestamp is in the time
    // base of the stream we're looking for which can be different for each stream.
    //
    // we happen to know that FLV is in milliseconds (or 1/1000 per second) so
    // we multiply our desired number of seconds by 1,000
    retval = container.seekKeyFrame(0, 20*1000, IURLProtocolHandler.SEEK_CUR);
    assertTrue("could not seek to key frame", retval >=0);
    
    long packetsRead = 0;
    IPacket packet = IPacket.make();
    while(container.readNextPacket(packet) >= 0)
    {
      if (packet.isComplete())
        ++packetsRead;
    }
    assertEquals("got unexpected number of packets in file", 6905, packetsRead);
  }

  @Test
  public void testSeekKeyFrameWithNegativeStream()
  {
    IContainer container = IContainer.make();
    
    int retval = -1;
    retval = container.open(mSampleFile, IContainer.Type.READ, null);
    assertTrue("could not open file", retval >= 0);

    // annoyingly we need to make sure that the timestamp is in the time
    // base of the stream we're looking for which can be different for each stream.
    //
    // we happen to know that FLV is in milliseconds (or 1/1000 per second) so
    // we multiply our desired number of seconds by 1,000
    retval = container.seekKeyFrame(-1, 20*1000, IURLProtocolHandler.SEEK_CUR);
    assertTrue("should fail", retval <0);
  }

  @Test
  public void testSeekKeyFrameWithInvalidStream()
  {
    IContainer container = IContainer.make();
    
    int retval = -1;
    retval = container.open(mSampleFile, IContainer.Type.READ, null);
    assertTrue("could not open file", retval >= 0);

    retval = container.seekKeyFrame(2, 20*1000, IURLProtocolHandler.SEEK_CUR);
    assertTrue("should fail as only 2 strems in this file", retval <0);
  }

  @Test
  public void testGetAndSetProperty()
  {
    IContainer container = IContainer.make();

    assertEquals("should only be able to set properties once file is open",
        0, container.getNumProperties());

    int retval = -1;
    retval = container.open(mSampleFile, IContainer.Type.READ, null);
    assertTrue("could not open file", retval >= 0);

    assertTrue("should have some properties", container.getNumProperties()>0);
    long probeSize = container.getPropertyAsLong("probesize");
    log.debug("probesize: {}", probeSize);
    assertTrue("should have a non-zero probesize",
        probeSize > 0);
    assertTrue("should succeed",
        container.setProperty("probesize", 53535353)>=0);
    assertEquals("should be equal",
        53535353,
        container.getPropertyAsLong("probesize"));
    assertTrue("should fail",
        container.setProperty("notapropertyatall", 15) <0);
    
  }
}
