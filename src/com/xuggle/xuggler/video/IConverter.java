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
 * {@link BufferedImage} type.  Converters can optionally resize images
 * during
 * the conversion process.
 */

public interface IConverter
{
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

  /** Test if this converter is going to re-sample during conversion.
   * For some conversions between {@link BufferedImage} and {@link
   * IVideoPicture}, the IVideoPicture will need to be re-sampled into
   * another pixel type, commonly between YUV and RGB types.  This
   * re-sample is time consuming, and may not be available for
   * all installations of Xuggler.
   * 
   * @return true if this converter will re-sample during conversion.
   * 
   * @see com.xuggle.xuggler.IVideoResampler
   * @see com.xuggle.xuggler.IVideoResampler#isSupported(com.xuggle.xuggler.IVideoResampler.Feature)
   */

  public boolean willResample();

  /** Converts a {@link BufferedImage} to an {@link IVideoPicture}.
   *
   * @param image the source buffered image.
   * @param timestamp the time stamp which should be attached to the the
   *        video picture (in microseconds).
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
