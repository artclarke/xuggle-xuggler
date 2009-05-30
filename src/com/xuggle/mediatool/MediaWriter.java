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

package com.xuggle.mediatool;

import java.util.Map;
import java.util.Vector;
import java.util.HashMap;
import java.util.Collection;
import java.util.concurrent.TimeUnit;

import java.awt.image.BufferedImage;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.mediatool.MediaReader;
import com.xuggle.ferry.IBuffer;
import com.xuggle.ferry.JNIReference;
import com.xuggle.xuggler.Global;
import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IError;
import com.xuggle.xuggler.IPacket;
import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IRational;
import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IPixelFormat;
import com.xuggle.xuggler.IStreamCoder;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IContainerFormat;
import com.xuggle.xuggler.video.IConverter;
import com.xuggle.xuggler.video.ConverterFactory;

import static com.xuggle.xuggler.ICodec.Type.CODEC_TYPE_VIDEO;
import static com.xuggle.xuggler.ICodec.Type.CODEC_TYPE_AUDIO;

import static java.util.concurrent.TimeUnit.MICROSECONDS;

/**
 * Opens an output container, lets the user add streams, and
 * then encodes and writes media to that container.
 * 
 * <p>
 * 
 * The MediaWriter class is a simplified interface to the Xuggler
 * library that opens up a media container, and allows media data to be
 * written into it.
 * 
 * </p>
 * 
 * <p>
 * The {@link MediaWriter} class implements {@link IMediaPipeListener}, and so
 * it can be attached to any {@link IMediaPipe} that generates raw
 * media events (e.g. {@link MediaReader}).
 * </p>
 * 
 * <p>
 * 
 * Calls to {@link #onAudioSamples}, and {@link #onVideoPicture} encode
 * media into packets and write those encoded packets.
 * 
 * </p>
 * <p>
 * 
 * If you are generating video from Java {@link BufferedImage} but you
 * don't have an {@link IVideoPicture} object handy, don't sweat.  You
 * can use {@link #pushImage(BufferedImage, int, long)}, and {@link MediaWriter}
 * will convert your {@link BufferedImage} into the right type.
 * 
 * </p>
 */

class MediaWriter extends AMediaTool
implements IMediaPipeListener, IMediaWriter
{
  final private Logger log = LoggerFactory.getLogger(this.getClass());
  { log.trace("<init>"); }

  static
  {
    com.xuggle.ferry.JNIMemoryManager.setMemoryModel(
      com.xuggle.ferry.JNIMemoryManager.MemoryModel.NATIVE_BUFFERS);
  }


  /** The default pixel type. */

  private static final IPixelFormat.Type DEFAULT_PIXEL_TYPE = 
    IPixelFormat.Type.YUV420P;

  /** The default sample format. */

  private static final IAudioSamples.Format DEFAULT_SAMPLE_FORMAT = 
    IAudioSamples.Format.FMT_S16;

  /** The default time base. */

  private static final IRational DEFAULT_TIMEBASE = IRational.make(
    1, (int)Global.DEFAULT_PTS_PER_SECOND);

  // the input container of packets
  
  private final IContainer mInputContainer;

  // the container format

  private IContainerFormat mContainerFormat;

  // a map between input stream indicies to output stream indicies

  private Map<Integer, Integer> mOutputStreamIndices = 
    new HashMap<Integer, Integer>();

  // a map between output stream indicies and streams

  private Map<Integer, IStream> mStreams = 
    new HashMap<Integer, IStream>();

  // a map between output stream indicies and video converters

  private Map<IStream, IConverter> mVideoConverters = 
    new HashMap<IStream, IConverter>();
  
  // streasm opened by this MediaWriter must be closed

  private final Collection<IStream> mOpenedStreams = new Vector<IStream>();

  // true if the writer should ask FFMPEG to interleave media

  private boolean mForceInterleave = true;

  // mask late stream exception policy
  
  private boolean mMaskLateStreamException = false;

  /**
   * Use a specified {@link IMediaReader} as a source for media data and
   * meta data about the container and it's streams.  The {@link
   * IMediaReader} must be configured such that streams will not be
   * dynamically added to the container, which is the default for {@link
   * IMediaReader}.
   * 
   * @param url the url or filename of the media destination
   * @param reader the media source
   * 
   * @throws IllegalArgumentException if the specifed {@link
   *         IMediaReader} is configure to allow dynamic adding of
   *         streams.
   */

  MediaWriter(String url, IMediaReader reader)
  {
    // construct around the source container

    this(url, reader.getContainer());

    // if the container can add streams dynamically, it is not
    // currently supported, throw an exception.  this kind of test needs
    // to be done both here and in the constructor which takes a
    // container because the MediaReader may not have opened it's
    // internal container and thus not set this flag yet

    if (reader.canAddDynamicStreams())
      throw new IllegalArgumentException(
        "inputContainer is improperly configured to allow " + 
        "dynamic adding of streams.");
  }

  /**
   * Use a specified {@link IContainer} as a source for and meta data
   * about the container and it's streams.  The {@link IContainer} must
   * be configured such that streams will not be dynamically added to the
   * container.
   * 
   * @param url the url or filename of the media destination
   * @param inputContainer the source media container
   * 
   * @throws IllegalArgumentException if the specifed {@link IContainer}
   *         is not a of type READ or is configure to allow dynamic
   *         adding of streams.
   */

  MediaWriter(String url, IContainer inputContainer)
  {
    super(url, IContainer.make());

    // verify that the input container is a readable type

    if (inputContainer.getType() != IContainer.Type.READ)
      throw new IllegalArgumentException(
        "inputContainer is improperly must be of type readable.");

    // verify that no streams will be added dynamically

    if (inputContainer.canStreamsBeAddedDynamically())
      throw new IllegalArgumentException(
        "inputContainer is improperly configured to allow " + 
        "dynamic adding of streams.");

    // record the input container and url

    mInputContainer = inputContainer;

    // create format 

    mContainerFormat = IContainerFormat.make();
    mContainerFormat.setOutputFormat(mInputContainer.getContainerFormat().
      getInputFormatShortName(), getUrl(), null);
  }

  /**
   * Create a MediaWriter which will require subsequent calls to {@link
   * #addVideoStream} and/or {@link #addAudioStream} to configure the
   * writer.  Streams may be added or further configured as needed until
   * the first attempt to write data.
   * 
   * <p>
   *
   * To write data call to {@link #onAudioSamples} and/or {@link
   * #onVideoPicture}.
   *
   * </p>
   *
   * @param url the url or filename of the media destination
   */

  MediaWriter(String url)
  {
    super(url, IContainer.make());

    // record the url and absense of the input container 

    mInputContainer = null;

    // create null container format
    
    mContainerFormat = null;
  }

  /** 
   * Add a audio stream.  The time base defaults to {@link
   * #DEFAULT_TIMEBASE} and the audio format defaults to {@link
   * #DEFAULT_SAMPLE_FORMAT}.  The new {@link IStream} is returned to
   * provide an easy way to further configure the stream.
   * 
   * @param inputIndex the index that will be passed to {@link
   *        #onAudioSamples} for this stream
   * @param streamId a format-dependent id for this stream
   * @param codec the codec to used to encode data, to establish the
   *        codec see {@link com.xuggle.xuggler.ICodec}
   * @param channelCount the number of audio channels for the stream
   * @param sampleRate sample rate in Hz (samples per seconds), common
   *        values are 44100, 22050, 11025, etc.
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

  public int addAudioStream(int inputIndex, int streamId, ICodec codec,
    int channelCount, int sampleRate)
  {
    // validate parameteres

    if (channelCount <= 0)
      throw new IllegalArgumentException(
        "invalid channel count " + channelCount);
    if (sampleRate <= 0)
      throw new IllegalArgumentException(
        "invalid sample rate " + sampleRate);

    // add the new stream at the correct index

    IStream stream = establishStream(inputIndex, streamId, codec);
    
    // configre the stream coder

    IStreamCoder coder = stream.getStreamCoder();
    coder.setChannels(channelCount);
    coder.setSampleRate(sampleRate);
    coder.setSampleFormat(getDefaultSampleFormat());

    // add the stream to the media writer
    
    addStream(stream, inputIndex, stream.getIndex());

    // return the new audio stream

    return stream.getIndex();
  }

  /** 
   * Add a video stream.  The time base defaults to {@link
   * #DEFAULT_TIMEBASE} and the pixel format defaults to {@link
   * #DEFAULT_PIXEL_TYPE}.  The new {@link IStream} is returned to
   * provide an easy way to further configure the stream.
   * 
   * @param inputIndex the index that will be passed to {@link
   *        #onVideoPicture} for this stream
   * @param streamId a format-dependent id for this stream
   * @param codec the codec to used to encode data, to establish the
   *        codec see {@link com.xuggle.xuggler.ICodec}
   * @param width width of video frames
   * @param height height of video frames
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

  public int addVideoStream(int inputIndex, int streamId, ICodec codec,
    int width, int height)
  {
    // validate parameteres

    if (width <= 0 || height <= 0)
      throw new IllegalArgumentException(
        "invalid video frame size [" + width + " x " + height + "]");

    // add the new stream at the correct index

    IStream stream = establishStream(inputIndex, streamId, codec);
    
    // configre the stream coder

    IStreamCoder coder = stream.getStreamCoder();
    coder.setWidth(width);
    coder.setHeight(height);
    coder.setPixelType(getDefaultPixelType());

    // add the stream to the media writer
    
    addStream(stream, inputIndex, stream.getIndex());

    // return the new video stream

    return stream.getIndex();
  }

  /** 
   * Add a generic stream the this writer.  This method is intended for
   * internal use.
   * 
   * @param inputIndex the index that will be passed to {@link
   *        #onVideoPicture} for this stream
   * @param streamId a format-dependent id for this stream
   * @param codec the codec to used to encode data
   *
   * @throws IllegalArgumentException if inputIndex < 0, the stream id <
   *         0, the codec is NULL or if the container is already open.
   */

  private IStream establishStream(int inputIndex, int streamId, ICodec codec)
  {
    // validate parameteres and conditions

    if (inputIndex < 0)
      throw new IllegalArgumentException("invalid input index " + inputIndex);
    if (streamId < 0)
      throw new IllegalArgumentException("invalid stream id " + streamId);
    if (null == codec)
      throw new IllegalArgumentException("null codec");

    // if the container is not opened, do so

    if (!isOpen())
      open();

    // add the new stream at the correct index

    IStream stream = getContainer().addNewStream(streamId);
    if (stream == null)
      throw new RuntimeException("Unable to create stream id " + streamId +
        ", index " + inputIndex + ", codec " + codec);
    
    // configure the stream coder

    IStreamCoder coder = stream.getStreamCoder();
    coder.setTimeBase(getDefaultTimebase());
    coder.setCodec(codec);

    // if the stream count is 1, don't force interleave

    setForceInterleave(getContainer().getNumStreams() != 1);

    // return the new video stream

    return stream;
  }


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

  public void setMaskLateStreamExceptions(boolean maskLateStreamExceptions)
  {
    mMaskLateStreamException = maskLateStreamExceptions;
  }
  
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

  public boolean willMaskLateStreamExceptions()
  {
    return mMaskLateStreamException;
  }

  /**
   * Set the force interleave option.
   *
   * <p>
   * 
   * If false the media data will be left in the order in which it is
   * presented to the MediaWriter.
   * 
   * </p>
   * <p>
   *
   * If true MediaWriter will ask Xuggler to place media data in time
   * stamp order, which is required for streaming media.
   *
   * <p>
   *
   * @param forceInterleave true if the MediaWriter should force
   *        interleaving of media data
   *
   * @see #willForceInterleave
   */

  public void setForceInterleave(boolean forceInterleave)
  {
    mForceInterleave = forceInterleave;
  }

  /**
   * Test if the MediaWriter will forcibly interleave media data.
   * The default value for this value is true.
   *
   * @return true if MediaWriter forces Xuggler to interleave media data.
   *
   * @see #setForceInterleave
   */

  public boolean willForceInterleave()
  {
    return mForceInterleave;
  }

  /** 
   * Map an input stream index to an output stream index.
   *
   * @param inputStreamIndex the input stream index value
   *
   * @return the associated output stream index or null, if the input
   *         stream index has not been mapped to an output index.
   */

  public Integer getOutputStreamIndex(int inputStreamIndex)
  {
    return mOutputStreamIndices.get(inputStreamIndex);
  }

  /** 
   * Push an image onto a video stream.
   * 
   * @param streamIndex the index of the video stream
   * @param image the image to push out
   * @param timeStamp the time stamp of the image
   * @param timeUnit the time unit of the timestamp
   */

  public void pushImage(int streamIndex, BufferedImage image, long timeStamp, 
    TimeUnit timeUnit)
  {
    // verify parameters

    Integer outputIndex = getOutputStreamIndex(streamIndex);
    if (null == outputIndex)
      throw new IllegalArgumentException("unknow stream index: " + streamIndex);
    if (null == image)
      throw new IllegalArgumentException("NULL input image");
    if (null == timeUnit)
      throw new IllegalArgumentException("NULL time unit");
    if (CODEC_TYPE_VIDEO  != mStreams.get(outputIndex).getStreamCoder()
      .getCodecType())
    {
      throw new IllegalArgumentException("stream[" + streamIndex + 
        "] is not video");
    }

    // convert the image to a picture and push it off to be encoded

    IVideoPicture picture = convertToPicture(getStream(streamIndex), 
      image, MICROSECONDS.convert(timeStamp, timeUnit));

    try
    {
      onVideoPicture(this, picture, null, streamIndex);
    } 
    finally 
    {
      if (picture != null)
        picture.delete();
    }
  }


  /** 
   * Push samples onto a audio stream.
   * 
   * @param streamIndex the index of the audio stream
   * @param samples the buffer containing the audio samples to push out
   * @param timeStamp the time stamp of the samples
   * @param timeUnit the time unit of the timestampb
   */

  public void pushSamples(int streamIndex, short[] samples, 
    long timeStamp, TimeUnit timeUnit)
  {
    // verify parameters

    Integer outputIndex = getOutputStreamIndex(streamIndex);
    if (null == outputIndex)
      throw new IllegalArgumentException("unknow stream index: " + streamIndex);
    if (null == samples)
      throw new IllegalArgumentException("NULL input samples");
    if (null == timeUnit)
      throw new IllegalArgumentException("NULL time unit");
    IStreamCoder coder = mStreams.get(outputIndex).getStreamCoder();
    if (CODEC_TYPE_AUDIO != coder.getCodecType())
    {
      throw new IllegalArgumentException("stream[" + streamIndex + 
        "] is not audio");
    }
    if (IAudioSamples.Format.FMT_S16 != coder.getSampleFormat())
    {
      throw new IllegalArgumentException("stream[" + streamIndex + 
        "] is not 16 bit audio");
    }

    // establish the number of samples

    long sampleCount = samples.length / coder.getChannels();


    // create the audio samples object and extract the internal buffer
    // as an array

//     log.debug("sampleCount: " + sampleCount);
//     log.debug("channelCount: " + coder.getChannels());
    IAudioSamples audioFrame = IAudioSamples.make(sampleCount, 
      coder.getChannels());

    audioFrame.setComplete(true, sampleCount, coder.getSampleRate(), 
      coder.getChannels(), coder.getSampleFormat(), 
      MICROSECONDS.convert(timeStamp, timeUnit));

    java.util.concurrent.atomic.AtomicReference<JNIReference> ref = new 
      java.util.concurrent.atomic.AtomicReference<JNIReference>(null);
    IBuffer buffer = audioFrame.getData();
    java.nio.ByteBuffer byteBuffer = buffer.getByteBuffer(0, 
      audioFrame.getSize(), ref);

    byteBuffer.asShortBuffer().put(samples);

    // copy samples into buffer
    
//     for (int i = 0; i < samples.length; ++i)
//       buffer[i] = samples[i];

    // complete audio frame
    
//     audioFrame.setComplete(true, sampleCount, coder.getSampleRate(), 
//       coder.getChannels(), coder.getSampleFormat(), 
//       MICROSECONDS.convert(timeStamp, timeUnit));

    // push out the samples for encoding

     onAudioSamples(this, audioFrame, streamIndex);

    audioFrame.delete();
    buffer.delete();
    ref.get().delete();
  }


  /** {@inheritDoc} */
  
  public void onVideoPicture(IMediaPipe tool, IVideoPicture picture, 
    BufferedImage image, int streamIndex)
  {
    IVideoPicture convertedPicture = null;
    // establish the stream, return silently if no stream returned

    IStream stream = getStream(streamIndex);
    if (null == stream)
      return;

    // if the BufferedImage exists, convert it to a picture
    try {
      if (null != image) {
        convertedPicture = convertToPicture(stream, image, picture.getPts());
        picture = convertedPicture;
      }

      // encode video picture

      encodeVideo(stream, picture);

      // inform listeners

      super.onVideoPicture(this, picture, image, streamIndex);
    } finally {
      if (convertedPicture != null)
        convertedPicture.delete();
    }
  }

  /** 
   * Convert an image to a picture for a given stream.
   * 
   * @param stream to destination stream of the image
   */

  private IVideoPicture convertToPicture(IStream stream, BufferedImage image,
    long timeStamp)
  {
    // lookup the converter

    IConverter videoConverter = mVideoConverters.get(stream);

    // if not found create one

    if (videoConverter == null)
    {
      IStreamCoder coder = stream.getStreamCoder();
      videoConverter = ConverterFactory.createConverter(
        ConverterFactory.findDescriptor(image),
        coder.getPixelType(),
        coder.getWidth(), coder.getHeight(),
        image.getWidth(), image.getHeight());
      mVideoConverters.put(stream, videoConverter);
    }

    // return the converter
    
    return videoConverter.toPicture(image, timeStamp);
  }
  
  /** {@inheritDoc} */
  
  public void onAudioSamples(IMediaPipe tool, IAudioSamples samples, 
    int streamIndex)
  {
    // establish the stream, return silently if no stream returned

    IStream stream = getStream(streamIndex);
    if (null == stream)
      return;

    // encode the audio

    encodeAudio(stream, samples);

    // inform listeners

    super.onAudioSamples(this, samples, streamIndex);
  }
  
  /** 
   * Get the correct {@link IStream} for a given stream index in the
   * container.  If this has been seen before, it
   * is assumed to be a new stream and construct the correct coder for
   * it.
   *
   * @param inputStreamIndex the input index of the stream for which to
   *        find the coder
   * 
   * @return the coder which will be used to encode data for the
   *         specified stream
   */

  private IStream getStream(int inputStreamIndex)
  {
    // the output container must be open

    if (!isOpen())
      open();
    
    // if the output stream index does not exists, create it

    if (null == getOutputStreamIndex(inputStreamIndex))
    {
      // If the header has already been written, then it is too late to
      // establish a new stream, throw, or mask optinally mask, and
      // exception regarding the tardy arival of the new stream

      if (getContainer().isHeaderWritten())
        if (willMaskLateStreamExceptions())
          return null;
        else
          throw new RuntimeException("Input stream index " + inputStreamIndex + 
            " has not been seen before, but the media header has already been " +
            "written.  To mask these exceptions call setMaskLateStreamExceptions()");

      // if an no input container exists, create new a stream from scratch

      if (null == mInputContainer)
      {
        //
        // NOTE: this is where the new stream code will go
        //

        throw new UnsupportedOperationException(
          "MediaWriter can not yet create streams without an input container.");
      }

      // otherwise use the input container as a guide to adding streams
      
      else
      {
        // the input container must be open

        if (!mInputContainer.isOpened())
          throw new RuntimeException(
            "Can't get stream information from a closed input IContainer.");

        // have a look through the input container streams

        for (int i = 0; i < mInputContainer.getNumStreams(); ++i)
        {
          // if input stream index does not map to an output stream
          // index, this is a new stream, add it

          if (null == mOutputStreamIndices.get(i))
            addStreamFromContainer(i);
        }
      }
    }

    // if the header has not been written, do so now
    
    if (!getContainer().isHeaderWritten())
    {
      // if any of the existing coders are not open, open them now

      for (IStream stream: mStreams.values())
        if (!stream.getStreamCoder().isOpen())
          openStream(stream);

      // write the header

      int rv = getContainer().writeHeader();
      if (0 != rv)
        throw new RuntimeException("Error " + IError.make(rv) +
          ", failed to write header to container " + getContainer() +
          " while establishing stream " + 
          mStreams.get(getOutputStreamIndex(inputStreamIndex)));

      // inform the listeners

      super.onWriteHeader(this);
    }
    
    // establish the coder for the output stream index

    IStream stream = mStreams.get(getOutputStreamIndex(inputStreamIndex));
    if (null == stream)
      throw new RuntimeException("invalid input stream index (no stream): "
         + inputStreamIndex);
    IStreamCoder coder = stream.getStreamCoder();
    if (null == coder)
      throw new RuntimeException("invalid input stream index (no coder): "
        + inputStreamIndex);
    
    // return the coder
    
    return stream;
  }

  /**
   * Test if the {@link MediaWriter} can write streams
   * of this {@link ICodec.Type}
   * 
   * @param type the type of codec to be tested
   *
   * @return true if codec type is supported type
   */

  public boolean isSupportedCodecType(ICodec.Type type)
  {
    return (CODEC_TYPE_VIDEO == type || CODEC_TYPE_AUDIO == type);
  }

  /**
   * Construct a stream  using the mInputContainer information.
   *
   * @param inputStreamIndex the index of the stream on the input
   *   container
   * 
   * @return true if the stream was added, false if it's not a supported
   *   stream type
   */

  private boolean addStreamFromContainer(int inputStreamIndex)
  {
    // get the input stream

    IStream inputStream = mInputContainer.getStream(inputStreamIndex);
    IStreamCoder inputCoder = inputStream.getStreamCoder();
    
    // if this stream is not a supported type, indicate failure

    if (!isSupportedCodecType(inputCoder.getCodecType()))
      return false;

    // add a new output stream, based on the id from the input stream
    
    IStream outputStream = getContainer().addNewStream(inputStream.getId());

    // create the coder
    
    IStreamCoder outputCoder = IStreamCoder.make(
      IStreamCoder.Direction.ENCODING, inputCoder);

    // set the codec for output coder

    IContainerFormat format = getContainer().getContainerFormat();
    try
    {
      outputCoder.setCodec(format.establishOutputCodecId(inputCoder
          .getCodecID()));
    }
    finally
    {
      if (format != null)
        format.delete();
    }

    // set output coder on the output stream

    outputStream.setStreamCoder(outputCoder);
    
    // add the new output stream

    addStream(outputStream, inputStreamIndex, outputStream.getIndex());
    
    // indicate success

    return true;
  }

  /**
   * Add a stream.
   */
  
  private void addStream(IStream stream, int inputStreamIndex, 
    int outputStreamIndex)
  {
    // map input to output stream indicies
    
    mOutputStreamIndices.put(inputStreamIndex, outputStreamIndex);

    // get the coder and add it to the index to coder map

    mStreams.put(outputStreamIndex, stream);

    // if this is a video coder, set the quality

    IStreamCoder coder = stream.getStreamCoder();
    if (CODEC_TYPE_VIDEO == coder.getCodecType())
      coder.setFlag(IStreamCoder.Flags.FLAG_QSCALE, true);
    
    // inform listeners

    super.onAddStream(this,outputStreamIndex);
  }
  
  /**
   * Open a newly added stream.
   */

  private void openStream(IStream stream)
  {
    // if the coder is not open, open it NOTE: MediaWriter currently
    // supports audio & video streams
    
    IStreamCoder coder = stream.getStreamCoder();
    try
    {
      ICodec.Type type = coder.getCodecType();
      if (!coder.isOpen() && isSupportedCodecType(type))
      {
        // open the coder

        int rv = coder.open();
        if (rv < 0)
          throw new RuntimeException("could not open stream " + stream + ": "
              + getErrorMessage(rv));
        mOpenedStreams.add(stream);

        // inform listeners
        super.onOpenCoder(this, stream.getIndex());
      }
    }
    finally
    {
      coder.delete();
    }
  }
  
  /**
   *  Encode vidoe and dispatch video packet.
   *
   * @param stream the stream the picture will be encoded onto
   * @param picture the picture to be encoded
   */

  private void encodeVideo(IStream stream, IVideoPicture picture)
  {
    // encode the video packet

    IPacket packet = IPacket.make();
    try {
      if (stream.getStreamCoder().encodeVideo(packet, picture, 0) < 0)
        throw new RuntimeException("failed to encode video");

      if (packet.isComplete())
        writePacket(packet);
    } finally {
      if (packet != null)
        packet.delete();
    }
  }

  /**
   *  Encode audio and dispatch audio packet.
   *
   * @param stream the stream the samples will be encoded onto
   * @param samples the samples to be encoded
   */

  private void encodeAudio(IStream stream, IAudioSamples samples)
  {
    IStreamCoder coder = stream.getStreamCoder();

    // convert the samples into a packet

    for (int consumed = 0; consumed < samples.getNumSamples(); /* in loop */)
    {
      // encode audio

      IPacket packet = IPacket.make();
      try {
        int result = coder.encodeAudio(packet, samples, consumed); 
        if (result < 0)
          throw new RuntimeException("failed to encode audio");

        // update total consumed

        consumed += result;

        // if a complete packed was produced write it out

        if (packet.isComplete())
          writePacket(packet);
      } finally {
        if (packet != null)
          packet.delete();
      }
    }
  }

  /**
   * Write packet to the output container
   * 
   * @param packet the packet to write out
   */

  private void writePacket(IPacket packet)
  {
    if (getContainer().writePacket(packet, mForceInterleave)<0)
      throw new RuntimeException("failed to write packet: " + packet);

    // inform listeners

    super.onWritePacket(this, packet);
  }

  /** 
   * Flush any remaining media data in the media coders.
   */

  public void flush()
  {
    // flush coders

    for (IStream stream: mStreams.values())
    {
      IStreamCoder coder = stream.getStreamCoder();
      if (!coder.isOpen())
        continue;

      // if it's audio coder flush that

      if (CODEC_TYPE_AUDIO == coder.getCodecType())
      {
        IPacket packet = IPacket.make();
        while (coder.encodeAudio(packet, null, 0) >= 0 && packet.isComplete())
        {
          writePacket(packet);
          packet.delete();
          packet = IPacket.make();
        }
        packet.delete();
      }
      
      // else flush video coder

      else if (CODEC_TYPE_VIDEO == coder.getCodecType())
      {
        IPacket packet = IPacket.make();
        while (coder.encodeVideo(packet, null, 0) >= 0 && packet.isComplete())
        {
          writePacket(packet);
          packet.delete();
          packet = IPacket.make();
        }
        packet.delete();
      }
    }

    // flush the container

    getContainer().flushPackets();

    // inform listeners

    super.onFlush(this);
  }

  /** {@inheritDoc} */

  public void open()
  {
    // open the container

    if (getContainer().open(getUrl(), IContainer.Type.WRITE, mContainerFormat,
        true, false) < 0)
      throw new IllegalArgumentException("could not open: " + getUrl());

    // inform listeners

    super.onOpen(this);
    
    // note that we should close the container opened here

    setShouldCloseContainer(true);
  }

  /** {@inheritDoc} */
  
  public void close()
  {
    int rv;

    // flush coders
    
    flush();

    // write the trailer on the output conteiner
    
    if ((rv = getContainer().writeTrailer()) < 0)
      throw new RuntimeException("error " + IError.make(rv) +
        ", failed to write trailer to "
        + getUrl());

    // inform the listeners

    super.onWriteTrailer(this);
    
    // close the coders opened by this MediaWriter

    for (IStream stream: mOpenedStreams)
    {
      IStreamCoder coder = stream.getStreamCoder();
      try
      {
        if ((rv = coder.close()) < 0)
          throw new RuntimeException("error "
              + getErrorMessage(rv)
              + ", failed close coder " + coder);

        // inform the listeners
        super.onCloseCoder(this, stream.getIndex());
      }
      finally
      {
        coder.delete();
      }
    }

    // expunge all referneces to the coders and resamplers
    
    mStreams.clear();
    mOpenedStreams.clear();
    mVideoConverters.clear();

    // if we're supposed to, close the container

    if (getShouldCloseContainer())
    {
      if ((rv = getContainer().close()) < 0)
        throw new RuntimeException("error " + IError.make(rv) +
          ", failed close IContainer " +
          getContainer() + " for " + getUrl());
      setShouldCloseContainer(false);
    }

    // inform the listeners

    super.onClose(this);
  }

  /** {@inheritDoc} */

  public String toString()
  {
    return "MediaWriter[" + getUrl() + "]";
  }

  /** {@inheritDoc} */

  public void onOpen(IMediaPipe tool)
  {
  }

  /** {@inheritDoc} */

  public void onClose(IMediaPipe tool)
  {
    if (isOpen())
      close();
  }

  /** {@inheritDoc} */

  public void onAddStream(IMediaPipe tool, int streamIndex)
  {
  }

  /** {@inheritDoc} */

  public void onOpenCoder(IMediaPipe tool, Integer stream)
  {
  }

  /** {@inheritDoc} */

  public void onCloseCoder(IMediaPipe tool, Integer stream)
  {
  }

  /** {@inheritDoc} */

  public void onReadPacket(IMediaPipe tool, IPacket packet)
  {
  }

  /** {@inheritDoc} */

  public void onWritePacket(IMediaPipe tool, IPacket packet)
  {
  }

  /** {@inheritDoc} */

  public void onWriteHeader(IMediaPipe tool)
  {
  }

  /** {@inheritDoc} */

  public void onFlush(IMediaPipe tool)
  {
  }

  /** {@inheritDoc} */

  public void onWriteTrailer(IMediaPipe tool)
  {
  }
  
  private static String getErrorMessage(int rv)
  {
    String errorString = "";
    IError error = IError.make(rv);
    if (error != null) {
       errorString = error.toString();
       error.delete();
    }
    return errorString;
  }

  /**
   * Get the default pixel type
   * @return the default pixel type
   */
  public IPixelFormat.Type getDefaultPixelType()
  {
    return DEFAULT_PIXEL_TYPE;
  }

  /**
   * Get the default audio sample format
   * @return the format
   */
  public IAudioSamples.Format getDefaultSampleFormat()
  {
    return DEFAULT_SAMPLE_FORMAT;
  }

  /**
   * Get the default time base we'll use on our encoders
   * if one is not specified by the codec.
   * @return the default time base
   */
  public IRational getDefaultTimebase()
  {
    return DEFAULT_TIMEBASE;
  }


}
