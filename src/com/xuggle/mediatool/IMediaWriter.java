/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated. All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler. If not, see <http://www.gnu.org/licenses/>.
 */

package com.xuggle.mediatool;

import java.awt.image.BufferedImage;
import java.util.concurrent.TimeUnit;

import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IPixelFormat;
import com.xuggle.xuggler.IRational;
import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IStreamCoder;
import com.xuggle.xuggler.IVideoPicture;

/**
 * An {@link IMediaCoder} that encodes and decodes media to an
 * {@link IContainer}, and can optionally read data for encoding from other
 * {@link IMediaGenerator} objects.
 * 
 * <p>
 * 
 * An {@link IMediaWriter} is a simplified interface to the Xuggler library that
 * opens up a media container, and allows media data to be written into it.
 * 
 * </p>
 * 
 * <p>
 * The {@link IMediaWriter} class implements {@link IMediaListener}, and so it
 * can be attached to any {@link IMediaGenerator} that generates raw media
 * events (e.g. {@link IMediaReader}). If will query the input pipe for all of
 * it's streams, and create the right output streams (and mappings) in the new
 * file.
 * </p>
 * 
 * <p>
 * 
 * Calls to {@link #encodeAudio(int, IAudioSamples)} and
 * {@link #encodeVideo(int, IVideoPicture)} encode media into packets and write
 * those packets to the specified container.
 * 
 * </p>
 * <p>
 * 
 * If you are generating video from Java {@link BufferedImage} but you don't
 * have an {@link IVideoPicture} object handy, don't sweat. You can use
 * {@link #encodeVideo(int, BufferedImage, long, TimeUnit)} for that.
 * 
 * </p>
 * <p>
 * 
 * If you are generating audio from in Java short arrays (16-bit audio) but
 * don't have an {@link IAudioSamples} object handy, don't sweat. You can use
 * {@link #encodeAudio(int, short[], long, TimeUnit)} for that.
 * 
 * </p>
 */

public interface IMediaWriter extends IMediaCoder, IMediaTool
{

  /**
   * Set late stream exception policy. When
   * {@link #encodeAudio(int, IAudioSamples)} or
   * {@link #encodeVideo(int, IVideoPicture)} is passed an unrecognized stream
   * index after the the header has been written, either an exception is raised,
   * or the media data is silently ignored. By default exceptions are raised,
   * not masked.
   * 
   * @param maskLateStreamExceptions true if late med
   * 
   * @see #willMaskLateStreamExceptions
   */

  public abstract void setMaskLateStreamExceptions(
      boolean maskLateStreamExceptions);

  /**
   * Get the late stream exception policy. When
   * {@link #encodeVideo(int, IVideoPicture)} or
   * {@link #encodeAudio(int, IAudioSamples)}is passed an unrecognized stream
   * index after the the header has been written, either an exception is raised,
   * or the media data is silently ignored. By default exceptions are raised,
   * not masked.
   * 
   * @return true if late stream data raises exceptions
   * 
   * @see #setMaskLateStreamExceptions
   */

  public abstract boolean willMaskLateStreamExceptions();

  /**
   * Set the force interleave option.
   * 
   * <p>
   * 
   * If false the media data will be left in the order in which it is presented
   * to the IMediaWriter.
   * 
   * </p>
   * <p>
   * 
   * If true {@link IMediaWriter} will buffer media data in time stamp order,
   * and only write out data when it has at least one same time or later packet
   * from all streams.
   * 
   * <p>
   * 
   * @param forceInterleave true if the IMediaWriter should force interleaving
   *        of media data
   * 
   * @see #willForceInterleave
   */

  public abstract void setForceInterleave(boolean forceInterleave);

  /**
   * Test if the {@link IMediaWriter} will forcibly interleave media data. The
   * default value for this value is true.
   * 
   * @return true if {@link IMediaWriter} interleaves media data.
   * 
   * @see #setForceInterleave
   */

  public abstract boolean willForceInterleave();

  /**
   * Test if this {@link IMediaWriter} can write streams of this type.
   * 
   * @param type the type of codec to be tested
   * 
   * @return true if codec type is supported type
   */

  public abstract boolean isSupportedCodecType(ICodec.Type type);

  /**
   * Get the default pixel type
   * 
   * @return the default pixel type
   */
  public abstract IPixelFormat.Type getDefaultPixelType();

  /**
   * Get the default audio sample format
   * 
   * @return the format
   */
  public abstract IAudioSamples.Format getDefaultSampleFormat();

  /**
   * Get the default time base we'll use on our encoders if one is not specified
   * by the codec.
   * 
   * @return the default time base
   */
  public abstract IRational getDefaultTimebase();

  /**
   * Add an audio stream that will later have data encoded with
   * {@link #encodeAudio(int, IAudioSamples)}.
   * <p>
   * The time base defaults to {@link #getDefaultTimebase()} and the audio
   * format defaults to {@link #getDefaultSampleFormat()}.
   * </p>
   * 
   * @param inputIndex the index that will be passed to
   *        {@link #encodeAudio(int, IAudioSamples)} for this stream
   * @param streamId a format-dependent id for this stream
   * @param codec the codec to used to encode data, to establish the codec see
   *        {@link ICodec}
   * @param channelCount the number of audio channels for the stream
   * @param sampleRate sample rate in Hz (samples per seconds), common values
   *        are 44100, 22050, 11025, etc.
   * 
   * @return <0 on failure; otherwise returns the index of the new stream added
   *         by the writer.
   * 
   * @throws IllegalArgumentException if inputIndex < 0, the stream id < 0, the
   *         codec is NULL or if the container is already open.
   * @throws IllegalArgumentException if width or height are <= 0
   * 
   * @see IContainer
   * @see IStream
   * @see IStreamCoder
   * @see ICodec
   */

  public abstract int addAudioStream(int inputIndex, int streamId,
      ICodec codec, int channelCount, int sampleRate);

  /**
   * Add a video stream that will later have data encoded with
   * {@link #encodeVideo(int, IVideoPicture)}.
   * 
   * <p>
   * 
   * The time base defaults to {@link #getDefaultTimebase()} and the pixel
   * format defaults to {@link #getDefaultPixelType()}.
   * 
   * </p>
   * 
   * @param inputIndex the index that will be passed to
   *        {@link #encodeVideo(int, IVideoPicture)} for this stream
   * @param streamId a format-dependent id for this stream
   * @param codec the codec to used to encode data, to establish the codec see
   *        {@link ICodec}
   * @param width width of video frames
   * @param height height of video frames
   * 
   * @throws IllegalArgumentException if inputIndex < 0, the stream id < 0, the
   *         codec is NULL or if the container is already open.
   * @throws IllegalArgumentException if width or height are <= 0
   * 
   * @return <0 on failure; otherwise returns the index of the new stream added
   *         by the writer.
   * @see IContainer
   * @see IStream
   * @see IStreamCoder
   * @see ICodec
   */

  public abstract int addVideoStream(int inputIndex, int streamId,
      ICodec codec, int width, int height);

  /**
   * Encodes audio from samples into the stream with the specified index.
   * 
   * <p>
   * 
   * Callers must ensure that {@link IAudioSamples#getTimeStamp()}, if specified
   * is always monotonically increasing or an error will be returned.
   * 
   * </p>
   * 
   * @param streamIndex The stream index, as returned from
   *        {@link #addAudioStream(int, int, ICodec, int, int)}.
   * @param samples A set of samples to add.
   */
  public abstract void encodeAudio(int streamIndex, IAudioSamples samples);

  /**
   * Encoded audio from samples into the stream with the specified index.
   * 
   * <p>
   * 
   * {@link IMediaWriter} will assume that these samples are to played
   * immediately after the last set of samples, or with the earliest time stamp
   * in the container if they are the first samples.
   * 
   * </p>
   * 
   * @param streamIndex The stream index, as returned from
   *        {@link #addAudioStream(int, int, ICodec, int, int)}.
   * @param samples A set of samples to add.
   */
  public abstract void encodeAudio(int streamIndex, short[] samples);

  /**
   * Encoded audio from samples into the stream with the specified index.
   * 
   * <p>
   * 
   * If <code>timeUnit</code> is null, {@link IMediaWriter} will assume that
   * these samples are to played immediately after the last set of samples, or
   * with the earliest time stamp in the container if they are the first
   * samples.
   * 
   * </p>
   * <p>
   * 
   * Callers must ensure that <code>timeStamp</code>, if <code>timeUnit</code>
   * is non-null, is always monotonically increasing or an runtime exception
   * will be raised.
   * 
   * </p>
   * 
   * @param streamIndex The stream index, as returned from
   *        {@link #addAudioStream(int, int, ICodec, int, int)}.
   * @param samples A set of samples to add.
   * @param timeStamp The time stamp for this media.
   * @param timeUnit The units of timeStamp, or null if you want
   *        {@link IMediaWriter} to assume these samples immediately precede any
   *        prior samples.
   */
  public abstract void encodeAudio(int streamIndex, short[] samples,
      long timeStamp, TimeUnit timeUnit);

  /**
   * Encodes video from the given picture into the stream with the specified
   * index.
   * 
   * <p>
   * 
   * Callers must ensure that {@link IVideoPicture#getTimeStamp()}, if specified
   * is always monotonically increasing or an {@link RuntimeException} will be
   * raised.
   * 
   * </p>
   * 
   * @param streamIndex The stream index, as returned from
   *        {@link #addVideoStream(int, int, ICodec, int, int)}.
   * @param picture A picture to encode
   */
  public abstract void encodeVideo(int streamIndex, IVideoPicture picture);

  /**
   * Encodes video from the given picture into the stream with the specified
   * index.
   * 
   * <p>
   * 
   * Callers must ensure that {@link IVideoPicture#getTimeStamp()}, if specified
   * is always monotonically increasing or an {@link RuntimeException} will be
   * raised.
   * 
   * </p>
   * 
   * @param streamIndex The stream index, as returned from
   *        {@link #addVideoStream(int, int, ICodec, int, int)}.
   * @param image A {@link BufferedImage} to encode
   * @param timeStamp The time stamp for this image
   * @param timeUnit The time unit of timeStamp. Cannot be null.
   */
  public abstract void encodeVideo(int streamIndex, BufferedImage image,
      long timeStamp, TimeUnit timeUnit);

  /**
   * Flushes all encoders and writes their contents.
   * 
   * <p>
   * 
   * Callers should call this when they have finished encoding all audio and
   * video to ensure that any cached data necessary for encoding was written.
   * 
   * </p>
   */
  public abstract void flush();

  /**
   * Map an input stream index to an output stream index.
   * 
   * @param inputStreamIndex the input stream index value
   * 
   * @return the associated output stream index or null, if the input stream
   *         index has not been mapped to an output index.
   */

  public abstract Integer getOutputStreamIndex(int inputStreamIndex);

}
