/*
 * This file is part of Xuggler.
 * 
 * Xuggler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public
 * License along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.xuggle.xuggler.demos;

import java.awt.image.BufferedImage;

import com.xuggle.xuggler.Global;
import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IPacket;
import com.xuggle.xuggler.IPixelFormat;
import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IStreamCoder;
import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IVideoResampler;
import com.xuggle.xuggler.Utils;

/**
 * Takes a media container, finds the first video stream,
 * decodes that stream, and then displays the video frames,
 * at the frame-rate specified by the container, on a 
 * window.
 * @author aclarke
 *
 */
public class DecodeAndPlayVideo
{

  /**
   * Takes a media container (file) as the first argument, opens it,
   * opens up a Swing window and displays
   * video frames with <i>roughly</i> the right timing.
   *  
   * @param args Must contain one string which represents a filename
   */
  @SuppressWarnings("deprecation")
  public static void main(String[] args)
  {
    if (args.length <= 0)
      throw new IllegalArgumentException("must pass in a filename" +
      		" as the first argument");

    String filename = args[0];

    // Let's make sure that we can actually convert video pixel formats.
    if (!IVideoResampler.isSupported(
        IVideoResampler.Feature.FEATURE_COLORSPACECONVERSION))
      throw new RuntimeException("you must install the GPL version" +
      		" of Xuggler (with IVideoResampler support) for " +
      		"this demo to work");

    // Create a Xuggler container object
    IContainer container = IContainer.make();

    // Open up the container
    if (container.open(filename, IContainer.Type.READ, null) < 0)
      throw new IllegalArgumentException("could not open file: " + filename);

    // query how many streams the call to open found
    int numStreams = container.getNumStreams();

    // and iterate through the streams to find the first video stream
    int videoStreamId = -1;
    IStreamCoder videoCoder = null;
    for(int i = 0; i < numStreams; i++)
    {
      // Find the stream object
      IStream stream = container.getStream(i);
      // Get the pre-configured decoder that can decode this stream;
      IStreamCoder coder = stream.getStreamCoder();

      if (coder.getCodecType() == ICodec.Type.CODEC_TYPE_VIDEO)
      {
        videoStreamId = i;
        videoCoder = coder;
        break;
      }
    }
    if (videoStreamId == -1)
      throw new RuntimeException("could not find video stream in container: "
          +filename);

    /*
     * Now we have found the video stream in this file.  Let's open up our decoder so it can
     * do work.
     */
    if (videoCoder.open() < 0)
      throw new RuntimeException("could not open video decoder for container: "
          +filename);

    IVideoResampler resampler = null;
    if (videoCoder.getPixelType() != IPixelFormat.Type.BGR24)
    {
      // if this stream is not in BGR24, we're going to need to
      // convert it.  The VideoResampler does that for us.
      resampler = IVideoResampler.make(videoCoder.getWidth(), 
          videoCoder.getHeight(), IPixelFormat.Type.BGR24,
          videoCoder.getWidth(), videoCoder.getHeight(), videoCoder.getPixelType());
      if (resampler == null)
        throw new RuntimeException("could not create color space " +
        		"resampler for: " + filename);
    }
    /*
     * And once we have that, we draw a window on screen
     */
    openJavaWindow();

    /*
     * Now, we start walking through the container looking at each packet.
     */
    IPacket packet = IPacket.make();
    long firstTimestampInStream = Global.NO_PTS;
    long systemClockStartTime = 0;
    while(container.readNextPacket(packet) >= 0)
    {
      /*
       * Now we have a packet, let's see if it belongs to our video stream
       */
      if (packet.getStreamIndex() == videoStreamId)
      {
        /*
         * We allocate a new picture to get the data out of Xuggler
         */
        IVideoPicture picture = IVideoPicture.make(videoCoder.getPixelType(),
            videoCoder.getWidth(), videoCoder.getHeight());

        int offset = 0;
        while(offset < packet.getSize())
        {
          /*
           * Now, we decode the video, checking for any errors.
           * 
           */
          int bytesDecoded = videoCoder.decodeVideo(picture, packet, offset);
          if (bytesDecoded < 0)
            throw new RuntimeException("got error decoding video in: "
                + filename);
          offset += bytesDecoded;

          /*
           * Some decoders will consume data in a packet, but will not be able to construct
           * a full video picture yet.  Therefore you should always check if you
           * got a complete picture from the decoder
           */
          if (picture.isComplete())
          {
            IVideoPicture newPic = picture;
            /*
             * If the resampler is not null, that means we didn't get the
             * video in BGR24 format and
             * need to convert it into BGR24 format.
             */
            if (resampler != null)
            {
              // we must resample
              newPic = IVideoPicture.make(resampler.getOutputPixelFormat(),
                  picture.getWidth(), picture.getHeight());
              if (resampler.resample(newPic, picture) < 0)
                throw new RuntimeException("could not resample video from: "
                    + filename);
            }
            if (newPic.getPixelType() != IPixelFormat.Type.BGR24)
              throw new RuntimeException("could not decode video" +
              		" as BGR 24 bit data in: " + filename);

            /**
             * We could just display the images as quickly as we decode them,
             * but it turns out we can decode a lot faster than you think.
             * 
             * So instead, the following code does a poor-man's version of
             * trying to match up the frame-rate requested for each
             * IVideoPicture with the system clock time on your computer.
             * 
             * Remember that all Xuggler IAudioSamples and IVideoPicture objects
             * always give timestamps in Microseconds, relative to the first
             * decoded item. If instead you used the packet timestamps, they can
             * be in different units depending on your IContainer, and IStream
             * and things can get hairy quickly.
             */
            if (firstTimestampInStream == Global.NO_PTS)
            {
              // This is our first time through
              firstTimestampInStream = picture.getTimeStamp();
              // get the starting clock time so we can hold up frames
              // until the right time.
              systemClockStartTime = System.currentTimeMillis();
            } else {
              long systemClockCurrentTime = System.currentTimeMillis();
              long millisecondsClockTimeSinceStartofVideo =
                systemClockCurrentTime - systemClockStartTime;
              // compute how long for this frame since the first frame in the
              // stream.
              // remember that IVideoPicture and IAudioSamples timestamps are
              // always in MICROSECONDS,
              // so we divide by 1000 to get milliseconds.
              long millisecondsStreamTimeSinceStartOfVideo =
                (picture.getTimeStamp() - firstTimestampInStream)/1000;
              final long millisecondsTolerance = 50; // and we give ourselfs 50 ms of tolerance
              final long millisecondsToSleep = 
                (millisecondsStreamTimeSinceStartOfVideo -
                  (millisecondsClockTimeSinceStartofVideo +
                      millisecondsTolerance));
              if (millisecondsToSleep > 0)
              {
                try
                {
                  Thread.sleep(millisecondsToSleep);
                }
                catch (InterruptedException e)
                {
                  // we might get this when the user closes the dialog box, so
                  // just return from the method.
                  return;
                }
              }
            }

            // And finally, convert the BGR24 to an Java buffered image
            BufferedImage javaImage = Utils.videoPictureToImage(newPic);

            // and display it on the Java Swing window
            updateJavaWindow(javaImage);
          }
        }
      }
      else
      {
        /*
         * This packet isn't part of our video stream, so we just
         * silently drop it.
         */
        ;
      }

    }
    /*
     * Technically since we're exiting anyway, these will be cleaned up by 
     * the garbage collector... but because we're nice people and want
     * to be invited places for Christmas, we're going to show how to clean up.
     */
    if (videoCoder != null)
    {
      videoCoder.close();
      videoCoder = null;
    }
    if (container !=null)
    {
      container.close();
      container = null;
    }
    closeJavaWindow();

  }

  /**
   * The window we'll draw the video on.
   * 
   */
  private static VideoImage mScreen = null;

  private static void updateJavaWindow(BufferedImage javaImage)
  {
    mScreen.setImage(javaImage);
  }

  /**
   * Opens a Swing window on screen.
   */
  private static void openJavaWindow()
  {
    mScreen = new VideoImage();
  }

  /**
   * Forces the swing thread to terminate; I'm sure there is a right
   * way to do this in swing, but this works too.
   */
  private static void closeJavaWindow()
  {
    System.exit(0);
  }
}
