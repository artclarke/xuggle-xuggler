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

import java.util.Map;
import java.util.HashMap;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;

import java.awt.image.BufferedImage;

import com.xuggle.xuggler.IPixelFormat;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IVideoResampler;

/** This factory class creates converters for translation between a
 * number of {@link IVideoPicture} and {@link BufferedImage} types.  Not
 * all image and picture types are supported.  When an unsupported
 * converter is requested, a descriptive {@link
 * UnsupportedOperationException} is thrown.  Each converter can
 * translate between any supported IPixelFormat.Type and a single {@link
 * BufferedImage} type.  Convertes can optionally resize during the
 * conversion process.
 */

public class ConverterFactory
{
  /** 
   * Create a converter which translates betewen {@link BufferedImage}
   * and {@link IVideoPicture} types.  The {@link
   * com.xuggle.xuggler.IPixelFormat.Type} and size are extracted from
   * the passed in picture.  This factory will attempt to create a
   * converter which can perform the translation.  If no converter can
   * be created, a descriptive {@link UnsupportedOperationException} is
   * thrown.
   *
   * @param converterType the type of converter which will be created
   * @param picture the picture from which size and type are extracted
   *
   * @throws UnsupportedOperationException if the found converter can
   *         not be properly initialized
   * @throws IllegalArgumentException if the passed {@link
   *         IVideoPicture} is NULL;
   */

  public static IConverter createConverter(
    IConverter.Type converterType,
    IVideoPicture picture)
  {
    if (picture == null)
      throw new IllegalArgumentException("The picture is NULL.");

    return createConverter(converterType, picture.getPixelType(), 
      picture.getWidth(), picture.getHeight());
  }

  /** 
   * Create a converter which translates betewen {@link BufferedImage}
   * and {@link IVideoPicture} types. The {@link BufferedImage} type and
   * size are extracted from the passed in image.  This factory will
   * attempt to create a converter which can perform the translation.
   * If no converter can be created, a descriptive {@link
   * UnsupportedOperationException} is thrown.
   *
   * @param pictureType the picture type of the converter
   * @param image the image from which size and type are extracted
   *
   * @throws UnsupportedOperationException if no converter for the
   *         specifed BufferedImage type exists
   * @throws UnsupportedOperationException if the found converter can
   *         not be properly initialized
   * @throws IllegalArgumentException if the passed {@link
   *         BufferedImage} is NULL;
   */

  public static IConverter createConverter(
    BufferedImage image,
    IPixelFormat.Type pictureType)
  {
    if (image == null)
      throw new IllegalArgumentException("The image is NULL.");

    // find the converter type based in image type

    IConverter.Type converterType = IConverter.Type.find(image.getType());
    if (converterType == null)
      throw new UnsupportedOperationException(
        "No converter found for BufferedImage type #" + 
        image.getType());

    // create and return the convert

    return createConverter(converterType, pictureType,
      image.getWidth(), image.getHeight());
  }

  /** 
   * Create a converter which translates betewen {@link BufferedImage}
   * and {@link IVideoPicture} types.  This factory will attempt to
   * create a converter which can perform the translation.  If no
   * converter can be created, a descriptive {@link
   * UnsupportedOperationException} is thrown.
   *
   * @param converterType the type of converter which will be created
   * @param pictureType the picture type of the converter
   * @param width the width of pictures and images
   * @param height the height of pictures and images
   *
   * @throws UnsupportedOperationException if the found converter can
   *         not be properly initialized
   */

  public static IConverter createConverter(
    IConverter.Type converterType,
    IPixelFormat.Type pictureType, 
    int width, int height)
  {
    return createConverter(converterType, pictureType, 
      width, height, width, height);
  }

  /** 
   * Create a converter which translates betewen {@link BufferedImage}
   * and {@link IVideoPicture} types.  This factory will attempt to
   * create a converter which can perform the translation.  If different
   * image and pictures sizes are passed the converter will resize
   * during translation.  If no converter can be created, a descriptive
   * {@link UnsupportedOperationException} is thrown.
   *
   * @param converterType the type of converter which will be created
   * @param pictureType the picture type of the converter
   * @param pictureWidth the width of pictures
   * @param pictureHeight the height of pictures
   * @param imageWidth the width of images
   * @param imageHeight the height of images
   *
   * @throws UnsupportedOperationException if the converter can not be
   *         properly created or initialized
   */

  public static IConverter createConverter(
    IConverter.Type converterType,
    IPixelFormat.Type pictureType, 
    int pictureWidth, int pictureHeight,
    int imageWidth, int imageHeight)
  {
    IConverter converter = null;

    try
    {
      // establish the constructor 

      Constructor<? extends IConverter> converterConstructor = 
        converterType.mConverterClass.getConstructor(IPixelFormat.Type.class,
          int.class, int.class, int.class, int.class);

      // create the converter

      converter = converterConstructor.newInstance(
        pictureType, pictureWidth, pictureHeight, imageWidth, imageHeight);
    }
    catch (NoSuchMethodException e)
    {
      throw new UnsupportedOperationException(
        "Converter " + converterType.mConverterClass + 
        " requries a constructor of the form "  +
        "(IPixelFormat.Type, int, int, int, int)");
    }
    catch (InvocationTargetException e)
    {
      throw new UnsupportedOperationException(
        "Converter " + converterType.mConverterClass + 
        " constructor failed with: " + e.getCause());
    }
    catch (IllegalAccessException e)
    {
      throw new UnsupportedOperationException(
        "Converter " + converterType.mConverterClass + 
        " constructor failed with: " + e);
    }
    catch (InstantiationException e)
    {
      throw new UnsupportedOperationException(
        "Converter " + converterType.mConverterClass + 
        " constructor failed with: " + e);
    }

    // return the newly created converter

    return converter;
  }
}
