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

import com.xuggle.xuggler.Global;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.ICodec;

import junit.framework.TestCase;

public class AudioSamplesTest extends TestCase
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());

  @Before
  public void setUp()
  {
    log.debug("Executing test case: {}", this.getName());
  }
  
  @Test
  public void testReadingSamples()
  {
    Helper h = new Helper();
    
    h.setupReadingObject(h.sampleFile);
    
    int retval = -1;
    int audioStream = -1;
    int totalSamples = 0;
    for (int i = 0; i < h.mContainer.getNumStreams(); i++)
    {
      if (h.mCoders[i].getCodecType() == ICodec.Type.CODEC_TYPE_AUDIO)
      {
        audioStream = i;
        // open our stream coder
        retval = h.mCoders[i].open();
        assertTrue("Could not open decoder", retval >=0);

        assertTrue("unexpected samples inbuffer",
            h.mSamples[i].getNumSamples() == 0);
        break;
      }
    }
    assertTrue("Could not find audio stream", audioStream >= 0);
    
    while (h.mContainer.readNextPacket(h.mPacket) == 0)
    {
      if (h.mPacket.getStreamIndex() == audioStream)
      {
        int offset = 0;
        while (offset < h.mPacket.getSize())
        {
          retval = h.mCoders[audioStream].decodeAudio(
              h.mSamples[audioStream],
              h.mPacket,
              offset);
          assertTrue("could not decode any audio", retval >0);
          offset += retval;
          assertTrue("did not write any samples",
              h.mSamples[audioStream].getNumSamples() > 0);
          log.debug("Decoded {} samples",
              h.mSamples[audioStream].getNumSamples());
          totalSamples += h.mSamples[audioStream].getNumSamples();
        }
      } else {
        log.debug("skipping video packet");
      }
    }
    h.mCoders[audioStream].close();
    log.debug("Total audio samples: {}", totalSamples);
    assertTrue("didn't get any audio", totalSamples > 0);
    // this will change if you change the file.
    assertTrue("unexpected # of samples", totalSamples == 3291264);
  }
  
  @Test
  public void testGetNextPts()
  {
    int sampleRate = 440;
    int channels=1;
    
    IAudioSamples samples = IAudioSamples.make(sampleRate, 1);
    assertNotNull(samples);

    samples.setComplete(true, sampleRate, sampleRate, channels, IAudioSamples.Format.FMT_S16, 0);
    assertTrue(samples.isComplete());
    assertEquals(0, samples.getPts());
    assertEquals(Global.DEFAULT_PTS_PER_SECOND, samples.getNextPts());
  }
  
  @Test
  public void testSetPts()
  {
    int sampleRate = 440;
    int channels=1;
    
    IAudioSamples samples = IAudioSamples.make(sampleRate, 1);
    assertNotNull(samples);

    samples.setComplete(true, sampleRate, sampleRate, channels, IAudioSamples.Format.FMT_S16, 0);
    assertTrue(samples.isComplete());
    assertEquals(0, samples.getPts());
    assertEquals(Global.DEFAULT_PTS_PER_SECOND, samples.getNextPts());
    
    samples.setPts(Global.DEFAULT_PTS_PER_SECOND);
    assertEquals(Global.DEFAULT_PTS_PER_SECOND, samples.getPts());
    assertEquals(2*Global.DEFAULT_PTS_PER_SECOND, samples.getNextPts());
    
    
  }
}
