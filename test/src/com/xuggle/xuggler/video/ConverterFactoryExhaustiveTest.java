/*******************************************************************************
 * Copyright (c) 2008, 2010 Xuggle Inc.  All rights reserved.
 *  
 * This file is part of Xuggle-Xuggler-Main.
 *
 * Xuggle-Xuggler-Main is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xuggle-Xuggler-Main is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Xuggle-Xuggler-Main.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/
package com.xuggle.xuggler.video;

import static java.lang.Math.abs;
import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertNull;

import java.awt.Color;
import java.awt.geom.AffineTransform;
import java.awt.image.AffineTransformOp;
import java.awt.image.BufferedImage;
import java.util.Collection;
import java.util.Vector;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.ferry.JNIMemoryManager;
import com.xuggle.ferry.JNIMemoryManager.MemoryModel;
import com.xuggle.xuggler.IPixelFormat;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IVideoResampler;
import com.xuggle.xuggler.IVideoResampler.Feature;

/**
 * Tests all converters under all memory models
 * 
 * @author aclarke
 * 
 */
@RunWith(Parameterized.class)
public class ConverterFactoryExhaustiveTest
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());

  private static final int TEST_WIDTH  = 100;
  private static final int TEST_HEIGHT = 100;
  private static final int TEST_ITERATIONS = 25;
  private final ConverterFactory.Type mConverterType;
  private final IPixelFormat.Type mPixelType;

  private MemoryModel mModel;

  private static final IPixelFormat.Type[] mIncludedPixelTypes =
  {
      IPixelFormat.Type.ARGB, 
      IPixelFormat.Type.BGR24,
      IPixelFormat.Type.YUV420P,
  };

  @Parameters
  public static Collection<Object[]> converterTypes()
  {
    Collection<Object[]> parameters = new Vector<Object[]>();

    final boolean TEST_ALL=true;
    if (TEST_ALL)
    {
    for (JNIMemoryManager.MemoryModel model :
      JNIMemoryManager.MemoryModel.values())
      for (IPixelFormat.Type pixelType : mIncludedPixelTypes)
        for (ConverterFactory.Type converterType : ConverterFactory
            .getRegisteredConverters())
      {
        Object[] tuple =
        {
            model, converterType, pixelType
        };
        parameters.add(tuple);
      }
    }
    else
    {
      Object[] tuple =
      {
          JNIMemoryManager.MemoryModel.NATIVE_BUFFERS,
          ConverterFactory.findRegisteredConverter(ConverterFactory.XUGGLER_BGR_24),
          IPixelFormat.Type.YUV420P
      };
      parameters.add(tuple);
      
    }
    return parameters;
  }

  public ConverterFactoryExhaustiveTest(
      JNIMemoryManager.MemoryModel model,
      ConverterFactory.Type converterType,
      IPixelFormat.Type pixelType)
  {
    mConverterType = converterType;
    mPixelType = pixelType;
    mModel = model;
    JNIMemoryManager.setMemoryModel(model);
  }

  @Test
  public void testImageToImageNoLeaks()
  {
    JNIMemoryManager.getMgr().flush();
    log.debug("testImageToImageNoLeaks; model: {}; converter type: {}; pixel type: {};",
        new Object[]{
        mModel,
        mConverterType,
        mPixelType
    });

    long start = System.nanoTime();
    for(int i =0; i < TEST_ITERATIONS; i++)
    {
      assertEquals("not all memory released", 0,
          JNIMemoryManager.getMgr().getNumPinnedObjects());
      helperImageToImageSolidColorWithResize();
      assertEquals("not all memory released", 0,
          JNIMemoryManager.getMgr().getNumPinnedObjects());
    }
    long end = System.nanoTime();
    long durationMillis = (end-start)/(1000*1000);
    log.debug("testImageToImageNoLeaks; duration: {}",
        durationMillis);
  }
  
  @Test
  public void testPictureToPictureNoLeaks()
  {
    JNIMemoryManager.getMgr().flush();
    log.debug("testPictureToPictureNoLeaks; model: {}; converter type: {}; pixel type: {};",
        new Object[]{
        mModel,
        mConverterType,
        mPixelType
    });

    long start = System.nanoTime();
    for(int i =0; i < TEST_ITERATIONS; i++)
    {
      assertEquals("not all memory released", 0,
          JNIMemoryManager.getMgr().getNumPinnedObjects());
      helperPictureToPictureWithRotate();
      assertEquals("not all memory released", 0,
          JNIMemoryManager.getMgr().getNumPinnedObjects());
    }
    long end = System.nanoTime();
    long durationMillis = (end-start)/(1000*1000);
    log.debug("testPictureToPictureNoLeaks; duration: {}",
        durationMillis);
  }
  
  public void helperPictureToPictureWithRotate()
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

    IVideoPicture picture = null;
 
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
  
    picture = converter.toPicture(image1, 0);
    BufferedImage image2 = converter.toImage(
      picture);
    picture.delete();
  
    // rotate image2 into image3
  
    AffineTransform t = AffineTransform.getRotateInstance(
      Math.PI/2, image2.getWidth() / 2, image2.getHeight() / 2);
    AffineTransformOp ato = new AffineTransformOp(t, 
      AffineTransformOp.TYPE_BICUBIC);
    BufferedImage image3 = new BufferedImage(
      size, size, mConverterType.getImageType());
    image3 = ato.filter(image2, image3);
  
    // convert image3 to a picture and then back to an image (4)
  
    picture = converter.toPicture(image3, 0);
    BufferedImage image4 = converter.toImage(picture);
    picture.delete();
  
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
    converter.delete();
  }

  public void helperImageToImageSolidColorWithResize()
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

    IVideoPicture picture = converter1.toPicture(image1, 0);
    BufferedImage image2 = converter2.toImage(picture);
    picture.delete();

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
    
    converter1.delete();
    converter2.delete();
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
