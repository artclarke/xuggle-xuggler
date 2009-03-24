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
import com.xuggle.xuggler.IError;

import java.awt.image.BufferedImage;

import static junit.framework.Assert.*;

//import junit.framework.TestCase;

public class MediaReaderTest //extends TestCase
{
  public static final int    TEST_FILE_20_SECONDS_VIDEO_FRAME_COUNT = 300;
  public static final int    TEST_FILE_20_SECONDS_AUDIO_FRAME_COUNT = 762;
  public static final String TEST_FILE_20_SECONDS = 
    "fixtures/testfile_videoonly_20sec.flv";
  
  private final Logger log = LoggerFactory.getLogger(this.getClass());

  @Before
  public void setUp()
  {
    log.trace("setUp");
  }
  // create a new media reader with a bad filename

  @Test(expected=IllegalArgumentException.class)
    public void testMediaSourceNotExist() 
  {
    new MediaReader("broken" + TEST_FILE_20_SECONDS, true);
  }  

  // test nominal read without buffered image creation

  @Test
    public void testMediaReaderOpenReadClose()
  {
    final int[] counts = new int[2];
 
    // create a new media reader

    MediaReader mr = new MediaReader(TEST_FILE_20_SECONDS, false);

    // setup the the listener

    MediaReader.IListener mrl = new MediaReader.IListener()
      {
        public void onVideoPicture(IVideoPicture picture, BufferedImage image,
          int streamIndex)
        {
          assertNotNull("picture should be created", picture);
          assertNull("no buffered image should be created", image);
          ++counts[0];
        }

        public void onAudioSamples(IAudioSamples samples, int streamIndex)
        {
          assertNotNull("audio samples should be created", samples);
          ++counts[1];
        }
      };
    mr.addListener(mrl);

    // read all the packets in the media file

    IError err = null;
    while ((err = mr.readPacket()) == null)
      ;

    // should be at end of file

    assertEquals("Loop should complete with an EOF",
        IError.Type.ERROR_EOF,
        err.getType());

    // should have read out the correct number of audio and video frames
    
    assertEquals("incorrect number of video frames:", 
        counts[0],
        TEST_FILE_20_SECONDS_VIDEO_FRAME_COUNT);
    assertEquals("incorrect number of audio frames:",
        counts[1],
        TEST_FILE_20_SECONDS_AUDIO_FRAME_COUNT);

    // the container should null

    assertNull("container should not longer exist", mr.getContainer());
  }

  // test nominal read with buffered image creation
  
  @Test
  public void testCreateBufferedImages()
  {
    if (!IVideoResampler.isSupported(IVideoResampler.Feature.FEATURE_COLORSPACECONVERSION))
      // we're using the LGPL version of Xuggler which can't do this... exit
      return;
    
    // create a new media reader

    MediaReader mr = new MediaReader(TEST_FILE_20_SECONDS, true);

    // setup the the listener

    MediaReader.IListener mrl = new MediaReader.IListener()
      {
        public void onVideoPicture(IVideoPicture picture, BufferedImage image,
          int streamIndex)
        {
          assertNotNull("picture should be created", picture);
          assertNotNull("buffered image should be created", image);
        }

        public void onAudioSamples(IAudioSamples samples, int streamIndex)
        {
          assertNotNull("audio samples should be created", samples);
        }
      };
    mr.addListener(mrl);

    // read all the packets in the media file

    while (mr.readPacket() == null)
      ;
  }
  
  // test nominal read with external container
  
  @Test
  public void testOpenWithContainer()
  {
    if (!IVideoResampler.isSupported(IVideoResampler.Feature.FEATURE_COLORSPACECONVERSION))
      // we're using the LGPL version of Xuggler which can't do this... exit
      return;
    
    final int[] counts = new int[2];

    // create the container
    
    IContainer container = IContainer.make();
    
    // open the container
    
    if (container.open(TEST_FILE_20_SECONDS, IContainer.Type.READ,
        null, true, false) < 0)
      throw new IllegalArgumentException(
        "could not open: " + TEST_FILE_20_SECONDS);
        
    // create a new media reader

    MediaReader mr = new MediaReader(container, true);

    // setup the the listener

    MediaReader.IListener mrl = new MediaReader.IListener()
      {
        public void onVideoPicture(IVideoPicture picture, BufferedImage image,
          int streamIndex)
        {
          assertNotNull("picture should be created", picture);
          assertNotNull("buffered image should be created", image);
          ++counts[0];
        }

        public void onAudioSamples(IAudioSamples samples, int streamIndex)
        {
          assertNotNull("audio samples should be created", samples);
          ++counts[1];
        }
      };
    mr.addListener(mrl);

    // read all the packets in the media file

    IError err = null;
    while ((err = mr.readPacket()) == null)
      ;

    // should be at end of file

    assertEquals("Loop should complete with an EOF",
        IError.Type.ERROR_EOF,
        err.getType());

    // should have read out the correct number of audio and video frames
    
    assertEquals("incorrect number of video frames:",
        counts[0],
        TEST_FILE_20_SECONDS_VIDEO_FRAME_COUNT);
    assertEquals("incorrect number of audio frames:",
        counts[1],
        TEST_FILE_20_SECONDS_AUDIO_FRAME_COUNT);

    // the container should exist
    
    assertTrue("container should be open", container.isOpened());

    // there should exist two streams

    assertEquals("container should have two streams", 
      2, container.getNumStreams());

    // both streams should be closed

    for (int i = 0; i < container.getNumStreams(); ++i)
      assertFalse(container.getStream(i).getStreamCoder().isOpen());
  }
}
