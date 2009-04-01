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

package com.xuggle.xuggler.demos;

import javax.imageio.ImageIO;

import java.io.File;

import java.awt.image.BufferedImage;

import com.xuggle.xuggler.Global;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.MediaReader;
import com.xuggle.xuggler.video.ConverterFactory;

/**
 * Using {@link MediaReader}, takes a media container, finds the first video stream, decodes that
 * stream, and then writes video frames out to a PNG image file every 5
 * seconds, based on the video presentation timestamps.
 *
 * @author aclarke
 * @author trebor
 */

public class MrDecodeAndCaptureFrames extends MediaReader.ListenerAdapter
{
  /** 
   * The number of seconds between frames.
   */

  public static final double SECONDS_BETWEEN_FRAMES = 5;

  /** 
   * The number of nano-seconds between frames. 
   */

  public static final long NANO_SECONDS_BETWEEN_FRAMES = 
    (long)(Global.DEFAULT_PTS_PER_SECOND * SECONDS_BETWEEN_FRAMES);
  
  /** Time of last frame write. */
  
  private static long mLastPtsWrite = Global.NO_PTS;

  /**
   * The video stream index, used to ensure we display frames from one
   * and only one video stream from the media container.
   */

  private int mVideoStreamIndex = -1;

  /**
   * Takes a media container (file) as the first argument, opens it and
   *  writes some of it's video frames to PNG image files in the
   *  temporary directory.
   *  
   * @param args must contain one string which represents a filename
   */

  public static void main(String[] args)
  {
    if (args.length <= 0)
      throw new IllegalArgumentException(
        "must pass in a filename as the first argument");
    
    // create a new mr. decode and capture frames
    
    new MrDecodeAndCaptureFrames(args[0]);
  }

  /** Construct a MrDecodeAndCaptureFrames which reads and captures
   * frames from a video file.
   * 
   * @param filename the name of the media file to read
   */

  public MrDecodeAndCaptureFrames(String filename)
  {
    // create a media reader for processing video, stipulate that we
    // want BufferedImages to created in BGR 24bit color space

    MediaReader mediaReader = new MediaReader(filename, true,
      ConverterFactory.XUGGLER_BGR_24);
    
    // note that MrDecodeAndCaptureFrames is derived from
    // MediaReader.ListenerAdapter and thus may be added as a listener
    // to the MediaReader. MrDecodeAndCaptureFrames implements
    // onVideoPicture().

    mediaReader.addListener(this);

    // read out the contents of the media file, note that nothing else
    // happens here.  action happens in the onVideoPicture() method
    // which is called when complete video pictures are extracted from
    // the media source

    while (mediaReader.readPacket() == null)
      ;
  }

  /** 
   * Called after a video frame has been decoded from a media stream.
   * Optionally a BufferedImage version of the frame may be passed
   * if the calling {@link MediaReader} instance was configured to
   * create BufferedImages.
   * 
   * This method blocks, so return quickly.
   *
   * @param picture a raw video picture
   * @param image the buffered image, which will be null if buffered
   *        image creation is de-selected for this MediaReader.
   * @param streamIndex the index of the stream this object was decoded from.
   */

  public void onVideoPicture(IVideoPicture picture, BufferedImage image,
    int streamIndex)
  {
    try
    {
      // if the stream index does not match the selected stream index,
      // then have a closer look
      
      if (streamIndex != mVideoStreamIndex)
      {
        // if the selected video stream id is not yet set, go ahead an
        // select this lucky video stream
        
        if (-1 == mVideoStreamIndex)
          mVideoStreamIndex = streamIndex;
        
        // otherwise return, no need to show frames from this video stream
        
        else
          return;
      }
      
      // if uninitialized, backdate mLastPtsWrite so we get the very
      // first frame

      if (mLastPtsWrite == Global.NO_PTS)
        mLastPtsWrite = picture.getPts() - NANO_SECONDS_BETWEEN_FRAMES;

      // if it's time to write the next frame

      if (picture.getPts() - mLastPtsWrite >= NANO_SECONDS_BETWEEN_FRAMES)
      {
        // Make a temporary file name

        File file = File.createTempFile("frame", ".png");

        // write out PNG

        ImageIO.write(image, "png", file);

        // indicate file written

        double seconds = ((double)picture.getPts())
          / Global.DEFAULT_PTS_PER_SECOND;
        System.out.printf("at elapsed time of %6.3f seconds wrote: %s\n",
          seconds, file);
        
        // update last write time
        
        mLastPtsWrite += NANO_SECONDS_BETWEEN_FRAMES;
      }
    }
    catch (Exception e)
    {
      e.printStackTrace();
    }
  }
}
