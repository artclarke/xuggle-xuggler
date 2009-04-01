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
import com.xuggle.xuggler.IError;
import com.xuggle.xuggler.IRational;
import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.MediaReader;
import com.xuggle.xuggler.IStreamCoder;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IContainerFormat;
import com.xuggle.xuggler.IContainerParameters;
import com.xuggle.xuggler.video.ConverterFactory;


/**
 * Takes a FFMPEG device driver name (ex: "video4linux2"), and a device
 * name (ex: /dev/video0), and displays video from that device.  For
 * example, a web camera.
 * 
 * <p> For example, to play the default camera on these operating
 * systems: <ul> <li>Microsoft Windows:<pre>java -cp
 * %XUGGLE_HOME%\share\java\jars\xuggle-xuggler.jar
 * com.xuggle.xuggler.demos.MrDisplayWebcamVideo vfwcap 0</pre></li>
 * <li>Linux:<pre>java -cp
 * $XUGGLE_HOME/share/java/jars/xuggle-xuggler.jar
 * com.xuggle.xuggler.demos.MrDisplayWebcamVideo video4linux2
 * /dev/video0</pre></li> </ul> </p>
 * 
 * @author aclarke
 * @author trebor
 *
 */

public class MrDisplayWebcamVideo extends MediaReader.ListenerAdapter
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
   * The media reader which will do much of the reading work.
   */

  private MediaReader mMediaReader;

  /**
   * Takes a FFMPEG webcam driver name, and a device name, opens the
   * webcam, and displays its video in a Swing window.
   * <p>
   * Examples of device formats are:
   * </p>
   * <table border="1">
   * <thead>
   *  <tr>
   *  <td>OS</td>
   *  <td>Driver Name</td>
   *  <td>Sample Device Name</td>
   *  </tr>
   *  </thead>
   *  <tbody>
   *  <tr>
   *  <td>Windows</td>
   *  <td>vfwcap</td>
   *  <td>0</td>
   *  </tr>
   *  <tr>
   *  <td>Linux</td>
   *  <td>video4linux2</td>
   *  <td>/dev/video0</td>
   *  </tr>
   *  </tbody>
   *  </table>
   * 
   * <p>
   * Webcam support is very limited; you can't query what devices are
   * available, nor can you query what their capabilities are without
   * actually opening the device.  Sorry, but that's how FFMPEG rolls.
   * </p>
   * 
   * @param args Must contain two strings: a FFMPEG driver name and a
   *   device name (which is dependent on the FFMPEG driver).
   */

  public static void main(String[] args)
  {
    if (args.length != 2)
      throw new IllegalArgumentException(
        "must pass in a driver name and a device name");
    
    // create a new mr. display webcam video
    
    new MrDisplayWebcamVideo(args[0], args[1]);
  }

  /** Construct a MrDisplayWebcamVideo which reads and plays a video
   * from an attached webcam.
   * 
   * @param driverName the name of the webcan drive
   * @param deviceName the name of the webcan device
   */

  public MrDisplayWebcamVideo(String driverName, String deviceName)
  {
    // create a Xuggler container object

    IContainer container = IContainer.make();

    // devices, unlike most files, need to have parameters set in order
    // for Xuggler to know how to configure them, for a webcam, these
    // parameters make sense

    IContainerParameters params = IContainerParameters.make();
    
    // the timebase is used as the camera frame rate

    params.setTimeBase(IRational.make(30,1));
    
    // tell the driver what video with and height to use

    params.setVideoWidth(320);
    params.setVideoHeight(240);
    
    // set these parameters on the container before
    // opening

    container.setParameters(params);
    
    // tell Xuggler about the device format

    IContainerFormat format = IContainerFormat.make();
    if (format.setInputFormat(driverName) < 0)
      throw new IllegalArgumentException(
        "couldn't open webcam device: " + driverName);
    
    // open the container

    int retval = container.open(deviceName, IContainer.Type.READ, format);
    if (retval < 0)
    {
      // this little trick converts the non friendly integer return
      // value into a slightly more friendly object to get a
      // human-readable error name

      IError error = IError.make(retval);
      throw new IllegalArgumentException(
        "could not open file: " + deviceName + "; Error: " + 
        error.getDescription());
    }      

    // create a media reader for processing video from a given
    // IContainer, stipulate that we want BufferedImages to created in
    // BGR 24bit color space

    mMediaReader = new MediaReader(container, true,
      ConverterFactory.XUGGLER_BGR_24);
    
    // note that MrDisplayWebcamVideo is derived from
    // MediaReader.ListenerAdapter and thus may be added as a listener
    // to the MediaReader. MrDisplayWebcamVideo implements
    // onVideoPicture().

    mMediaReader.addListener(this);

    // zero out the time stamps

    mFirstVideoTimestampInStream = Global.NO_PTS;
    mSystemVideoClockStartTime = 0;

    // open the video screen

    openJavaVideo();

    // read out the contents of the media file, note that nothing else
    // happens here.  action happens in the onVideoPicture() method
    // which is called when complete video pictures are extracted from
    // the media source

    while (mMediaReader.readPacket() == null)
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
   *
   * @param picture a raw video picture
   * @param image the buffered image, which will be null if buffered
   *        image creation is de-selected for this MediaReader.
   * @param streamIndex the index of the stream this object was decoded from.
   */

  public void onVideoPicture(IVideoPicture picture, BufferedImage image,
    int streamIndex)
  {
    mScreen.setImage(image);
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
