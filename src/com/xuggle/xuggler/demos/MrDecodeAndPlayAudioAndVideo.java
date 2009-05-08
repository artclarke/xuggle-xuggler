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

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.DataLine;
import javax.sound.sampled.LineUnavailableException;
import javax.sound.sampled.SourceDataLine;

import com.xuggle.xuggler.Global;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IStreamCoder;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.mediatool.IMediaTool;
import com.xuggle.xuggler.mediatool.MediaAdapter;
import com.xuggle.xuggler.mediatool.MediaReader;
import com.xuggle.xuggler.video.ConverterFactory;


/**
 * Using {@link MediaReader}, takes a media container, finds the first video stream,
 * decodes that stream, and then plays the audio and video.
 *
 * This code does a VERY coarse job of matching time-stamps, and thus
 * the audio and video will float in and out of slight sync.  Getting
 * time-stamps syncing-up with audio is very system dependent and left
 * as an exercise for the reader.
 * 
 * @author aclarke
 * @author trebor
 */

public class MrDecodeAndPlayAudioAndVideo extends MediaAdapter
{
  /**
   * The audio line we'll output sound to; it'll be the default audio
   * device on your system if available
   */

  private static SourceDataLine mLine;

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
   * The audio stream index, used to ensure we display samples from one
   * and only one audio stream from the media container.
   */

  private int mAudioStreamIndex = -1;


  /**
   * The media reader which will do much of the reading work.
   */

  private MediaReader mMediaReader;

  /**
   * Takes a media container (file) as the first argument, opens it,
   * plays audio as quickly as it can, and opens up a Swing window and
   * displays video frames with <i>roughly</i> the right timing.
   *  
   * @param args Must contain one string which represents a filename
   */

  public static void main(String[] args)
  {
    if (args.length <= 0)
      throw new IllegalArgumentException(
        "must pass in a filename as the first argument");
    
    // create a new mr. decode an play audio and video
    
    new MrDecodeAndPlayAudioAndVideo(args[0]);
  }

  /** Construct a MrDecodeAndPlayAudioAndVideo which reads and plays a
   * video file.
   * 
   * @param filename the name of the media file to read
   */

  public MrDecodeAndPlayAudioAndVideo(String filename)
  {
    // create a media reader for processing video, stipulate that we
    // want BufferedImages to created in BGR 24bit color space

    mMediaReader = new MediaReader(filename, true,
      ConverterFactory.XUGGLER_BGR_24);
    
    // note that MrDecodeAndPlayAudioAndVideo is derived from
    // MediaReader.IListener and thus may be added as a listener to the
    // MediaReader. MrDecodeAndPlayAudioAndVideo implements
    // onVideoPicture() and onAudioSamples().

    mMediaReader.addListener(this);

    // open the video output, the audio output is opend once we know
    // what kind of audio stream exists in the media file
    
    openJavaVideo();

    // zero out the time stamps

    mFirstVideoTimestampInStream = Global.NO_PTS;
    mSystemVideoClockStartTime = 0;

    // read out the contents of the media file, note that nothing else
    // happens here.  action happens in the onVideoPicture() and
    // onAudioSamples() methods which are called when complete video
    // pictures and audio sample sets are extracted from the media
    // source

    while (mMediaReader.readPacket() == null)
      ;

    // close sound and video outputs
    
    closeJavaSound();
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
  
  /** Called after audio samples have been decoded from a media
   * stream.  This method blocks, so return quickly.
   * @param samples a audio samples
   * @param streamIndex the index of the stream
   */
  
  public void onAudioSamples(IMediaTool tool, IAudioSamples samples, int streamIndex)
  {
    // if the stream index does not match the selected stream index,
    // then have a closer look

    if (streamIndex != mAudioStreamIndex)
    {
      // if the selected audio stream id is not yet set, go ahead an
      // select this lucky audio stream
      
      if (-1 == mAudioStreamIndex)
        mAudioStreamIndex = streamIndex;
      
      // otherwise return, no need to play samples from this audio
      // stream
      
      else
        return;
    }

    // if the audio line is closed open it

    if (mLine == null)
    {
      // get the media container from the media reader, then get the
      // stream using the stream index passed into this method, then get
      // the stream coder for that stream, and pass that stream coder
      // off to open the audio out put stream with

      openJavaSound(mMediaReader.getContainer()
        .getStream(streamIndex).getStreamCoder());
    }

    // dump all the samples into the line

    byte[] rawBytes = samples.getData().getByteArray(0, samples.getSize());
    mLine.write(rawBytes, 0, samples.getSize());
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
     * Remember that all Xuggler IAudioSamples and IVideoPicture objects always
     * give timestamps in Microseconds, relative to the first decoded item.  If
     * instead you used the packet timestamps, they can be in different units depending
     * on your IContainer, and IStream and things can get hairy quickly.
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
      // stream.  remember that IVideoPicture and IAudioSamples
      // timestamps are always in MICROSECONDS, so we divide by 1000 to
      // get milliseconds.
      
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

  /**
   * Open a java audio line out to play the audio samples into.
   *
   * @param audioCoder the audio coder which contains details about the
   *        format of the audio.
   */

  private static void openJavaSound(IStreamCoder audioCoder) 
  {
    try
    {
      // estabish the adhio format, NOTE: xuggler defaults to signed 16 bit
      // samples
      
      AudioFormat audioFormat = new AudioFormat(audioCoder.getSampleRate(),
        (int)IAudioSamples.findSampleBitDepth(audioCoder.getSampleFormat()),
        audioCoder.getChannels(), true, false);
      
      // create the audio line out
      
      DataLine.Info info = new DataLine.Info(SourceDataLine.class, audioFormat);
      mLine = (SourceDataLine) AudioSystem.getLine(info);
      
      // open the line and start the line
      
      mLine.open(audioFormat);
      mLine.start();
    }
    catch (LineUnavailableException lue)
    {
      System.out
        .println("WARINGIN: No audio line out available: " + lue);
    }
  }

  /**
   * Close the java audio line out.
   */

  private static void closeJavaSound()
  {
    if (mLine != null)
    {
      // drain the audio snake, tap-tap

      mLine.drain();
      
      // close the line.

      mLine.close();
      mLine=null;
    }
  }
}
