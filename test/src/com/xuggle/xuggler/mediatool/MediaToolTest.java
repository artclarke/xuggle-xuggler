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

import org.junit.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;

import com.xuggle.xuggler.mediatool.MediaReader;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IError;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IPacket;
import com.xuggle.xuggler.IStream;

import java.awt.image.BufferedImage;

import static junit.framework.Assert.*;

public class MediaToolTest
{
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
    "MediaTool-output.flv";

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

    MediaEventCounter readerCounter = new MediaEventCounter();
    MediaEventCounter writerCounter = new MediaEventCounter();

    reader.addListener(readerCounter);
    writer.addListener(writerCounter);
    
    IError rv;
    while ((rv = reader.readPacket()) == null)
      ;

    assertEquals(IError.Type.ERROR_EOF    , rv.getType());

    assertEquals(READER_VIDEO_FRAME_COUNT , readerCounter.mOnVideoPictureCount);
    assertEquals(READER_AUDIO_FRAME_COUNT , readerCounter.mOnAudioSamplesCount);
    assertEquals(1                        , readerCounter.mOnOpenCount);
    assertEquals(1                        , readerCounter.mOnCloseCount);
    assertEquals(2                        , readerCounter.mOnOpenStreamCount);
    assertEquals(2                        , readerCounter.mOnCloseStreamCount);
    assertEquals(READER_PACKET_READ_COUNT , readerCounter.mOnReadPacketCount);
    assertEquals(READER_PACKET_WRITE_COUNT, readerCounter.mOnWritePacketCount);
    assertEquals(0                        , readerCounter.mOnWriteHeader);
    assertEquals(0                        , readerCounter.mOnFlushCount);
    assertEquals(0                        , readerCounter.mOnWriteTrailer);

    assertEquals(WRITER_VIDEO_FRAME_COUNT , writerCounter.mOnVideoPictureCount);
    assertEquals(WRITER_AUDIO_FRAME_COUNT , writerCounter.mOnAudioSamplesCount);
    assertEquals(1                        , writerCounter.mOnOpenCount);
    assertEquals(1                        , writerCounter.mOnCloseCount);
    assertEquals(2                        , writerCounter.mOnOpenStreamCount);
    assertEquals(2                        , writerCounter.mOnCloseStreamCount);
    assertEquals(WRITER_PACKET_READ_COUNT , writerCounter.mOnReadPacketCount);
    assertEquals(WRITER_PACKET_WRITE_COUNT, writerCounter.mOnWritePacketCount);
    assertEquals(1                        , writerCounter.mOnWriteHeader);
    assertEquals(1                        , writerCounter.mOnFlushCount);
    assertEquals(1                        , writerCounter.mOnWriteTrailer);

    assert(outputFile.exists());
    assertEquals(OUTPUT_FILE_SIZE, outputFile.length(), 200);
  }

  class MediaEventCounter implements IMediaListener
  {
    public int mOnVideoPictureCount = 0;
    public int mOnAudioSamplesCount = 0;
    public int mOnOpenCount = 0;
    public int mOnCloseCount = 0;
    public int mOnOpenStreamCount = 0;
    public int mOnCloseStreamCount = 0;
    public int mOnReadPacketCount = 0;
    public int mOnWritePacketCount = 0;
    public int mOnWriteHeader = 0;
    public int mOnFlushCount = 0;
    public int mOnWriteTrailer = 0;

    public void onVideoPicture(IMediaTool tool, IVideoPicture picture,
      BufferedImage image, int streamIndex)
    {
      ++mOnVideoPictureCount;
    }

    public void onAudioSamples(IMediaTool tool, IAudioSamples samples,
      int streamIndex)
    {
      ++mOnAudioSamplesCount;
    }

    public void onOpen(IMediaTool tool)
    {
      ++mOnOpenCount;
    }

    public void onClose(IMediaTool tool)
    {
      ++mOnCloseCount;
    }

    public void onOpenStream(IMediaTool tool, IStream stream)
    {
      ++mOnOpenStreamCount;
    }

    public void onCloseStream(IMediaTool tool, IStream stream)
    {
      ++mOnCloseStreamCount;
    }

    public void onReadPacket(IMediaTool tool, IPacket packet)
    {
      ++mOnReadPacketCount;
    }

    public void onWritePacket(IMediaTool tool, IPacket packet)
    {
      ++mOnWritePacketCount;
    }

    public void onWriteHeader(IMediaTool tool)
    {
      ++mOnWriteHeader;
    }

    public void onFlush(IMediaTool tool)
    {
      ++mOnFlushCount;
    }

    public void onWriteTrailer(IMediaTool tool)
    {
      ++mOnWriteTrailer;
    }
  };
}
