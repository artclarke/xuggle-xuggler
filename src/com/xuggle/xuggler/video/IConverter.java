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

import java.awt.image.BufferedImage;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IPixelFormat;

/** This interface describes a converter which can perform bidirectional
 * translation between a given {@link IVideoPicture} type and a {@link
 * BufferedImage} type.  Converters are created by {@link
 * ConverterFactory}.  Each converter can translate between any
 * supported {@link com.xuggle.xuggler.IPixelFormat.Type} and a single
 * {@link BufferedImage} type.  Convertes can optionally resize during
 * the conversion process.
 */

public interface IConverter
{
  public enum Type 
  {
    /** Converter between {@link com.xuggle.xuggler.IPixelFormat.Type#ARGB}
     * and {@link BufferedImage#TYPE_INT_ARGB}.  */

    ARGB_32(ArgbConverter.class, IPixelFormat.Type.ARGB, 
      BufferedImage.TYPE_INT_ARGB, 32),

    /** Converter between {@link com.xuggle.xuggler.IPixelFormat.Type#BGR24}
     * and {@link BufferedImage#TYPE_3BYTE_BGR}.  */

    BGR_24(BgrConverter.class, IPixelFormat.Type.BGR24, 
      BufferedImage.TYPE_3BYTE_BGR, 24);

    /** The class responsible for converting between types. */

    final public Class<? extends IConverter> mConverterClass;

    /**
     * The {@link com.xuggle.xuggler.IPixelFormat.Type} which the
     * picture must be in to convert it to a {@link BufferedImage}
     */

    final public IPixelFormat.Type mPictureType;

    /**
     * The {@link BufferedImage} type which the image must be in to
     * convert it to a {@link com.xuggle.xuggler.IVideoPicture}
     */

    final public int mImageType;

    /** The number of bits per pixel. */

    final public int mBitsPerPixel;

    /** Construct a complete converter type description.
     *
     * @param converterClass the class which converts between pictures
     *        and images
     * @param pictureType the {@link
     *        com.xuggle.xuggler.IPixelFormat.Type} type which the
     *        picture must be in to convert it to an image
     * @param imageType the {@link BufferedImage} type which the picture
     *        must be in to convert it to a {@link BufferedImage}
     */

    Type(Class<? extends IConverter> converterClass, 
      IPixelFormat.Type pictureType, int imageType, int bitsPerPixel)
    {
      mConverterClass = converterClass;
      mPictureType = pictureType;
      mImageType = imageType;
      mBitsPerPixel = bitsPerPixel;
    }

    /** 
     * Find a converter by {@link com.xuggle.xuggler.IPixelFormat.Type}.
     * 
     * @return the matching converter or null of not found.
     */
    
    public static Type find(IPixelFormat.Type pictureType)
    {
      for (Type type: values())
        if (type.mPictureType == pictureType)
          return type;
      
      return null;
    }

    /** 
     * Find a converter by {@link BufferedImage} type number.
     * 
     * @return the matching converter or null of not found.
     */
    
    public static Type find(int imageType)
    {
      for (Type type: values())
        if (type.mImageType == imageType)
          return type;
      
      return null;
    }
  };


  /** Get the picture type, as defined by {@link
   * com.xuggle.xuggler.IPixelFormat.Type}, which this converter
   * recognizes.
   * 
   * @return the picture type of this converter.
   *
   * @see com.xuggle.xuggler.IPixelFormat.Type
   */

  public IPixelFormat.Type getPictureType();

  /** Get the image type, as defined by {@link BufferedImage}, which
   * this converter recognizes.
   * 
   * @return the image type of this converter.
   *
   * @see BufferedImage
   */

  public int getImageType();

  /** Test if this converter is going to resample during conversion.
   * For some conversions between {@link BufferedImage} and {@link
   * IVideoPicture}, the IVideoPicture will need to be resampled into
   * another pixel type, commonly between YUV and RGB types.  This
   * resample is time consuming, and often relies on the SwScale
   * libraries which may or may not be available.
   * 
   * @return true if this converter will resample during conversion.
   * 
   * @see com.xuggle.xuggler.IVideoResampler
   */

  public boolean willResample();

  /** Converts a {@link BufferedImage} to an {@link IVideoPicture}.
   *
   * @param image the source bufferd image.
   * @param timestamp the timestamp which should be attached to the the
   *        video picture.
   *
   * @throws IllegalArgumentException if the passed {@link
   *         BufferedImage} is NULL;
   * @throws IllegalArgumentException if the passed {@link
   *         BufferedImage} is not the correct type. See {@link
   *         #getImageType}.
   * @throws IllegalArgumentException if the underlying data buffer of
   *         the {@link BufferedImage} is composed elements other bytes
   *         or integers.
   */

  public IVideoPicture toPicture(BufferedImage image, long timestamp);

  /** Converts an {@link IVideoPicture} to a {@link BufferedImage}.
   *
   * @param picture the source video picture.
   *
   * @throws IllegalArgumentException if the passed {@link
   *         IVideoPicture} is NULL;
   * @throws IllegalArgumentException if the passed {@link
   *         IVideoPicture} is not the correct type. See {@link
   *         #getPictureType}.
   */

  public BufferedImage toImage(IVideoPicture picture);

  /** Return a written description of the converter. 
   *
   * @return a detailed description of what this converter does.
   */

  public String getDescription();
}
