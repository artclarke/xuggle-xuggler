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
import java.util.Collection;
import java.util.Collections;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationTargetException;

import java.awt.image.BufferedImage;

import com.xuggle.xuggler.IPixelFormat;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IVideoResampler;

/**
 * This factory class creates {@link IConverter} objects for
 * translation between a
 * number of {@link IVideoPicture} and {@link BufferedImage} types.  Not
 * all image and picture types are supported.  When an unsupported
 * converter is requested, a descriptive {@link
 * UnsupportedOperationException} is thrown.
 * <p>  Each converter can
 * translate between any supported {@link com.xuggle.xuggler.IPixelFormat.Type}
 *  and a single {@link
 * BufferedImage} type.  Converters can optionally resize during the
 * conversion process.
 * </p>
 * <p>
 * Each converter will re-sample the {@link IVideoPicture} it is converting
 * to get the data in the same native byte layout as a {@link BufferedImage}
 * if needed (using {@link IVideoResampler}).  This step takes time and creates
 * a new temporary copy of the image data.  To avoid this step
 * you can make sure the {@link IVideoPicture#getPixelType()} is of the same
 * binary type as a {@link BufferedImage}.
 * </p>
 * <p>  Put another way, if you're
 * converting to {@link BufferedImage#TYPE_3BYTE_BGR}, we will not resample
 * if the {@link IVideoPicture#getPixelType()} is
 *  {@link com.xuggle.xuggler.IPixelFormat.Type#BGR24}. 
 * </p>
 */

public class ConverterFactory
{
  /**
   * Default constructor
   */
  protected ConverterFactory() {
    
  }
  /** Converts between IVideoPictures and {@link BufferedImage} of type
   *  {@link BufferedImage#TYPE_INT_ARGB}.  CATION: The ARGB converter
   *  is not currently registered as a converter because the underlying
   *  FFMPEG support for ARGB is unstable.  If you really need ARGB
   *  type, manually register the converter, but be aware that the ARGB
   *  images produces may be wrong or broken.
   */

  public static final String XUGGLER_ARGB_32 = "XUGGLER-ARGB-32";

  /** Converts between IVideoPictures and {@link BufferedImage} of type
   * {@link BufferedImage#TYPE_3BYTE_BGR} */

  public static final String XUGGLER_BGR_24 = "XUGGLER-BGR-24";

  // the registered converter types
  
  private static Map<String, Type> mConverterTypes = new HashMap<String, Type>();

  // register the known converters

  static
  {
    // The ARGB converter is not currently registered as a converter
    // because the underlying FFMPEG support for ARGB is unstable.  If
    // you really need ARGB type, manually register the converter, but
    // be aware that the ARGB images produces may be wrong or broken.
    
    //registerConverter(new Type(XUGGLER_ARGB_32, ArgbConverter.class, 
    //IPixelFormat.Type.ARGB, BufferedImage.TYPE_INT_ARGB));

    registerConverter(new Type(XUGGLER_BGR_24, BgrConverter.class, 
        IPixelFormat.Type.BGR24, BufferedImage.TYPE_3BYTE_BGR));
  }

  /**
   * Register a converter with this factory. The factory oragnizes the
   * converters by descriptor, and thus each should be unique unless one
   * whishes to replace an existing converter.
   *
   * @param converterType the type for the converter to be registered
   *
   * @return the converter type which this converter displaced, or NULL
   * of this is a novel converter
   */

  public static Type registerConverter(Type converterType)
  {
    return mConverterTypes.put(converterType.getDescriptor(), converterType);
  }

  /**
   * Unregister a converter with this factory.
   *
   * @param converterType the type for the converter to be unregistered
   *
   * @return the converter type which removed, or NULL of this converter
   * was not recognized
   */

  public static Type unregisterConverter(Type converterType)
  {
    return mConverterTypes.remove(converterType.getDescriptor());
  }

  /**
   * Get a collection of the registered converters.  The collection is unmodifiable.
   *
   * @return an unmodifiable collection of converter types.
   */

  public static Collection<Type> getRegisteredConverters()
  {
    return Collections.unmodifiableCollection(mConverterTypes.values());
  }

  /**
   * Find a converter given a type descriptor.
   *
   * @param descriptor a unique string which describes this converter
   * 
   * @return the converter found or NULL if it was not found.
   */

  public static Type findRegisteredConverter(String descriptor)
  {
    return mConverterTypes.get(descriptor);
  }

  /** 
   * Create a converter which translates betewen {@link BufferedImage}
   * and {@link IVideoPicture} types.  The {@link
   * com.xuggle.xuggler.IPixelFormat.Type} and size are extracted from
   * the passed in picture.  This factory will attempt to create a
   * converter which can perform the translation.  If no converter can
   * be created, a descriptive {@link UnsupportedOperationException} is
   * thrown.
   *
   * @param converterDescriptor the unique string descriptor of the
   *        converter which is to be created
   * @param picture the picture from which size and type are extracted
   *
   * @throws UnsupportedOperationException if the converter can not be
   *         found
   * @throws UnsupportedOperationException if the found converter can
   *         not be properly initialized
   * @throws IllegalArgumentException if the passed {@link
   *         IVideoPicture} is NULL;
   */

  public static IConverter createConverter(
    String converterDescriptor,
    IVideoPicture picture)
  {
    if (picture == null)
      throw new IllegalArgumentException("The picture is NULL.");

    return createConverter(converterDescriptor, picture.getPixelType(), 
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
   * @param image the image from which size and type are extracted
   * @param pictureType the picture type of the converter
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

    String converterDescriptor = null;
    for (Type converterType: getRegisteredConverters())
      if (converterType.getImageType() == image.getType())
        converterDescriptor = converterType.getDescriptor();
    if (converterDescriptor == null)
      throw new UnsupportedOperationException(
        "No converter found for BufferedImage type #" + 
        image.getType());

    // create and return the converter

    return createConverter(converterDescriptor, pictureType,
      image.getWidth(), image.getHeight());
  }

  /** 
   * Create a converter which translates betewen {@link BufferedImage}
   * and {@link IVideoPicture} types.  This factory will attempt to
   * create a converter which can perform the translation.  If no
   * converter can be created, a descriptive {@link
   * UnsupportedOperationException} is thrown.
   *
   * @param converterDescriptor the unique string descriptor of the
   *        converter which is to be created
   * @param pictureType the picture type of the converter
   * @param width the width of pictures and images
   * @param height the height of pictures and images
   *
   * @throws UnsupportedOperationException if the converter can not be
   *         found
   * @throws UnsupportedOperationException if the found converter can
   *         not be properly initialized
   */

  public static IConverter createConverter(
    String converterDescriptor,
    IPixelFormat.Type pictureType, 
    int width, int height)
  {
    return createConverter(converterDescriptor, pictureType, 
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
   * @param converterDescriptor the unique string descriptor of the
   *        converter which is to be created
   * @param pictureType the picture type of the converter
   * @param pictureWidth the width of pictures
   * @param pictureHeight the height of pictures
   * @param imageWidth the width of images
   * @param imageHeight the height of images
   *
   * @throws UnsupportedOperationException if the converter can not be
   *         found
   * @throws UnsupportedOperationException if the converter can not be
   *         properly created or initialized
   */

  public static IConverter createConverter(
    String converterDescriptor,
    IPixelFormat.Type pictureType, 
    int pictureWidth, int pictureHeight,
    int imageWidth, int imageHeight)
  {
    IConverter converter = null;
    
    // establish the converter type

    Type converterType = findRegisteredConverter(converterDescriptor);
    if (null == converterType)
      throw new UnsupportedOperationException(
        "No converter \"" + converterDescriptor + "\" found.");

    // create the converter

    try
    {
      // establish the constructor 

      Constructor<? extends IConverter> converterConstructor = 
        converterType.getConverterClass().getConstructor(IPixelFormat.Type.class,
          int.class, int.class, int.class, int.class);

      // create the converter

      converter = converterConstructor.newInstance(
        pictureType, pictureWidth, pictureHeight, imageWidth, imageHeight);
    }
    catch (NoSuchMethodException e)
    {
      throw new UnsupportedOperationException(
        "Converter " + converterType.getConverterClass() + 
        " requries a constructor of the form "  +
        "(IPixelFormat.Type, int, int, int, int)");
    }
    catch (InvocationTargetException e)
    {
      throw new UnsupportedOperationException(
        "Converter " + converterType.getConverterClass() + 
        " constructor failed with: " + e.getCause());
    }
    catch (IllegalAccessException e)
    {
      throw new UnsupportedOperationException(
        "Converter " + converterType.getConverterClass() + 
        " constructor failed with: " + e);
    }
    catch (InstantiationException e)
    {
      throw new UnsupportedOperationException(
        "Converter " + converterType.getConverterClass() + 
        " constructor failed with: " + e);
    }

    // return the newly created converter

    return converter;
  }


  /**
   * This class describes a converter type and is used to register and
   * unregister types with {@link ConverterFactory}.  The factory
   * oragnizes the converters by descriptor, and thus each should be
   * unique unless you wish to replace an existing converter.
   */

  public static class Type 
  {
    /** The unique string which describes this converter. */

    final private String mDescriptor;

    /** The class responsible for converting between types. */

    final private Class<? extends IConverter> mConverterClass;

    /**
     * The {@link com.xuggle.xuggler.IPixelFormat.Type} which the
     * picture must be in to convert it to a {@link BufferedImage}
     */

    final private IPixelFormat.Type mPictureType;

    /**
     * The {@link BufferedImage} type which the image must be in to
     * convert it to a {@link com.xuggle.xuggler.IVideoPicture}
     */

    final private int mImageType;

    /** Construct a complete converter type description.
     *
     * @param descriptor a unique string which describes this converter
     * @param converterClass the class which converts between pictures
     *        and images
     * @param pictureType the {@link
     *        com.xuggle.xuggler.IPixelFormat.Type} type which the
     *        picture must be in to convert it to an image
     * @param imageType the {@link BufferedImage} type which the picture
     *        must be in to convert it to a {@link BufferedImage}
     */

    public Type(String descriptor, Class<? extends IConverter> converterClass, 
      IPixelFormat.Type pictureType, int imageType)
    {
      mDescriptor = descriptor;
      mConverterClass = converterClass;
      mPictureType = pictureType;
      mImageType = imageType;
    }

    /** Get the unique string which describes this converter. */

    public String getDescriptor()
    {
      return mDescriptor;
    }

    /** Get the class responsible for converting between types. */

    public Class<? extends IConverter> getConverterClass()
    {
      return mConverterClass;
    }

    /**
     * Get the {@link com.xuggle.xuggler.IPixelFormat.Type} which the
     * picture must be in to convert it to a {@link BufferedImage}
     */

    public IPixelFormat.Type getPictureType()
    {
      return mPictureType;
    }

    /**
     * Get the {@link BufferedImage} type which the image must be in to
     * convert it to a {@link com.xuggle.xuggler.IVideoPicture}
     */

    public int getImageType()
    {
      return mImageType;
    }

    /**
     * Get a string description of this conveter type.
     */

    public String toString()
    {

      return getDescriptor() + ": picture type " + getPictureType() +
        ", image type " + getImageType();
    }
  }
}
