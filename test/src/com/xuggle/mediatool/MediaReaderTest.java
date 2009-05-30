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

package com.xuggle.mediatool;

import org.junit.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.mediatool.IMediaPipeListener;
import com.xuggle.mediatool.IMediaPipe;
import com.xuggle.mediatool.MediaPipeAdapter;
import com.xuggle.mediatool.MediaReader;
import com.xuggle.mediatool.MediaViewer;
import com.xuggle.xuggler.IError;
import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IVideoResampler;
import com.xuggle.xuggler.video.ConverterFactory;

import java.awt.image.BufferedImage;

import static junit.framework.Assert.*;

import static com.xuggle.mediatool.IMediaViewer.Mode.*;

public class MediaReaderTest
{
  // the log

  private final Logger log = LoggerFactory.getLogger(this.getClass());
  { log.trace("<init>"); }

  // the media view mode
  
  final MediaViewer.Mode mViewerMode = IMediaViewer.Mode.valueOf(
    System.getProperty(this.getClass().getName() + ".ViewerMode", 
      DISABLED.name()));

  // standard test name prefix

  final String PREFIX = this.getClass().getName() + "-";

  // location of test files

  public static final String TEST_FILE_DIR = "fixtures";

  // some counts

  public static final int    TEST_FILE_20_SECONDS_VIDEO_FRAME_COUNT = 300;
  public static final int    TEST_FILE_20_SECONDS_AUDIO_FRAME_COUNT = 762;
  public static final String TEST_FILE_20_SECONDS = 
    TEST_FILE_DIR + "/testfile_videoonly_20sec.flv";

  // create a new media reader with a bad filename

  @Test(expected=RuntimeException.class)
    public void testMediaSourceNotExist() 
  {
    IMediaReader mr = new MediaReader("broken" + TEST_FILE_20_SECONDS,
      ConverterFactory.XUGGLER_BGR_24);
    mr.readPacket();
  }  

  // test nominal read without buffered image creation

  @Test
    public void testMediaReaderOpenReadClose()
  {
    final int[] counts = new int[2];
 
    // create a new media reader

    MediaReader mr = new MediaReader(TEST_FILE_20_SECONDS);
    mr.addListener(new MediaViewer(mViewerMode, true));

    // setup the the listener

    IMediaPipeListener mrl = new MediaPipeAdapter()
      {
        public void onVideoPicture(IMediaPipe tool, IVideoPicture picture,
          BufferedImage image, int streamIndex)
        {
          assertNotNull("picture should be created", picture);
          assertNull("no buffered image should be created", image);
          ++counts[0];
        }

        public void onAudioSamples(IMediaPipe tool, IAudioSamples samples, 
          int streamIndex)
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
  }

  // test nominal read with buffered image creation
  
  @Test
  public void testCreateBufferedImages()
  {
    // if color space conversion is not supported, skip this test 

    if (!IVideoResampler.isSupported(
        IVideoResampler.Feature.FEATURE_COLORSPACECONVERSION))
      return;
    
    // create a new media reader

    MediaReader mr = new MediaReader(TEST_FILE_20_SECONDS, 
      ConverterFactory.XUGGLER_BGR_24);
    mr.addListener(new MediaViewer(mViewerMode, true));

    // setup the the listener

    IMediaPipeListener mrl = new MediaPipeAdapter()
      {
        public void onVideoPicture(IMediaPipe tool, IVideoPicture picture,
          BufferedImage image, int streamIndex)
        {
          assertNotNull("picture should be created", picture);
          assertNotNull("buffered image should be created", image);
        }

        public void onAudioSamples(IMediaPipe tool, IAudioSamples samples, int streamIndex)
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
    // if color space conversion is not supported, skip this test 

    if (!IVideoResampler.isSupported(
        IVideoResampler.Feature.FEATURE_COLORSPACECONVERSION))
      return;
    
    final int[] counts = new int[2];

    // create the container
    
    IContainer container = IContainer.make();
    
    // open the container
    
    if (container.open(TEST_FILE_20_SECONDS, IContainer.Type.READ,
        null, false, true) < 0)
      throw new IllegalArgumentException(
        "could not open: " + TEST_FILE_20_SECONDS);
        
    // create a new media reader

    MediaReader mr = new MediaReader(container, 
      ConverterFactory.XUGGLER_BGR_24);
    mr.addListener(new MediaViewer(mViewerMode, true));

    // setup the the listener

    IMediaPipeListener mrl = new MediaPipeAdapter()
      {
        public void onVideoPicture(IMediaPipe tool, IVideoPicture picture,
          BufferedImage image, int streamIndex)
        {
          assertNotNull("picture should be created", picture);
          assertNotNull("buffered image should be created", image);
          ++counts[0];
        }

        public void onAudioSamples(IMediaPipe tool, IAudioSamples samples,
          int streamIndex)
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
