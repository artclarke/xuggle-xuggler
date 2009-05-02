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
package com.xuggle.xuggler;

import com.xuggle.ferry.IBuffer;
import com.xuggle.test_utils.NameAwareTestClassRunner;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IPixelFormat;
import com.xuggle.xuggler.ITimeValue;
import com.xuggle.xuggler.Utils;
import com.xuggle.xuggler.IVideoResampler.Feature;

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
public class UtilsTest
{
  private String mTestName;
  @Before
  public void setUp()
  {
    mTestName = NameAwareTestClassRunner.getTestMethodName();
  }
  
  @Test
  public void testGetBlankFrame()
  {
    IVideoPicture frame = null;
    byte y = 62;
    byte u = 15;
    byte v = 33;
    int w = 100;
    int h = 200;
    long pts = 102832;
    
    frame = Utils.getBlankFrame(w, h, y, u, v, pts);
    assertTrue("got a frame", frame != null);
    assertTrue("is complete", frame.isComplete());
    assertTrue("correct pts", frame.getPts() == pts);

    // now, loop through the array and ensure it is set
    // correctly.
    IBuffer data = frame.getData();
    java.nio.ByteBuffer buffer = data.getByteBuffer(0, data.getBufferSize());
    assertNotNull(buffer);
    // we have the raw data; now we set it to the specified YUV value.
    int lineLength = 0;
    int offset = 0;

    // first let's check the L
    offset = 0;
    lineLength = frame.getDataLineSize(0);
    assertTrue("incorrect format", lineLength == w);
    for(int i = offset ; i < offset + lineLength * h; i++)
    {
      byte val = buffer.get(i);
      assertTrue("color not set correctly: " + i, val == y);
    }

    // now, check the U value
    offset = (frame.getDataLineSize(0)*h);
    lineLength = frame.getDataLineSize(1);
    assertTrue("incorrect format", lineLength == w / 2);
    for(int i = offset ; i < offset + lineLength * h / 2; i++)
    {
      byte val = buffer.get(i);
      assertTrue("color not set correctly: " + i, val == u);
    }

    // and finally the V
    offset = (frame.getDataLineSize(0)*h) + (frame.getDataLineSize(1)*h/2);
    lineLength = frame.getDataLineSize(2);
    assertTrue("incorrect format", lineLength == w / 2);
    for(int i = offset; i < offset + lineLength * h / 2; i++)
    {
      byte val = buffer.get(i);
      assertTrue("color not set correctly: " + i, val == v);
    }
    
    // And check another way
    for(int i = 0; i < w; i++)
    {
      for(int j = 0; j < h; j++)
      {
        assertEquals(y, IPixelFormat.getYUV420PPixel(frame, i, j, IPixelFormat.YUVColorComponent.YUV_Y));
        assertEquals(u, IPixelFormat.getYUV420PPixel(frame, i, j, IPixelFormat.YUVColorComponent.YUV_U));
        assertEquals(v, IPixelFormat.getYUV420PPixel(frame, i, j, IPixelFormat.YUVColorComponent.YUV_V));
      }
    }
  }
  
  @Test
  public void testSamplesToTimeValue()
  {
    ITimeValue result = Utils.samplesToTimeValue(22050, 22050);
    assertEquals(1000, result.get(ITimeValue.Unit.MILLISECONDS));
  }
  
  @Test(expected=IllegalArgumentException.class)
  public void testSamplesToTimeValueNoSampleRate()
  {
    Utils.samplesToTimeValue(1024, 0); 
  }
  
  @Test
  public void testTimeValueToSamples()
  {
    ITimeValue input = ITimeValue.make(16, ITimeValue.Unit.SECONDS);
    long result = Utils.timeValueToSamples(input, 44100);
    assertEquals(44100*16, result);
  }
  
  @Test(expected=IllegalArgumentException.class)
  public void testTimeValueToSamplesNoSampleRate()
  {
    Utils.timeValueToSamples(ITimeValue.make(1, ITimeValue.Unit.SECONDS), 0);
  }
  
  @Test(expected=IllegalArgumentException.class)
  public void testTimeValueToSamplesNoTimeValue()
  {
    Utils.timeValueToSamples(null, 22050);
  }

  @SuppressWarnings("deprecation")
  @Test(expected=IllegalArgumentException.class)
  public void testVideoPictureToImageNullInput()
  {
    Utils.videoPictureToImage(null);
  }

  @SuppressWarnings("deprecation")
  @Test
  public void testVideoPictureToImageWrongFormatInput()
  {
    if (!IVideoResampler.isSupported(Feature.FEATURE_COLORSPACECONVERSION))
      return;
    IVideoPicture picture = Utils.getBlankFrame(50, 50, 0);
    assertEquals(IPixelFormat.Type.YUV420P, picture.getPixelType());
    Utils.videoPictureToImage(picture);
  }
  
  @SuppressWarnings("deprecation")
  @Test(expected=UnsupportedOperationException.class)
  public void testLGPLBuild()
  {
    if (IVideoResampler.isSupported(Feature.FEATURE_COLORSPACECONVERSION))
      throw new UnsupportedOperationException();
    IVideoPicture picture = Utils.getBlankFrame(50, 50, 0);
    assertEquals(IPixelFormat.Type.YUV420P, picture.getPixelType());
    Utils.videoPictureToImage(picture);
  }

  @SuppressWarnings("deprecation")
  @Test(expected=IllegalArgumentException.class)
  public void testImageToVideoPictureNullInput()
  {
    Utils.imageToVideoPicture(null, 0);
  }

  @SuppressWarnings("deprecation")
  @Test(expected=IllegalArgumentException.class)
  public void testImageToVideoPictureWrongFormatInput()
  {
    BufferedImage image = new BufferedImage(
      50, 50, BufferedImage.TYPE_INT_RGB);
    Utils.imageToVideoPicture(image, 0);
  }

  @SuppressWarnings("deprecation")
  @Test
  public void testImageToImageSolidColor()
  {
    int w = 50;
    int h = 50;
    int gray  = Color.GRAY .getRGB();

    // construct an all gray image

    BufferedImage image1 = new BufferedImage(
      w, h, BufferedImage.TYPE_3BYTE_BGR);
    for (int x = 0; x < w; ++x)
      for (int y = 0; y < h; ++y)
        image1.setRGB(x, y, gray);

    // convert image1 to a picture and then back to image2

    BufferedImage image2 = Utils.videoPictureToImage(
      Utils.imageToVideoPicture(image1, 0));

    // test that all the pixels in image2 are gray, but not black or
    // white

    for (int x = 0; x < w; ++x)
      for (int y = 0; y < h; ++y)
      {
        int pixel = image2.getRGB(x, y);
        assertTrue("color value missmatch", pixel == gray);
      }
  }

  @SuppressWarnings("deprecation")
  @Test
  public void testImageToImageRandomColor()
  {
    int w = 50;
    int h = 50;
    Random rnd = new Random();

    // construct an image of random colors

    BufferedImage image1 = new BufferedImage(
      w, h, BufferedImage.TYPE_3BYTE_BGR);
    for (int x = 0; x < w; ++x)
      for (int y = 0; y < h; ++y)
      {
        Color c = new Color(rnd.nextInt(255), 
          rnd.nextInt(255), rnd.nextInt(255));
        image1.setRGB(x, y, c.getRGB());
      }

    // convert image1 to a picture and then back to image2

    BufferedImage image2 = Utils.videoPictureToImage(
      Utils.imageToVideoPicture(image1, 0));

    // test that all the pixels in image2 are the same as image1

    for (int x = 0; x < w; ++x)
      for (int y = 0; y < h; ++y)
      {
        int pixel1 = image1.getRGB(x, y);
        int pixel2 = image2.getRGB(x, y);
        assertTrue("color value missmatch", pixel1 == pixel2);
      }
  }

  @SuppressWarnings("deprecation")
  @Test
  public void testPictureToPictureWithRotate()
  {
    // note that the image is square in this test to make rotation
    // easier to handle

    int size = 50;
    int black = Color.BLACK.getRGB();
    int white = Color.WHITE.getRGB();

    // construct an image with black and white stripped columns

    BufferedImage image1 = new BufferedImage(
      size, size, BufferedImage.TYPE_3BYTE_BGR);
    for (int x = 0; x < size; ++x)
      for (int y = 0; y < size; ++y)
      {
        int color = x % 2 == 0 ? black : white;
        image1.setRGB(x, y, color);
      }

    // convert image1 to a picture and then back to image2

    BufferedImage image2 = Utils.videoPictureToImage(
      Utils.imageToVideoPicture(image1, 0));

    // rotae image2

    AffineTransform t = AffineTransform.getRotateInstance(
      Math.PI/2, image2.getWidth() / 2, image2.getHeight() / 2);
    AffineTransformOp ato = new AffineTransformOp(t, 
      AffineTransformOp.TYPE_BICUBIC);
    BufferedImage image3 = new BufferedImage(
        size, size, BufferedImage.TYPE_3BYTE_BGR);
    ato.filter(image2, image3);

    // convert image2 to a picture and then back to image3

    BufferedImage image4 = Utils.videoPictureToImage(
      Utils.imageToVideoPicture(image3, 0));

    // test that image4 now contains stripped rows (not columns)

    for (int x = 0; x < size; ++x)
      for (int y = 0; y < size; ++y)
      {
        int pixel = image4.getRGB(x, y);
        int color = y % 2 == 0 ? black : white;
        assertTrue("color value missmatch", pixel == color);
      }
  }

  /**
   * Reads in an input file, and converts it to a new format, but replacing
   * all images with a cross-hatch test pattern.
   * 
   * @throws ParseException
   */
  @Test
  public void testUtilsCrossHatch() throws ParseException
  {
    Converter converter = new Converter() {
      @Override
      protected IVideoPicture alterVideoFrame(IVideoPicture videoFrame)
      {
        // override the image passed in, and instead output a cross hatch.
        // but alternate colors every second
        final int CROSSHATCH_WIDTH = 20;
        final int CROSSHATCH_HEIGHT = 20;
        if ((videoFrame.getPts()/(Global.DEFAULT_PTS_PER_SECOND))%2==1)
          
          return Utils.getBlankFrame(
              videoFrame.getWidth(),
              videoFrame.getHeight(),
              0, 0, 0,
              videoFrame.getTimeStamp(),
              CROSSHATCH_WIDTH, CROSSHATCH_HEIGHT,
              255, 255, 255);
        else
          return Utils.getBlankFrame(
              videoFrame.getWidth(),
              videoFrame.getHeight(),
              255, 255, 255,
              videoFrame.getTimeStamp(),
              CROSSHATCH_WIDTH, CROSSHATCH_HEIGHT,
              0, 0, 0);
      }
    };

    String[] args = new String[]{
        "--containerformat",
        "mov",
        "--acodec",
        "libmp3lame",
        "--asamplerate",
        "22050",
        "--achannels",
        "2",
        "--abitrate",
        "64000",
        "--aquality",
        "0",
        "--vcodec",
        "mpeg4",
        "--vscalefactor",
        "1.0",
        "--vbitrate",
        "300000",
        "--vbitratetolerance",
        "12000000",
        "--vquality",
        "0",
        "fixtures/testfile_videoonly_20sec.flv",
        this.getClass().getName() + "_" + mTestName + ".mov"
    };

    Options options = converter.defineOptions();

    CommandLine cmdLine = converter.parseOptions(options, args);
    assertTrue("all commandline options successful", cmdLine != null);

    converter.run(cmdLine);
  }
}
