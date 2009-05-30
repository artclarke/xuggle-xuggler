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

package com.xuggle.xuggler.video;

import com.xuggle.ferry.JNIReference;
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
import java.util.concurrent.atomic.AtomicReference;

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

    final AtomicReference<JNIReference> ref = new AtomicReference<JNIReference>(null);
    IVideoPicture resamplePicture = null;
    try
    {
      IVideoPicture picture = IVideoPicture.make(getRequiredPictureType(), image.getWidth(),
          image.getHeight());

      ByteBuffer pictureByteBuffer = picture.getByteBuffer(ref);

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
      picture.setComplete(true, getRequiredPictureType(), image.getWidth(),
          image.getHeight(), timestamp);

      // resample as needed
      if (willResample()) {
        resamplePicture = picture;
        picture = resample(resamplePicture, mToPictureResampler);
      }
      return picture;
    }
    finally
    {
      if (resamplePicture != null)
        resamplePicture.delete();
      if (ref.get() != null)
        ref.get().delete();
    }
  }

  /** {@inheritDoc} */

  public BufferedImage toImage(IVideoPicture picture)
  {
    // test that the picture is valid

    validatePicture(picture);

    // resample as needed

    IVideoPicture resamplePic = null;
    final AtomicReference<JNIReference> ref = 
      new AtomicReference<JNIReference>(null);
    try
    {
      if (willResample())
      {
        resamplePic = resample(picture, mToImageResampler);
        picture = resamplePic;
      }
      // get picture parameters

      final int w = picture.getWidth();
      final int h = picture.getHeight();

      // make a copy of the raw bytes in the picture and convert those to
      // integers

      final ByteBuffer byteBuf = picture.getByteBuffer(ref);

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

      final SampleModel sm = new SinglePixelPackedSampleModel(db.getDataType(),
          w, h, mBitMasks);
      final WritableRaster wr = Raster.createWritableRaster(sm, db, null);

      // return a new image created from the color model and raster

      return new BufferedImage(mColorModel, wr, false, null);
    }
    finally
    {
      if (resamplePic != null)
        resamplePic.delete();
      if (ref.get() != null)
        ref.get().delete();
    }
  }

  public void delete()
  {
    super.close();
  }
}
