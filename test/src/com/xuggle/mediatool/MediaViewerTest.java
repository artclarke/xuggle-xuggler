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
import java.nio.ShortBuffer;

import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.geom.Ellipse2D;
import java.awt.RenderingHints;
import java.awt.geom.Rectangle2D;
import java.awt.image.BufferedImage;

import java.util.Random;
import java.util.Vector;
import java.util.Collection;
import java.util.concurrent.TimeUnit;

import org.junit.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.swing.WindowConstants;

import com.xuggle.mediatool.event.IAudioSamplesEvent;
import com.xuggle.mediatool.event.IVideoPictureEvent;
import com.xuggle.xuggler.IError;
import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IAudioSamples;
  
import static com.xuggle.mediatool.IMediaViewer.Mode.*;

import static java.util.concurrent.TimeUnit.SECONDS;
import static java.util.concurrent.TimeUnit.MILLISECONDS;
import static java.util.concurrent.TimeUnit.MICROSECONDS;

import static junit.framework.Assert.*;

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
