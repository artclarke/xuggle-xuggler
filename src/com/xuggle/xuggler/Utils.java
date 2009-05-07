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
import com.xuggle.xuggler.video.IConverter;
import com.xuggle.xuggler.video.ConverterFactory;

import java.awt.image.BufferedImage;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * A collection of useful utilities for creating blank {@link IVideoPicture} objects
 * and managing audio time stamp to sample conversions.
 * 
 * @author aclarke
 *
 */
public class Utils
{
  static private final Logger log = LoggerFactory.getLogger(Utils.class);
  static {
    log.trace("Utils loaded");
  }

  /**
   * Get a new blank frame object encoded in {@link IPixelFormat.Type#YUV420P} format.
   * 
   * If crossHatchW and crossHatchH are greater than zero, then we'll draw a cross-hatch
   * pattern with boxes of those sizes alternating on the screen.  This can be useful to
   * spot coding and translation errors.  Word of warning though -- due to how the YUV420P
   * colorspace works, you'll need to make sure crossHatchW and crossHatchH are even numbers
   * to avoid unslightly u/v lines at box edges.
   * 
   * @param w width of object
   * @param h height of object
   * @param yColor Y component of background color.
   * @param uColor U component of background color.
   * @param vColor V component of background color.
   * @param pts The time stamp, in microseconds, you want this frame to have, or {@link Global#NO_PTS} for none.
   * @param crossHatchW width of cross hatch on image, or <=0 for none
   * @param crossHatchH height of cross hatch on image, or <= 0 for none
   * @param crossHatchYColor Y component of cross-hatch color.
   * @param crossHatchUColor U component of cross-hatch color.
   * @param crossHatchVColor V component of cross-hatch color.
   * @return A new frame, or null if we can't create it.
   */
  public static IVideoPicture getBlankFrame(
      int w,
      int h, 
      int yColor,
      int uColor,
      int vColor,
      long pts,
      int crossHatchW,
      int crossHatchH,
      int crossHatchYColor,
      int crossHatchUColor,
      int crossHatchVColor)
  {
    IVideoPicture frame = IVideoPicture.make(IPixelFormat.Type.YUV420P, w, h);
    if (frame == null)
      throw new OutOfMemoryError("could not allocate frame");
    
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
        int sliceLen = lineLength*h;
        for(int i = offset ; i < offset + sliceLen; i++)
        {
          int x = (i-offset) % lineLength;
          int y = (i-offset) / lineLength;
          if (crossHatchH > 0 && crossHatchW > 0 && ( 
              ((x / crossHatchW)%2==1 && (y / crossHatchH)%2==0) ||
              ((x / crossHatchW)%2==0 && (y / crossHatchH)%2==1)
          ))
            buffer.put(i, (byte)crossHatchYColor);
          else
            buffer.put(i, (byte)yColor);
        }

        // now, check the U value
        offset = (frame.getDataLineSize(0)*h);
        lineLength = frame.getDataLineSize(1);
        sliceLen = lineLength * ((h+1) / 2);
        for(int i = offset ; i < offset + sliceLen; i++)
        {
          if (crossHatchH > 0 && crossHatchW > 0)
          {
            // put x and y in bottom right of pixel range
            int x = ((i-offset) % lineLength)*2;
            int y = ((i-offset) / lineLength)*2;
            
            int[] xCoords = new int[] { x, x+1, x, x+1 };
            int[] yCoords = new int[] { y, y, y+1, y+1};
            int finalColor = 0;
            for(int j =0; j < xCoords.length; j++)
            {
              int color = uColor;
              x = xCoords[j];
              y = yCoords[j];
              if (
                  ((x / crossHatchW)%2==1 && (y / crossHatchH)%2==0) ||
                  ((x / crossHatchW)%2==0 && (y / crossHatchH)%2==1)
                )
              {
                color = crossHatchUColor;
              }
              finalColor += color;
            }
            finalColor /= xCoords.length;
            buffer.put(i, (byte)finalColor);
          }
          else
            buffer.put(i, (byte)uColor);
        }

        // and finally the V
        offset = (frame.getDataLineSize(0)*h) + (frame.getDataLineSize(1)*((h+1)/2));
        lineLength = frame.getDataLineSize(2);
        sliceLen = lineLength * ((h+1) / 2);
        for(int i = offset; i < offset + sliceLen; i++)
        {
          if (crossHatchH > 0 && crossHatchW > 0)
          {
            // put x and y in bottom right of pixel range
            int x = ((i-offset) % lineLength)*2;
            int y = ((i-offset) / lineLength)*2;
            
            int[] xCoords = new int[] { x, x+1, x, x+1 };
            int[] yCoords = new int[] { y, y, y+1, y+1};
            int finalColor = 0;
            for(int j =0; j < xCoords.length; j++)
            {
              int color = vColor;
              x = xCoords[j];
              y = yCoords[j];
              if (
                  ((x / crossHatchW)%2==1 && (y / crossHatchH)%2==0) ||
                  ((x / crossHatchW)%2==0 && (y / crossHatchH)%2==1)
                )
                color =crossHatchVColor;
              finalColor += color;
            }
            finalColor /= xCoords.length;
            buffer.put(i, (byte)finalColor);
          }
          else
            buffer.put(i, (byte)vColor);
        }
      }
      // set it complete
      frame.setComplete(true, IPixelFormat.Type.YUV420P, w, h, pts);
    }
    return frame;
  }
  
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
    return getBlankFrame(w, h, y, u, v, pts, 0, 0, 0, 0, 0);
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
  
  /**
   * Convert an {@link IVideoPicture} to a {@link BufferedImage}.  The
   * input picture should be of type {@link IPixelFormat.Type#BGR24}
   * to avoid making unnecessary copies.
   *
   * The image data ultimately resides in java memory space, which
   * means the caller does not need to concern themselves with memory
   * management issues.
   *
   * @param picture The {@link IVideoPicture} to be converted.
   * 
   * @return the resultant {@link BufferedImage} which will contain
   * the video frame.
   * 
   * @throws IllegalArgumentException if the passed {@link
   * IVideoPicture} is NULL;
   * @throws IllegalArgumentException if the passed {@link
   * IVideoPicture} is not complete.
   *
   * @deprecated Image and picture conversion functionality has been
   * replaced by {@link com.xuggle.xuggler.video.ConverterFactory}.  The
   * current implementation of {@link #videoPictureToImage} creates a new
   * {@link com.xuggle.xuggler.video.IConverter} on each call, which is
   * not very efficient.
   */
  
  @Deprecated
    public static BufferedImage videoPictureToImage(IVideoPicture picture)
  {
    // if the pictre is NULL, throw up
    
    if (picture == null)
      throw new IllegalArgumentException("The video picture is NULL.");

    // create the converter

    IConverter converter = ConverterFactory.createConverter(
      ConverterFactory.XUGGLER_BGR_24, picture);

    // return the conveter

    return converter.toImage(picture);
  }
  
  /**
   * Convert a {@link BufferedImage} to an {@link IVideoPicture} of
   * type {@link IPixelFormat.Type#BGR24}.  The {@link BufferedImage} must be a 
   * {@link BufferedImage#TYPE_3BYTE_BGR} type.
   *
   * @param image The source {@link BufferedImage}.
   * @param pts The presentation time stamp of the picture.
   *
   * @return An {@link IVideoPicture} in {@link
   * IPixelFormat.Type#BGR24} format.
   *
   * @throws IllegalArgumentException if the passed {@link
   * BufferedImage} is NULL;
   * @throws IllegalArgumentException if the passed {@link
   * BufferedImage} is not of type {@link BufferedImage#TYPE_3BYTE_BGR}.
   * @throws IllegalArgumentException if the underlying data buffer of
   * the {@link BufferedImage} is composed of types other bytes or
   * integers.
   *
   * @deprecated Image and picture conversion functionality has been
   * replaced by {@link com.xuggle.xuggler.video.ConverterFactory}.  The
   * current implementation of {@link #imageToVideoPicture} creates a new
   * {@link com.xuggle.xuggler.video.IConverter} on each call, which is
   * not very efficient.
   */
  
  @Deprecated
    public static IVideoPicture imageToVideoPicture(BufferedImage image, long pts)
  {
    // if the image is NULL, throw up
    
    if (image == null)
      throw new IllegalArgumentException("The image is NULL.");

    if (image.getType() != BufferedImage.TYPE_3BYTE_BGR)
      throw new IllegalArgumentException(
        "The image is of type #" + image.getType() +
        " but is required to be BufferedImage.TYPE_3BYTE_BGR of type #" +
        BufferedImage.TYPE_3BYTE_BGR + ".");

    // create the converter

    IConverter converter = ConverterFactory.createConverter(
      image, IPixelFormat.Type.BGR24);

    // return the converted picture

    return converter.toPicture(image, pts);
  }
}
