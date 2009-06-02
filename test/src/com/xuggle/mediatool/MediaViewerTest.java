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

import java.util.Random;
import java.util.Vector;
import java.util.Collection;
import java.util.concurrent.TimeUnit;

import org.junit.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.swing.WindowConstants;

import com.xuggle.mediatool.event.AudioSamplesEvent;
import com.xuggle.mediatool.event.VideoPictureEvent;
import com.xuggle.xuggler.IError;
import com.xuggle.xuggler.ICodec;
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
        @Override
        public void onVideoPicture(VideoPictureEvent event)
        {
          Graphics2D g = event.getImage().createGraphics();
          String timeStampStr = event.getPicture().getFormattedTimeStamp();
          Rectangle2D bounds = g.getFont().getStringBounds(timeStampStr,
            g.getFontRenderContext());
          
          double inset = bounds.getHeight() / 2;
          g.translate(inset, event.getImage().getHeight() - inset);
          
          g.setColor(Color.WHITE);
          g.fill(bounds);
          g.setColor(Color.BLACK);
          g.drawString(timeStampStr, 0, 0);

          super.onVideoPicture(event);
        }
  
        @Override
        public void onAudioSamples(AudioSamplesEvent event)
        {
          // get the raw audio byes and reduce the value to 1 quarter

          ShortBuffer buffer = event.getAudioSamples().getByteBuffer().asShortBuffer();
          for (int i = 0; i < buffer.limit(); ++i)
            buffer.put(i, (short)(buffer.get(i) / 4));

          // pas modifed audio to the writer to be written

          super.onAudioSamples(event);
        }
      };

    // add a listener to the writer to see media modified media
    
    writer.addListener(new MediaViewer(mViewerMode));

    // read and decode packets from the source file and
    // then encode and write out data to the output file
    
    while (reader.readPacket() == null)
      ;
  }
  
  @Test()
    public void testCreateMedia()
  {
    int ballCount = 3;

    // total duration of the media

    long duration = TIME_UNIT.convert(20, SECONDS);

    // video parameters

    int videoStreamIndex = 0;
    int videoStreamId = 0;
    long frameRate = TIME_UNIT.convert(15, MILLISECONDS);
    final int w = 320;
    final int h = 200;
    
    // audio parameters

    int audioStreamIndex = 1;
    int audioStreamId = 0;
    int channelCount = 1;
    int sampleRate = 44100; // Hz
    int sampleCount = 1000;

    // create a media writer and specify the output file

    IMediaWriter writer = new MediaWriter("output.flv");

    // add a viewer so we can see the media as it is created

    writer.addListener(new MediaViewer(mViewerMode, true));

    // add the video stream

    ICodec videoCodec = ICodec.findEncodingCodec(ICodec.ID.CODEC_ID_FLV1);
    writer.addVideoStream(videoStreamIndex, videoStreamId, videoCodec, w, h);

    // add the audio stream

    ICodec audioCodec = ICodec.findEncodingCodec(ICodec.ID.CODEC_ID_MP3);
    writer.addAudioStream(audioStreamIndex, audioStreamId, audioCodec, 
      channelCount, sampleRate);

    // create some balls to show on the screen

    Balls balls = new Balls(ballCount, w, h, sampleCount);

    // the clock time of the next frame

    long nextFrameTime = 0;

    // the total number of audio samples

    long totalSampleCount = 0;

    // loop through clock time, which starts at zero and increases based
    // on the total number of samples created thus far

    for (long clock = 0; clock < duration; clock = IAudioSamples
           .samplesToDefaultPts(totalSampleCount, sampleRate))
    {
      // while the clock time exceeds the time of the next video frame,
      // get and encode the next video frame

      while (clock >= nextFrameTime)
      {
        BufferedImage frame = balls.getVideoFrame(frameRate);
        writer.encodeVideo(videoStreamIndex, frame, nextFrameTime, TIME_UNIT);
        nextFrameTime += frameRate;
      }

      // compute and encode the audio for the balls

      short[] samples = balls.getAudioFrame(sampleRate);
      writer.encodeAudio(audioStreamIndex, samples, clock, TIME_UNIT);
      totalSampleCount += sampleCount;
    }

    // manually close the writer
      
    writer.close();
  }

  /**
   * Add a signal of a given frequency to a set of audio samples.  If
   * the total signal value exceeds the percision of the samples, the
   * signal is clipped.
   * 
   * @param frequency the frequency of the signal to add
   * @param sampleRate the number samples in a second
   * @param volume the amplitude of the signal
   * @param progress the start position the signal, initally should be
   *        zero
   * @param samples the array to which the samples will be added
   * 
   * @return the progress at the end of the sample period
   */

  public static double addSignal(int frequency, int sampleRate, double volume, 
    double progress, short[] samples)
  {
    final double amplitude = Short.MAX_VALUE * volume;
    final double epsilon = ((Math.PI * 2) * frequency) / sampleRate;

    // for each sample

    for (int i = 0; i < samples.length; ++i)
    {
      // compute sample as int

      int sample = samples[i] + (short)(amplitude * Math.sin(progress));

      // clip if nedded

      sample = Math.max(Short.MIN_VALUE, sample);
      sample = Math.min(Short.MAX_VALUE, sample);

      // insert new sample

      samples[i] = (short)sample;

      // update progress
        
      progress += epsilon;
    }

    // return progress

    return progress;
  }

  static class Balls
  {
    // the balls

    private final Collection<Ball> mBalls;

    // a place to store frame images
      
    private final BufferedImage mImage;
    
    // the graphics for the image

    private final Graphics2D mGraphics;

    // a place to put the audio samples

    private final short[] mSamples;

    /** 
     * Grow a set of balls.
     */

    public Balls(int ballCount, int width, int height, int sampleCount)
    {
      // create the balls

      mBalls = new Vector<Ball>();
      while (mBalls.size() < ballCount)
        mBalls.add(new Ball(width, height));

      // create a place to put images
      
      mImage = new BufferedImage(width, height, BufferedImage.TYPE_3BYTE_BGR);
      
      // create graphics for the images
    
      mGraphics = mImage.createGraphics();
      mGraphics.setRenderingHint(
        RenderingHints.KEY_ANTIALIASING,
        RenderingHints.VALUE_ANTIALIAS_ON);

      // create a place to put the audio samples

      mSamples = new short[sampleCount];
    }

    /**
     * Get a video frame containing the balls.
     * 
     * @param elapsedTime the time which has elapsed since the last
     *        video frame
     */

    public BufferedImage getVideoFrame(long elapsedTime)
    {
      // clear the frame

      mGraphics.setColor(Color.WHITE);
      mGraphics.fillRect(0, 0, mImage.getWidth(), mImage.getHeight());

      // update and paint the balls

      for (Ball ball: mBalls)
      {
        ball.update(elapsedTime);
        ball.paint(mGraphics);
      }

      // return the frame image

      return mImage;
    }

    /**
     * Get a video frame containing the ball sounds.
     * 
     * @param elapsedTime the time which has elapsed since the last
     *        audio update
     */

    public short[] getAudioFrame(int sampleRate)
    {
      // zero out the audio
      
      for (int i = 0; i < mSamples.length; ++i)
        mSamples[i] = 0;
      
      // add audio for each ball
      
      for (Ball ball: mBalls)
        ball.mAudioAngle = addSignal(ball.getFrequency(), sampleRate, 
          1d / mBalls.size(), ball.mAudioAngle, mSamples);

      // return new audio samples

      return mSamples;
    }

  }

  /** A ball that bounces around inside a bounding box. */

  static class Ball extends Ellipse2D.Double
  {
    public static final long serialVersionUID = 0;

    private static final int MIN_FREQ_HZ = 220;
    private static final int MAX_FREQ_HZ = 880;

    private final int mWidth;
    private final int mHeight;
    private final int mRadius;
    private final double mSpeed;
    private static final Random rnd = new Random();
    private double mAngle = 0;
    private Color mColor  = Color.BLUE;
    public double mAudioAngle = 0;

    /** Construct a ball.
     *  
     * @param width width of ball bounding box
     * @param height height of ball bounding box
     */

    public Ball(int width, int height)
    {
      mWidth = width;
      mHeight = height;

      // set a random radius

      mRadius = rnd.nextInt(10) + 10;

      // set the speed

      mSpeed = (rnd.nextInt(200) + 100d) / TIME_UNIT.convert(1, SECONDS);

      // start in the middle

      setLocation(
        (mWidth - 2 * mRadius) / 2,
        (mHeight - 2 * mRadius) / 2);

      // set random angle

      mAngle = rnd.nextDouble() * Math.PI * 2;

      // set random color

      mColor = new Color(
        rnd.nextInt(256),
        rnd.nextInt(256),
        rnd.nextInt(256));
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
      double angle = (Math.toDegrees(mAngle) % 360 + 360) % 360;
      return (int)(angle / 360 * (MAX_FREQ_HZ - MIN_FREQ_HZ) + MIN_FREQ_HZ);
    }

    /** 
     * Update the state of the ball.
     * 
     * @param elapsedTime the time which has elapsed since the last update
     */

    public void update(long elapsedTime)
    {
      double x = getX() + Math.cos(mAngle) * mSpeed * elapsedTime;
      double y = getY() + Math.sin(mAngle) * mSpeed * elapsedTime;
      
      if (x < 0 || x > mWidth - mRadius * 2)
      {
        mAngle = Math.PI - mAngle;
        x = getX();
      }
      
      if (y < 0 || y > mHeight - mRadius * 2)
      {
        mAngle = -mAngle;
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
}
