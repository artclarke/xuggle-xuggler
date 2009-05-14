/*
 * Copyright (c) 2008-2009 by Xuggle Inc. All rights reserved.
 *
 * It is REQUESTED BUT NOT REQUIRED if you use this library, that you
 * let us know by sending e-mail to info@xuggle.com telling us briefly
 * how you're using the library and what you like or don't like about
 * it.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

package com.xuggle.xuggler.mediatool;

import java.io.File;

import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.image.BufferedImage;
import java.awt.geom.AffineTransform;

import java.util.concurrent.TimeUnit;

import org.junit.Test;
import org.junit.Before;
import org.junit.After;

import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IPixelFormat;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IVideoResampler;
import com.xuggle.xuggler.mediatool.MediaReader;
import com.xuggle.xuggler.mediatool.MediaWriter;
import com.xuggle.xuggler.video.ConverterFactory;

import static junit.framework.Assert.*;

public class MediaWriterTest
{
  // show the videos during transcoding?

  public static final boolean SHOW_VIDEO = false;

  // one of the stock test fiels
  
  public static final String TEST_FILE = "fixtures/testfile_bw_pattern.flv";
  public static final String PREFIX = MediaWriterTest.class.getName() + "-";

  // the reader for these tests

  MediaReader mReader;

  @Before
    public void beforeTest()
  {
    mReader = new MediaReader(TEST_FILE, true, ConverterFactory.XUGGLER_BGR_24);
  }

  @After
    public void afterTest()
  {
    if (mReader.isOpen())
      mReader.close();
  }

  @Test(expected=IllegalArgumentException.class)
    public void improperReaderConfigTest()
  {
    File file = new File(PREFIX + "should-not-be-created.flv");
    file.delete();
    assert(!file.exists());
    mReader.setAddDynamicStreams(true);
    new MediaWriter(file.toString(), mReader);
    assert(!file.exists());
  }

  @Test(expected=IllegalArgumentException.class)
    public void improperUrlTest()
  {
    File file = new File("/tmp/foo/bar/baz/should-not-be-created.flv");
    file.delete();
    assert(!file.exists());
    new MediaWriter(file.toString(), mReader);
    mReader.readPacket();
    assert(!file.exists());
  }

  @Test(expected=IllegalArgumentException.class)
    public void improperInputContainerTypeTest()
  {
    File file = new File(PREFIX + "should-not-be-created.flv");
    MediaWriter writer = new MediaWriter(file.toString(), mReader);
    mReader.readPacket();
    file.delete();
    new MediaWriter(file.toString(), writer.getContainer());
  }

  @Test
    public void transcodeToFlvTest()
  {
    if (!IVideoResampler.isSupported(
        IVideoResampler.Feature.FEATURE_COLORSPACECONVERSION))
      return;
    File file = new File(PREFIX + "transcode-to-flv.flv");
    file.delete();
    assert(!file.exists());
    MediaWriter writer = new MediaWriter(file.toString(), mReader);
    
    while (mReader.readPacket() == null)
      ;
    assert(file.exists());
    assertEquals(file.length(), 1062946);
    System.out.println("manually check: " + file);
  }

  @Test
    public void transcodeToMovTest()
  {
    if (!IVideoResampler.isSupported(
        IVideoResampler.Feature.FEATURE_COLORSPACECONVERSION))
      return;
    File file = new File(PREFIX + "transcode-to-mov.mov");
    file.delete();
    assert(!file.exists());
    new MediaWriter(file.toString(), mReader);
    while (mReader.readPacket() == null)
      ;
    assert(file.exists());
    assertEquals(file.length(), 1061745);
    System.out.println("manually check: " + file);
  }
 
  @Test
    public void transcodeWithContainer()
  {
    if (!IVideoResampler.isSupported(
        IVideoResampler.Feature.FEATURE_COLORSPACECONVERSION))
      return;
    File file = new File(PREFIX + "transcode-container.mov");
    file.delete();
    assert(!file.exists());
    MediaWriter writer = new MediaWriter(file.toString(), 
      mReader.getContainer());
    mReader.addListener(writer);
    while (mReader.readPacket() == null)
      ;
    assert(file.exists());
    assertEquals(file.length(), 1061745);
    System.out.println("manually check: " + file);
  }


  @Test
    public void customVideoStream()
  {
    File file = new File(PREFIX + "customVideo.flv");
    file.delete();
    assert(!file.exists());

    int w = 200;
    int h = 200;
    long deltaTime = 15000;
    long time = 0;
    IVideoPicture picture = IVideoPicture.make(IPixelFormat.Type.YUV420P, w, h);
    
    MediaWriter writer = new MediaWriter(file.toString());
    writer.addListener(new DebugListener(DebugListener.META_DATA));
    writer.addListener(new MediaViewer());
    ICodec codec = ICodec.findEncodingCodec(ICodec.ID.CODEC_ID_FLV1);
    writer.addVideoStream(0, 0, codec, w, h);

    double deltaTheta = (Math.PI * 2) / 200;
    for (double theta = 0; theta < Math.PI * 2; theta += deltaTheta)
    {
      BufferedImage image = new BufferedImage(w, h, 
        BufferedImage.TYPE_3BYTE_BGR);
      
      Graphics2D g = image.createGraphics();
      g.setRenderingHint(
        RenderingHints.KEY_ANTIALIASING,
        RenderingHints.VALUE_ANTIALIAS_ON);
      
      g.setColor(Color.RED);
      g.rotate(theta, w / 2, h / 2);
      
      g.fillRect(50, 50, 100, 100);

      picture.setPts(time);
      writer.onVideoPicture(writer, picture, image, 0);
      
      time += deltaTime;
    }

    writer.close();

    assert(file.exists());
    assertEquals(291186, file.length());
    System.out.println("manually check: " + file);
  }
}
