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

package com.xuggle.xuggler.video;

import com.xuggle.ferry.IBuffer;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IPixelFormat;

import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;
import java.awt.image.DataBufferByte;
import java.awt.image.DataBuffer;
import java.awt.image.ColorModel;
import java.awt.color.ColorSpace;
import java.awt.image.ComponentColorModel;
import java.awt.image.SampleModel;
import java.awt.image.PixelInterleavedSampleModel;
import java.awt.image.WritableRaster;
import java.awt.image.Raster;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;

/** A converter to translate {@link IVideoPicture}s to and from
 * {@link BufferedImage}s of type {@link BufferedImage#TYPE_3BYTE_BGR}. */

public class BgrConverter extends AConverter
{
  // band offsets requried by the sample model
    
  private static final int[] mBandOffsets = {2, 1, 0};

  // color space for this converter
  
  private static final ColorSpace mColorSpace = 
    ColorSpace.getInstance(ColorSpace.CS_sRGB);

  /** Construct as converter to translate {@link IVideoPicture}s to and
   * from {@link BufferedImage}s of type {@link
   * BufferedImage#TYPE_3BYTE_BGR}.
   *
   * @param pictureType the picture type recognized by this converter
   * @param pictureWidth the width of pictures
   * @param pictureHeight the height of pictures
   * @param imageWidth the width of images
   * @param imageHeight the height of images
   */

  public BgrConverter(IPixelFormat.Type pictureType, 
    int pictureWidth, int pictureHeight,
    int imageWidth, int imageHeight)
  {
    super(pictureType, IPixelFormat.Type.BGR24,
      BufferedImage.TYPE_3BYTE_BGR, pictureWidth, 
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
      true, getRequiredPictureType(),
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
    
    final int w = picture.getWidth();
    final int h = picture.getHeight();
    
    // make a copy of the raw bytes int a DataBufferByte which the
    // writable raster can operate on

    final ByteBuffer byteBuf = picture.getData().getByteBuffer(
        0, picture.getSize());
    final byte[] bytes = new byte[picture.getSize()];
    byteBuf.get(bytes, 0, bytes.length);
   
    // create the data buffer from the bytes
    
    final DataBufferByte db = new DataBufferByte(bytes, bytes.length);
    
    // create an a sample model which matches the byte layout of the
    // image data and raster which contains the data which now can be
    // properly interpreted
    
    final SampleModel sm = new PixelInterleavedSampleModel(
      db.getDataType(), w, h, 3, 3 * w, mBandOffsets);
    final WritableRaster wr = Raster.createWritableRaster(sm, db, null);
    
    // create a color model
    
    final ColorModel colorModel = new ComponentColorModel(
      mColorSpace, false, false, ColorModel.OPAQUE, db.getDataType());
    
    // return a new image created from the color model and raster
    
    return new BufferedImage(colorModel, wr, false, null);
  }
}
