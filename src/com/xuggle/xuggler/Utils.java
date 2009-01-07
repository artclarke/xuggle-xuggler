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
package com.xuggle.xuggler;

import java.util.concurrent.TimeUnit;

import com.xuggle.ferry.IBuffer;
import com.xuggle.xuggler.Global;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IPixelFormat;
import com.xuggle.xuggler.ITimeValue;

/**
 * A collection of useful utilities for creating blank {@link IVideoPicture} objects
 * and managing audio time stamp to sample conversions.
 * 
 * @author aclarke
 *
 */
public class Utils
{
  /**
   * Get a new blank frame object encoded in {@link IPixelFormat.Type#YUV420P} format.
   * 
   * @param w width of object
   * @param h height of object
   * @param y Y component of background color.
   * @param u U component of background color.
   * @param v V component of background color.
   * @param pts The time stamp, in microseconds, you want this frame to have, or {@link Global#NO_PTS} for none.
   * @return A new frame, or null if we can't create it.
   */
  public static IVideoPicture getBlankFrame(int w, int h, int y, int u, int v, long pts)
  {
    IVideoPicture frame = IVideoPicture.make(IPixelFormat.Type.YUV420P, w, h);
    if (frame != null)
    {
      IBuffer data = frame.getData();
      int bufSize = frame.getSize();
      java.nio.ByteBuffer buffer = data.getByteBuffer(0, bufSize);
      if (buffer != null)
      {
        // we have the raw data; now we set it to the specified YUV value.
        int lineLength = 0;
        int offset = 0;
        
        // first let's check the L
        offset = 0;
        lineLength = frame.getDataLineSize(0);
        for(int i = offset ; i < offset + lineLength * h; i++)
        {
          buffer.put(i, (byte)y);
        }

        // now, check the U value
        offset = (frame.getDataLineSize(0)*h);
        lineLength = frame.getDataLineSize(1);
        for(int i = offset ; i < offset + lineLength * ((h+1) / 2); i++)
        {
          buffer.put(i, (byte)u);
        }

        // and finally the V
        offset = (frame.getDataLineSize(0)*h) + (frame.getDataLineSize(1)*((h+1)/2));
        lineLength = frame.getDataLineSize(2);
        for(int i = offset; i < offset + lineLength * ((h+1) / 2); i++)
        {
          buffer.put(i, (byte)v);
        }
      }
      // set it complete
      frame.setComplete(true, IPixelFormat.Type.YUV420P, w, h, pts);
    }
    
    return frame;
  }
  
  /**
   * Returns a blank frame with a green-screen background.
   * 
   * @see #getBlankFrame(int, int, int, int, int, long)
   * 
   * @param w width in pixels
   * @param h height in pixels 
   * @param pts presentation timestamp (in {@link TimeUnit#MICROSECONDS} you want set
   * @return a new blank frame
   */
  public static IVideoPicture getBlankFrame(int w, int h, int pts)
  {
    return getBlankFrame(w, h, 0, 0, 0, pts);
  }

  /**
   * For a given sample rate, returns how long it would take to play a number of samples.
   * @param numSamples The number of samples you want to find the duration for
   * @param sampleRate The sample rate in Hz
   * @return The duration it would take to play numSamples of sampleRate audio
   */
  public static ITimeValue samplesToTimeValue(long numSamples, int sampleRate)
  {
    if (sampleRate <= 0)
      throw new IllegalArgumentException("sampleRate must be greater than zero");
    
    return ITimeValue.make(
        IAudioSamples.samplesToDefaultPts(numSamples, sampleRate),
        ITimeValue.Unit.MICROSECONDS);
  }
  
  /**
   * For a given time duration and sample rate, return the number of samples it would take to fill.
   * @param duration duration
   * @param sampleRate sample rate of audio
   * @return number of samples required to fill that duration
   */
  public static long timeValueToSamples(ITimeValue duration, int sampleRate)
  {
    if (duration == null)
      throw new IllegalArgumentException("must pass in a valid duration");
    if (sampleRate <= 0)
      throw new IllegalArgumentException("sampleRate must be greater than zero");
    return IAudioSamples.defaultPtsToSamples(duration.get(ITimeValue.Unit.MICROSECONDS), sampleRate); 
  }
  
}
