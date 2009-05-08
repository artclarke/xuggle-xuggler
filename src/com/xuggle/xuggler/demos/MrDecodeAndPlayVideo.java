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

import java.awt.image.BufferedImage;

import com.xuggle.xuggler.Global;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.mediatool.IMediaTool;
import com.xuggle.xuggler.mediatool.MediaAdapter;
import com.xuggle.xuggler.mediatool.MediaReader;
import com.xuggle.xuggler.video.ConverterFactory;


/**
 * Using {@link MediaReader}, takes a media container, finds the first video stream,
 * decodes that stream, and plays the video.
 *
 * This code does a coarse job of matching time-stamps, and thus the
 * video may not play in exact real time.
 * 
 * @author aclarke
 * @author trebor
 */

public class MrDecodeAndPlayVideo extends MediaAdapter
{
  /**
   * The window we'll draw the video on.
   */

  private static VideoImage mScreen = null;

  /**
   * The moment the video started to play.
   */

  private static long mSystemVideoClockStartTime;

  /**
   * The first time stamp of the video stream.
   */

  private static long mFirstVideoTimestampInStream;

  /**
   * The video stream index, used to ensure we display frames from one
   * and only one video stream from the media container.
   */

  private int mVideoStreamIndex = -1;

  /**
   * Takes a media container (file) as the first argument, opens it,
   * opens up a Swing window and displays video frames with
   * <i>roughly</i> the right timing.
   *  
   * @param args Must contain one string which represents a filename
   */

  public static void main(String[] args)
  {
    if (args.length <= 0)
      throw new IllegalArgumentException(
        "must pass in a filename as the first argument");
    
    // create a new mr. decode an play video
    
    new MrDecodeAndPlayVideo(args[0]);
  }

  /** Construct a MrDecodeAndPlayVideo which reads and plays a
   * video file.
   * 
   * @param filename the name of the media file to read
   */

  public MrDecodeAndPlayVideo(String filename)
  {
    // create a media reader for processing video, stipulate that we
    // want BufferedImages to created in BGR 24bit color space

    MediaReader mediaReader = new MediaReader(filename, true,
      ConverterFactory.XUGGLER_BGR_24);
    
    // note that MrDecodeAndPlayVideo is derived from
    // MediaReader.ListenerAdapter and thus may be added as a listener
    // to the MediaReader. MrDecodeAndPlayVideo implements
    // onVideoPicture().

    mediaReader.addListener(this);

    // zero out the time stamps

    mFirstVideoTimestampInStream = Global.NO_PTS;
    mSystemVideoClockStartTime = 0;

    // open the video screen

    openJavaVideo();

    // read out the contents of the media file, note that nothing else
    // happens here.  action happens in the onVideoPicture() method
    // which is called when complete video pictures are extracted from
    // the media source

    while (mediaReader.readPacket() == null)
      ;

    // close video screen
    
    closeJavaVideo();
  }

  /** 
   * Called after a video frame has been decoded from a media stream.
   * Optionally a BufferedImage version of the frame may be passed
   * if the calling {@link MediaReader} instance was configured to
   * create BufferedImages.
   * 
   * This method blocks, so return quickly.
   * @param picture a raw video picture
   * @param image the buffered image, which will be null if buffered
   *        image creation is de-selected for this MediaReader.
   * @param streamIndex the index of the stream this object was decoded from.
   */

  public void onVideoPicture(IMediaTool tool, IVideoPicture picture,
    BufferedImage image, int streamIndex)
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

    // put the image onto the scren

    mScreen.setImage(image);

    // establish how long to delay

    long delay = millisecondsUntilTimeToDisplay(picture);

    // delay as needed to keep the video playing in real time
    
    try
    {
      if (delay > 0)
        Thread.sleep(delay);
    }
    catch (InterruptedException e)
    {
      return;
    }
  }
  
  /**
   * Compute how long to display a video picture.
   * 
   * @param picture the video picture about to be displayed
   * 
   * @return the amount of time to sleep after the vidoe picture is
   * displayed.
   */

  private static long millisecondsUntilTimeToDisplay(IVideoPicture picture)
  {
    /**
     * We could just display the images as quickly as we decode them, but it turns
     * out we can decode a lot faster than you think.
     * 
     * So instead, the following code does a poor-man's version of trying to
     * match up the frame-rate requested for each IVideoPicture with the system
     * clock time on your computer.
     * 
     * Remember that all Xuggler IVideoPicture objects always give
     * timestamps in Microseconds, relative to the first decoded item.
     * If instead you used the packet timestamps, they can be in
     * different units depending on your IContainer, and IStream and
     * things can get hairy quickly.
     */

    long millisecondsToSleep = 0;
    if (mFirstVideoTimestampInStream == Global.NO_PTS)
    {
      // This is our first time through
      mFirstVideoTimestampInStream = picture.getTimeStamp();
      // get the starting clock time so we can hold up frames
      // until the right time.
      mSystemVideoClockStartTime = System.currentTimeMillis();
      millisecondsToSleep = 0;
    } 
    else
    {
      long systemClockCurrentTime = System.currentTimeMillis();
      long millisecondsClockTimeSinceStartofVideo = 
        systemClockCurrentTime - mSystemVideoClockStartTime;
      
      // compute how long for this frame since the first frame in the
      // stream.  remember that IVideoPicture timestamps are always in
      // MICROSECONDS, so we divide by 1000 to get milliseconds.
      
      long millisecondsStreamTimeSinceStartOfVideo = 
        (picture.getTimeStamp() - mFirstVideoTimestampInStream) / 1000;

      // and we give ourselfs 50 ms of tolerance

      final long millisecondsTolerance = 50;

      millisecondsToSleep = (millisecondsStreamTimeSinceStartOfVideo -
          (millisecondsClockTimeSinceStartofVideo+millisecondsTolerance));

    }

    return millisecondsToSleep;
  }

  /**
   * Opens a Swing window on screen.
   */

  private static void openJavaVideo()
  {
    mScreen = new VideoImage();
  }

  /**
   * Forces the swing thread to terminate; I'm sure there is a right
   * way to do this in swing, but this works too.
   */

  private static void closeJavaVideo()
  {
    System.exit(0);
  }
}
