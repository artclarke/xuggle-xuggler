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

import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;
import java.awt.image.DataBufferByte;
import java.awt.image.DataBuffer;
import java.awt.image.ColorModel;
import java.awt.image.DirectColorModel;
import java.awt.image.SampleModel;
import java.awt.image.SinglePixelPackedSampleModel;
import java.awt.image.WritableRaster;
import java.awt.image.Raster;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;

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
  private static final Logger log = LoggerFactory.getLogger(Utils.class);
  
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
  
  /**
   * Convert an {@link IVideoPicture} to a {@link BufferedImage}.  This
   * input picture must be of type {@link IPixelFormat.Type#ARGB}.
   * This method makes several copies of the raw picture bytes, which
   * is by no means the fastest way to do this.  
   *
   * The image data ultimatly resides in java memory space, which
   * means the caller does not need to concern themselfs with memory
   * management issues.
   *
   * @param aPicture The {@link IVideoPicture} to be converted.
   * 
   * @return the resultant {@link BufferedImage} which will contain
   * the video frame.
   * 
   * @throws IllegalArgumentException if the passed {@link
   * IVideoPicture} is NULL;
   * @throws IllegalArgumentException if the passed {@link
   * IVideoPicture} is not of type {@link IPixelFormat.Type#ARGB}.
   * @throws IllegalArgumentException if the passed {@link
   * IVideoPicture} is not complete.
   */
  
  public static BufferedImage videoPictureToImage(IVideoPicture aPicture)
  {
    // if the pictre is NULL, throw up
    
    if (aPicture == null)
      throw new IllegalArgumentException("The video picture is NULL.");

    // if the picture is not in ARGB, throw up
    
    if (aPicture.getPixelType() != IPixelFormat.Type.ARGB)
      throw new IllegalArgumentException(
        "The video picture is of type " + aPicture.getPixelType() +
        " but is required to be of type " + IPixelFormat.Type.ARGB);
    
    // if the picture is not complete, throw up
    
    if (!aPicture.isComplete())
      throw new IllegalArgumentException("The video picture is not complete.");
    
    // get picture parameters
    
    final int w = aPicture.getWidth();
    final int h = aPicture.getHeight();
    
    // make a copy of the raw bytes in the picture and convert those
    // to integers
    final ByteBuffer byteBuf = aPicture.getData().getByteBuffer(
        0, aPicture.getSize());
    // now, for this class of problems, we don't want the code
    // to switch byte order, so we'll pretend it's in native java order
    byteBuf.order(ByteOrder.BIG_ENDIAN);
    final IntBuffer intBuf = byteBuf.asIntBuffer();
    
    final int[] ints = new int[aPicture.getSize()/4];
    intBuf.get(ints, 0, ints.length);
   
    // create the data buffer from the ints
    
    final DataBufferInt db = new DataBufferInt(ints, ints.length);
    
    // create an a sample model which matches the byte layout of the
    // image data and raster which contains the data which now can be
    // properly interpreted
    
    final int[] bitMasks = {0xff0000, 0xff00, 0xff, 0xff000000};
    final SampleModel sm = new SinglePixelPackedSampleModel(
      db.getDataType(), w, h, bitMasks);
    final WritableRaster wr = Raster.createWritableRaster(sm, db, null);
    
    // create a color model
    
    final ColorModel cm = new DirectColorModel(
      32, 0xff0000, 0xff00, 0xff, 0xff000000);
    
    // return a new image created from the color model and raster
    
    return new BufferedImage(cm, wr, false, null);
  }
  
  /**
   * Convert a {@link BufferedImage} to an {@link IVideoPicture} of
   * type {@link IPixelFormat.Type#ARGB}.  This is NOT the most
   * efficient way to do this conversion and is thus ripe for
   * optimization.  The {@link BufferedImage} must be a 32 RGBA type.
   * Further more the underlying data buffer of the {@link
   * BufferedImage} must be composed of types bytes or integers (which
   * is the most typical case).
   *
   * @param aImage The source {@link BufferedImage}.
   * @param aPts The presentation time stamp of the picture.
   *
   * @return An {@link IVideoPicture} in {@link
   * IPixelFormat.Type#ARGB} format.
   *

   * @throws IllegalArgumentException if the passed {@link
   * BufferedImage} is NULL;
   * @throws IllegalArgumentException if the passed {@link
   * BufferedImage} is not of type {@link BufferedImage#TYPE_INT_ARGB}.
   * @throws IllegalArgumentException if the underlying data buffer of
   * the {@link BufferedImage} is composed of types other bytes or
   * integers.
   */

  public static IVideoPicture imageToVideoPicture(
    BufferedImage aImage, long aPts)
  {
    //if the is NULL, throw up
    
    if (aImage == null)
      throw new IllegalArgumentException("The image is NULL.");

     // if image is not in TYPE_INT_ARGB, throw up
    
    if (aImage.getType() != BufferedImage.TYPE_INT_ARGB)
      throw new IllegalArgumentException(
        "The image is of type #" + aImage.getType() +
        " but is required to be BufferedImage.TYPE_INT_ARGB of type #" +
        BufferedImage.TYPE_INT_ARGB + ".");

    // get the image byte buffer buffer
    
    DataBuffer imageBuffer = aImage.getRaster().getDataBuffer();
    byte[] imageBytes=null;
    int[] imageInts = null;
    
    // handle byte buffer case
    
    if (imageBuffer instanceof DataBufferByte)
    {
      imageBytes = ((DataBufferByte)imageBuffer).getData();
    }
    
    // handel integer buffer case
    
    else if (imageBuffer instanceof DataBufferInt)
    {
      imageInts = ((DataBufferInt)imageBuffer).getData();
    }
    
    // if it's some other type, throw 
    
    else
    {
      throw new IllegalArgumentException(
        "Unsupported BufferedImage data buffer type: " + 
        imageBuffer.getDataType());
    }
    
    // create the video picture and get it's underling buffer
    
    IVideoPicture picture = IVideoPicture.make(
      IPixelFormat.Type.ARGB, aImage.getWidth(), aImage.getHeight());
    IBuffer pictureBuffer = picture.getData();
    ByteBuffer pictureByteBuffer = pictureBuffer.getByteBuffer(
      0, pictureBuffer.getBufferSize());
    
    // cram the image bytes into the picture
    log.debug("src size: {}; bb pos: {}; bb limit: {}; bb capacity: {}; ib size: {}; ivp size: {}; ivp width: {}; ivp height: {}; ivp format: {}",
        new Object[]{
          imageBytes.length,
          pictureByteBuffer.position(),
          pictureByteBuffer.limit(),
          pictureByteBuffer.capacity(),
          pictureBuffer.getBufferSize(),
          picture.getSize(),
          picture.getWidth(),
          picture.getHeight(),
          picture.getPixelType().toString()
    });
    if (imageInts != null)
    {
      pictureByteBuffer.order(ByteOrder.BIG_ENDIAN);
      IntBuffer pictureIntBuffer = pictureByteBuffer.asIntBuffer();
      pictureIntBuffer.put(imageInts);
    } else {
      pictureByteBuffer.put(imageBytes);      
    }
    pictureByteBuffer = null;
    picture.setComplete(
      true, IPixelFormat.Type.ARGB, 
      aImage.getWidth(), aImage.getHeight(), aPts);
    
    // return the ARGB picture
    
    return picture;
  }
}
