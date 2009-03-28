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
import com.xuggle.test_utils.NameAwareTestClassRunner;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IPixelFormat;
import com.xuggle.xuggler.ITimeValue;
import com.xuggle.xuggler.Utils;

import java.awt.geom.AffineTransform;
import java.awt.image.AffineTransformOp;
import java.awt.image.BufferedImage;
import java.awt.Color;
import java.util.Random;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.junit.*;
import org.junit.runner.RunWith;


import static junit.framework.Assert.*;

@RunWith(NameAwareTestClassRunner.class)
public class ConverterFactoryTest
{
  public static final int TEST_WIDTH  = 50;
  public static final int TEST_HEIGHT = 50;

  private String mTestName;
  @Before
  public void setUp()
  {
    mTestName = NameAwareTestClassRunner.getTestMethodName();
  }
  
  @Test(expected=IllegalArgumentException.class)
  public void testVideoPictureToImageNullInput()
  {
    IConverter c = ConverterFactory.createConverter(
      IPixelFormat.Type.YUV420P, BufferedImage.TYPE_INT_ARGB, 
      TEST_WIDTH, TEST_HEIGHT);
    
    c.toImage(null);
  }

  @Test(expected=IllegalArgumentException.class)
  public void testVideoPictureToImageIncompletePicture()
  {
    IConverter c = ConverterFactory.createConverter(
      IPixelFormat.Type.YUV420P, BufferedImage.TYPE_INT_ARGB, 
      TEST_WIDTH, TEST_HEIGHT);

    IVideoPicture picture = IVideoPicture.make(
      IPixelFormat.Type.ARGB, TEST_WIDTH, TEST_HEIGHT);

    c.toImage(picture);
  }

  @Test(expected=IllegalArgumentException.class)
  public void testVideoPictureToImageWrongFormat()
  {
    IConverter c = ConverterFactory.createConverter(
      IPixelFormat.Type.YUV420P, BufferedImage.TYPE_INT_ARGB, 
      TEST_WIDTH, TEST_HEIGHT);

    IVideoPicture picture = IVideoPicture.make(
      IPixelFormat.Type.GRAY16BE, TEST_WIDTH, TEST_HEIGHT);


    c.toImage(picture);
  }

  @Test(expected=IllegalArgumentException.class)
  public void testImageToVideoPictureNullInput()
  {
    IConverter c = ConverterFactory.createConverter(
      IPixelFormat.Type.YUV420P, BufferedImage.TYPE_INT_ARGB, 
      TEST_WIDTH, TEST_HEIGHT);
    
    c.toPicture(null, 0);
  }

  @Test(expected=IllegalArgumentException.class)
  public void testImageToVideoPictureWrongFormatInput()
  {
    IConverter c = ConverterFactory.createConverter(
      IPixelFormat.Type.YUV420P, BufferedImage.TYPE_INT_ARGB, 
      TEST_WIDTH, TEST_HEIGHT);

    BufferedImage image = new BufferedImage(
      TEST_WIDTH, TEST_HEIGHT, BufferedImage.TYPE_INT_RGB);

    c.toPicture(image, 0);
  }

  @Test
  public void testImageToImageSolidColor()
  {
    int w = TEST_WIDTH;
    int h = TEST_HEIGHT;
    int gray  = Color.GRAY.getRGB();

    // create the converter

    IConverter c = ConverterFactory.createConverter(
      IPixelFormat.Type.ARGB, BufferedImage.TYPE_INT_ARGB, 
      w, h);

    // construct an all gray image

    BufferedImage image1 = new BufferedImage(
      w, h, BufferedImage.TYPE_INT_ARGB);
    for (int x = 0; x < w; ++x)
      for (int y = 0; y < h; ++y)
        image1.setRGB(x, y, gray);

    // convert image1 to a picture and then back to image2

    BufferedImage image2 = c.toImage(c.toPicture(image1, 0));

    // test that all the pixels in image2 are gray, but not black or
    // white

    for (int x = 0; x < w; ++x)
      for (int y = 0; y < h; ++y)
      {
        int pixel = image2.getRGB(x, y);
        assertTrue("color value missmatch, pixel(" + x + "," + y +
          ") is " + pixel + " but should be " + gray, pixel == gray);
      }
  }

  @Test
  public void testImageToImageRandomColor()
  {
    int w = TEST_WIDTH;
    int h = TEST_HEIGHT;
    Random rnd = new Random();

    // create the converter

    IConverter converter = ConverterFactory.createConverter(
      IPixelFormat.Type.ARGB, BufferedImage.TYPE_INT_ARGB, 
      w, h);

    // construct an image of random colors

    BufferedImage image1 = new BufferedImage(
      w, h, BufferedImage.TYPE_INT_ARGB);
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
        assertTrue("color value missmatch", pixel1 == pixel2);
      }
  }

  @Test
  public void testPictureToPictureWithRotate()
  {
    // note that the image is square in this test to make rotation
    // easier to handle

    int size = TEST_WIDTH;
    int black = Color.BLACK.getRGB();
    int white = Color.WHITE.getRGB();

    // create the converter

    IConverter converter = ConverterFactory.createConverter(
      IPixelFormat.Type.ARGB, BufferedImage.TYPE_INT_ARGB, 
      size, size);

    // construct an image with black and white stripped columns

    BufferedImage image1 = new BufferedImage(
      size, size, BufferedImage.TYPE_INT_ARGB);
    for (int x = 0; x < size; ++x)
      for (int y = 0; y < size; ++y)
      {
        int color = x % 2 == 0 ? black : white;
        image1.setRGB(x, y, color);
      }

    // convert image1 to a picture and then back to image2

    BufferedImage image2 = converter.toImage(
      converter.toPicture(image1, 0));

    // rotae image2

    AffineTransform t = AffineTransform.getRotateInstance(
      Math.PI/2, image2.getWidth() / 2, image2.getHeight() / 2);
    AffineTransformOp ato = new AffineTransformOp(t, 
      AffineTransformOp.TYPE_BICUBIC);
    image2 = ato.filter(image2, null);

    // convert image2 to a picture and then back to image3

    BufferedImage image3 = converter.toImage(
      converter.toPicture(image2, 0));

    // test that image3 now contains stripped rows (not columns)

    for (int x = 0; x < size; ++x)
      for (int y = 0; y < size; ++y)
      {
        int pixel = image3.getRGB(x, y);
        int color = y % 2 == 0 ? black : white;
        assertTrue("color value missmatch", pixel == color);
      }
  }
}
