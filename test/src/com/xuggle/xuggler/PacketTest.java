/*
 * This file is part of Xuggler.
 * 
 * Xuggler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public
 * License along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.xuggle.xuggler;

import org.junit.*;
import junit.framework.TestCase;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IPacket;

public class PacketTest extends TestCase
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());

  private IPacket mPacket=null;
  
  @Before
  public void setUp()
  {
    log.debug("Executing test case: {}", this.getName());
    if (mPacket != null)
      mPacket.delete();
    mPacket = null;
  }
  
  @Test
  public void testPacketCreation()
  {
    mPacket = IPacket.make();
    assertTrue("could not create a packet", mPacket != null);
    mPacket.delete();
    mPacket = null;
  }

  @Test
  public void testReadPacket()
  {
    Helper h = new Helper();
    int retval = 0;
    
    long pts=-1;
    long dts=-1;
    int size=-1;
    int index=-1;
    int flags = -1;
    long duration=-1;
    long position = -1;

    h.setupReadingObject(h.sampleFile);

    pts = h.mPacket.getPts();
    dts = h.mPacket.getDts();
    size = h.mPacket.getSize();
    index = h.mPacket.getStreamIndex();
    flags = h.mPacket.getFlags();
    duration = h.mPacket.getDuration();
    position = h.mPacket.getPosition();
    
    // for an uninitialized packet, everything but
    // position should be garbage.
    assertTrue(position == -1);

    retval = h.mContainer.readNextPacket(h.mPacket);
    assertTrue("Could not read packet", retval >= 0);
    
    // I happen to know the first packet is from
    // stream 0.
    pts = h.mPacket.getPts();
    dts = h.mPacket.getDts();
    size = h.mPacket.getSize();
    index = h.mPacket.getStreamIndex();
    flags = h.mPacket.getFlags();
    duration = h.mPacket.getDuration();
    position = h.mPacket.getPosition();
    
    // log the info
    log.debug("First packet from url: {}",
        h.sampleFile);
    log.debug("PTS: {}", pts);
    log.debug("DTS: {}", dts);
    log.debug("Size: {}", size);
    log.debug("Stream Index: {}", index);
    log.debug("Flags: {}", flags);
    log.debug("Duration: {}", duration);
    log.debug("Position: {}", position);
    
    // Now, I know some things about this particular
    // URL, so I assert they stay true.
    
    // Note: If you change the sampleURL to another
    // file, these asserts will fail.
    assertTrue(pts == 0);
    assertTrue(dts == 0);
    assertTrue(size == 5516);
    assertTrue(flags == 1);
    assertTrue(duration == 0);
    assertTrue(position == 284);
    
  }
  
  @Test
  public void testReadFile()
  {
    Helper h = new Helper();
    int retval = 0;
    int audioStream = -1;
    int videoStream = -1;
    int numVidPkts=0;
    int numAudPkts=0;

    h.setupReadingObject(h.sampleFile);

    assertTrue("unexpected number of streams",
        h.mContainer.getNumStreams() == 2);
    for (int i = 0; i < h.mContainer.getNumStreams(); i++)
    {
      if (h.mCoders[i].getCodecType() == ICodec.Type.CODEC_TYPE_AUDIO)
      {
        audioStream = i;
      }
      else if (h.mCoders[i].getCodecType() == ICodec.Type.CODEC_TYPE_VIDEO)
      {
        videoStream = i;
      }
      else {
        fail("Unknown stream type");
      }
    }
    assertTrue("no audio stream", audioStream >= 0);
    assertTrue("no video stream", videoStream >= 0);
    
    do {
      retval = h.mContainer.readNextPacket(h.mPacket);
      if (retval == 0)
      {
        if (h.mPacket.getStreamIndex() == videoStream)
          numVidPkts++;
        else if (h.mPacket.getStreamIndex() == audioStream)
          numAudPkts++;
        else
          fail ("unexpected stream: " + h.mPacket.getStreamIndex());
      }
    } while (retval >= 0);
    log.debug("got {} video packets", numVidPkts);
    log.debug("got {} audio packets", numAudPkts);
    assertTrue("no video packets", numVidPkts>0);
    assertTrue("no audio packets", numAudPkts>0);
    
    // these are specific to the file we use.  this
    // will break if you change the test file
    assertTrue("unexpected number of video packets", numVidPkts == 2236);
    assertTrue("unexpected number of audio packets", numAudPkts == 5714);
    
  }
}
