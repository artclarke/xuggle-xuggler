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

package com.xuggle.xuggler.mediatool;

import java.util.Map;
import java.util.HashMap;
import java.util.Vector;
import java.util.Collection;

import java.awt.image.BufferedImage;
import java.awt.Dimension;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.Global;
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

/**
 * General purpose media writer.
 *
 * <p>
 * 
 * The MediaWriter class is a simplified interface to the Xuggler
 * library that opens up an {@link IContainer} object, and allows media
 * data to be writen into it.  Calls to {@link #onAudioSamples}, and
 * {@link #onVideoPicture} encode media into packets and dispatch those
 * packets.
 *
 * </p>
 * <p>
 *
 * When {@link #onAudioSamples} or {@link #onVideoPicture} is called a
 * stream index is specifed.  The only requirement of these indicies is
 * that they consistantly map to specific streams.  If a new index is
 * encountered a new stream will be created.
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

  /** The default pxiel type. */

  public static final IPixelFormat.Type DEFAULT_PIXEL_TYPE = 
    IPixelFormat.Type.YUV420P;

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

  // a map between output stream indicies and coders

  protected Map<Integer, IStreamCoder> mCoders = 
    new HashMap<Integer, IStreamCoder>();

  // a map between output stream indicies and video converters

  protected Map<Integer, IConverter> mVideoConverters = 
    new HashMap<Integer, IConverter>();
  
  // streasm opened by this MediaWriter must be closed

  protected final Collection<IStream> mOpenedStreams = new Vector<IStream>();

  // true if the writer should ask FFMPEG to interleave media

  protected boolean mForceInterleave = true;

  /**
   * Use a specified {@link MediaReader} as a source for media data and
   * meta data about the container and it's streams.  The {@link
   * MediaReader} must be cofigured such that streams will not be
   * dynamically added to the container.  This is the defaul for {@link
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

    // this writer as a listener to the reader
    
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
    // if the container is not opened, do so

    if (!isOpen())
      open();

    // validate parameteres and conditions

    if (width <= 0 || height <= 0)
      throw new IllegalArgumentException(
        "invalid video frame size [" + width + " x " + height + "]");

    // add the new stream at the correct index

    IStream stream = addStream(inputIndex, streamId, codec);
    
    // configre the stream coder

    IStreamCoder coder = stream.getStreamCoder();
    coder.setWidth(width);
    coder.setHeight(height);
    coder.setPixelType(DEFAULT_PIXEL_TYPE);

    openStream(stream, inputIndex, stream.getIndex());

    mOutputStreamIndices.put(inputIndex, stream.getIndex());

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

  protected IStream addStream(int inputIndex, int streamId, ICodec codec)
  {
    // validate parameteres and conditions

    if (inputIndex < 0)
      throw new IllegalArgumentException("invalid input index " + inputIndex);
    if (streamId < 0)
      throw new IllegalArgumentException("invalid stream id " + streamId);
    if (null == codec)
      throw new IllegalArgumentException("null codec");
    if (!isOpen())
      throw new IllegalArgumentException(
        "output IContainer is not open, new streams may not be added");

    // add the new stream at the correct index

    IStream stream = mContainer.addNewStream(streamId);
    if (stream == null)
      throw new RuntimeException("Unable to create stream id " + streamId +
        ", index " + inputIndex + ", codec " + codec);
    
    // configure the stream coder

    IStreamCoder coder = stream.getStreamCoder();
    coder.setTimeBase(DEFAULT_TIMEBASE);
    coder.setCodec(codec);

    // if the stream count is 1, don't force interleave

    setForceInterleave(mContainer.getNumStreams() != 1);

    // return the new video stream

    return stream;
  }

  /**
   * Test if the MediaWriter will force FFMPEG to interleave media data.
   * The default value for this value is true.
   *
   * @return true if MediaWriter forces ffmepg to interleave media data.
   *
   * @see #setForceInterleave
   */

  public boolean willForceInterleave()
  {
    return mForceInterleave;
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
   * If true MediaWriter will ask FFMPEG to place media data in time
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
    IStreamCoder coder = getStreamCoder(streamIndex);

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
    // encode the audio

    encodeAudio(getStreamCoder(streamIndex), samples);

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
            "Attempting to extract stream information from a closed input IContainer.");

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

    // if the header has not been writen, do so now
    
    if (!mContainer.isHeaderWritten())
    {
      int errorNo = mContainer.writeHeader();
      if (0 != errorNo)
        throw new RuntimeException("Error " + errorNo +
          ", failed to write header to " + getUrl());

      // inform the listeners

      for (IMediaListener l: getListeners())
        l.onWriteHeader(this);
    }
    
    // establish the coder for the output stream index

    IStreamCoder coder = mCoders.get(getOutputStreamIndex(inputStreamIndex));
    if (null == coder)
      throw new RuntimeException("invalid input stream index: "
        + inputStreamIndex);
    
    // return the coder
    
    return coder;
  }

  /**
   * Construct a stream  using the mInputContainer information.
   */

  protected void addStreamFromContainer(int inputStreamIndex)
  {
    // add a new output stream, based on the id from the input
    // stream
    
    IStream stream = mContainer.addNewStream(
      mInputContainer.getStream(inputStreamIndex).getId());

    // create the coder
    
    IStreamCoder newCoder = IStreamCoder.make(IStreamCoder.Direction.ENCODING,
      mInputContainer.getStream(inputStreamIndex).getStreamCoder());

    // an stick the coder in the stream

    stream.setStreamCoder(newCoder);

    // open the new stream

    openStream(stream, inputStreamIndex, stream.getIndex());
  }

  /**
   * Open a newly added stream.
   */

  protected void openStream(IStream stream, int inputStreamIndex, 
    int outputStreamIndex)
  {
    // map input to output stream indicies and output index to coder
    
    mOutputStreamIndices.put(inputStreamIndex, outputStreamIndex);
  
    // get the coder and add it to the index to coder map

    IStreamCoder coder = stream.getStreamCoder();
    mCoders.put(outputStreamIndex, coder);
    
    // if the coder is not open, open it 
    // NOTE: MediaWriter currently supports audio & video streams
    
    ICodec.Type type = coder.getCodecType();
    if (!coder.isOpen() && (ICodec.Type.CODEC_TYPE_AUDIO == type ||
        ICodec.Type.CODEC_TYPE_VIDEO == type))
    {
      if (ICodec.Type.CODEC_TYPE_VIDEO == type)
        coder.setFlag(IStreamCoder.Flags.FLAG_QSCALE, true);

      if (coder.open() < 0)
        throw new RuntimeException("could not open coder " + coder + 
          " for stream " + stream);
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
    if (videoCoder.encodeVideo(packet, picture, -1) == -1)
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
      if (-1 == result)
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
    if (-1 == mContainer.writePacket(packet, mForceInterleave))
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

    for (IStreamCoder coder: mCoders.values())
    {
      // if it's audio coder flush that

      if (ICodec.Type.CODEC_TYPE_AUDIO == coder.getCodecType())
      {
        IPacket packet = IPacket.make();
        while (coder.encodeAudio(packet, null, 0) >= 0 && packet.isComplete())
        {
          writePacket(packet);
          packet = IPacket.make();
        }
      }
      
      // else flush video coder

      else if (ICodec.Type.CODEC_TYPE_VIDEO == coder.getCodecType())
      {
        IPacket packet = IPacket.make();
        while (coder.encodeVideo(packet, null, -1) >= 0 && packet.isComplete())
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
      throw new RuntimeException("error " + rv + ", failed to write trailer to "
        + getUrl());

    // inform the listeners

    for (IMediaListener l: getListeners())
      l.onWriteTrailer(this);

    // close the coders opened by this MediaWriter

    for (IStream stream: mOpenedStreams)
    {
      if ((rv = stream.getStreamCoder().close()) < 0)
        throw new RuntimeException("error " + rv + ", failed close coder " +
          stream.getStreamCoder());

      // inform the listeners
      
      for (IMediaListener l: getListeners())
        l.onCloseStream(this, stream);
    }

    // expunge all referneces to the coders and resamplers
    
    mCoders.clear();
    mOpenedStreams.clear();
    mVideoConverters.clear();

    // if we're supposed to, close the container

    if (mCloseContainer)
    {
      if ((rv = mContainer.close()) < 0)
        throw new RuntimeException("error " + rv + ", failed close IContainer " +
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
