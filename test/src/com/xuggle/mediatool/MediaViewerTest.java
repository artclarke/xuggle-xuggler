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
import java.util.concurrent.TimeUnit;

import org.junit.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.swing.WindowConstants;

import static java.util.concurrent.TimeUnit.MICROSECONDS;

import static com.xuggle.mediatool.IMediaViewer.Mode.*;

public class MediaViewerTest
{
  // the log

  private static final Logger log = LoggerFactory.getLogger(MediaViewerTest.class);
  { log.trace("<init>"); }

  // the xuggle system time unit

  public static final TimeUnit TIME_UNIT = MICROSECONDS;

  // the media view mode
  
  final MediaViewer.Mode mViewerMode = IMediaViewer.Mode.valueOf(
    System.getProperty(this.getClass().getName() + ".ViewerMode", 
      DISABLED.name()));

  // standard test name prefix

  final String PREFIX = this.getClass().getName() + "-";

  // location of test files

  public static final String TEST_FILE_DIR = "fixtures";
 
  public static final String INPUT_FILENAME = TEST_FILE_DIR + "/testfile.flv";
  //  public static final String INPUT_FILENAME = TEST_FILE_DIR + "/goose.wmv";
//   public static final String INPUT_FILENAME = 
//     TEST_FILE_DIR + "/testfile_bw_pattern.flv";

  // test thet the proper number of events are signaled during reading
  // and writng of a media file

  @Test()
    public void testViewer()
  {
    File inputFile = new File(INPUT_FILENAME);
    assert (inputFile.exists());

    MediaReader reader = new MediaReader(INPUT_FILENAME);

    reader.addListener(new MediaViewer(mViewerMode, true, 
        WindowConstants.EXIT_ON_CLOSE));

    while (reader.readPacket() == null)
      ;
  }
}
