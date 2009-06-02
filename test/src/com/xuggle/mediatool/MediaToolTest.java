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

import org.junit.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;

import com.xuggle.mediatool.MediaDebugListener;
import com.xuggle.mediatool.MediaReader;
import com.xuggle.mediatool.MediaViewer;
import com.xuggle.mediatool.MediaWriter;
import com.xuggle.xuggler.IError;

import static com.xuggle.mediatool.IMediaDebugListener.Event.*;
import static com.xuggle.mediatool.IMediaDebugListener.Mode.*;
import static com.xuggle.mediatool.IMediaViewer.Mode.*;

import static junit.framework.Assert.*;

public class MediaToolTest
{
  // the log

  private final Logger log = LoggerFactory.getLogger(this.getClass());
  { log.trace("<init>"); }

  // show the videos during transcoding?
  
  final MediaViewer.Mode mViewerMode = IMediaViewer.Mode.valueOf(
    System.getProperty(this.getClass().getName() + ".ViewerMode", 
      AUDIO_VIDEO.name()));

  // standard test name prefix

  final String PREFIX = this.getClass().getName() + "-";

  // location of test files

  public static final String TEST_FILE_DIR = "fixtures";

  // one of the stock test files
  
  public static final String INPUT_FILENAME  = TEST_FILE_DIR +
    "/testfile_videoonly_20sec.flv";

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

  public final String OUTPUT_FILENAME = PREFIX + "output.flv";

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
    reader.addListener(writer);

    writer.addListener(new MediaViewer(mViewerMode, true, 0));

    MediaDebugListener readerCounter = new MediaDebugListener(URL, META_DATA);
    MediaDebugListener writerCounter = new MediaDebugListener(EVENT, META_DATA);

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

    log.debug("reader " + readerCounter);
    log.debug("writer " + writerCounter);
  }
}
