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

import java.io.File;


import java.util.Collection;
import java.util.Vector;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import org.junit.Test;

import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

import com.xuggle.mediatool.MediaListenerAdapter;
import com.xuggle.mediatool.MediaDebugListener;
import com.xuggle.mediatool.MediaReader;
import com.xuggle.mediatool.MediaViewer;
import com.xuggle.mediatool.MediaWriter;

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

    {"testfile_mpeg1video_mp2audio.mpg", "mov"}, // QUICKTIME fail (VLC OK)
    
    // converting to avi
    
    {"ucl_h264_aac.mp4",                 "avi"}, // falis in VLC
    {"testfile_h264_mp4a_tmcd.mov",      "avi"}, // failed to write header
    {"subtitled_video.mkv",              "avi"}, // Non-aligned pointer freed
    
    // converting to flv
    
    {"subtitled_video.mkv",              "flv"}, // failed to write header
    {"ucl_h264_aac.mp4",                 "flv"}, // no video VLC
    {"testfile_h264_mp4a_tmcd.mov",      "flv"}, // no video VLC
    {"testfile_mpeg1video_mp2audio.mpg", "flv"}, // failed to write header
    
    // converting to mpg
    
    {"testfile_h264_mp4a_tmcd.mov",      "mpg"}, // failed to write header
    {"subtitled_video.mkv",              "mpg"}, // Non-aligned pointer freed
    {"testfile_bw_pattern.flv",          "mpg"}, // QUICKTIME fail
    {"testfile_videoonly_20sec.flv",     "mpg"}, // QUICKTIME fail
    {"testfile.flv",                     "mpg"}, // QUICKTIME fail
    {"ucl_h264_aac.mp4",                 "mpg"}, // QUICKTIME fail, no audio VLC
    {"testfile_mpeg1video_mp2audio.mpg", "mpg"}, // buffer underflow
    
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
          public void onVideoPicture(MediaVideoPictureEvent event)
          {
            writer.onVideoPicture(event);
          }
          
          /** {@inheritDoc} */
          
          public void onAudioSamples(MediaAudioSamplesEvent event)
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
