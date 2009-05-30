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

public interface IMediaWriter extends IMediaTool, IMediaPipeListener
{

  /** 
   * Add a audio stream.  The time base defaults to {@link
   * #getDefaultTimebase()} and the audio format defaults to {@link
   * #getDefaultSampleFormat()}.  
   * 
   * @param inputIndex the index that will be passed to {@link
   *        #onAudioSamples} for this stream
   * @param streamId a format-dependent id for this stream
   * @param codec the codec to used to encode data, to establish the
   *        codec see {@link ICodec}
   * @param channelCount the number of audio channels for the stream
   * @param sampleRate sample rate in Hz (samples per seconds), common
   *        values are 44100, 22050, 11025, etc.
   *
   * @return <0 on failure; otherwise returns the index of
   *   the new stream added by the writer.
   *   
   * @throws IllegalArgumentException if inputIndex < 0, the stream id <
   *         0, the codec is NULL or if the container is already open.
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
   * Add a video stream.  The time base defaults to {@link
   * #getDefaultTimebase()} and the pixel format defaults to {@link
   * #getDefaultPixelType()}.
   * 
   * @param inputIndex the index that will be passed to {@link
   *        #onVideoPicture} for this stream
   * @param streamId a format-dependent id for this stream
   * @param codec the codec to used to encode data, to establish the
   *        codec see {@link ICodec}
   * @param width width of video frames
   * @param height height of video frames
   *
   * @throws IllegalArgumentException if inputIndex < 0, the stream id <
   *         0, the codec is NULL or if the container is already open.
   * @throws IllegalArgumentException if width or height are <= 0
   * 
   * @return <0 on failure; otherwise returns the index of
   *   the new stream added by the writer.
   * @see IContainer
   * @see IStream
   * @see IStreamCoder
   * @see ICodec
   */

  public abstract int addVideoStream(int inputIndex, int streamId,
      ICodec codec, int width, int height);

  /**
   * Set late stream exception policy.  When onVideoPicture or
   * onAudioSamples is passed an unrecognized stream index after the the
   * header has been written, either an exception is raised, or the
   * media data is silently ignored.  By default exceptions are raised,
   * not masked.
   *
   * @param maskLateStreamExceptions true if late med
   * 
   * @see #willMaskLateStreamExceptions
   */

  public abstract void setMaskLateStreamExceptions(
      boolean maskLateStreamExceptions);

  /** 
   * Get the late stream exception policy.  When onVideoPicture or
   * onAudioSamples is passed an unrecognized stream index after the the
   * header has been written, either an exception is raised, or the
   * media data is silently ignored.  By default exceptions are raised,
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
   * If false the media data will be left in the order in which it is
   * presented to the IMediaWriter.
   * 
   * </p>
   * <p>
   *
   * If true IMediaWriter will ask Xuggler to place media data in time
   * stamp order, which is required for streaming media.
   *
   * <p>
   *
   * @param forceInterleave true if the IMediaWriter should force
   *        interleaving of media data
   *
   * @see #willForceInterleave
   */

  public abstract void setForceInterleave(boolean forceInterleave);

  /**
   * Test if the IMediaWriter will forcibly interleave media data.
   * The default value for this value is true.
   *
   * @return true if {@link IMediaWriter} forces Xuggler to interleave media data.
   *
   * @see #setForceInterleave
   */

  public abstract boolean willForceInterleave();

  /** 
   * Map an input stream index to an output stream index.
   *
   * @param inputStreamIndex the input stream index value
   *
   * @return the associated output stream index or null, if the input
   *         stream index has not been mapped to an output index.
   */

  public abstract Integer getOutputStreamIndex(int inputStreamIndex);

  /** 
   * Push an image onto a video stream.
   * 
   * @param streamIndex the index of the video stream
   * @param image the image to push out
   * @param timeStamp the time stamp of the image
   * @param timeUnit the time unit of the timestamp
   */

  public abstract void pushImage(int streamIndex, BufferedImage image,
      long timeStamp, TimeUnit timeUnit);

  /** 
   * Push samples onto a audio stream.
   * 
   * @param streamIndex the index of the audio stream
   * @param samples the buffer containing the audio samples to push out
   * @param timeStamp the time stamp of the samples
   * @param timeUnit the time unit of the timestampb
   */

  public abstract void pushSamples(int streamIndex, short[] samples,
      long timeStamp, TimeUnit timeUnit);

  /**
   * Test if the {@link IMediaWriter} can write streams
   * of this type.
   * 
   * @param type the type of codec to be tested
   *
   * @return true if codec type is supported type
   */

  public abstract boolean isSupportedCodecType(ICodec.Type type);

  /**
   * Get the default pixel type
   * @return the default pixel type
   */
  public abstract IPixelFormat.Type getDefaultPixelType();

  /**
   * Get the default audio sample format
   * @return the format
   */
  public abstract IAudioSamples.Format getDefaultSampleFormat();

  /**
   * Get the default time base we'll use on our encoders
   * if one is not specified by the codec.
   * @return the default time base
   */
  public abstract IRational getDefaultTimebase();

  public abstract void flush();

}
