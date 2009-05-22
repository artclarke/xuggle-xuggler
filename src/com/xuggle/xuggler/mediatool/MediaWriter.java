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

package com.xuggle.xuggler.mediatool;

import java.util.Map;
import java.util.Vector;
import java.util.HashMap;
import java.util.Collection;

import java.awt.image.BufferedImage;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

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
import com.xuggle.xuggler.mediatool.MediaReader;
import com.xuggle.xuggler.video.ConverterFactory;

import static com.xuggle.xuggler.ICodec.Type.CODEC_TYPE_VIDEO;
import static com.xuggle.xuggler.ICodec.Type.CODEC_TYPE_AUDIO;

/**
 * General purpose media writer.
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
 * 
 * Calls to {@link #onAudioSamples}, and {@link #onVideoPicture} encode
 * media into packets and write those encoded packets.
 * 
 * </p>
 * <p>
 * 
 * When {@link #onAudioSamples} or {@link #onVideoPicture} is called a
 * stream index is specified. The only requirement of these stream
 * indices is that they consistently map to specific streams.
 * 
 * </p>
 * <p>
 * 
 * The idea is to abstract away the more intricate details of the
 * Xuggler API, and let you concentrate on what you want.
 * 
 * </p>
 */

public class MediaWriter extends AMediaTool implements IMediaListener
{
  final private Logger log = LoggerFactory.getLogger(this.getClass());
  { log.trace("<init>"); }

  /** The default pixel type. */

  public static final IPixelFormat.Type DEFAULT_PIXEL_TYPE = 
    IPixelFormat.Type.YUV420P;

  /** The default sample format. */

  public static final IAudioSamples.Format DEFAULT_SAMPLE_FORMAT = 
    IAudioSamples.Format.FMT_S16;

  /** The default time base. */

  public static final IRational DEFAULT_TIMEBASE = IRational.make(
    1, (int)Global.DEFAULT_PTS_PER_SECOND);

  // the input container of packets
  
  protected final IContainer mInputContainer;

  // the container format

  protected IContainerFormat mContainerFormat;

  // a map between input stream indicies to output stream indicies

  protected Map<Integer, Integer> mOutputStreamIndices = 
    new HashMap<Integer, Integer>();

  // a map between output stream indicies and streams

  protected Map<Integer, IStream> mStreams = 
    new HashMap<Integer, IStream>();

  // a map between output stream indicies and video converters

  protected Map<Integer, IConverter> mVideoConverters = 
    new HashMap<Integer, IConverter>();
  
  // streasm opened by this MediaWriter must be closed

  protected final Collection<IStream> mOpenedStreams = new Vector<IStream>();

  // true if the writer should ask FFMPEG to interleave media

  protected boolean mForceInterleave = true;

  // mask late stream exception policy
  
  protected boolean mMaskLateStreamException = false;


  /**
   * Use a specified {@link MediaReader} as a source for media data and
   * meta data about the container and it's streams.  The {@link
   * MediaReader} must be cofigured such that streams will not be
   * dynamically added to the container, which is the defaul for {@link
   * MediaReader}.
   * 
   * <p>
   *
   * This MediaWriter will be added as a listener to the {@link
   * MediaReader}.  Once this constructer has returned, calles to {@link
   * MediaReader#readPacket} will effectivy transcode the media.
   *
   * </p>
   *
   * @param url the url or filename of the media destination
   * @param reader the media source
   * 
   * @throws IllegalArgumentException if the specifed {@link
   *         MediaReader} is configure to allow dynamic adding of
   *         streams.
   */

  public MediaWriter(String url, MediaReader reader)
  {
    this(url, reader, true);
  }


  /**
   * Use a specified {@link MediaReader} as a source for media data and
   * meta data about the container and it's streams.  The {@link
   * MediaReader} must be cofigured such that streams will not be
   * dynamically added to the container.  This is the defaul for {@link
   * MediaReader}.
   * 
   * <p>
   *
   * The MediaWriter may optionally be added as a listener to the {@link
   * MediaReader}.  If added as a listener, and once this constructer
   * has returned, calles to {@link MediaReader#readPacket} will
   * effectivy transcode the media.
   *
   * </p>
   *
   * @param url the url or filename of the media destination
   * @param reader the media source
   * @param addAsListener add this writer to the reader as a listener
   * 
   * @throws IllegalArgumentException if the specifed {@link
   *         MediaReader} is configure to allow dynamic adding of
   *         streams.
   */

  public MediaWriter(String url, MediaReader reader, boolean addAsListener)
  {
    // construct around the source container

    this(url, reader.getContainer());

    // if the container can adde streams dynamically, it is not
    // currently supported, throw an exception.  this kind of test needs
    // to be done both here and in the consructor which takes a
    // container because the MediaReader may not have opened it's
    // internal container and thus not set this flag yet

    if (reader.canAddDynamicStreams())
      throw new IllegalArgumentException(
        "inputContainer is improperly configured to allow " + 
        "dynamic adding of streams.");

    // optionally add this writer as a listener to the reader
    
    if (addAsListener)
      reader.addListener(this);
  }

  /**
   * Use a specified {@link IContainer} as a source for and meta data
   * about the container and it's streams.  The {@link IContainer} must
   * be cofigured such that streams will not be dynamically added to the
   * container.
   * 
   * <p>
   *
   * To write data call to {@link #onAudioSamples} and/or {@link
   * #onVideoPicture}.
   *
   * </p>
   *
   * @param url the url or filename of the media destination
   * @param inputContainer the source media container
   * 
   * @throws IllegalArgumentException if the specifed {@link IContainer}
   *         is not a of type READ or is configure to allow dynamic
   *         adding of streams.
   */

  public MediaWriter(String url, IContainer inputContainer)
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

  public MediaWriter(String url)
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

  public IStream addAudioStream(int inputIndex, int streamId, ICodec codec,
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
    coder.setSampleFormat(DEFAULT_SAMPLE_FORMAT);

    // return the new audio stream

    return stream;
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

  public IStream addVideoStream(int inputIndex, int streamId, ICodec codec,
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
    coder.setPixelType(DEFAULT_PIXEL_TYPE);

    // return the new video stream

    return stream;
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

  protected IStream establishStream(int inputIndex, int streamId, ICodec codec)
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

    IStream stream = mContainer.addNewStream(streamId);
    if (stream == null)
      throw new RuntimeException("Unable to create stream id " + streamId +
        ", index " + inputIndex + ", codec " + codec);
    
    // configure the stream coder

    IStreamCoder coder = stream.getStreamCoder();
    coder.setTimeBase(DEFAULT_TIMEBASE);
    coder.setCodec(codec);

    // add the stream to the media writer
    
    addStream(stream, inputIndex, stream.getIndex());

    // if the stream count is 1, don't force interleave

    setForceInterleave(mContainer.getNumStreams() != 1);

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

  /** {@inheritDoc} */
  
  public void onVideoPicture(IMediaTool tool, IVideoPicture picture, 
    BufferedImage image, int streamIndex)
  {
    // establish the coder, return silently if no coder returned

    IStreamCoder coder = getStreamCoder(streamIndex);
    if (null == coder)
      return;
      
    // if the BufferedImage exists use that

    if (null != image)
    {
      // find or create a video converter

      IConverter videoConverter = mVideoConverters.get(streamIndex);
      if (videoConverter == null)
      {
        videoConverter = ConverterFactory.createConverter(
          ConverterFactory.findDescriptor(image),
          coder.getPixelType(),
          coder.getWidth(), coder.getHeight(),
          image.getWidth(), image.getHeight());
        mVideoConverters.put(streamIndex, videoConverter);
      }

      // convert image

      picture = videoConverter.toPicture(image, picture.getPts());
    }

    // encode video picture
    
    encodeVideo(coder, picture);

    // inform listeners

    for (IMediaListener listener: getListeners())
      listener.onVideoPicture(this, picture, image, streamIndex);
  }
  
  /** {@inheritDoc} */
  
  public void onAudioSamples(IMediaTool tool, IAudioSamples samples, int streamIndex)
  {
    // establish the coder, return silently if no coder returned

    IStreamCoder coder = getStreamCoder(streamIndex);
    if (null == coder)
      return;
      
    // encode the audio

    encodeAudio(coder, samples);

    // inform listeners

    for (IMediaListener listener: getListeners())
      listener.onAudioSamples(this, samples, streamIndex);
  }
  
  /** 
   * Get the correct {@link IStreamCoder} for a given stream in the
   * container.  If this is a new stream, which not been seen before, it
   * is assumed to be a new stream and construct the correct coder for
   * it.
   *
   * @param inputStreamIndex the input index of the stream for which to
   *        find the coder
   * 
   * @return the coder which will be used to encode data for the
   *         specified stream
   */

  protected IStreamCoder getStreamCoder(int inputStreamIndex)
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

      if (mContainer.isHeaderWritten())
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
    
    if (!mContainer.isHeaderWritten())
    {
      // if any of the existing coders are not open, open them now

      for (IStream stream: mStreams.values())
        if (!stream.getStreamCoder().isOpen())
          openStream(stream);

      // write the header

      int rv = mContainer.writeHeader();
      if (0 != rv)
        throw new RuntimeException("Error " + IError.make(rv) +
          ", failed to write header to container " + getContainer() +
          " while establishing stream " + 
          mStreams.get(getOutputStreamIndex(inputStreamIndex)));

      // inform the listeners

      for (IMediaListener l: getListeners())
        l.onWriteHeader(this);
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
    
    return coder;
  }

  /**
   * Make the best effort to establish the ouput codec id for a given
   * input codec and output container format.  This method relies on
   * FFMPEGs internal database of codec IDs to identify the correct
   * output codec ID.
   *
   * @param inputCodec the input codec
   * @param outputContainerFormat the format of the output container
   *
   * @return the best guess output codec ID, or null if none found
   */

  public static ICodec.ID establishOutputCodecId(ICodec inputCodec,
    IContainerFormat outputContainerFormat)
  {
    return establishOutputCodecId(inputCodec, outputContainerFormat);
  }

  /**
   * Make the best effort to establish the ouput codec id for a given
   * input codec id and output container format.  This method relies on
   * FFMPEGs internal database of codec IDs to identify the correct
   * output codec ID.
   *
   * @param inputCodecId the ID input codec
   * @param outputContainerFormat the format of the output container
   *
   * @return the best guess output codec ID, or null if none found
   */

  public static ICodec.ID establishOutputCodecId(ICodec.ID inputCodecId, 
    IContainerFormat outputContainerFormat)
  {
    // test parameteres

    if (null == inputCodecId)
      throw new IllegalArgumentException("null input codec id");
    if (ICodec.ID.CODEC_ID_NONE == inputCodecId)
      throw new IllegalArgumentException("input codec id is \"NONE\"");
    if (null == outputContainerFormat)
      throw new IllegalArgumentException("null output container format");
    if (!outputContainerFormat.isOutput())
      throw new IllegalArgumentException(
        "passed output container format, actally an input container format");

    // if the output container supports in teh input codec, and can
    // encode, return the input codec

    if (outputContainerFormat.isCodecSupportedForOutput(inputCodecId) &&
      ICodec.findEncodingCodec(inputCodecId).canEncode())
    {
      return inputCodecId;
    }

    // establish the input codec type

    ICodec.Type inputCodecType = ICodec.findDecodingCodec(inputCodecId)
      .getType();

    // the would be output codec 

    ICodec.ID outputCodecId = null;

    // find the default codec for the output container by input codec type
    
    switch (inputCodecType)
    {
    case CODEC_TYPE_AUDIO:
      outputCodecId = outputContainerFormat.getOutputDefaultAudioCodec();
      break;
    case CODEC_TYPE_VIDEO:
      outputCodecId = outputContainerFormat.getOutputDefaultVideoCodec();
      break;
    case CODEC_TYPE_SUBTITLE:
      outputCodecId = outputContainerFormat.getOutputDefaultSubtitleCodec();
      break;
    }

    // if there still isn't a valid codec, hunt through all the codecs
    // for the output format and see if ANY match the input codec type

    if (null == outputCodecId || ICodec.ID.CODEC_ID_NONE == outputCodecId ||
      !ICodec.findEncodingCodec(inputCodecId).canEncode())
    {
      for(ICodec.ID codecId: outputContainerFormat.getOutputCodecsSupported())
      {
        ICodec codec = ICodec.findEncodingCodec(codecId);
        if (codec.getType() == inputCodecType)
        {
          // if it is a valid codec break out of the search

          outputCodecId = codec.getID();
          if (null != outputCodecId && 
            ICodec.ID.CODEC_ID_NONE != outputCodecId || 
            ICodec.findEncodingCodec(inputCodecId).canEncode())
          {
            break;
          }
        }
      }
    }

    // return found ouput codec id, or null if not found

    return outputCodecId;
  }

  /**
   * Test if a given codec type is supported by the media writer.
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

  protected boolean addStreamFromContainer(int inputStreamIndex)
  {
    // get the input stream

    IStream inputStream = mInputContainer.getStream(inputStreamIndex);
    IStreamCoder inputCoder = inputStream.getStreamCoder();
    
    // if this stream is not a supported type, indicate failure

    if (!isSupportedCodecType(inputCoder.getCodecType()))
      return false;

    // add a new output stream, based on the id from the input stream
    
    IStream outputStream = mContainer.addNewStream(inputStream.getId());

    // create the coder
    
    IStreamCoder outputCoder = IStreamCoder.make(
      IStreamCoder.Direction.ENCODING, inputCoder);

    // set the codec for output coder

    outputCoder.setCodec(establishOutputCodecId(inputCoder.getCodecID(), 
        mContainer.getContainerFormat()));

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
  
  protected void addStream(IStream stream, int inputStreamIndex, 
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

    for (IMediaListener listener: getListeners())
      listener.onAddStream(this, stream);
  }
  
  /**
   * Open a newly added stream.
   */

  protected void openStream(IStream stream)
  {
    // if the coder is not open, open it NOTE: MediaWriter currently
    // supports audio & video streams
    
    IStreamCoder coder = stream.getStreamCoder();
    ICodec.Type type = coder.getCodecType();
    if (!coder.isOpen() && isSupportedCodecType(type))
    {
      // open the coder
      
      int rv = coder.open();
      if (rv < 0)
        throw new RuntimeException("could not open stream " + stream
          + ": " + IError.make(rv));
      mOpenedStreams.add(stream);
      
      // inform listeners

      for (IMediaListener listener: getListeners())
        listener.onOpenStream(this, stream);
    }
  }
  
  /**
   * Encode and dispatch a video packet.
   *
   * @param videoCoder the video coder
   */

  protected void encodeVideo(IStreamCoder videoCoder, IVideoPicture picture)
  {
    // encode the video packet

    IPacket packet = IPacket.make();
    if (videoCoder.encodeVideo(packet, picture, 0) < 0)
      throw new RuntimeException("failed to encode video");

    if (packet.isComplete())
      writePacket(packet);
  }

  /** Encode and dispatch a audio packet.
   *
   * @param audioCoder the audio coder
   */

  protected void encodeAudio(IStreamCoder audioCoder, IAudioSamples samples)
  {
    // convert the samples into a packet

    IPacket packet = null;
    for (int consumed = 0; consumed < samples.getNumSamples(); /* in loop */)
    {
      // if null, create packet

      if (null == packet)
        packet = IPacket.make();

      // encode audio

      int result = audioCoder.encodeAudio(packet, samples, consumed); 
      if (result < 0)
        throw new RuntimeException("failed to encode audio");
      consumed += result;

      // if a complete packed was produced write it out

      if (packet.isComplete())
      {
        writePacket(packet);
        packet = null;
      }
    }
  }

  /**
   * Write packet to the output container
   * 
   * @param packet the packet to write out
   */

  protected void writePacket(IPacket packet)
  {
    if (mContainer.writePacket(packet, mForceInterleave)<0)
      throw new RuntimeException("failed to write packet: " + packet);

    // inform listeners

    for (IMediaListener listener: getListeners())
      listener.onWritePacket(this, packet);
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
          packet = IPacket.make();
        }
      }
      
      // else flush video coder

      else if (CODEC_TYPE_VIDEO == coder.getCodecType())
      {
        IPacket packet = IPacket.make();
        while (coder.encodeVideo(packet, null, 0) >= 0 && packet.isComplete())
        {
          writePacket(packet);
          packet = IPacket.make();
        }
      }
    }

    // flush the container

    mContainer.flushPackets();

    // inform listeners

    for (IMediaListener listener: getListeners())
      listener.onFlush(this);
  }

  /** {@inheritDoc} */

  public void open()
  {
    // open the container

    if (mContainer.open(getUrl(), IContainer.Type.WRITE, mContainerFormat,
        true, false) < 0)
      throw new IllegalArgumentException("could not open: " + getUrl());

    // inform listeners

    for (IMediaListener listener: getListeners())
      listener.onOpen(this);

    // note that we should close the container opened here

    mCloseContainer = true;
  }

  /** {@inheritDoc} */
  
  public void close()
  {
    int rv;

    // flush coders
    
    flush();

    // write the trailer on the output conteiner
    
    if ((rv = mContainer.writeTrailer()) < 0)
      throw new RuntimeException("error " + IError.make(rv) +
        ", failed to write trailer to "
        + getUrl());

    // inform the listeners

    for (IMediaListener l: getListeners())
      l.onWriteTrailer(this);

    // close the coders opened by this MediaWriter

    for (IStream stream: mOpenedStreams)
    {
      if ((rv = stream.getStreamCoder().close()) < 0)
        throw new RuntimeException("error " + IError.make(rv) +
          ", failed close coder " +
          stream.getStreamCoder());

      // inform the listeners
      
      for (IMediaListener l: getListeners())
        l.onCloseStream(this, stream);
    }

    // expunge all referneces to the coders and resamplers
    
    mStreams.clear();
    mOpenedStreams.clear();
    mVideoConverters.clear();

    // if we're supposed to, close the container

    if (mCloseContainer)
    {
      if ((rv = mContainer.close()) < 0)
        throw new RuntimeException("error " + IError.make(rv) +
          ", failed close IContainer " +
          mContainer + " for " + getUrl());
      mCloseContainer = false;
    }

    // inform the listeners

    for (IMediaListener l: getListeners())
      l.onClose(this);
  }

  /** {@inheritDoc} */

  public String toString()
  {
    return "MediaWriter[" + getUrl() + "]";
  }

  /** {@inheritDoc} */

  public void onOpen(IMediaTool tool)
  {
  }

  /** {@inheritDoc} */

  public void onClose(IMediaTool tool)
  {
    if (isOpen())
      close();
  }

  /** {@inheritDoc} */

  public void onAddStream(IMediaTool tool, IStream stream)
  {
  }

  /** {@inheritDoc} */

  public void onOpenStream(IMediaTool tool, IStream stream)
  {
  }

  /** {@inheritDoc} */

  public void onCloseStream(IMediaTool tool, IStream stream)
  {
  }

  /** {@inheritDoc} */

  public void onReadPacket(IMediaTool tool, IPacket packet)
  {
  }

  /** {@inheritDoc} */

  public void onWritePacket(IMediaTool tool, IPacket packet)
  {
  }

  /** {@inheritDoc} */

  public void onWriteHeader(IMediaTool tool)
  {
  }

  /** {@inheritDoc} */

  public void onFlush(IMediaTool tool)
  {
  }

  /** {@inheritDoc} */

  public void onWriteTrailer(IMediaTool tool)
  {
  }
}
