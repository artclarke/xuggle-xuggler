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

package com.xuggle.xuggler.mediatool;

import java.io.File;


import org.junit.Test;
import org.junit.Before;

import com.xuggle.xuggler.mediatool.MediaReader;
import com.xuggle.xuggler.mediatool.MediaWriter;
import com.xuggle.xuggler.video.ConverterFactory;

import static junit.framework.Assert.*;

public class MediaWriterTest
{
  // show the videos during transcoding?

  public static final boolean SHOW_VIDEO = false;

  // one of the stock test fiels
  
  public static final String TEST_FILE = "fixtures/testfile_bw_pattern.flv";

  // the reader for these tests

  MediaReader mReader;

  @Before
    public void beforeTest()
  {
    mReader = new MediaReader(TEST_FILE, true, ConverterFactory.XUGGLER_BGR_24);
  }
  
  @Test(expected=IllegalArgumentException.class)
    public void improperReaderConfigTest()
  {
    File file = new File("fixtures/should-not-be-created.flv");
    file.delete();
    assert(!file.exists());
    mReader.setAddDynamicStreams(true);
    new MediaWriter(file.toString(), mReader);
    assert(!file.exists());
  }

  @Test(expected=IllegalArgumentException.class)
    public void improperInputContainerTypeTest()
  {
    File file = new File("fixtures/should-not-be-created.flv");
    file.delete();
    assert(!file.exists());
    MediaWriter writer = new MediaWriter(file.toString(), mReader);
    new MediaWriter(file.toString(), writer.getContainer());
    assert(!file.exists());
    assert(false);
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

  @Test
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

  @Test
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

  @Test
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
}
