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

package com.xuggle.mediatool;

import java.io.File;

import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.image.BufferedImage;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import org.junit.Test;
import org.junit.Before;
import org.junit.After;

import com.xuggle.mediatool.MediaReader;
import com.xuggle.mediatool.MediaViewer;
import com.xuggle.mediatool.MediaWriter;
import com.xuggle.xuggler.Global;
import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IRational;
import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IPixelFormat;
import com.xuggle.xuggler.IStreamCoder;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IVideoResampler;
import com.xuggle.xuggler.TestAudioSamplesGenerator;

import static junit.framework.Assert.*;

import static com.xuggle.mediatool.IMediaViewer.Mode.*;

public class MediaWriterTest
{
  // the log

  private final Logger log = LoggerFactory.getLogger(this.getClass());
  { log.trace("<init>"); }

  // the media view mode
  
  final MediaViewer.Mode mViewerMode = IMediaViewer.Mode.valueOf(
    System.getProperty(this.getClass().getName() + ".ViewerMode", 
      DISABLED.name()));

  // standard test name prefix

  final String PREFIX = this.getClass().getName() + "-";

  // location of test files

  public static final String TEST_FILE_DIR = "fixtures";

  // one of the stock test files
  
  public static final String TEST_FILE = TEST_FILE_DIR +
    "/testfile_bw_pattern.flv";

  // the reader for these tests

  MediaReader mReader;

  @Before
    public void beforeTest()
  {
    mReader = new MediaReader(TEST_FILE);
    mReader.setBufferedImageTypeToGenerate(BufferedImage.TYPE_3BYTE_BGR);
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
    mReader.addListener(new MediaWriter(file.toString(), mReader));
    assert(!file.exists());
  }

  @Test(expected=IllegalArgumentException.class)
    public void improperUrlTest()
  {
    File file = new File("/tmp/foo/bar/baz/should-not-be-created.flv");
    file.delete();
    assert(!file.exists());
    mReader.addListener(new MediaWriter(file.toString(), mReader));
    
    mReader.readPacket();
    assert(!file.exists());
  }

  @Test(expected=IllegalArgumentException.class)
    public void improperInputContainerTypeTest()
  {
    File file = new File(PREFIX + "should-not-be-created.flv");
    MediaWriter writer = new MediaWriter(file.toString(), mReader);
    mReader.addListener(writer);
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
    writer.addListener(new MediaViewer(mViewerMode, true));
    mReader.addListener(writer);
    while (mReader.readPacket() == null)
      ;
    assert(file.exists());
    assertEquals(1062946, file.length(), 300);

    log.debug("manually check: " + file);
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
    MediaWriter writer = new MediaWriter(file.toString(), mReader);
    writer.addListener(new MediaViewer(mViewerMode, true));
    mReader.addListener(writer);
    while (mReader.readPacket() == null)
      ;
    assert(file.exists());
    // allow 100k difference for debug builds
    assertEquals(928504, file.length(), 100000);
    log.debug("manually check: " + file);
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
    writer.addListener(new MediaViewer(mViewerMode, true));
    mReader.addListener(writer);
    while (mReader.readPacket() == null)
      ;
    assert(file.exists());
    // allow 100k difference for debug builds
    assertEquals(928504, file.length(), 100000);
    log.debug("manually check: " + file);
  }

  @Test
    public void customVideoStream()
  {
    if (!IVideoResampler.isSupported(
        IVideoResampler.Feature.FEATURE_COLORSPACECONVERSION))
      return;

    File file = new File(PREFIX + "customVideo.flv");
    file.delete();
    assert(!file.exists());

    // video parameters

    int videoStreamIndex = 0;
    int videoStreamId = 0;
    long deltaTime = 15000;
    long time = 0;
    int w = 200;
    int h = 200;

    // create the writer
    
    MediaWriter writer = new MediaWriter(file.toString());
    writer.addListener(new MediaViewer(mViewerMode, true));

    // add the video stream

    ICodec codec = ICodec.findEncodingCodec(ICodec.ID.CODEC_ID_FLV1);
    writer.addVideoStream(videoStreamIndex, videoStreamId, codec, w, h);

    // create a place for video pictures

    IVideoPicture picture = IVideoPicture.make(IPixelFormat.Type.YUV420P, w, h);

    // make some pictures

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
      writer.encodeVideo(videoStreamIndex, image,
          time, Global.DEFAULT_TIME_UNIT);
      
      time += deltaTime;
    }

    // close the writer

    writer.close();

    assert(file.exists());
    assertEquals(file.length(), 291186, 60000);
    log.debug("manually check: " + file);
  }

  @Test
    public void customAudioStream()
  {
    File file = new File(PREFIX + "customAudio.mp3");
    file.delete();
    assert(!file.exists());

    // audio parameters

    int audioStreamIndex = 0;
    int audioStreamId = 0;
    int channelCount = 2;
    int sampleRate = 44100;
    int totalSeconds = 5;

    // create the writer
    
    IMediaWriter writer = new MediaWriter(file.toString());

    // add the audio stream

    ICodec codec = ICodec.findEncodingCodec(ICodec.ID.CODEC_ID_MP3);
    IContainer container = writer.getContainer();
    IStream stream = container.getStream(
        writer.addAudioStream(audioStreamIndex, audioStreamId,
            codec, channelCount, sampleRate));
    int sampleCount = stream.getStreamCoder().getDefaultAudioFrameSize();

    // create a place for audio samples

    IAudioSamples samples = IAudioSamples.make(sampleCount, channelCount);

    // create the tone generator

    TestAudioSamplesGenerator generator = new TestAudioSamplesGenerator();
    generator.prepare(channelCount, sampleRate);

    // let's make some noise!

    int totalSamples = 0;
    while (totalSamples < sampleRate * totalSeconds)
    {
      generator.fillNextSamples(samples, sampleCount);
      writer.encodeAudio(audioStreamIndex, samples);
      totalSamples += samples.getNumSamples();
    }

    // close the writer

    writer.close();

    assert(file.exists());
    assertEquals(file.length(), 40365, 100);
    log.debug("manually check: " + file);
  }

  @Test
  public void customAudioStreamWithoutTimeStamps()
  {
    File file = new File(PREFIX + "customAudioWithoutTimeStamps.mp3");
    file.delete();
    assert (!file.exists());

    // audio parameters

    int audioStreamIndex = 0;
    int audioStreamId = 0;
    int channelCount = 2;
    int sampleRate = 44100;
    int totalSeconds = 5;

    // create the writer

    IMediaWriter writer = new MediaWriter(file.toString());

    // add the audio stream

    ICodec codec = ICodec.findEncodingCodec(ICodec.ID.CODEC_ID_MP3);
    IContainer container = writer.getContainer();
    int streamIndex = writer.addAudioStream(
        audioStreamIndex, audioStreamId, codec, channelCount, sampleRate); 
    IStream stream = container.getStream(streamIndex);
    int sampleCount = stream.getStreamCoder().getDefaultAudioFrameSize();

    // create a place for audio samples

    IAudioSamples samples = IAudioSamples.make(sampleCount, channelCount);

    // create the tone generator

    TestAudioSamplesGenerator generator = new TestAudioSamplesGenerator();
    generator.prepare(channelCount, sampleRate);

    // let's make some noise!

    int totalSamples = 0;
    short[] javaSamples = new short[sampleCount*channelCount];
    while (totalSamples < sampleRate * totalSeconds)
    {
      generator.fillNextSamples(samples, sampleCount);
      samples.get(0, javaSamples, 0, javaSamples.length);
      writer.encodeAudio(streamIndex, javaSamples);
      totalSamples += samples.getNumSamples();
    }

    // close the writer

    writer.close();

    assert (file.exists());
    assertEquals(file.length(), 40365, 100);
    log.debug("manually check: " + file);
  }


   @Test
    public void customAudioVideoStream()
  {
    if (!IVideoResampler.isSupported(
        IVideoResampler.Feature.FEATURE_COLORSPACECONVERSION))
      return;

    File file = new File(PREFIX + "customAudioVideo.flv");
    file.delete();
    assert(!file.exists());

    // video parameters

    int videoStreamIndex = 0;
    int videoStreamId = 0;
    long deltaTime = 15000;
    int w = 200;
    int h = 200;

    // audio parameters

    int audioStreamIndex = 1;
    int audioStreamId = 0;
    int channelCount = 2;
    int sampleRate = 44100;

    // create the writer
    
    MediaWriter writer = new MediaWriter(file.toString());
    writer.addListener(new MediaViewer(mViewerMode, true));

    // add the video stream

    ICodec videoCodec = ICodec.findEncodingCodec(ICodec.ID.CODEC_ID_FLV1);
    writer.addVideoStream(videoStreamIndex, videoStreamId, videoCodec, w, h);

    // add the audio stream

    ICodec audioCodec = ICodec.findEncodingCodec(ICodec.ID.CODEC_ID_MP3);
    IContainer container = writer.getContainer();
    IStream stream = container.getStream(writer.addAudioStream(audioStreamIndex, audioStreamId,
      audioCodec, channelCount, sampleRate));
    int sampleCount = stream.getStreamCoder().getDefaultAudioFrameSize();

    // create a place for audio samples and video pictures

    IAudioSamples samples = IAudioSamples.make(sampleCount, channelCount);
    IVideoPicture picture = IVideoPicture.make(IPixelFormat.Type.YUV420P, w, h);

    // create the tone generator

    TestAudioSamplesGenerator generator = new TestAudioSamplesGenerator();
    generator.prepare(channelCount, sampleRate);

    // make some media

    long videoTime = 0;
    long audioTime = 0;
    long totalSamples = 0;
    long totalSeconds = 6;

    // the goal is to get 6 seconds of audio and video, in this case
    // driven by audio, but kicking out a video frame at about the right
    // time

    while (totalSamples < sampleRate * totalSeconds)
    {
      // comput the time based on the number of samples

      audioTime = (totalSamples * 1000 * 1000) / sampleRate;

      // if the audioTime i>= videoTime then it's time for a video frame

      if (audioTime >= videoTime)
      {
        BufferedImage image = new BufferedImage(w, h, 
          BufferedImage.TYPE_3BYTE_BGR);
      
        Graphics2D g = image.createGraphics();
        g.setRenderingHint(
          RenderingHints.KEY_ANTIALIASING,
          RenderingHints.VALUE_ANTIALIAS_ON);

        double theta = videoTime / 1000000d;
        g.setColor(Color.RED);
        g.rotate(theta, w / 2, h / 2);
        
        g.fillRect(50, 50, 100, 100);
        
        picture.setPts(videoTime);
        writer.encodeVideo(videoStreamIndex, image, videoTime,
            Global.DEFAULT_TIME_UNIT);
      
        videoTime += deltaTime;
      }

      // generate audio
      
      generator.fillNextSamples(samples, sampleCount);
      writer.encodeAudio(audioStreamIndex, samples);
      totalSamples += samples.getNumSamples();
    }

    // close the writer

    writer.close();

    assert(file.exists());
    assertEquals(file.length(), 521938, 120000);
    log.debug("manually check: " + file);
  }

  @Test(expected=RuntimeException.class)
    public void lateStreamException()
  {
    File file = new File(PREFIX + "late-stream-exception.mp3");
    file.delete();
    assert(!file.exists());

    // audio parameters

    int audioStreamIndex = 0;
    int audioStreamId = 0;
    int channelCount = 2;
    int sampleRate = 44100;

    // create the writer
    
    IMediaWriter writer = new MediaWriter(file.toString());

    // add the audio stream

    ICodec codec = ICodec.findEncodingCodec(ICodec.ID.CODEC_ID_MP3);
    IContainer container = writer.getContainer();
    IStream stream = container.getStream(
        writer.addAudioStream(audioStreamIndex, audioStreamId,
            codec, channelCount, sampleRate));
    int sampleCount = stream.getStreamCoder().getDefaultAudioFrameSize();

    // create a place for audio samples

    IAudioSamples samples = IAudioSamples.make(sampleCount, channelCount);

    // create the tone generator

    TestAudioSamplesGenerator generator = new TestAudioSamplesGenerator();
    generator.prepare(channelCount, sampleRate);

    // write some data, so that the media header will be written

    generator.fillNextSamples(samples, sampleCount);
    writer.encodeAudio(audioStreamIndex, samples);

    // re-delete the output file so no broke media files persist after
    // the test
    
    file.delete();

    // now write some data on a different index

    generator.fillNextSamples(samples, sampleCount);
    writer.encodeAudio(audioStreamIndex+1, samples);

    // should no get here

    assert(false);
  }

  @Test
    public void lateStreamExceptionMask()
  {
    File file = new File(PREFIX + "late-stream-exception.mp3");
    file.delete();
    assert(!file.exists());

    // audio parameters

    int audioStreamIndex = 0;
    int audioStreamId = 0;
    int channelCount = 2;
    int sampleRate = 44100;

    // create the writer
    
    IMediaWriter writer = new MediaWriter(file.toString());
    
    // mask late stream exceptoins

    writer.setMaskLateStreamExceptions(true);

    // add the audio stream

    ICodec codec = ICodec.findEncodingCodec(ICodec.ID.CODEC_ID_MP3);
    IContainer container = writer.getContainer();
    IStream stream = container.getStream(
        writer.addAudioStream(audioStreamIndex, audioStreamId,
            codec, channelCount, sampleRate));
    int sampleCount = stream.getStreamCoder().getDefaultAudioFrameSize();

    // create a place for audio samples

    IAudioSamples samples = IAudioSamples.make(sampleCount, channelCount);

    // create the tone generator

    TestAudioSamplesGenerator generator = new TestAudioSamplesGenerator();
    generator.prepare(channelCount, sampleRate);

    // write some data, so that the media header will be written

    generator.fillNextSamples(samples, sampleCount);
    writer.encodeAudio(audioStreamIndex, samples);

    // now write some data on a different index

    generator.fillNextSamples(samples, sampleCount);
    writer.encodeAudio(audioStreamIndex+1, samples);

    // delete the output file so no broke media files persist after the
    // test
    
    file.delete();
  }
  
  @Test
  public void testTimebaseGuessingWhenCodecSpecifiedAllowed()
  {
    IMediaWriter writer = new MediaWriter(PREFIX+
        "testTimebaseGuessingWhenCodecSpecifiedAllowed1.mpg");
    try {
      writer.addVideoStream(0,
          0,
          IRational.make(27,1),
          100, 100);
      fail("shouldn't get here");
    } catch (UnsupportedOperationException e) {}
    writer = new MediaWriter(PREFIX+
      "testTimebaseGuessingWhenCodecSpecifiedAllowed2.mpg");
    writer.addVideoStream(0,
        0,
        100, 100);
    IStreamCoder coder = writer.getContainer().getStream(0).getStreamCoder();
    // should have set the highest possible
    assertEquals(1, coder.getTimeBase().getNumerator());
    assertEquals(60, coder.getTimeBase().getDenominator());
  }
}
