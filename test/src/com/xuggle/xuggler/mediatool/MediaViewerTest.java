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

package com.xuggle.xuggler.mediatool;

import org.junit.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;

import javax.swing.WindowConstants;

import com.xuggle.xuggler.IError;

import static com.xuggle.xuggler.mediatool.MediaViewer.Mode.*;

import static junit.framework.Assert.*;

public class MediaViewerTest
{
  // the log

  private final Logger log = LoggerFactory.getLogger(this.getClass());
  { log.trace("<init>"); }

  // the media view mode
  
  final MediaViewer.Mode mViewerMode = MediaViewer.Mode.valueOf(
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
    //com.xuggle.ferry.JNIMemoryAllocator.setMirroringNativeMemoryInJVM(false);
    
    File inputFile = new File(INPUT_FILENAME);
    assert (inputFile.exists());

    MediaReader reader = new MediaReader(INPUT_FILENAME);

    reader.addListener(new MediaViewer(mViewerMode, true, 
        WindowConstants.EXIT_ON_CLOSE));
    // reader.addListener(new DebugListener(DebugListener.Mode.EVENT,
    // DebugListener.Event.ALL));

    IError rv;
    while ((rv = reader.readPacket()) == null)
      ;

    assertEquals(IError.Type.ERROR_EOF, rv.getType());
  }
}
