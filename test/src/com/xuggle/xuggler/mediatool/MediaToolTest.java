/*
 * Copyright (c) 2008-2009 by Xuggle Inc. All rights reserved.
 *
 * It is REQUESTED BUT NOT REQUIRED if you use this library, that you
 * let us know by sending e-mail to info@xuggle.com telling us briefly
 * how you're using the library and what you like or don't like about
 * it.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

package com.xuggle.xuggler.mediatool;

import org.junit.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;

import com.xuggle.xuggler.IError;

import static com.xuggle.xuggler.mediatool.DebugListener.*;

import static junit.framework.Assert.*;

public class MediaToolTest
{
  // show the videos during tests

  public static final boolean SHOW_VIDEO = System.getProperty(
    MediaToolTest.class.getName()+".ShowVideo") != null;

  // stock output prefix

  public static final String PREFIX = MediaToolTest.class.getName() + "-";

  public static final String INPUT_FILENAME  =
    "fixtures/testfile_videoonly_20sec.flv";

  public static final int    READER_VIDEO_FRAME_COUNT  = 300;
  public static final int    READER_AUDIO_FRAME_COUNT  = 762;
  public static final int    READER_PACKET_READ_COUNT  = 
    READER_VIDEO_FRAME_COUNT + READER_AUDIO_FRAME_COUNT;
  public static final int    READER_PACKET_WRITE_COUNT = 0;

  public static final int    WRITER_VIDEO_FRAME_COUNT  = 300;
  public static final int    WRITER_AUDIO_FRAME_COUNT  = 762;
  public static final int    WRITER_PACKET_READ_COUNT  = 0;
  public static final int    WRITER_PACKET_WRITE_COUNT =  
    WRITER_VIDEO_FRAME_COUNT + WRITER_AUDIO_FRAME_COUNT + 3;

  public static final int    OUTPUT_FILE_SIZE          = 3826466;

//  public static final String INPUT_FILENAME  = 
//    "fixtures/ucl_h264_aac.mp4";
//
//   public static final int    READER_VIDEO_FRAME_COUNT  = 477;
//   public static final int    READER_AUDIO_FRAME_COUNT  = 684;
//   public static final int    READER_PACKET_READ_COUNT  = 
//     READER_VIDEO_FRAME_COUNT + READER_AUDIO_FRAME_COUNT;
//   public static final int    READER_PACKET_WRITE_COUNT = 0;

//   public static final int    WRITER_VIDEO_FRAME_COUNT  = 447;
//   public static final int    WRITER_AUDIO_FRAME_COUNT  = 684;
//   public static final int    WRITER_PACKET_READ_COUNT  = 0;
//   public static final int    WRITER_PACKET_WRITE_COUNT =  
//     WRITER_VIDEO_FRAME_COUNT + WRITER_AUDIO_FRAME_COUNT + 3;
//
//  public static final int    OUTPUT_FILE_SIZE          = ?;

  public static final String OUTPUT_FILENAME = 
    PREFIX + "output.flv";

  private final Logger log = LoggerFactory.getLogger(this.getClass());

  public MediaToolTest() {
    log.trace("<init>");
  }
  // test thet the proper number of events are signaled during reading
  // and writng of a media file

  @Test()
    public void testEventCounts()
  {
    File inputFile = new File(INPUT_FILENAME);
    File outputFile = new File(OUTPUT_FILENAME);
    assert(inputFile.exists());
    outputFile.delete();
    assert(!outputFile.exists());

    MediaReader reader = new MediaReader(INPUT_FILENAME);
    MediaWriter writer = new MediaWriter(OUTPUT_FILENAME, reader);

    if (SHOW_VIDEO)
      writer.addListener(new MediaViewer(false, true, 0));

    DebugListener readerCounter = new DebugListener(OPEN);
    DebugListener writerCounter = new DebugListener(CLOSE);

    reader.addListener(readerCounter);
    writer.addListener(writerCounter);

    
    IError rv;
    while ((rv = reader.readPacket()) == null)
      ;

    assertEquals(IError.Type.ERROR_EOF    , rv.getType());

    assertEquals(READER_VIDEO_FRAME_COUNT , readerCounter.getCount(VIDEO));
    assertEquals(READER_AUDIO_FRAME_COUNT , readerCounter.getCount(AUDIO));
    assertEquals(1                        , readerCounter.getCount(OPEN));
    assertEquals(1                        , readerCounter.getCount(CLOSE));
    assertEquals(2                        , readerCounter.getCount(ADD_STREAM));
    assertEquals(2                        , readerCounter.getCount(OPEN_STREAM));
    assertEquals(2                        , readerCounter.getCount(CLOSE_STREAM));
    assertEquals(READER_PACKET_READ_COUNT , readerCounter.getCount(READ_PACKET));
    assertEquals(READER_PACKET_WRITE_COUNT, readerCounter.getCount(WRITE_PACKET));
    assertEquals(0                        , readerCounter.getCount(HEADER));
    assertEquals(0                        , readerCounter.getCount(FLUSH));
    assertEquals(0                        , readerCounter.getCount(TRAILER));

    assertEquals(WRITER_VIDEO_FRAME_COUNT , writerCounter.getCount(VIDEO));
    assertEquals(WRITER_AUDIO_FRAME_COUNT , writerCounter.getCount(AUDIO));
    assertEquals(1                        , writerCounter.getCount(OPEN));
    assertEquals(1                        , writerCounter.getCount(CLOSE));
    assertEquals(2                        , writerCounter.getCount(ADD_STREAM));
    assertEquals(2                        , writerCounter.getCount(OPEN_STREAM));
    assertEquals(2                        , writerCounter.getCount(CLOSE_STREAM));
    assertEquals(WRITER_PACKET_READ_COUNT , writerCounter.getCount(READ_PACKET));
    assertEquals(WRITER_PACKET_WRITE_COUNT, writerCounter.getCount(WRITE_PACKET));
    assertEquals(1                        , writerCounter.getCount(HEADER));
    assertEquals(1                        , writerCounter.getCount(FLUSH));
    assertEquals(1                        , writerCounter.getCount(TRAILER));

    assert(outputFile.exists());
    assertEquals(OUTPUT_FILE_SIZE, outputFile.length(), 200);
  }
}
