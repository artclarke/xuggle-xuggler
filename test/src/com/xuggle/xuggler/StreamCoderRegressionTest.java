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

import static org.junit.Assert.*;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.test_utils.NameAwareTestClassRunner;

/**
 * Contains unit tests that demonstrate bugs and check for regressions
 * after fixes.
 * @author aclarke
 *
 */
@RunWith(NameAwareTestClassRunner.class)
public class StreamCoderRegressionTest
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());
  private String mTestName = null;
  @Before
  public void setUp()
  {
    mTestName = NameAwareTestClassRunner.getTestMethodName();
    log.debug("-----START----- {}", mTestName);
  }
  @After
  public void tearDown()
  {
    log.debug("----- END ----- {}", mTestName);
  }
  /**
   * Unit test for: http://code.google.com/p/xuggle/issues/detail?id=55
   * 
   * For some files that had different time bases in IStreams than in
   * IStreamCoders, StreamCoder would use the wrong (StreamCoder) timebase
   * for recalculating timestamps, which would lead to timestamps being
   * off by over 30,000 times.
   * 
   * This test replicates the issue by using one of those sample files
   */
  @Test
  public void testIssue55()
  {
    IContainer container = IContainer.make();
    assertNotNull(container);
    
    int retval = -1;
    
    retval = container.open("fixtures/testfile_mpeg1video_mp2audio.mpg",
        IContainer.Type.READ, null);
    assertTrue("could not open file", retval >= 0);
    
    // find the first video stream
    int videoId = -1;
    for(int i = 0; i < container.getNumStreams(); i++)
    {
      IStream stream = container.getStream(i);
      IStreamCoder coder = stream.getStreamCoder();
      if (coder.getCodecType() == ICodec.Type.CODEC_TYPE_VIDEO)
      {
        log.debug("found video on stream: {}", i);
        videoId = i;
        retval = coder.open();
        assertTrue("could not open decoder", retval >= 0);
        break;
      }
    }
    assertTrue("no video found", videoId >= 0);
    
    // read through the file
    IPacket pkt = IPacket.make();
    int vidPackets = 0;
    long lastPts = Global.NO_PTS;
    while(container.readNextPacket(pkt) >= 0)
    {
      // see if the packet is video
      if (pkt.getStreamIndex() == videoId)
      {
        IStreamCoder vidCoder = container.getStream(videoId).getStreamCoder();
        // Decode the video
        IVideoPicture pict = IVideoPicture.make(
            vidCoder.getPixelType(),
            vidCoder.getWidth(),
            vidCoder.getHeight());
        retval = vidCoder.decodeVideo(pict, pkt, 0);
        assertTrue("could not decode", retval >= 0);
       
        if (lastPts != Global.NO_PTS)
        {
          // And this is the test for issue 55; if this is working, picture
          // time stamps in this video should never be more than 50 milliseconds
          // apart
          if (lastPts + 50000 < pict.getPts())
          {
            log.debug("last pts: {}; now: {}", lastPts, pict.getPts());
            fail("issue 55: timestamps not correctly computed");
          }
        }
        lastPts = pict.getPts();
        ++vidPackets;
      } else {
        //log.debug("skipping packet in stream: {}", pkt.getStreamIndex());
      }
    }
    assertEquals("should be this number video packets",
        470, vidPackets);
    // close the coder
    if (videoId >= 0)
      container.getStream(videoId).getStreamCoder().close();
    container.close();
  }
}
