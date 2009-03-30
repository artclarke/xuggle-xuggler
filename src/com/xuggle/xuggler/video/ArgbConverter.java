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

package com.xuggle.xuggler.video;

import com.xuggle.ferry.IBuffer;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IPixelFormat;

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

/** A converter to translate {@link IVideoPicture}s to and from
 * {@link BufferedImage}s of type {@link BufferedImage#TYPE_INT_ARGB}. */

public class ArgbConverter extends AConverter
{
  // bit masks requried by the sample model
    
  private static final int[] mBitMasks = {0xff0000, 0xff00, 0xff, 0xff000000};

  // the ARGB color model

  private static final ColorModel mColorModel = new DirectColorModel(
    32, 0xff0000, 0xff00, 0xff, 0xff000000);

  /** Construct as converter to translate {@link IVideoPicture}s to and
   * from {@link BufferedImage}s of type {@link
   * BufferedImage#TYPE_INT_ARGB}.
   *
   * @param pictureType the picture type recognized by this converter
   * @param pictureWidth the width of pictures
   * @param pictureHeight the height of pictures
   * @param imageWidth the width of images
   * @param imageHeight the height of images
   */

  public ArgbConverter(IPixelFormat.Type pictureType, 
    int pictureWidth, int pictureHeight,
    int imageWidth, int imageHeight)
  {
    super(pictureType, IPixelFormat.Type.ARGB,
      BufferedImage.TYPE_INT_ARGB, pictureWidth, 
      pictureHeight, imageWidth, imageHeight);
  }

  /** {@inheritDoc} */

  public IVideoPicture toPicture(BufferedImage image, long timestamp)
  {
    // validate the image

    validateImage(image);

    // get the image byte buffer buffer

    DataBuffer imageBuffer = image.getRaster().getDataBuffer();
    byte[] imageBytes = null;
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
      getRequiredPictureType(), image.getWidth(), image.getHeight());
    IBuffer pictureBuffer = picture.getData();
    ByteBuffer pictureByteBuffer = pictureBuffer.getByteBuffer(
      0, pictureBuffer.getBufferSize());

    if (imageInts != null)
    {
      pictureByteBuffer.order(ByteOrder.BIG_ENDIAN);
      IntBuffer pictureIntBuffer = pictureByteBuffer.asIntBuffer();
      pictureIntBuffer.put(imageInts);
    }
    else
    {
      pictureByteBuffer.put(imageBytes);
    }
    pictureByteBuffer = null;
    picture.setComplete(
      true, IPixelFormat.Type.ARGB,
      image.getWidth(), image.getHeight(), timestamp);

    // resample as needed

    return toPictureResample(picture);
  }

  /** {@inheritDoc} */

  public BufferedImage toImage(IVideoPicture picture)
  {
    // test that the picture is valid

    validatePicture(picture);

    // resample as needed

    picture = toImageResample(picture);

    // get picture parameters
    
    final int w = picture.getWidth ();
    final int h = picture.getHeight();
    
    // make a copy of the raw bytes in the picture and convert those to
    // integers

    final ByteBuffer byteBuf = picture.getData().getByteBuffer(
        0, picture.getSize());

    // now, for this class of problems, we don't want the code
    // to switch byte order, so we'll pretend it's in native java order

    byteBuf.order(ByteOrder.BIG_ENDIAN);
    final IntBuffer intBuf = byteBuf.asIntBuffer();
    final int[] ints = new int[picture.getSize() / 4];
    intBuf.get(ints, 0, ints.length);
   
    // create the data buffer from the ints
    
    final DataBufferInt db = new DataBufferInt(ints, ints.length);
    
    // create an a sample model which matches the byte layout of the
    // image data and raster which contains the data which now can be
    // properly interpreted
    
    final SampleModel sm = new SinglePixelPackedSampleModel(
      db.getDataType(), w, h, mBitMasks);
    final WritableRaster wr = Raster.createWritableRaster(sm, db, null);
    
    // return a new image created from the color model and raster
    
    return new BufferedImage(mColorModel, wr, false, null);
  }
}
