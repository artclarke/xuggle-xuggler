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

import java.io.File;

import java.awt.image.BufferedImage;

import java.util.Set;
import java.util.Vector;
import java.util.HashSet;
import java.util.Collection;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import org.junit.Test;
import org.junit.Before;

import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

import com.xuggle.xuggler.video.ConverterFactory;
import com.xuggle.xuggler.IError;

import static junit.framework.Assert.*;

@RunWith(Parameterized.class)
public class MediaWriterExhaustiveTest
{
  final private Logger log = LoggerFactory.getLogger(this.getClass());
  { log.trace("<init>"); }

  // location of test files

  public static final String TEST_FILE_DIR = "fixtures";

  // show the videos during transcoding?

  final boolean SHOW_VIDEO = System.getProperty(
    MediaWriterExhaustiveTest.class.getName()+".ShowVideo") != null;

  // test the good files or the broken ones?

  public static final boolean TEST_BROKEN = System.getProperty(
    MediaWriterExhaustiveTest.class.getName()+".TestBroken") != null;

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

    {"subtitled_video.mkv",              "mov"}, // failed to write header
    {"testfile_h264_mp4a_tmcd.mov",      "mov"}, // failed to write header
    {"testfile_mpeg1video_mp2audio.mpg", "mov"}, // ERROR_IO not EOF

    // converting to avi

    {"subtitled_video.mkv",              "avi"}, // failed to write header
    {"testfile_h264_mp4a_tmcd.mov",      "avi"}, // failed to write header
    {"testfile_mpeg1video_mp2audio.mpg", "avi"}, // ERROR_IO not EOF

    // converting to flv

    {"subtitled_video.mkv",              "flv"}, // failed to write header
    {"testfile_h264_mp4a_tmcd.mov",      "flv"}, // failed to write header
    {"testfile_mpeg1video_mp2audio.mpg", "flv"}, // failed to write header

    // converting to mpg

    {"testfile_h264_mp4a_tmcd.mov",      "mpg"}, // failed to write header
    {"subtitled_video.mkv",              "mpg"}, // Non-aligned pointer freed
    {"testfile_bw_pattern.flv",          "mpg"}, // fails in QUICKTIME
    {"testfile_videoonly_20sec.flv",     "mpg"}, // fails in QUICKTIME
    {"testfile.flv",                     "mpg"}, // fails in QUICKTIME
    {"testfile_mpeg1video_mp2audio.mpg", "mpg"}, // ERROR_IO not EOF

    // converting to mkv

    {"testfile_h264_mp4a_tmcd.mov",      "mkv"}, // "unsupported operation" on read
    {"testfile_mpeg1video_mp2audio.mpg", "mkv"}, // ERROR_IO not EOF
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
          String destination = "MW-" +
            (testContainer ? "container" : "reader") + "-" +
            parts[0] + "-" + parts[1] + "." + destExt;


          boolean skipTest = isBroken && !TEST_BROKEN || !isBroken && TEST_BROKEN;
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
  private final boolean mIsBroken;
  private final boolean mTestContainer;
  
  public MediaWriterExhaustiveTest(String source, String destination, 
    boolean isBroken, boolean testContainer, String description)
  {
    System.out.println(description);
    mSource = TEST_FILE_DIR + "/" + source;
    mDestination = TEST_FILE_DIR + "/" + destination;
    mIsBroken = isBroken;
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
    if (SHOW_VIDEO)
      reader.addListener(new MediaViewer());

    // construct a writer which does not get called directly by the
    // MediaReader, and thus many things need to be handled manually

    if (mTestContainer)
    {
      final MediaWriter writer = new MediaWriter(mDestination, reader.getContainer());

      reader.addListener(new MediaReader.ListenerAdapter()
        {
          public void onVideoPicture(IVideoPicture picture, BufferedImage image, 
            int streamIndex)
          {
            writer.onVideoPicture(picture, image, streamIndex);
          }
          
          /** {@inheritDoc} */
          
          public void onAudioSamples(IAudioSamples samples, int streamIndex)
          {
            writer.onAudioSamples(samples, streamIndex);
          }
        });

      while (reader.readPacket() == null)
        ;
      
      writer.close();
    }

    // construct a writre give a reader, the easy simple case

    else
    {
      // constructe the writer, no need to keep a reference to the
      // writer, it's maintained in the reader

      new MediaWriter(mDestination, reader);

      // transcode

      IError error;
      while ((error = reader.readPacket()) == null)
        ;

      assertEquals(IError.Type.ERROR_EOF, error.getType());
    }

    // confirm file exists and has at least 50k of data

    assert(destinationFile.exists());
    assert(destinationFile.length() > 50000);
  }
}
