/*******************************************************************************
 * Copyright (c) 2008, 2010 Xuggle Inc.  All rights reserved.
 *  
 * This file is part of Xuggle-Xuggler-Main.
 *
 * Xuggle-Xuggler-Main is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xuggle-Xuggler-Main is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Xuggle-Xuggler-Main.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/

package com.xuggle.mediatool.demos;

import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.geom.Ellipse2D;
import java.awt.RenderingHints;
import java.awt.image.BufferedImage;

import java.util.Random;
import java.util.Vector;
import java.util.Collection;

import static java.util.concurrent.TimeUnit.SECONDS;
import static com.xuggle.xuggler.Global.DEFAULT_TIME_UNIT;

/**
 * An implementation of {@link Balls} that moves the balls
 * around in a rectangle, and plays a sound for each
 * ball that changes when it hits a wall.
 * 
 * @author trebor
 *
 */
public class MovingBalls implements Balls
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
  
  public MovingBalls(int ballCount, int width, int height, int sampleCount)
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
  
  /* (non-Javadoc)
   * @see com.xuggle.mediatool.demos.Balls#getVideoFrame(long)
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
  
  /* (non-Javadoc)
   * @see com.xuggle.mediatool.demos.Balls#getAudioFrame(int)
   */
  
  public short[] getAudioFrame(int sampleRate)
  {
    // zero out the audio
    
    for (int i = 0; i < mSamples.length; ++i)
      mSamples[i] = 0;
    
    // add audio for each ball
    
    for (Ball ball: mBalls)
      ball.setAudioProgress(addSignal(ball.getFrequency(), sampleRate, 
          1d / mBalls.size(), ball.getAudioProgress(), mSamples));
    
    // return new audio samples
    
    return mSamples;
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
   *        zero, it will be updated by addSignal and returned, pass the
   *        returned value into subsquent calls to addSignal
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

  /** A ball that bounces around inside a bounding box. */

  static class Ball extends Ellipse2D.Double
  {
    public static final long serialVersionUID = 0;

    private static final int MIN_FREQ_HZ = 220;
    private static final int MAX_FREQ_HZ = 880;

    // ball parameters

    private final int mWidth;
    private final int mHeight;
    private final int mRadius;
    private final double mSpeed;
    private static final Random rnd = new Random();
    private double mAngle = 0;
    private Color mColor  = Color.BLUE;
    private double mAudioProgress = 0;

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

      mSpeed = (rnd.nextInt(200) + 100d) / DEFAULT_TIME_UNIT.convert(1, SECONDS);

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

    // set the auido progress

    public void setAudioProgress(double audioProgress)
    {
      mAudioProgress = audioProgress;
    }
    
    // get audio progress

    public double getAudioProgress()
    {
      return mAudioProgress;
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
  }
}
