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

package com.xuggle.mediatool.demos;

import javax.imageio.ImageIO;

import java.io.File;

import java.awt.image.BufferedImage;

import com.xuggle.mediatool.IMediaReader;
import com.xuggle.mediatool.MediaListenerAdapter;
import com.xuggle.mediatool.ToolFactory;
import com.xuggle.xuggler.Global;

/**
 * Using {@link IMediaReader}, takes a media container, finds the first video stream, decodes that
 * stream, and then writes video frames out to a PNG image file every 5
 * seconds, based on the video presentation timestamps.
 *
 * @author aclarke
 * @author trebor
 */

public class DecodeAndCaptureFrames extends MediaListenerAdapter
{
  /** 
   * The number of seconds between frames.
   */

  public static final double SECONDS_BETWEEN_FRAMES = 5;

  /** 
   * The number of micro-seconds between frames. 
   */

  public static final long MICRO_SECONDS_BETWEEN_FRAMES = 
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
    
    new DecodeAndCaptureFrames(args[0]);
  }

  /** Construct a DecodeAndCaptureFrames which reads and captures
   * frames from a video file.
   * 
   * @param filename the name of the media file to read
   */

  public DecodeAndCaptureFrames(String filename)
  {
    // create a media reader for processing video

    IMediaReader reader = ToolFactory.makeReader(filename);
    
    // stipulate that we want BufferedImages created in BGR 24bit color space
    reader.setBufferedImageTypeToGenerate(BufferedImage.TYPE_3BYTE_BGR);

    
    // note that DecodeAndCaptureFrames is derived from
    // MediaReader.ListenerAdapter and thus may be added as a listener
    // to the MediaReader. DecodeAndCaptureFrames implements
    // onVideoPicture().

    reader.addListener(this);

    // read out the contents of the media file, note that nothing else
    // happens here.  action happens in the onVideoPicture() method
    // which is called when complete video pictures are extracted from
    // the media source

    while (reader.readPacket() == null)
      ;
  }

  /** 
   * Called after a video frame has been decoded from a media stream.
   * Optionally a BufferedImage version of the frame may be passed
   * if the calling {@link IMediaReader} instance was configured to
   * create BufferedImages.
   * 
   * This method blocks, so return quickly.
   */

  public void onVideoPicture(MediaVideoPictureEvent event)
  {
    try
    {
      // if the stream index does not match the selected stream index,
      // then have a closer look
      
      if (event.getStreamIndex() != mVideoStreamIndex)
      {
        // if the selected video stream id is not yet set, go ahead an
        // select this lucky video stream
        
        if (-1 == mVideoStreamIndex)
          mVideoStreamIndex = event.getStreamIndex();
        
        // otherwise return, no need to show frames from this video stream
        
        else
          return;
      }
      
      // if uninitialized, backdate mLastPtsWrite so we get the very
      // first frame

      if (mLastPtsWrite == Global.NO_PTS)
        mLastPtsWrite = event.getTimeStamp() - MICRO_SECONDS_BETWEEN_FRAMES;

      // if it's time to write the next frame

      if (event.getTimeStamp() - mLastPtsWrite >= MICRO_SECONDS_BETWEEN_FRAMES)
      {
        // Make a temporary file name

        File file = File.createTempFile("frame", ".png");

        // write out PNG

        ImageIO.write(event.getBufferedImage(), "png", file);

        // indicate file written

        double seconds = ((double)event.getTimeStamp())
          / Global.DEFAULT_PTS_PER_SECOND;
        System.out.printf("at elapsed time of %6.3f seconds wrote: %s\n",
          seconds, file);
        
        // update last write time
        
        mLastPtsWrite += MICRO_SECONDS_BETWEEN_FRAMES;
      }
    }
    catch (Exception e)
    {
      e.printStackTrace();
    }
  }
}
