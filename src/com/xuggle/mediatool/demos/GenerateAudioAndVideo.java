/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated.  All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any
 * later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.xuggle.mediatool.demos;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.xuggler.ICodec;

import java.awt.image.BufferedImage;

import com.xuggle.mediatool.IMediaViewer;
import com.xuggle.mediatool.IMediaWriter;
import com.xuggle.mediatool.ToolFactory;
import com.xuggle.xuggler.IAudioSamples;

import static java.util.concurrent.TimeUnit.SECONDS;
import static java.util.concurrent.TimeUnit.MILLISECONDS;
import static com.xuggle.xuggler.Global.DEFAULT_TIME_UNIT;

/**
 * Generate andio and video frames and use the {@link IMediaWriter} to
 * encode that media and write it out to a file.
 *
 * @author trebor
 * @author aclarke
 */

public class GenerateAudioAndVideo
{
  // the log

  private static final Logger log = LoggerFactory.getLogger(
    GenerateAudioAndVideo.class);
  { log.trace("<init>"); }

  /**
   * Create and display a number of bouncing balls on the 
   */

  public static void main(String[] args)
  {
    // the number of balls to bounce around

    final int ballCount = 3;

    // total duration of the media

    final long duration = DEFAULT_TIME_UNIT.convert(5, SECONDS);

    // video parameters

    final int videoStreamIndex = 0;
    final int videoStreamId = 0;
    final long frameRate = DEFAULT_TIME_UNIT.convert(15, MILLISECONDS);
    final int with = 320;
    final int height = 200;
    
    // audio parameters

    final int audioStreamIndex = 1;
    final int audioStreamId = 0;
    final int channelCount = 1;
    final int sampleRate = 44100; // Hz
    final int sampleCount = 1000;

    // create a media writer and specify the output file

    final IMediaWriter writer = ToolFactory.makeWriter("output.flv");

    // add a viewer so we can see the media as it is created

    writer.addListener(ToolFactory.makeViewer(
        IMediaViewer.Mode.AUDIO_VIDEO, true, 
        javax.swing.WindowConstants.EXIT_ON_CLOSE));

    // add the video stream

    ICodec videoCodec = ICodec.findEncodingCodec(ICodec.ID.CODEC_ID_FLV1);
    writer.addVideoStream(videoStreamIndex, videoStreamId, videoCodec, 
      with, height);

    // add the audio stream

    ICodec audioCodec = ICodec.findEncodingCodec(ICodec.ID.CODEC_ID_MP3);
    writer.addAudioStream(audioStreamIndex, audioStreamId, audioCodec, 
      channelCount, sampleRate);

    // create some balls to show on the screen

    Balls balls = new Balls(ballCount, with, height, sampleCount);

    // the clock time of the next frame

    long nextFrameTime = 0;

    // the total number of audio samples

    long totalSampleCount = 0;

    // loop through clock time, which starts at zero and increases based
    // on the total number of samples created thus far

    for (long clock = 0; clock < duration; clock = IAudioSamples
           .samplesToDefaultPts(totalSampleCount, sampleRate))
    {
      // while the clock time exceeds the time of the next video frame,
      // get and encode the next video frame

      while (clock >= nextFrameTime)
      {
        BufferedImage frame = balls.getVideoFrame(frameRate);
        writer.encodeVideo(videoStreamIndex, frame, nextFrameTime, 
          DEFAULT_TIME_UNIT);
        nextFrameTime += frameRate;
      }

      // compute and encode the audio for the balls

      short[] samples = balls.getAudioFrame(sampleRate);
      writer.encodeAudio(audioStreamIndex, samples, clock, 
        DEFAULT_TIME_UNIT);
      totalSampleCount += sampleCount;
    }

    // manually close the writer
    
    writer.close();
  }  
}
