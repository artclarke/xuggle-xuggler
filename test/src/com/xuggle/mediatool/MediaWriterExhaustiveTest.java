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

package com.xuggle.mediatool;

import java.io.File;


import java.util.Collection;
import java.util.Vector;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import org.junit.Ignore;
import org.junit.Test;

import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

import com.xuggle.mediatool.MediaListenerAdapter;
import com.xuggle.mediatool.MediaDebugListener;
import com.xuggle.mediatool.MediaReader;
import com.xuggle.mediatool.MediaViewer;
import com.xuggle.mediatool.MediaWriter;
import com.xuggle.mediatool.event.IAudioSamplesEvent;
import com.xuggle.mediatool.event.IVideoPictureEvent;

import static com.xuggle.mediatool.IMediaDebugListener.Event.*;
import static com.xuggle.mediatool.IMediaDebugListener.Mode.*;
import static com.xuggle.mediatool.IMediaViewer.Mode.*;

@RunWith(Parameterized.class)
public class MediaWriterExhaustiveTest
{
  // the log

  private final Logger log = LoggerFactory.getLogger(this.getClass());
  { log.trace("<init>"); }

  // show the videos during transcoding?
  
  final MediaViewer.Mode mViewerMode = IMediaViewer.Mode.valueOf(
    System.getProperty(this.getClass().getName() + ".ViewerMode", 
      DISABLED.name()));

  // test broken media files

  final static boolean mTestBroken = !System.getProperty(
    MediaWriterExhaustiveTest.class.getName() + ".TestBroken", "false")
    .equals("false");

  // standard test name prefix

  final String PREFIX = this.getClass().getName() + "-";

  // location of test files

  public static final String TEST_FILE_DIR = "fixtures";

  // test container (true) or MediaReader (false) cases

  public static final boolean[] CONTAINER_READER_CASES = 
  {
    true,
    false,
  };

  // all video test files
  
  public static final String[] TEST_VIDEO_FILES =
  {
    "testfile_bw_pattern.flv",
    "testfile_videoonly_20sec.flv",
    "subtitled_video.mkv",
    "testfile.flv",
    "ucl_h264_aac.mp4",
    "testfile_h264_mp4a_tmcd.mov",
    "testfile_mpeg1video_mp2audio.mpg",
  };

  // all video types to convert to

  public static final String[] TEST_VIDEO_TYPES =
  {
    "mov", 
    "avi",
    "flv",
    "mpg",
    "mkv",
  };

  // the following combinations currently fail to transcode
  // ideally this list gets smaller and smaller as times goes

  public static final String[][] BROKEN_TEST_COMBINATIONS =
  {
    // converting to mov

    {"testfile_mpeg1video_mp2audio.mpg", "mov"}, // QUICKTIME fail (VLC OK); I'd guess this works now that right codec is chosen

    //    // converting to avi
    {"subtitled_video.mkv",              "avi"}, // Works on Linux; non-aligned pointer on Mac
    
    {"subtitled_video.mkv",              "flv"}, // Fails due to unsupported sample rate in FLV; should add audio resampler
    {"testfile_mpeg1video_mp2audio.mpg", "flv"}, // Fails due to unsupported sample rate in FLV; should add audio resampler
    
    // converting to mpg
    
    {"subtitled_video.mkv",              "mpg"}, // Fails due to unsupported 359/12 frame frate for MPEG
    {"testfile_bw_pattern.flv",          "mpg"}, // Fails due to unsupported 239/4 frame frate for MPEG
    {"testfile_videoonly_20sec.flv",     "mpg"}, // Fails due to unsupported 15/1 frame rate for MPEG
    {"testfile.flv",                     "mpg"}, // Fails due to unsupported 15/1 frame rate for MPEG
    
    
    // Now Working
    
//    {"ucl_h264_aac.mp4",                 "avi"}, // falis in VLC
//    {"testfile_h264_mp4a_tmcd.mov",      "avi"}, // failed to write header
//    {"ucl_h264_aac.mp4",                 "flv"}, // no video VLC
//    {"testfile_h264_mp4a_tmcd.mov",      "flv"}, // no video VLC
//    {"testfile_h264_mp4a_tmcd.mov",      "mpg"}, // failed to write header
//    {"ucl_h264_aac.mp4",                 "mpg"}, // QUICKTIME fail, no audio VLC
//    {"testfile_mpeg1video_mp2audio.mpg", "mpg"}, // buffer underflow, but creates valid file
    
    // converting to mkv (currently all working)
  };

  // construct parameters for tests

  @Parameters
    public static Collection<Object[]> converterTypes()
  {
    Collection<Object[]> parameters = new Vector<Object[]>();

    for (boolean testContainer: CONTAINER_READER_CASES)
      for (String source: TEST_VIDEO_FILES)
        for (String destExt: TEST_VIDEO_TYPES)
        {
          boolean isBroken = false;
          
          for (String[] pair: BROKEN_TEST_COMBINATIONS)
            if (source.equals(pair[0]) && destExt.equals(pair[1]))
            {
              isBroken = true;
              break;
            }

          String[] parts = source.split("\\x2E");
          String destination = 
            (testContainer ? "container" : "reader") + "-" +
            parts[0] + "-" + parts[1] + "." + destExt;


          boolean skipTest = isBroken && !mTestBroken || 
            !isBroken && mTestBroken;
          String description = 
            (skipTest      ? "SKIPPING"  : " converting ") + " " +
            (isBroken      ? "BROKEN"    : " good "      ) + " " + 
            (testContainer ? "container" : " reader  "   ) + " " + 
            source + " -> " + destination;

          if (skipTest)
          {
            System.out.println(description);
          }
          else
          {
            Object[] tuple = {source, destination, isBroken, 
                              testContainer, description};
            parameters.add(tuple);
          }
        }

    return parameters;
  }

  // test parameters

  private final String  mSource;
  private final String  mDestination;
  private final boolean mTestContainer;
  
  public MediaWriterExhaustiveTest(String source, String destination, 
    boolean isBroken, boolean testContainer, String description)
  {
    System.out.println(description);
    mSource = TEST_FILE_DIR + "/" + source;
    mDestination = PREFIX + destination;
    mTestContainer = testContainer;
  }

  @Test
  @Ignore // ignore for 3.3
    public void transcodeTest()
  {
    // setup and test file status

    File sourceFile = new File(mSource);
    File destinationFile = new File(mDestination);
    assert(sourceFile.exists());
    destinationFile.delete();
    assert(!destinationFile.exists());

    // create the reader

    MediaReader reader = new MediaReader(mSource);

    // construct a writer which does not get called directly by the
    // MediaReader, and thus many things need to be handled manually

    if (mTestContainer)
    {
      final MediaWriter writer = new MediaWriter(mDestination, 
        reader.getContainer());
      writer.setMaskLateStreamExceptions(false);
      writer.addListener(new MediaViewer(mViewerMode, true));

      writer.addListener(new MediaDebugListener(OPEN, CLOSE));

      reader.addListener(new MediaListenerAdapter()
        {
          public void onVideoPicture(IVideoPictureEvent event)
          {
            writer.onVideoPicture(event);
          }
          
          /** {@inheritDoc} */
          
          public void onAudioSamples(IAudioSamplesEvent event)
          {
            writer.onAudioSamples(event);
          }
        });

      // transcode

      while (reader.readPacket() == null)
        ;

      // close the container
      
      writer.close();
    }

    // construct a writer give a reader, the easy simple case

    else
    {
      // construct the writer, no need to keep a reference to the
      // writer, it's maintained in the reader

      MediaWriter writer = new MediaWriter(mDestination, reader);
      reader.addListener(writer);
      writer.setMaskLateStreamExceptions(false);
      writer.addListener(new MediaViewer(mViewerMode, true));

      writer.addListener(new MediaDebugListener(EVENT, META_DATA));

      // transcode

      while (reader.readPacket() == null)
        ;
    }

    // confirm file exists and has at least 50k of data

    assert(destinationFile.exists());
    assert(destinationFile.length() > 50000);
  }
}
