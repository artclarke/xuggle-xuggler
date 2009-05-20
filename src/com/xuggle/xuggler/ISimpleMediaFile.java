/*
 * This file is part of Xuggler.
 * 
 * Xuggler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public
 * License along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.xuggle.xuggler;

import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IContainerFormat;
import com.xuggle.xuggler.IPixelFormat;
import com.xuggle.xuggler.IRational;
import com.xuggle.xuggler.ITimeValue;

/**
 * This class contains meta-information about simple media files.
 *
 * A simple media file is defined as a file with at most one
 * video and at most one audio stream, and no other streams.
 * 
 * This is useful or objects that want to contain all the
 * interesting data about a media file in one handy dandy
 * object.
 * 
 * @author aclarke
 */

public interface ISimpleMediaFile
{

  /**
   * Set the audio bitrate
   * @param bitRate bitrate to use
   */
  public void setAudioBitRate(int bitRate);
  
  /**
   * Get the audio bit rate
   * 
   * @return bit rate, in Hz.
   */
  public int getAudioBitRate();
  
  /**
   * Set the number of audio channels.
   * 
   * @param numChannels Number of channels; only 1 or 2 supported
   */
  public void setAudioChannels(int numChannels);
  
  /**
   * Get the number of audio channels
   * 
   * @return number of audio channels
   */
  public int getAudioChannels();
  
  /**
   * Set the audio sample rate
   * 
   * @param sampleRate Audio sample rate in Hz (e.g. 22050)
   */
  public void setAudioSampleRate(int sampleRate);

  /**
   * Get the audio sample rate.
   * 
   * @return sample rate, in Hz.
   */
  public int getAudioSampleRate();
  
  /**
   * Set the audio sample format you want to use in this file.
   * @param format The sample format to use.
   */
  public void setAudioSampleFormat(IAudioSamples.Format format);
  
  /**
   * Get the audio sample format.
   * @return The audio sample format.  Defaults to {@link IAudioSamples.Format#FMT_S16}
   *   if not specified.
   */
  public IAudioSamples.Format getAudioSampleFormat();
  
  /**
   * Set the audio codec to use.
   * 
   * @param audioCodec The audio codec to use; if null, a user may
   *   guess the codec from other information.
   */
  public void setAudioCodec(ICodec.ID audioCodec);
  
  /**
   * Get the audio codec set on this configuration object.
   * 
   * @return The audiocodec.   if null, a user may
   *   guess the codec from other information.
   */
  public ICodec.ID getAudioCodec();
  
  /**
   * Set the width of the video frames in broadcasted packets
   * @param width width in pixels
   */
  public void setVideoWidth(int width);
  
  /**
   * Get the video width of video frames broadcasted
   * @return width in pixels
   */
  public int getVideoWidth();

  /**
   * Set the height of the video frames in broadcasted packets
   * @param height height in pixels
   */
  public void setVideoHeight(int height);

  /**
   * Get the video height of video frames broadcasted
   * @return height in pixels
   */
  public int getVideoHeight();
  
  /**
   * Set the timebase that we encode video for.
   *
   * @param bitRate The bitrate to use for encoding video
   */
  public void setVideoBitRate(int bitRate);

  /**
   * Get the video bit rate
   * @return video bit rate
   */
  public int getVideoBitRate();

  /**
   * Set the TimeBase that video packets are encoded with.
   * @param timeBase The timebase to use with this configuration, or null if unknowbn.
   */
  public void setVideoTimeBase(IRational timeBase);
  
  /**
   * Get the timebase that packets should be encoded with.
   * @return The timebase or null if unknown.
   */
  public IRational getVideoTimeBase();
  /**
   * Set the video codec that packets are encoded with.
   * @param videoCodec video codec that packets are encoded with.
   */
  public void setVideoCodec(ICodec.ID videoCodec);
  
  /**
   * Get the video codec that packets are encoded with.
   * @return the video codec
   */
  public ICodec.ID getVideoCodec();

  /**
   * Return the pixel format to use
   * @return The pixel format.
   */
  public IPixelFormat.Type getVideoPixelFormat();

  /**
   * Sets the pixel format to use
   * @param aPixelFormat Pixel format
   */
  void setVideoPixelFormat(IPixelFormat.Type aPixelFormat);


  /**
   * Set the MPEG2 Num Pictures In Group Of Pictures (GOPS) settings
   * @param gops Set the GOPS
   */
  public void setVideoNumPicturesInGroupOfPictures(int gops);
  
  /**
   * Get the GOPS.
   * @see #setVideoNumPicturesInGroupOfPictures(int)
   * @return The GOPS
   */
  public int getVideoNumPicturesInGroupOfPictures();
  
  /**
   * Set the Video Frame Rate as a rational.
   * @param frameRate  The frame rate (e.g. 15/1), or null if unknown.
   */
  public void setVideoFrameRate(IRational frameRate);
  
  /**
   * Get the Video Frame Rate
   * @see #setVideoFrameRate(IRational)
   * @return The video frame rate, or null if unknown
   */
  public IRational getVideoFrameRate();
  
  /**
   * Set the default quality quality setting for video.  0 is highest
   * quality.  > 0 is decreasing quality.
   * @param quality  The quality to use.
   */
  public void setVideoGlobalQuality(int quality);
  
  /**
   * Get the VideoGlobalQuality setting.
   * @see #setVideoGlobalQuality(int)
   * @return The global quality for video
   */
  public int getVideoGlobalQuality();
  
  /**
   * Set whether you want the output stream to have video
   * @param hasVideo Whether you want the output stream to have video
   */
  public void setHasVideo(boolean hasVideo);
  
  /**
   * Tells whether or not the output stream should have video
   * @return Should the output stream have video?
   */
  public boolean hasVideo();

  /**
   * Set whether you want the output stream to have audio
   * @param hasAudio Whether you want the output stream to have audio
   */
  public void setHasAudio(boolean hasAudio);
  
  /**
  * Tells whether or not the output stream should have audio
  * @return Should the output stream have audio?
  */
  public boolean hasAudio();

  /**
   * Sets the container format for this stream
   * @param format The container format
   */
  public void setContainerFormat(IContainerFormat format);
  
  /**
   * Get the container format, if known, for this container.
   * @return The container format
   */
  public IContainerFormat getContainerFormat();

  /**
   * is the audio bit rate known?
   * @return the audioBitRateKnown
   */
  public boolean isAudioBitRateKnown();
  
  /**
   * set if the audio channels are known
   * @param audioChannelsKnown the audioChannelsKnown to set
   */
  public void setAudioChannelsKnown(boolean audioChannelsKnown);

  /**
   * are the # of audio channels known?
   * @return the audioChannelsKnown
   */
  public boolean isAudioChannelsKnown();

  /**
   * set if the audio sample rate is known.
   * @param audioSampleRateKnown the audioSampleRateKnown to set
   */
  public void setAudioSampleRateKnown(boolean audioSampleRateKnown);

  /**
   * is the audio sample rate known?
   * @return the audioSampleRateKnown
   */
  public boolean isAudioSampleRateKnown();

  /**
   * set if the video height is known.
   * @param videoHeightKnown the videoHeightKnown to set
   */
  public void setVideoHeightKnown(boolean videoHeightKnown);

  /**
   * is the video height known?
   * @return the videoHeightKnown
   */
  public boolean isVideoHeightKnown();

  /**
   * set if the video width is known.
   * @param videoWidthKnown the videoWidthKnown to set
   */
  public void setVideoWidthKnown(boolean videoWidthKnown);

  /**
   * is the video width known?
   * @return the videoWidthKnown
   */
  public boolean isVideoWidthKnown();

  /**
   * set if the video bit rate is known.
   * @param videoBitRateKnown the videoBitRateKnown to set
   */
  public void setVideoBitRateKnown(boolean videoBitRateKnown);

  /**
   * is the video bit rate known?
   * @return the videoBitRateKnown
   */
  public boolean isVideoBitRateKnown();

  /**
   * set if the MPEG number of pictures in a group of pictures is known?
   * @param videoGOPSKnown the videoGOPSKnown to set
   */
  public void setVideoNumPicturesInGroupOfPicturesKnown(boolean videoGOPSKnown);

  /**
   * is the MPEG number of pictures in a group of pictures known?
   * @return the videoGOPSKnown
   */
  public boolean isVideoNumPicturesInGroupOfPicturesKnown();

  /**
   * set if the global quality setting is known.
   * @param videoGlobalQualityKnown the videoGlobalQualityKnown to set
   */
  public void setVideoGlobalQualityKnown(boolean videoGlobalQualityKnown);

  /**
   * is the global quality setting known?
   * @return the videoGlobalQualityKnown
   */
  public boolean isVideoGlobalQualityKnown();

  /**
   * set if the video pixel format is known.
   * @param videoPixelFormatKnown the videoPixelFormatKnown to set
   */
  public void setVideoPixelFormatKnown(boolean videoPixelFormatKnown);

  /**
   * is the video pixel format known?
   * @return the videoPixelFormatKnown
   */
  public boolean isVideoPixelFormatKnown();

  /**
   * Set the time base to use when encoding or decoding audio
   * @param aTimeBase The timebase to use, or null if unknown.
   */
  public void setAudioTimeBase(IRational aTimeBase);
  
  /**
   * Get the time base used when encoding or decoding audio.
   * @return the timebase, or null if unknown.
   */
  public IRational getAudioTimeBase();

  /**
   * Set the duration that we'll assume for the stream we're representing.
   * 
   * @param duration the duration to assume for this file.  null means unknown.
   */
  void setDuration(ITimeValue duration);

  /**
   * Get the duration that we'll assume for the stream we're representing.
   * @return the duration, or null if unknown.
   */
  ITimeValue getDuration();

  /**
   * set if the audio bit rate is known.
   * @param audioBitRateKnown the audioBitRateKnown to set
   */
  void setAudioBitRateKnown(boolean audioBitRateKnown);

  /**
   * Set the URL that this stream contains info for.
   * @param aUrl The url, including protocol string, or null if unknown.
   */
  void setURL(String aUrl);

  /**
   * Return the URL for this stream.
   * @return The URL, or null if unknown.
   */
  String getURL();
}
