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
public class RegressionsExhaustiveTest
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
  
  @Test
  public void testRegressionIssue119()
  {
    IVideoPicture picture = Utils.getBlankFrame(10, 10, 0);
    assertNotNull(picture);
    IStreamCoder coder = IStreamCoder.make(IStreamCoder.Direction.ENCODING,
        ICodec.findEncodingCodec(ICodec.ID.CODEC_ID_FLV1));
    assertNotNull(coder);
    coder.setTimeBase(IRational.make(1,1000000));
    coder.setPixelType(IPixelFormat.Type.YUV420P);
    coder.setWidth(20);
    coder.setHeight(20);
    assertTrue("could not open coder", coder.open(null, null)>= 0);
    
    IPacket packet = IPacket.make();
    assertNotNull(packet);
    // now this should fail!
    assertTrue(coder.encodeVideo(packet, picture, 0)<0);
  }

  /**
   * Tests for http://code.google.com/p/xuggle/issues/detail?id=203
   * 
   * Failure with setting input rates
   * 
   * @throws ParseException if we can't parse.
   */
  @Test
  public void testRegressionIssue203() throws ParseException
  {
    String outFilename = this.getClass().getName() + "_" + mTestName + ".wav";
    String[] args = new String[]{
        "--icontainerformat",
        "s16le",
        "--iasamplerate",
        "16000",
        "--iachannels",
        "1",
        "fixtures/test.raw",
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

  @Test
  public void testRegressionIssue203_2()
  {
    IContainerFormat fmt = IContainerFormat.make();
    fmt.setInputFormat("s16le");
    
    IContainer container = IContainer.make();
    int retval;
    retval = container.open("fixtures/test.raw",
        IContainer.Type.READ,
        fmt,
        false,
        false);
    assertTrue(retval >= 0);
    int numStreams = container.getNumStreams();
    assertEquals(1, numStreams);
    for(int i = 0; i < numStreams; i++)
    {
      IStream stream = container.getStream(i);
      IStreamCoder coder = stream.getStreamCoder();
      coder.setSampleRate(16000);
    }
    IPacket pkt = IPacket.make();
    
    // read one packet; this will core dump before the fix.
    retval = container.readNextPacket(pkt);
    // should hopefully get error instead of core dump.
    System.out.println("Duration: " + pkt.getDuration());
    assertTrue(retval >= 0);
        
  }

  @Test
  public void testRegressionIssue219()
  {
    for (ICodec c: ICodec.getInstalledCodecs())
      c.getID();
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


}
