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

import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IPixelFormat;
import com.xuggle.xuggler.IRational;
import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IStreamCoder;

import junit.framework.TestCase;

public class StreamCoderTest extends TestCase
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());
  private final String sampleFile = "fixtures/testfile.flv";

  private IStream mStream=null;
  private IContainer mContainer=null;
  private IStreamCoder mCoder=null;

  @Before
  public void setUp()
  {
    log.debug("Executing test case: {}", this.getName());
    if (mContainer != null)
      mContainer.delete();
    mContainer = null;
    if (mStream != null)
      mStream.delete();
    mStream = null;
    if (mCoder != null)
      mCoder.delete();
    mCoder = null;
  }

  @Test
  public void testGetDirection()
  {
    mCoder = getStreamCoder(sampleFile, 1);
    
    IStreamCoder.Direction direction = mCoder.getDirection();
    assertTrue(direction == IStreamCoder.Direction.DECODING);
  }
  @Test
  public void testGetCodec()
  {
    // First the video stream
    ICodec codec = null;
    ICodec.Type type = null;
    ICodec.ID id = null;

    mCoder = getStreamCoder(sampleFile, 0);
    
    codec = mCoder.getCodec();
    assertTrue(codec != null);
    
    type = mCoder.getCodecType();
    assertTrue(type == codec.getType());
    assertTrue(type == ICodec.Type.CODEC_TYPE_VIDEO);
    
    id = mCoder.getCodecID();
    assertTrue(id == codec.getID());
    log.debug("Video type: {}", id);
    assertTrue(id == ICodec.ID.CODEC_ID_FLV1);
    
    // Then the audio stream.
    mCoder = getStreamCoder(sampleFile, 1);
    
    codec = mCoder.getCodec();
    assertTrue(codec != null);
    
    type = mCoder.getCodecType();
    assertTrue(type == codec.getType());
    assertTrue(type == ICodec.Type.CODEC_TYPE_AUDIO);
    
    id = mCoder.getCodecID();
    assertTrue(id == codec.getID());
    assertTrue(id == ICodec.ID.CODEC_ID_MP3);
  }
  
  @Test
  public void testVideoGetters()
  {
    int bitRate = -1;
    int height = -1;
    int width = -1;
    IRational timebase = null;
    int gops = -1;
    IPixelFormat.Type pixFmt= IPixelFormat.Type.NONE;
    int sampleRate = -1;
    int channels = -1;
    
    // get the video stream
    mCoder = getStreamCoder(sampleFile, 0);
    bitRate = mCoder.getBitRate();
    height = mCoder.getHeight();
    width = mCoder.getWidth();
    timebase = mCoder.getTimeBase();
    gops = mCoder.getNumPicturesInGroupOfPictures();
    pixFmt = mCoder.getPixelType();
    sampleRate = mCoder.getSampleRate();
    channels = mCoder.getChannels();
    
    // Log them all
    log.debug("Bitrate: {}", bitRate);
    log.debug("Height: {}", height);
    log.debug("Width: {}", width);
    log.debug("Timebase: {}/{}", timebase.getNumerator(),
        timebase.getDenominator());
    log.debug("Num Group of Pictures: {}", gops);
    log.debug("Pixel Format: {}", pixFmt);
    log.debug("Sample Rate: {}", sampleRate);
    log.debug("Channels: {}", channels);
    
    // now our assertions
    assertEquals(0, bitRate);
    assertEquals(176, height);
    assertEquals(424, width);
    assertEquals(1, timebase.getNumerator());
    assertEquals(1000, timebase.getDenominator());
    assertEquals(12, gops);
    assertEquals(IPixelFormat.Type.YUV420P, pixFmt);
    assertEquals(0, sampleRate);
    assertEquals(0, channels);    
  }
  
  @Test
  public void testAudioGetters()
  {
    int bitRate = -1;
    int height = -1;
    int width = -1;
    IRational timebase = null;
    int gops = -1;
    IPixelFormat.Type pixFmt= IPixelFormat.Type.NONE;
    int sampleRate = -1;
    int channels = -1;
    
    // get the audio stream
    mCoder = getStreamCoder(sampleFile, 1);
    bitRate = mCoder.getBitRate();
    height = mCoder.getHeight();
    width = mCoder.getWidth();
    timebase = mCoder.getTimeBase();
    gops = mCoder.getNumPicturesInGroupOfPictures();
    pixFmt = mCoder.getPixelType();
    sampleRate = mCoder.getSampleRate();
    channels = mCoder.getChannels();
    
    // Log them all
    log.debug("Bitrate: {}", bitRate);
    log.debug("Height: {}", height);
    log.debug("Width: {}", width);
    log.debug("Timebase: {}/{}", timebase.getNumerator(),
        timebase.getDenominator());
    log.debug("Num Group of Pictures: {}", gops);
    log.debug("Pixel Format: {}", pixFmt);
    log.debug("Sample Rate: {}", sampleRate);
    log.debug("Channels: {}", channels);
    
    // now our assertions
    assertEquals(64000, bitRate);
    assertEquals(0, height);
    assertEquals(0, width);
    assertEquals(1, timebase.getNumerator());
    assertEquals(1000, timebase.getDenominator());
    assertEquals(12, gops);
    assertEquals(IPixelFormat.Type.NONE, pixFmt);
    assertEquals(22050, sampleRate);
    assertEquals(1, channels);    
  }

  @Test
  public void testOpenAndCloseCodec()
  {
    int retval = -1;
    mCoder = getStreamCoder(sampleFile, 1);

    retval = mCoder.open();
    assertTrue("Could not open codec", retval >= 0);
    retval = mCoder.close();
    assertTrue("Could not close codec", retval >= 0);
  }

  /**
   * Regression test for https://sourceforge.net/tracker2/?func=detail&aid=2508480&group_id=248424&atid=1126411
   */
  @Test
  public void testGetCodecTypeBugFix2508480()
  {
    IContainer container = IContainer.make();
    assertTrue("should be able to open",
        container.open("fixtures/subtitled_video.mkv", IContainer.Type.READ, null) >= 0);
    assertEquals("unexpected codec type", ICodec.Type.CODEC_TYPE_VIDEO, container.getStream(0).getStreamCoder().getCodecType());
    assertEquals("unexpected codec type", ICodec.Type.CODEC_TYPE_AUDIO, container.getStream(1).getStreamCoder().getCodecType());
    assertEquals("unexpected codec type", ICodec.Type.CODEC_TYPE_SUBTITLE, container.getStream(2).getStreamCoder().getCodecType());
  }
  
  private IStreamCoder getStreamCoder(String url, int index)
  {
    IStreamCoder retval = null;
    int errorVal = 0;
    mContainer = IContainer.make();
    assertTrue(mContainer != null);
    
    errorVal = mContainer.open(sampleFile, IContainer.Type.READ, null);
    assertTrue(errorVal >= 0);
    
    assertTrue(mContainer.getNumStreams() == 2);
    
    mStream = mContainer.getStream(index);
    assertTrue(mStream != null);
    assertTrue(mStream.getIndex() == index);
    
    retval = mStream.getStreamCoder();
    assertTrue(retval != null);
    
    return retval;
  }
}
