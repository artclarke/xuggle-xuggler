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

import junit.framework.TestCase;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;

public class ConverterExhaustiveTest extends TestCase
{
  private Converter converter=null;

  /**
   * This test makes sure that real-time writing of data works; this
   * MUST run at least longer than the file in question, so use a short
   * file for testing.
   * @throws ParseException 
   */
  public void testRealTimeConversion() throws ParseException
  {
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
        "--realtime",
        "fixtures/testfile_videoonly_20sec.flv",
        this.getClass().getName() + "_" + this.getName() + ".mov"
    };
    
    converter = new Converter();
    
    Options options = converter.defineOptions();
  
    CommandLine cmdLine = converter.parseOptions(options, args);
    assertTrue("all commandline options successful", cmdLine != null);
    
    final long startTime = System.nanoTime();
    converter.run(cmdLine);
    final long endTime = System.nanoTime();
    final long delta = endTime - startTime;
    // 20 second test file;
    assertTrue("did not take long enough", delta >= 18L*1000*1000*1000);
    assertTrue("took too long", delta <= 60L*1000*1000*1000);
  }

  public void testConversionOggVorbisTheora() throws ParseException
  {
    String[] args = new String[]{
        "--vcodec",
        "libtheora",
        "--acodec",
        "libvorbis",
        "fixtures/testfile.flv",
        this.getClass().getName() + "_" + this.getName() + ".ogg"
    };
    
    converter = new Converter();
    
    Options options = converter.defineOptions();

    CommandLine cmdLine = converter.parseOptions(options, args);
    assertTrue("all commandline options successful", cmdLine != null);
    
    converter.run(cmdLine);
    
  }

  /**
   * See: http://code.google.com/p/xuggle/issues/detail?id=165
   * @throws ParseException if it feels like it
   */
  public void testIssue165() throws ParseException
  {
    String[] args = new String[]{
        "fixtures/youtube_h264_mp3.flv",
        this.getClass().getName() + "_" + this.getName() + ".mov"
    };
    
    converter = new Converter();
    
    Options options = converter.defineOptions();

    CommandLine cmdLine = converter.parseOptions(options, args);
    assertTrue("all commandline options successful", cmdLine != null);
    
    converter.run(cmdLine);
  }

  public void testConversionH264() throws ParseException
  {
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
        "libx264",
        "--vscalefactor",
        "2.0",
        "--vpreset",
        "fixtures/"+this.getClass().getName()+".vpresets.txt",
        "--vbitrate",
        "300000",
        "--vbitratetolerance",
        "12000000",
        "--vquality",
        "0",
        "fixtures/testfile_videoonly_20sec.flv",
        this.getClass().getName() + "_" + this.getName() + ".mp4"
    };
    
    // check if we have the libx264 encoder
    ICodec codec = ICodec.findEncodingCodecByName("libx264");
    if (codec == null)
      // we don't have libx264 so bail
      return;
    
    converter = new Converter();
    
    Options options = converter.defineOptions();

    CommandLine cmdLine = converter.parseOptions(options, args);
    assertTrue("all commandline options successful", cmdLine != null);
    
    converter.run(cmdLine);
  }

  public void testConversionAac() throws ParseException
  {
    // We do this to determine if this version of Xuggler can
    // support resampling
    boolean testResampling = IVideoResampler.isSupported(IVideoResampler.Feature.FEATURE_IMAGERESCALING);
    // and if not, we skip this test because resampling is only supported in GPL
    // build, and now libfaac is only supported in GPL as well.
    if (!testResampling) return;
    
    String[] args = new String[]{
        "--containerformat",
        "mov",
        "--acodec",
        "libvo_aacenc",
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
        testResampling ? "2.0" : "1.0",
        "--vbitrate",
        "300000",
        "--vbitratetolerance",
        "12000000",
        "--vquality",
        "0",
        "fixtures/testfile_videoonly_20sec.flv",
        this.getClass().getName() + "_" + this.getName() + ".mov"
    };
    
    converter = new Converter();
    
    Options options = converter.defineOptions();

    CommandLine cmdLine = converter.parseOptions(options, args);
    assertTrue("all commandline options successful", cmdLine != null);
    
    converter.run(cmdLine);
  }

}
