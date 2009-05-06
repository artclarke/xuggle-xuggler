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

import java.util.Set;
import java.util.HashSet;

import org.junit.Test;
import org.junit.Before;

import com.xuggle.xuggler.video.ConverterFactory;

import static junit.framework.Assert.*;

public class MediaWriterTest
{
  // show the videos during transcoding?

  public static final boolean SHOW_VIDEO = false;

  // test the good files or the broken ones?

  public static final boolean TEST_BROKEN = false;

  // one of the stock test fiels
  
  public static final String TEST_FILE = "fixtures/testfile_bw_pattern.flv";

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

  // the following combinations fail

  public static final String[][] BROKEN_TEST_COMBINATIONS =
  {
    // converting to mov

    {"subtitled_video.mkv",              "mov"}, // failed to write header
    {"testfile_h264_mp4a_tmcd.mov",      "mov"}, // failed to write header
    {"testfile_mpeg1video_mp2audio.mpg", "mov"}, // fails in QUICKTIME

    // converting to avi

    {"subtitled_video.mkv",              "avi"}, // failed to write header
    {"testfile_h264_mp4a_tmcd.mov",      "avi"}, // failed to write header

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
  };

  // all target output types

  public static final String[] TEST_AUDIO_FILES =
  {
    "testfile.mp3",
  };

  MediaReader mReader;

  @Before
    public void beforeTest()
  {
    mReader = new MediaReader(TEST_FILE, true, ConverterFactory.XUGGLER_BGR_24);
  }
  
   @Test(expected=IllegalArgumentException.class)
    public void improperReaderConfigTest()
  {
    System.out.println("---- improperReaderConfigTest ----");
    File file = new File("fixtures/should-not-be-created.flv");
    file.delete();
    assert(!file.exists());
    mReader.setAddDynamicStreams(true);
    new MediaWriter(file.toString(), mReader);
    assert(!file.exists());
  }

  @Test(expected=IllegalArgumentException.class)
    public void improperUrlTest()
  {
    File file = new File("/tmp/foo/bar/baz/should-not-be-created.flv");
    file.delete();
    assert(!file.exists());
    new MediaWriter(file.toString(), mReader);
    assert(!file.exists());
  }

  //@Test
    public void transcodeToFlvTest()
  {
    File file = new File("fixtures/MediaWriter-transcode.flv");
    file.delete();
    assert(!file.exists());
    new MediaWriter(file.toString(), mReader);
    while (mReader.readPacket() == null)
      ;
    assert(file.exists());
    assertEquals(file.length(), 1062946);
    System.out.println("manually check: " + file);
  }

  //  @Test
    public void transcodeToMovTest()
  {
    File file = new File("fixtures/MediaWriter-transcode.mov");
    file.delete();
    assert(!file.exists());
    new MediaWriter(file.toString(), mReader);
    while (mReader.readPacket() == null)
      ;
    assert(file.exists());
    assertEquals(file.length(), 1061745);
    System.out.println("manually check: " + file);
  }

  //@Test
    public void transcodeWithContainer()
  {
    File file = new File("fixtures/MediaWriter-transcode-container.mov");
    file.delete();
    assert(!file.exists());
    MediaWriter writer = new MediaWriter(file.toString(), 
      mReader.getContainer());
    mReader.addListener(writer);
    while (mReader.readPacket() == null)
      ;
    assert(file.exists());
    assertEquals(file.length(), 1061745);
    System.out.println("manually check: " + file);
  }

  @Test
    public void transcodeManyToMany()
  {
    Set<String> destinations = new HashSet<String>();

    for (String source: TEST_VIDEO_FILES)
      for (String destExt: TEST_VIDEO_TYPES)
      {
        boolean broken = false;

        for (String[] pair: BROKEN_TEST_COMBINATIONS)
          if (source.equals(pair[0]) && destExt.equals(pair[1]))
          {
            broken = true;
            break;
          }

        String[] parts = source.split("\\x2E");
        String destination = "fixtures/MediaWriter-" + 
          parts[0] + "-" + parts[1] + "." + destExt;

        String description = (broken ? "BROKEN " : " good  ") + source + 
          " -> " + destination;
          
        if (broken && !TEST_BROKEN || !broken && TEST_BROKEN)
          System.out.printf(" SKIPPING  %s\n", description);
        else
        {
          System.out.printf("converting %s\n", description);
          transcode("fixtures/" + source, destination);
          destinations.add(destination);
        }
      }

    System.out.println("manually check: ");
    for(String destination: destinations)
      System.out.println("  " + destination);
  }

  public void transcode(String source, String destination)
  {
    // setup and test file status

    File sourceFile = new File(source);
    File destinationFile = new File(destination);
    assert(sourceFile.exists());
    destinationFile.delete();
    assert(!destinationFile.exists());

    // setup reader

    MediaReader reader = null;
    if (SHOW_VIDEO)
    {
      reader = new MediaReader(source, true, ConverterFactory.XUGGLER_BGR_24);
      reader.addListener(new MediaViewer());
    }
    else
      reader = new MediaReader(source, false, null);

    // and the writer, no need to keep a reference around it's kept in
    // the reader

    new MediaWriter(destination, reader);

    // transcode

    while (reader.readPacket() == null)
      ;

    // confirm results

    assert(destinationFile.exists());
  }
}
