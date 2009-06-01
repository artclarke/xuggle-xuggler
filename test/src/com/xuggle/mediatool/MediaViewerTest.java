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

import java.io.File;
import java.nio.ShortBuffer;

import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.geom.Ellipse2D;
import java.awt.RenderingHints;
import java.awt.geom.Rectangle2D;
import java.awt.image.BufferedImage;

import java.util.concurrent.TimeUnit;
import java.util.Random;
import java.util.Vector;

import org.junit.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.swing.WindowConstants;

import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IError;
import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IAudioSamples;

import static com.xuggle.mediatool.IMediaViewer.Mode.*;

import static java.util.concurrent.TimeUnit.SECONDS;
import static java.util.concurrent.TimeUnit.MILLISECONDS;
import static java.util.concurrent.TimeUnit.MICROSECONDS;

import static junit.framework.Assert.*;

public class MediaViewerTest
{
  // the log

  private static final Logger log = LoggerFactory.getLogger(MediaViewerTest.class);
  { log.trace("<init>"); }

  // the xuggle system time unit

  public static final TimeUnit TIME_UNIT = MICROSECONDS;

  // the media view mode
  
  final MediaViewer.Mode mViewerMode = IMediaViewer.Mode.valueOf(
    System.getProperty(this.getClass().getName() + ".ViewerMode", 
      DISABLED.name()));

  // standard test name prefix

  final String PREFIX = this.getClass().getName() + "-";

  // location of test files

  public static final String TEST_FILE_DIR = "fixtures";
 
  public static final String INPUT_FILENAME = TEST_FILE_DIR + "/testfile.flv";
  //  public static final String INPUT_FILENAME = TEST_FILE_DIR + "/goose.wmv";
//   public static final String INPUT_FILENAME = 
//     TEST_FILE_DIR + "/testfile_bw_pattern.flv";

  // test thet the proper number of events are signaled during reading
  // and writng of a media file

  //@Test()
  public void testViewer()
  {
    //com.xuggle.ferry.JNIMemoryAllocator.setMirroringNativeMemoryInJVM(false);
    
    File inputFile = new File(INPUT_FILENAME);
    assert (inputFile.exists());

    MediaReader reader = new MediaReader(INPUT_FILENAME);

    reader.addListener(new MediaViewer(mViewerMode, true, 
        WindowConstants.EXIT_ON_CLOSE));
    // reader.addListener(new DebugListener(DebugListener.Mode.EVENT,
    // DebugListener.Event.ALL));

    IError rv;
    while ((rv = reader.readPacket()) == null)
      ;

    assertEquals(IError.Type.ERROR_EOF, rv.getType());
  }

 //@Test()
  public void testModifyMedia()
  {
    //com.xuggle.ferry.JNIMemoryAllocator.setMirroringNativeMemoryInJVM(false);
    
    File inputFile = new File(INPUT_FILENAME);
    assert (inputFile.exists());

    // create a media reader

    IMediaReader reader = new MediaReader(INPUT_FILENAME);
    reader.setBufferedImageTypeToGenerate(BufferedImage.TYPE_3BYTE_BGR);

    
    // add a writer to the reader which receives the decoded media,
    // encodes it and writes it out to the specified file
    
    MediaWriter writer = new MediaWriter("output.mov", reader)
      {
        public void onVideoPicture(MediaVideoPictureEvent event)
        {
          Graphics2D g = event.getBufferedImage().createGraphics();
          String timeStampStr = event.getVideoPicture().getFormattedTimeStamp();
          Rectangle2D bounds = g.getFont().getStringBounds(timeStampStr,
            g.getFontRenderContext());
          
          double inset = bounds.getHeight() / 2;
          g.translate(inset, event.getBufferedImage().getHeight() - inset);
          
          g.setColor(Color.WHITE);
          g.fill(bounds);
          g.setColor(Color.BLACK);
          g.drawString(timeStampStr, 0, 0);

          super.onVideoPicture(event);
        }
  
        public void onAudioSamples(IMediaGenerator tool, IAudioSamples samples, 
          int streamIndex)
        {
          // get the raw audio byes and reduce the value to 1 quarter

          ShortBuffer buffer = samples.getByteBuffer().asShortBuffer();
          for (int i = 0; i < buffer.limit(); ++i)
            buffer.put(i, (short)(buffer.get(i) / 4));

          // pas modifed audio to the writer to be written

          super.onAudioSamples(tool, samples, streamIndex);
        }
      };

    // add a listener to the writer to see media modified media
    
    writer.addListener(new MediaViewer(mViewerMode));

    // read and decode packets from the source file and
    // then encode and write out data to the output file
    
    while (reader.readPacket() == null)
      ;
  }

  /** A ball that bounces around inside a bounding box. */

  static class Ball extends Ellipse2D.Double
  {
    public static final long serialVersionUID = 0;

    private static final int MIN_FREQ_KHZ = 60;
    private static final int MAX_FREQ_KHZ = 880;

    private final int mWidth;
    private final int mHeight;
    private final int mRadius   = 10;
    private final double mSpeed = 200d / TIME_UNIT.convert(1, SECONDS);
    private static final Random rnd = new Random();
    private double mTheta = Math.PI / 4;
    private Color mColor  = Color.BLUE;

    /** Construct a ball.
     *  
     * @param width width of ball bounding box
     * @param height height of ball bounding box
     */

    public Ball(int width, int height)
    {
      mWidth = width;
      mHeight = height;
      
      setLocation(
        (mWidth - 2 * mRadius) / 2,
        (mHeight - 2 * mRadius) / 2);

      mTheta = rnd.nextDouble() * Math.PI * 2;
      mColor = new Color(
        Math.abs(rnd.nextInt() % 256),
        Math.abs(rnd.nextInt() % 256),
        Math.abs(rnd.nextInt() % 256));
    }
    
    // set ball location

    private void setLocation(double x, double y)
    {
      setFrame(x, y, 2 * mRadius, 2 * mRadius);
    }

    /** 
     * The current frequency of this ball.
     */
    
    public int getFrequency()
    {
      double angle = Math.abs(mTheta % 2 * Math.PI) / (2 * Math.PI);
      return (int)(1000 * 
        ((angle * (MAX_FREQ_KHZ - MIN_FREQ_KHZ)) + MIN_FREQ_KHZ));
    }

    /** 
     * Updat the state of the ball.
     * 
     * @param elapsedTime the time which has elapsed since the last update
     */

    public void update(long elapsedTime)
    {
      double x = getX() + Math.cos(mTheta) * mSpeed * elapsedTime;
      double y = getY() + Math.sin(mTheta) * mSpeed * elapsedTime;
      
      if (x < 0)
      {
        mTheta = Math.PI - mTheta;
        x = getX();
      }

      else if (x > mWidth - mRadius * 2)
      {
        mTheta = Math.PI - mTheta;
        x = getX();
      }
      
      if (y < 0)
      {
        mTheta = -mTheta;
        y = getY();
      }
      else if (y > mHeight - mRadius * 2)
      {
        mTheta = -mTheta;
        y = getY();
      }

      setLocation(x, y);
    }

    /** Paint the ball onto a graphics canvas
     *
     * @param g the graphics area to paint onto
     */

    public void paint(Graphics2D g)
    {
      g.setColor(mColor);
      g.fill(this);
    }
  };
  

  @Test()
  public void testCreateMedia()
  {
    // total duration of the video

    long duration = TIME_UNIT.convert(5000, MILLISECONDS);

    // video parameters

    int videoStreamIndex = 0;
    int videoStreamId = 0;
    long frameRate = TIME_UNIT.convert(15, MILLISECONDS);
    final int w = 200;
    final int h = 200;
    
    // audio parameters

    int audioStreamIndex = 1;
    int audioStreamId = 0;
    int channelCount = 1;
    int sampleRate = 44100; // kHz

    // create a media writer and specify the output file

    IMediaWriter writer = new MediaWriter("output.flv");

    // add a viewer so we can see the media as it is created

    //writer.addListener(new MediaViewer(FAST_VIDEO_ONLY, true));

    // add the video stream

    ICodec videoCodec = ICodec.findEncodingCodec(ICodec.ID.CODEC_ID_FLV1);
    writer.addVideoStream(videoStreamIndex, videoStreamId, videoCodec, w, h);

    // add the audio stream

    ICodec audioCodec = ICodec.findEncodingCodec(ICodec.ID.CODEC_ID_MP3);
    IContainer container = writer.getContainer();
    IStream stream = container.getStream(
        writer.addAudioStream(audioStreamIndex, audioStreamId,
      audioCodec, channelCount, sampleRate));
    int sampleCount = stream.getStreamCoder().getDefaultAudioFrameSize();
    int samplePeriod = sampleRate / sampleCount;

    // create some balls to show on the screen

    Vector<Ball> balls = new Vector<Ball>();
    for (int i = 0; i < 3; ++i)
      balls.add(new Ball(w, h));

    // create a place to put images
        
    BufferedImage image = new BufferedImage(w, h, 
      BufferedImage.TYPE_3BYTE_BGR);
    
    // create graphics for the images
    
    Graphics2D g = image.createGraphics();
    g.setRenderingHint(
      RenderingHints.KEY_ANTIALIASING,
      RenderingHints.VALUE_ANTIALIAS_ON);

    // create a place to put the audio samples

    short[] audioSamples = new short[sampleCount];

    // compute clock increment

    long clockIncrement = samplePeriod;

    // create the media

    long nextFrameTime = 0;
    for (long clock = 0; clock <= duration; clock += clockIncrement)
    {
      // if it is time to push a video frame do so

      if (clock >= nextFrameTime)
      {
        g.setColor(Color.WHITE);
        g.fillRect(0, 0, w, h);
        for (Ball ball: balls)
        {
          ball.update(frameRate);
          ball.paint(g);
        }
        
        writer.encodeVideo(videoStreamIndex, image, nextFrameTime, TIME_UNIT);
        nextFrameTime += frameRate;
      }

//       // zero out the audio
      
       for (int i = 0; i < audioSamples.length; ++i)
         audioSamples[i] = 0;

//       // add audio for each ball

      for (Ball ball: balls)
        addSignal(ball.getFrequency(), sampleRate, 0.25d, 0, audioSamples);

      // push out the audio samples
      
       writer.encodeAudio(audioStreamIndex, audioSamples, clock, TIME_UNIT);
    }

    // manually close the writer
      
    writer.close();
  }

  public void addSignal(int frequency, int sampleRate, double volume, 
    int startValue, short[] destination)
  {
    double theta = 0;
    double amplitude = Short.MAX_VALUE * volume;
    double epsilon = (Math.PI * 2) / (sampleRate / frequency);

    for (int i = 0; i < destination.length; ++i)
    {
      destination[i] += (short)(amplitude * Math.sin(theta));
      theta += epsilon;
    }
  }
}
