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

package com.xuggle.xuggler.mediatool.demos;

import java.awt.image.BufferedImage;

import com.xuggle.xuggler.Global;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.mediatool.IMediaTool;
import com.xuggle.xuggler.mediatool.MediaReader;
import com.xuggle.xuggler.mediatool.MediaViewer;
import com.xuggle.xuggler.video.ConverterFactory;

/**
 * Using {@link MediaReader}, takes a media container, finds the first video
 * stream, decodes that stream, and plays the video.
 * 
 * This code does a coarse job of matching time-stamps, and thus the video may
 * not play in exact real time.
 * 
 * @author aclarke
 * @author trebor
 */

public class DecodeAndPlayVideo
{
  private long mStartTimeStamp = Global.NO_PTS;
  private long mStartClockTime = 0L;

  /**
   * Takes a media container (file) as the first argument, opens it, opens up a
   * Swing window and displays video frames with <i>roughly</i> the right
   * timing.
   * 
   * @param args
   *          Must contain one string which represents a filename
   */

  public static void main(String[] args)
  {
    if (args.length <= 0)
      throw new IllegalArgumentException(
          "must pass in a filename as the first argument");

    // create a new mr. decode an play video

    new DecodeAndPlayVideo(args[0]);
  }

  /**
   * Construct a DecodeAndPlayVideo which reads and plays a video file.
   * 
   * @param filename
   *          the name of the media file to read
   */

  public DecodeAndPlayVideo(String filename)
  {
    // create a media reader for processing video, stipulate that we
    // want BufferedImages to created in BGR 24bit color space

    MediaReader mediaReader = new MediaReader(filename,
        ConverterFactory.XUGGLER_BGR_24);

    //
    // Create a MediaViewer object, but override the onVideoPicture
    // so it slows down the reader to only display the video when
    // it is due to be displayed on the screen.
    //
    mediaReader.addListener(new MediaViewer()
    {
      // These just exist as AtomicLongs so that the closure can set it to a new
      // value on the first time through the listener.
      @Override
      public void onVideoPicture(IMediaTool tool, IVideoPicture picture,
          BufferedImage image, int index)
      {
        long now = System.nanoTime() / 1000;
        if (mStartTimeStamp == Global.NO_PTS)
        {
          mStartTimeStamp = picture.getTimeStamp();
          mStartClockTime = now;
        }
        long containerOffsetFromStart = picture.getTimeStamp()
            - mStartTimeStamp;
        long clockOffsetFromStart = now - mStartClockTime;
        long waitOffsetMilliseconds =
          (containerOffsetFromStart - clockOffsetFromStart) / 1000;
        if (waitOffsetMilliseconds > 0)
        {
          try
          {
            Thread.sleep(waitOffsetMilliseconds);
          }
          catch (InterruptedException e)
          {
            return;
          }
        }
        super.onVideoPicture(tool, picture, image, index);
      }
    });

    // read out the contents of the media file, note that nothing else
    // happens here. action happens in the onVideoPicture() method
    // which is called when complete video pictures are extracted from
    // the media source

    while (mediaReader.readPacket() == null)
      ;
  }
}
