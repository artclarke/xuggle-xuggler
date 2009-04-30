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

import java.io.File;

import com.xuggle.test_utils.NameAwareTestClassRunner;
import com.xuggle.xuggler.Converter;
import com.xuggle.xuggler.IVideoResampler;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;

import static org.junit.Assert.*;

import org.junit.*;
import org.junit.runner.RunWith;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@RunWith(NameAwareTestClassRunner.class)
public class RegressionsTest
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());
  private String mTestName = null;
  private Converter converter=null;

  @Before
  public void setUp()
  {
    mTestName = NameAwareTestClassRunner.getTestMethodName();
    log.debug("-----START----- {}", mTestName);
  }
  @After
  public void tearDown()
  {
    log.debug("----- END ----- {}", mTestName);
  }

  /**
   * This test should fail, but should not crash the JVM.  It's
   * a regression test for:
   * http://code.google.com/p/xuggle/issues/detail?id=18
   * @throws ParseException
   */
  @Test
  public void testWrongPictureFormatConversionRegression18() throws ParseException
  {
    String[] args = new String[]{
        "--acodec",
        "libmp3lame",
        "fixtures/testfile.flv",
        this.getClass().getName() + "_" + mTestName + ".mov"
    };
    converter = new ConverterWrongPictureFormat();

    Options options = converter.defineOptions();

    CommandLine cmdLine = converter.parseOptions(options, args);
    assertTrue("all commandline options successful", cmdLine != null);
    try {
      converter.run(cmdLine);
      fail("should fail with an error");
    } catch (RuntimeException e)
    {
      // if we get here everything was fine.
    }
  }

  class ConverterWrongPictureFormat extends Converter
  {
    @Override
    protected IVideoPicture alterVideoFrame(IVideoPicture picture1)
    {
      // get picture dimentions

      int w = picture1.getWidth();
      int h = picture1.getHeight();

      // make a resampler

      IVideoResampler resampleToRgb32 = IVideoResampler.make(
          w, h, IPixelFormat.Type.ARGB, w, h, picture1.getPixelType());

      // resample the picture

      final IVideoPicture picture2 = IVideoPicture.make(
          resampleToRgb32.getOutputPixelFormat(), w, h);
      if (resampleToRgb32.resample(picture2, picture1) < 0)
        throw new RuntimeException("could not resample picture.");

      // return picture in rgb32

      return picture2;
    }
  }


  /**
   * This test attempts to convert a file using the FFMPEG
   * nellymoser encodec.
   *
   * It is currently disabled as the Nellymoser codec appears
   * to override areas of native memory, and hence can crash
   * Valgrind or a JVM.  It's here so once we fix this in FFMPEG
   * we can add this back to our regression suite.
   */
  @Test
  public void testConversionNellymoser() throws ParseException
  {
    // We do this to determine if this version of Xuggler can
    // support resampling
    boolean testResampling = IVideoResampler.isSupported(IVideoResampler.Feature.FEATURE_IMAGERESCALING);
    String[] args = new String[]{
        "--containerformat",
        "flv",
        "--acodec",
        "nellymoser",
        "--asamplerate",
        "22050",
        "--achannels",
        "1",
        "--abitrate",
        "64000",
        "--aquality",
        "0",
        "--vcodec",
        "flv",
        "--vscalefactor",
        !testResampling  ? "1.0" : "2.0",
            "--vbitrate",
            "300000",
            "--vquality",
            "0",
            "fixtures/testfile.flv",
            this.getClass().getName()+"_"+mTestName+".flv"
    };
    converter = new Converter();

    Options options = converter.defineOptions();

    CommandLine cmdLine = converter.parseOptions(options, args);
    assertTrue("all commandline options successful", cmdLine != null);

    converter.run(cmdLine);
  }

  /**
   * Tests for http://code.google.com/p/xuggle/issues/detail?id=51
   * 
   * sometime encoding audio can lead to non-monotone-timestamps
   */
  @Test
  public void testRegressionIssue51() throws ParseException
  {
    String[] args = new String[]{
        "--acodec",
        "libmp3lame",
        "--asamplerate",
        "22050",
        "fixtures/testfile_mpeg1video_mp2audio.mpg",
        this.getClass().getName() + "_" + mTestName + ".flv"
    };
    converter = new Converter();

    Options options = converter.defineOptions();

    CommandLine cmdLine = converter.parseOptions(options, args);
    assertTrue("all commandline options successful", cmdLine != null);

    converter.run(cmdLine);
  }

  /**
   * Tests for http://code.google.com/p/xuggle/issues/detail?id=69
   * 
   * Failure encoding as FLAC audio
   * 
   * @throws ParseException if we can't parse.
   */
  @Test
  public void testRegressionIssue69() throws ParseException
  {
    String outFilename = this.getClass().getName() + "_" + mTestName + ".ogg";
    String[] args = new String[]{
        "--vno",
        "--acodec",
        "flac",
        "--asamplerate",
        "22050",
        "fixtures/testfile_videoonly_20sec.flv",
        outFilename
    };
    converter = new Converter();

    Options options = converter.defineOptions();

    CommandLine cmdLine = converter.parseOptions(options, args);
    assertTrue("all commandline options successful", cmdLine != null);

    converter.run(cmdLine);
    
    File outFile = new File(outFilename);
    
    assertTrue("output file not large enough", outFile.length() > 160);

  }

  /**
   * Tests for http://code.google.com/p/xuggle/issues/detail?id=104
   * 
   * Failure encoding when input file has wrong time stamps
   * 
   * @throws ParseException if we can't parse.
   */
  @Test
  public void testRegressionIssue104() throws ParseException
  {
    String outFilename = this.getClass().getName() + "_" + mTestName + ".flv";
    String[] args = new String[]{
        "--vno",
        "--acodec",
        "libmp3lame",
        "--asamplerate",
        "22050",
        "fixtures/testfile_wmv3_wmav2_bad_audio_timestamps.wmv",
        outFilename
    };
    converter = new Converter();

    Options options = converter.defineOptions();

    CommandLine cmdLine = converter.parseOptions(options, args);
    assertTrue("all commandline options successful", cmdLine != null);

    converter.run(cmdLine);
    
    File outFile = new File(outFilename);
    
    assertTrue("output file not large enough", outFile.length() > 160);

  }

}
