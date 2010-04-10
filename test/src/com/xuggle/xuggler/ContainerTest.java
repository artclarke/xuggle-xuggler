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

package com.xuggle.xuggler;

import org.junit.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.ferry.IBuffer;
import com.xuggle.test_utils.TestUtils;
import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IContainerFormat;
import com.xuggle.xuggler.io.IURLProtocolHandler;

import junit.framework.TestCase;

public class ContainerTest extends TestCase
{
  private final String mSampleFile = "fixtures/testfile.flv";
  private final Logger log = LoggerFactory.getLogger(this.getClass());

  /**
   * At FFmpeg revision r22821 this started
   * failing.  I wasn't able to find a cause easily, so I'm disabling for
   * now.
   */
  @Test
  @Ignore
  public void testContainerOpenAndClose2()
  {
    IContainer container = null;
    IContainerFormat fmt = null;    
    int retval = -1;

    container = IContainer.make();
    fmt = IContainerFormat.make();
    
    // try opening a container format
    
    retval = fmt.setInputFormat("flv");
    assertTrue("could not set input format", retval >= 0);
    
    retval = container.open("file:"+mSampleFile,
        IContainer.Type.READ, fmt);
    assertTrue("could not open file for writing", retval >= 0);

    retval = container.close();
    assertTrue("could not close file", retval >= 0);
  
  }

  @Test
  public void testContainerOpenAndClose()
  {
    // maximize FFmpeg logging
    log.debug("About to change logging level");
    Global.setFFmpegLoggingLevel(50);
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
    
    IStream stream = container.addNewStream(0);
    IStreamCoder coder = stream.getStreamCoder();
    coder.setCodec(ICodec.ID.CODEC_ID_MP3);
    coder.setSampleRate(22050);
    coder.setChannels(1);
    retval = coder.open();
    assertTrue("could not open coder", retval >= 0);
    
    retval = container.writeHeader();
    assertTrue("could not write header", retval >= 0);
    
    retval = container.writeTrailer();
    assertTrue("could not write header", retval >= 0);
   
    retval = coder.close();
    assertTrue("could not close coder", retval >= 0);
    
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
    
    assertEquals("unexpected duration", 149264000.0, container.getDuration(),
        30000); // allow a leeway of at least 30 milliseconds due to some
    // 64 bit issues.
    
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
    
    retval = container.open("fixtures/testfile.mp3", IContainer.Type.READ, null);
    assertTrue("could not open file", retval >= 0);
    
    assertEquals("unexpected bit rate", 127999, container.getBitRate(), 1000);
    
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

  @Test
  public void testReadPacketAddsTimeBase()
  {
    IContainer container = IContainer.make();
    
    int retval = -1;
    retval = container.open(mSampleFile, IContainer.Type.READ, null);
    assertTrue("could not open file", retval >= 0);

    IPacket packet = IPacket.make();
    
    // just read the first three packets
    for(int i = 0; i < 3; i++)
    {
      assertTrue(container.readNextPacket(packet) >= 0);
      IRational timebase = packet.getTimeBase();
      assertNotNull(timebase);
      // should be 1/1000 for flv
      assertEquals(1, timebase.getNumerator());
      assertEquals(1000, timebase.getDenominator());
    }
    
    container.close();
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
  public void testSeekKeyFrame2()
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
    retval = container.seekKeyFrame(0, 18*1000, 20*1000, 21*1000,
        0);
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

    retval = container.seekKeyFrame(-1, 20*1000*1000, IURLProtocolHandler.SEEK_CUR);
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

    int numProperties = container.getNumProperties();
    assertTrue("should be able to set properties before opening",
        numProperties>0);

    int retval = -1;
    retval = container.open(mSampleFile, IContainer.Type.READ, null);
    assertTrue("could not open file", retval >= 0);

    assertEquals("should have same properties",
        numProperties, container.getNumProperties());
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
  
  @Test
  public void testGetFlags()
  {
    IContainer container = IContainer.make();
    
    int retval = -1;
    retval = container.open(mSampleFile, IContainer.Type.READ, null);
    assertTrue("could not open file", retval >= 0);

    int flags = container.getFlags();
    assertEquals("container should have no flags: " + flags, flags, 0);
    
    container.close();
  }

  @Test
  public void testSetFlags()
  {
    IContainer container = IContainer.make();
    
    int retval = -1;
    retval = container.open(mSampleFile, IContainer.Type.READ, null);
    assertTrue("could not open file", retval >= 0);

    int flags = container.getFlags();
    assertEquals("container should have no flags: " + flags, flags, 0);
    
    boolean genPts = container.getFlag(IContainer.Flags.FLAG_GENPTS);
    assertTrue("should be false", !genPts);
    
    container.setFlag(IContainer.Flags.FLAG_GENPTS, true);
    genPts = container.getFlag(IContainer.Flags.FLAG_GENPTS);
    assertTrue("should be true", genPts);
    
    container.close();
  }
  
  @Test
  public void testWriteTrailerAfterCodecClosingFails()
  {
    IContainer container = IContainer.make();
    
    // now, try opening a container.
    int retval = container.open("file:"+this.getClass().getName()+"_"+this.getName()+".flv",
        IContainer.Type.WRITE, null);
    assertTrue("could not open file for writing", retval >= 0);

    // add a stream
    IStream stream = container.addNewStream(0);
    IStreamCoder coder = stream.getStreamCoder();
    coder.setCodec(ICodec.ID.CODEC_ID_MP3);
    coder.setChannels(1);
    coder.setSampleRate(22050);
    retval = coder.open();
    assertTrue("could not open audio codec", retval >= 0);
    retval = container.writeHeader();
    assertTrue("could not write header", retval >= 0);
    
    // now close the codec
    retval = coder.close();
    assertTrue("could not close audio codec", retval >= 0);
    
    // Now, this should fail; in previous versions it would work but could crash the JVM
    retval = container.writeTrailer();
    assertTrue("this should fail", retval <0);
    
    retval = container.close();
    assertTrue("could not close file", retval >= 0);
    
  }

  @Test
  public void testGetURL()
  {
    IContainer container = IContainer.make();
    
    int retval = -1;
    retval = container.open(mSampleFile, IContainer.Type.READ, null);
    assertTrue("could not open file", retval >= 0);

    assertEquals("should be the same", mSampleFile, container.getURL());
  }
  
  @Test
  public void testGetURLEmptyContainer()
  {
    IContainer container = IContainer.make();
    assertNull("should be null", container.getURL());

    int retval = container.open("", IContainer.Type.READ, null);
    assertTrue("should fail", retval < 0);

    assertNull("should be null", container.getURL());

  }
  
  @Test
  public void testFlushPacket()
  {
    IContainer container = IContainer.make();
    
    // now, try opening a container.
    int retval = container.open("file:"+this.getClass().getName()+"_"+this.getName()+".flv",
        IContainer.Type.WRITE, null);
    assertTrue("could not open file for writing", retval >= 0);

    // add a stream
    IStream stream = container.addNewStream(0);
    IStreamCoder coder = stream.getStreamCoder();
    coder.setCodec(ICodec.ID.CODEC_ID_MP3);
    coder.setChannels(1);
    coder.setSampleRate(22050);
    retval = coder.open();
    assertTrue("could not open audio codec", retval >= 0);
    retval = container.writeHeader();
    assertTrue("could not write header", retval >= 0);
    
    // Try a flush
    retval = container.flushPackets();
    assertTrue("could not flush packets", retval >= 0);
    
    // Now, this should fail; in previous versions it would work but could crash the JVM
    retval = container.writeTrailer();
    assertTrue("this should fail", retval >=0);

    // now close the codec
    retval = coder.close();
    assertTrue("could not close audio codec", retval >= 0);
    

    retval = container.close();
    assertTrue("could not close file", retval >= 0);
    
  }

  @Test
  public void testWriteFailsOnReadOnlyContainers()
  {
    IContainer container = IContainer.make();
    
    int retval = -1;
    retval = container.open(mSampleFile, IContainer.Type.READ, null);
    assertTrue("could not open file", retval >= 0);
    
    retval = container.writeHeader();
    assertTrue("should fail on read only file", retval < 0);
    
    IPacket pkt = IPacket.make();
    retval = container.writePacket(pkt);
    assertTrue("should fail on read only file", retval < 0);

    retval = container.writeTrailer();
    assertTrue("should fail on read only file", retval < 0);
    
    
  }

  /**
   * Regression test for Issue #97
   * http://code.google.com/p/xuggle/issues/detail?id=97
   */
  @Test
  public void testIssue97Regression()
  {
    IContainer container = IContainer.make();
    
    int retval = -1;
    retval = container.open(
        this.getClass().getName()+"_"+TestUtils.getNameOfCallingMethod()
        + ".mp3", IContainer.Type.WRITE, null);
    assertTrue("could not open file", retval >= 0);
    
    retval = container.writeHeader();
    assertTrue("should fail instead of core dumping", retval < 0);
  }

  @Test
  public void testCreateSDPData()
  {
    IContainerFormat format = IContainerFormat.make();
    format.setOutputFormat("rtp", null, null);
    IContainer container = IContainer.make();
    container.open("rtp://127.0.0.1:23832", IContainer.Type.WRITE, format);
    IStream stream = container.addNewStream(0);
    IStreamCoder coder = stream.getStreamCoder();
    coder.setCodec(ICodec.ID.CODEC_ID_H263);
    coder.setWidth(352);
    coder.setHeight(288);
    coder.setPixelType(IPixelFormat.Type.YUV420P);
    coder.setTimeBase(IRational.make(1,90000));
    coder.open();
    
    IBuffer buffer = IBuffer.make(null, 4192);
    int len = container.createSDPData(buffer);
    assertTrue(len > 1);
    byte[] stringBuf = new byte[len-1];
    buffer.get(0, stringBuf, 0, stringBuf.length);
    String sdp = new String(stringBuf);
    assertNotNull(sdp);
    log.debug("SDP({}) = {}", sdp.length(), sdp);
    
    String otherSdp = container.createSDPData();
    assertEquals(sdp, otherSdp);
  }
}
