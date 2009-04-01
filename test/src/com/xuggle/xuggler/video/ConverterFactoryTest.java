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

import java.util.Collection;
import java.util.Vector;

import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IPixelFormat;
import com.xuggle.xuggler.IVideoResampler;
import com.xuggle.xuggler.IVideoResampler.Feature;

import java.awt.geom.AffineTransform;
import java.awt.image.AffineTransformOp;
import java.awt.image.BufferedImage;
import java.awt.Color;
import java.util.Random;

import org.junit.*;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;


import static junit.framework.Assert.*;
import static java.lang.Math.*;

@RunWith(Parameterized.class)
public class ConverterFactoryTest
{
  // default width and height for test images, note that the width is
  // (and should remain) a perfect multiple of 16 which intentionally
  // avoids some issue with image resampling.  those issues are
  // exersized in specific tests such that can be ignored until the
  // underly cause can be estabish.

  public static final int TEST_WIDTH  = 48;
  public static final int TEST_HEIGHT = 48;

  private final ConverterFactory.Type mConverterType;
  private final IPixelFormat.Type mPixelType;

  // pixel types to included for resampling tests, as we really want
  // them to work properly
  
  public static final IPixelFormat.Type[] mIncludedPixelTypes =
  {
    IPixelFormat.Type.ARGB,    
    IPixelFormat.Type.BGR24,
    IPixelFormat.Type.YUV420P,
  };

  // pixel types to exclude from test as no resamplers exist for them,
  // if new types appear, NOTE: this list is not currently being used

  public static final IPixelFormat.Type[] mExcludePixelTypes =
  {
    IPixelFormat.Type.NONE,    
    IPixelFormat.Type.PAL8,
    IPixelFormat.Type.XVMC_MPEG2_MC,
    IPixelFormat.Type.XVMC_MPEG2_IDCT,
    IPixelFormat.Type.UYYVYY411,
    IPixelFormat.Type.BGR4,
    IPixelFormat.Type.RGB4,
    IPixelFormat.Type.NV12,
    IPixelFormat.Type.NV21,
    IPixelFormat.Type.VDPAU_H264,
    IPixelFormat.Type.VDPAU_MPEG1,
    IPixelFormat.Type.VDPAU_MPEG2,
    IPixelFormat.Type.VDPAU_WMV3,
    IPixelFormat.Type.VDPAU_VC1,
    IPixelFormat.Type.RGB48BE,
    IPixelFormat.Type.RGB48LE,
    IPixelFormat.Type.RGB565BE,
    IPixelFormat.Type.RGB555BE,
    IPixelFormat.Type.BGR565BE,
    IPixelFormat.Type.BGR555BE,
    IPixelFormat.Type.VAAPI_MOCO,
    IPixelFormat.Type.VAAPI_IDCT,
    IPixelFormat.Type.VAAPI_VLD,
    IPixelFormat.Type.NB,
  };

  public ConverterFactoryTest(ConverterFactory.Type converterType, 
    IPixelFormat.Type pixelType)
  {
    mConverterType = converterType;
    mPixelType = pixelType;
    System.out.println("Testing " + mConverterType + ", " + mPixelType);
  }

  // create a parameter list of different types of converters

  @Parameters
    public static Collection<Object[]> converterTypes()
  {
    Collection<Object[]> parameters = new Vector<Object[]>();
    
    for (IPixelFormat.Type pixelType: mIncludedPixelTypes)
      for (ConverterFactory.Type converterType:
             ConverterFactory.getRegisteredConverters())
        {
          Object[] tuple = {converterType, pixelType};
          parameters.add(tuple);
        }
      
    return parameters;
  }
  
  @Test(expected=IllegalArgumentException.class)
  public void testVideoPictureToImageNullInput()
  {
    if (!IVideoResampler.isSupported(Feature.FEATURE_COLORSPACECONVERSION))
      throw new IllegalArgumentException();
    
    IConverter c = ConverterFactory.createConverter(
      mConverterType.getDescriptor(), mConverterType.getPictureType(),
      TEST_WIDTH, TEST_HEIGHT);
    
    c.toImage(null);
  }

  @Test(expected=IllegalArgumentException.class)
  public void testImageToVideoPictureNullInput()
  {
    if (!IVideoResampler.isSupported(Feature.FEATURE_COLORSPACECONVERSION))
      throw new IllegalArgumentException();

    IConverter c = ConverterFactory.createConverter(
      mConverterType.getDescriptor(), mConverterType.getPictureType(),
      TEST_WIDTH, TEST_HEIGHT);
    
    c.toPicture(null, 0);
  }

  @Test(expected=IllegalArgumentException.class)
  public void testVideoPictureToImageIncompletePicture()
  {
    if (!IVideoResampler.isSupported(Feature.FEATURE_COLORSPACECONVERSION))
      throw new IllegalArgumentException();

    IConverter c = ConverterFactory.createConverter(
      mConverterType.getDescriptor(), mConverterType.getPictureType(),
      TEST_WIDTH, TEST_HEIGHT);

    IVideoPicture picture = IVideoPicture.make(
      mConverterType.getPictureType(), TEST_WIDTH, TEST_HEIGHT);

    c.toImage(picture);
  }

  @Test(expected=IllegalArgumentException.class)
  public void testVideoPictureToImageWrongFormat()
  {
    if (!IVideoResampler.isSupported(Feature.FEATURE_COLORSPACECONVERSION))
      throw new IllegalArgumentException();

    IConverter c = ConverterFactory.createConverter(
      mConverterType.getDescriptor(), IPixelFormat.Type.YUV420P,
      TEST_WIDTH, TEST_HEIGHT);

    IVideoPicture picture = IVideoPicture.make(
      IPixelFormat.Type.GRAY16BE, TEST_WIDTH, TEST_HEIGHT);

    c.toImage(picture);
  }

  @Test(expected=IllegalArgumentException.class)
  public void testImageToVideoPictureWrongFormatInput()
  {
    if (!IVideoResampler.isSupported(Feature.FEATURE_COLORSPACECONVERSION))
      throw new IllegalArgumentException();

    IConverter c = ConverterFactory.createConverter(
      mConverterType.getDescriptor(), IPixelFormat.Type.YUV420P,
      TEST_WIDTH, TEST_HEIGHT);

    BufferedImage image = new BufferedImage(
      TEST_WIDTH, TEST_HEIGHT, BufferedImage.TYPE_INT_RGB);

    c.toPicture(image, 0);
  }

  // this test makes user of mPixelType which, and thus the solid color
  // test attempts to test across colors spaces.

  @Test
  public void testImageToImageSolidColor()
  {
    if (!IVideoResampler.isSupported(Feature.FEATURE_COLORSPACECONVERSION))
      return;
    
    int w = TEST_WIDTH;
    int h = TEST_HEIGHT;
    int gray  = Color.GRAY.getRGB();

    // create the converter

    IConverter converter = ConverterFactory.createConverter(
      mConverterType.getDescriptor(), mPixelType, w, h);

    // construct an all gray image

    BufferedImage image1 = new BufferedImage(
      w, h, mConverterType.getImageType());
    for (int x = 0; x < w; ++x)
      for (int y = 0; y < h; ++y)
        image1.setRGB(x, y, gray);

    // convert image1 to a picture and then back to image2

    BufferedImage image2 = converter.toImage(
      converter.toPicture(image1, 0));

    // test that all the pixels in image2 are gray, but not black or
    // white

    for (int x = 0; x < w; ++x)
      for (int y = 0; y < h; ++y)
      {
        int pixel1 = image1.getRGB(x, y);
        int pixel2 = image2.getRGB(x, y);

        String message = testPixels(
          mConverterType.getPictureType() == converter.getPictureType(),
          pixel1, pixel2, x, y, 
          converter.getPictureType());
        assertNull(message, message);
      }
  }


  // this test makes user of mPixelType which, and thus the solid color
  // test attempts to test across colors spaces.

  @Test
  public void testImageToImageSolidColorWithResize()
  {
    if (!IVideoResampler.isSupported(Feature.FEATURE_COLORSPACECONVERSION))
      return;

    int w1 = TEST_WIDTH;
    int h1 = TEST_HEIGHT;
    int w2 = TEST_WIDTH * 2;
    int h2 = TEST_HEIGHT * 2;
    int gray  = Color.GRAY.getRGB();

    // create the converters

    IConverter converter1 = ConverterFactory.createConverter(
      mConverterType.getDescriptor(), mPixelType, w2, h2, w1, h1);

    IConverter converter2 = ConverterFactory.createConverter(
      mConverterType.getDescriptor(), mPixelType, w2, h2, w2, h2);

    // construct an all gray image

    BufferedImage image1 = new BufferedImage(
      w1, h1, mConverterType.getImageType());
    for (int x = 0; x < w1; ++x)
      for (int y = 0; y < h1; ++y)
        image1.setRGB(x, y, gray);

    // convert image1 to a picture and then back to image2

    IVideoPicture picutre = converter1.toPicture(image1, 0);
    BufferedImage image2 = converter2.toImage(picutre);

    assertEquals("image2 wrong width", w2, image2.getWidth());
    assertEquals("image2 wrong height", h2, image2.getHeight());

    // test that all the pixels in image2 are gray, but not black or
    // white

    for (int x = 0; x < w1; ++x)
      for (int y = 0; y < h1; ++y)
      {
        int pixel1 = image1.getRGB(x, y);
        int pixel2 = image2.getRGB(x * 2, y * 2);

        String message = testPixels(
          false, pixel1, pixel2, x, y, 
          converter1.getPictureType());
        assertNull(message, message);
      }
  }

  // this test makes user of mPixelType which, and thus the solid color
  // test attempts to test across colors spaces.

  @Ignore @Test
  public void testImageToImageSolidColorWithCustomSizes()
  {
    if (!IVideoResampler.isSupported(Feature.FEATURE_COLORSPACECONVERSION))
      return;
    
    int w = TEST_WIDTH + 15;
    int h = TEST_HEIGHT;
    int gray  = Color.GRAY.getRGB();

    // create the converter

    IConverter converter = ConverterFactory.createConverter(
      mConverterType.getDescriptor(), mPixelType, w, h);

    // construct an all gray image

    BufferedImage image1 = new BufferedImage(
      w, h, mConverterType.getImageType());
    for (int x = 0; x < w; ++x)
      for (int y = 0; y < h; ++y)
        image1.setRGB(x, y, gray);

    // convert image1 to a picture and then back to image2

    BufferedImage image2 = converter.toImage(
      converter.toPicture(image1, 0));

    // test that all the pixels in image2 are gray, but not black or
    // white

    for (int x = 0; x < w; ++x)
      for (int y = 0; y < h; ++y)
      {
        int pixel1 = image1.getRGB(x, y);
        int pixel2 = image2.getRGB(x, y);

        String message = testPixels(
          mConverterType.getPictureType() == converter.getPictureType(),
          pixel1, pixel2, x, y, 
          converter.getPictureType());
        assertNull(message, message);
      }
  }

  @Test
  public void testImageToImageRandomColor()
  {
    if (!IVideoResampler.isSupported(Feature.FEATURE_COLORSPACECONVERSION))
      return;

    int w = TEST_WIDTH;
    int h = TEST_HEIGHT;
    Random rnd = new Random();

    // create the converter

    IConverter converter = ConverterFactory.createConverter(
      mConverterType.getDescriptor(), mConverterType.getPictureType(),
      w, h);

    // construct an image of random colors

    BufferedImage image1 = new BufferedImage(
      w, h, mConverterType.getImageType());
    for (int x = 0; x < w; ++x)
      for (int y = 0; y < h; ++y)
      {
        Color c = new Color(rnd.nextInt(255), 
          rnd.nextInt(255), rnd.nextInt(255));
        image1.setRGB(x, y, c.getRGB());
      }

    // convert image1 to a picture and then back to image2

    BufferedImage image2 = converter.toImage(
      converter.toPicture(image1, 0));

    // test that all the pixels in image2 are the same as image1

    for (int x = 0; x < w; ++x)
      for (int y = 0; y < h; ++y)
      {
        int pixel1 = image1.getRGB(x, y);
        int pixel2 = image2.getRGB(x, y);

        String message = testPixels(
          mConverterType.getPictureType() == converter.getPictureType(),
          pixel1, pixel2, x, y, 
          converter.getPictureType());
        assertNull(message, message);
      }
  }

  @Test
  public void testPictureToPictureWithRotate()
  {
    if (!IVideoResampler.isSupported(Feature.FEATURE_COLORSPACECONVERSION))
      return;

    // note that the image is square in this test to make rotation
    // easier to handle

    int size = TEST_WIDTH;
    int black = Color.BLACK.getRGB();
    int white = Color.WHITE.getRGB();

    // create the converter

    IConverter converter = ConverterFactory.createConverter(
      mConverterType.getDescriptor(), mConverterType.getPictureType(),
      size, size);

    // construct an image with black and white stripped columns

    BufferedImage image1 = new BufferedImage(
      size, size, mConverterType.getImageType());
    for (int x = 0; x < size; ++x)
      for (int y = 0; y < size; ++y)
      {
        int color = x % 2 == 0 ? black : white;
        image1.setRGB(x, y, color);
      }

    // convert image1 to a picture and then back to image2

    BufferedImage image2 = converter.toImage(
      converter.toPicture(image1, 0));

    // rotae image2 into image3

    AffineTransform t = AffineTransform.getRotateInstance(
      Math.PI/2, image2.getWidth() / 2, image2.getHeight() / 2);
    AffineTransformOp ato = new AffineTransformOp(t, 
      AffineTransformOp.TYPE_BICUBIC);
    BufferedImage image3 = new BufferedImage(
      size, size, mConverterType.getImageType());
    image3 = ato.filter(image2, image3);

    // convert image3 to a picture and then back to an image (4)

    BufferedImage image4 = converter.toImage(converter.toPicture(image3, 0));

    // test that image4 now contains stripped rows (not columns)

    for (int x = 0; x < size; ++x)
      for (int y = 0; y < size; ++y)
      {
        int pixel1 = y % 2 == 0 ? black : white;
        int pixel2 = image4.getRGB(x, y);

        String message = testPixels(
          mConverterType.getPictureType() == converter.getPictureType(),
          pixel1, pixel2, x, y, 
          converter.getPictureType());
        assertNull(message, message);
      }
  }

  /**
   * Test two pixels, if the pixels are different, a detailed
   * description of the condition is returned, otherwise null is
   * returned.  If the pixel type matches that of the converter, the
   * pixel confirms an exact value match, otherwise it confirms that the
   * pixels are mearly fairly similar in color value.
   */

  private String testPixels(boolean exact, int pixel1, int pixel2, int x, int y, 
    IPixelFormat.Type pixelType)
  {
    String message = "Color value missmatch whith pixel type " + 
      pixelType + ", converter " + mConverterType + 
      ", at pixel (" + x + "," + y + ").  Value is " + 
      pixel2 + " but should be " + pixel1 + ".";

    // if types match, test exact pixels values

    if (exact)
      return (pixel1 == pixel2)  ? null : message;

    // test color with margin for error

    int margin = 8;
    Color c1 = new Color(pixel1);
    Color c2 = new Color(pixel2);
    if (
      !closeEnough(c1.getRed  (), c2.getRed  (), margin) ||
      !closeEnough(c1.getGreen(), c2.getGreen(), margin) ||
      !closeEnough(c1.getBlue (), c2.getBlue (), margin))
    {
      
      System.out.println("missmatch at: (" + x + "x" + y + ")");
      System.out.println("red:   " + (c1.getRed  () ) + " vs. " + (c2.getRed  () ));
      System.out.println("green: " + (c1.getGreen() ) + " vs. " + (c2.getGreen() ));
      System.out.println("blue:  " + (c1.getBlue () ) + " vs. " + (c2.getBlue () ));

      return message;
    }
    
    // pixels are close enough

    return null;
  }

  private static boolean closeEnough(int v1, int v2, int margin)
  {
    return abs(v2 - v1) <= margin;
  }
}
