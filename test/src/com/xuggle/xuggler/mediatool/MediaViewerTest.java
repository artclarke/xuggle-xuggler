/*
 * Copyright (c) 2008-2009 by Xuggle Inc. All rights reserved.
 * 
 * It is REQUESTED BUT NOT REQUIRED if you use this library, that you let us
 * know by sending e-mail to info@xuggle.com telling us briefly how you're using
 * the library and what you like or don't like about it.
 * 
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 * 
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

package com.xuggle.xuggler.mediatool;

import org.junit.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;

import com.xuggle.xuggler.IError;

import static junit.framework.Assert.*;

public class MediaViewerTest
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());
  {
    log.trace("<init>");
  }

  // show the videos during tests

  public static final boolean SHOW_VIDEO = 
    System.getProperty(MediaViewerTest.class.getName() + ".ShowVideo") != null;
    //true;
  // test filename

  // public static final String INPUT_FILENAME =
  // "fixtures/testfile_bw_pattern.flv";
  public static final String INPUT_FILENAME = "fixtures/testfile.flv";

  // test thet the proper number of events are signaled during reading
  // and writng of a media file

  @Test()
  public void testViewer()
  {
    File inputFile = new File(INPUT_FILENAME);
    assert (inputFile.exists());

    MediaReader reader = new MediaReader(INPUT_FILENAME);
    reader.setAddDynamicStreams(false);
    reader.setQueryMetaData(true);

    if (SHOW_VIDEO)
      reader.addListener(new MediaViewer(true, true, 0));

    IError rv;
    while ((rv = reader.readPacket()) == null)
      ;

    assertEquals(IError.Type.ERROR_EOF, rv.getType());
  }
}
