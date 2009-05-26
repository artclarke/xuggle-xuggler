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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.awt.image.BufferedImage;

import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IError;
import com.xuggle.xuggler.IPacket;
import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IStreamCoder;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IVideoResampler;
import com.xuggle.xuggler.IContainerFormat;
import com.xuggle.xuggler.video.IConverter;
import com.xuggle.xuggler.video.ConverterFactory;

/**
 * General purpose media reader.
 *
 * <p>
 *
 * The MediaReader class is a simplified interface to the Xuggler
 * library that opens up an {@link IContainer} object, and then every
 * time you call {@link MediaReader#readPacket()}, attempts to decode
 * the packet and call any registered {@link IMediaListener} objects for
 * that packet.
 *
 * </p><p>
 * 
 * The idea is to abstract away the more intricate details of the
 * Xuggler API, and let you concentrate on what you want -- the decoded
 * data.
 *
 * </p> 
 */

public class MediaReader extends AMediaTool
{
  final private Logger log = LoggerFactory.getLogger(this.getClass());
  { log.trace("<init>"); }

  // a map between stream IDs and coders

  protected Map<Integer, IStreamCoder> mCoders = 
    new HashMap<Integer, IStreamCoder>();
  
  // a map between stream IDs and resamplers

  protected Map<Integer, IVideoResampler> mVideoResamplers = 
    new HashMap<Integer, IVideoResampler>();
  
  // all the coders opened by this MediaReader which are candidates for
  // closing

  protected final Collection<IStream> mOpenedStreams = new Vector<IStream>();

  // the type of converter to use, NULL if no conversion should occur

  protected final ConverterFactory.Type mConverterType;

  // true if new streams can may appear during a read packet at any time

  protected boolean mStreamsCanBeAddedDynamically = false;

  //  will reader attempt to establish meta data when container opened

  protected boolean mQueryStreamMetaData = true;

  // video picture converter

  protected IConverter mVideoConverter;

  // close on EOF only

  protected boolean mCloseOnEofOnly = false;

  /**
   * Create a MediaReader which reads and dispatches data from a media
   * stream for a given source URL. The media stream is opened, and
   * subsequent calls to {@link #readPacket()} will read stream content
   * and dispatch it to attached listeners. When the end of the stream
   * is encountered the media container and it's contained streams are
   * all closed.
   * 
   * <p>
   * 
   * No {@link BufferedImage}s are created.  To create {@link
   * BufferedImage}s see {@link #MediaReader(String, String)}.
   * 
   * </p>
   *
   * @param url the location of the media content, a file name will also
   *        work here
   */

  public MediaReader(String url)
  {
    this(url, null);
  }

  /**
   * Create a MediaReader which reads and dispatches data from a media
   * stream for a given source URL, and optionally converts pictures to
   * images.  The media stream is opened, and subsequent calls to {@link
   * #readPacket()} will read stream content and dispatch it to attached
   * listeners.  When the end of the stream is encountered the media
   * container and it's contained streams are all closed.
   *
   * @param url the location of the media content, a file name will also
   *        work here
   * @param converterDescriptor the descriptor of converter to use to
   *        create buffered images; or NULL if no BufferedImages should
   *        be created see {@link
   *        com.xuggle.xuggler.video.ConverterFactory}
   *
   * @throws UnsupportedOperationException if the converter can not be
   *         found
   */

  public MediaReader(String url, String converterDescriptor)
  {
    super(url, IContainer.make());

    // if converterDescriptor is present, create buffered images during
    // video decoding

    if (null != converterDescriptor)
    {
      mConverterType = ConverterFactory 
        .findRegisteredConverter(converterDescriptor);
      if (mConverterType == null)
        throw new UnsupportedOperationException(
          "No converter \"" + converterDescriptor + "\" found.");
    }
    else
      mConverterType = null;
  }

  /**
   * Create a MediaReader which reads and dispatches data from a media
   * container. Calls to {@link #readPacket} will read stream content and
   * dispatch it to attached listeners. If the end of the media stream is
   * encountered, the MediaReader does NOT close the container, that is left to
   * the calling context (you). {@link IStreamCoder}s opened by the MediaReader
   * will be closed by the MediaReader, however {@link IStreamCoder}s opened
   * outside the MediaReader will not be closed. In short MediaReader closes
   * what it opens.
   * 
   * <p>
   * 
   * No {@link BufferedImage}s are created. To create {@link BufferedImage}s see
   * {@link #MediaReader(IContainer, String)}.
   * 
   * </p>
   * 
   * @param container
   *          on already open media container
   */

  public MediaReader(IContainer container)
  {
    this(container, null);
  }

  /**
   * Create a MediaReader which reads and dispatches data from a media
   * container. Calls to {@link #readPacket} will read stream content and
   * dispatch it to attached listeners. If the end of the media stream is
   * encountered, the MediaReader does NOT close the container, that is left to
   * the calling context (you). {@link IStreamCoder}s opened by the MediaReader
   * will be closed by the MediaReader, however {@link IStreamCoder}s outside
   * the MediaReader will not be closed. In short MediaReader closes what it
   * opens.
   * 
   * @param container on already open media container
   * @param converterDescriptor the descriptor of converter to use to
   *        create buffered images; or NULL if no BufferedImages should
   *        be created see {@link
   *        com.xuggle.xuggler.video.ConverterFactory}
   * 
   * @throws UnsupportedOperationException if the converter can not be
   *         found
   */

  public MediaReader(IContainer container, String converterDescriptor)
  {
    super(container.getURL(), container);

    // get dynamic stream add ability

    mStreamsCanBeAddedDynamically = container.canStreamsBeAddedDynamically();

    // if converterDescriptor is present, create buffered images during
    // video decoding

    if (null != converterDescriptor)
    {
      mConverterType = ConverterFactory 
        .findRegisteredConverter(converterDescriptor);
      if (mConverterType == null)
        throw new UnsupportedOperationException(
          "No converter \"" + converterDescriptor + "\" found.");
    }
    else
      mConverterType = null;
  }

  /**
   * Set if the underlying media container supports adding dynamic
   * streams. See {@link IContainer#open(String, IContainer.Type,
   * IContainerFormat, boolean, boolean)} . The default value for this
   * is false.
   * 
   * <p>
   * 
   * If set to false, Xuggler can assume no new streams will be added
   * after {@link #open()} has been called, and may decide to query the
   * entire media file to find all meta data. If true then Xuggler will
   * not read ahead; instead it will only query meta data for a stream
   * when a {@link #readPacket()} returns the first packet in a new
   * stream. Note that a {@link MediaWriter} can only initialize itself
   * from a {@link MediaReader} that has this parameter set to false.
   * 
   * </p>
   * 
   * <p>
   * 
   * To have an effect, the MediaReader must not have been created with
   * an already open {@link IContainer}, and this method must be called
   * before the first call to {@link #readPacket}.
   * 
   * </p>
   * 
   * @param streamsCanBeAddedDynamically
   *          true if new streams may appear at any time during a
   *          {@link #readPacket} call
   * 
   * @throws RuntimeException
   *           if the media container is already open
   */

  public void setAddDynamicStreams(boolean streamsCanBeAddedDynamically)
  {
    if (isOpen())
      throw new RuntimeException("media container is already open");
    mStreamsCanBeAddedDynamically = streamsCanBeAddedDynamically;
  }

  /** 
   * Report if the underlying media container supports adding dynamic
   * streams.  See {@link IContainer#open(String, IContainer.Type,
   * IContainerFormat, boolean, boolean)}.
   * 
   * @return true if new streams can may appear at any time during a
   *         {@link #readPacket} call
   */

  public boolean canAddDynamicStreams()
  {
    return mStreamsCanBeAddedDynamically;
  }

  /** 
   * Set if the underlying media container will attempt to establish all
   * meta data when the container is opened, which will potentially
   * block until it has ready enough data to find all streams in a
   * container.  If false, it will only block to read a minimal header
   * for this container format.  See {@link IContainer#open(String,
   * IContainer.Type, IContainerFormat, boolean, boolean)}.  The default
   * value for this is true.
   *
   * <p>
   * 
   * To have an effect, the MediaReader must not have been created with
   * an already open {@link IContainer}, and this method must be called
   * before the first call to {@link #readPacket}.
   *
   * </p>
   * 
   * @param queryStreamMetaData true if meta data is to be queried
   * 
   * @throws RuntimeException if the media container is already open
   */

  public void setQueryMetaData(boolean queryStreamMetaData)
  {
    if (isOpen())
      throw new RuntimeException("media container is already open");
    
    mQueryStreamMetaData = queryStreamMetaData;
  }

  /** 
   * Report if the underlying media container will attempt to establish
   * all meta data when the container is opened, which will potentially
   * block until it has ready enough data to find all streams in a
   * container.  If false, it will only block to read a minimal header
   * for this container format.  See {@link IContainer#open(String,
   * IContainer.Type, IContainerFormat, boolean, boolean)}.
   * 
   * @return true meta data will be queried
   */

  public boolean willQueryMetaData()
  {
    return mQueryStreamMetaData;
  }

  /** 
   * Set to close, if and only if ERROR_EOF is returned from {@link
   * #readPacket}.  Otherwise close is called when any error value
   * is returned.  The default value for this is false.
   *
   * @param closeOnEofOnly true if meta data is to be queried
   * 
   * @throws RuntimeException if the media container is already open
   */

  public void setCloseOnEofOnly(boolean closeOnEofOnly)
  {
    mCloseOnEofOnly = closeOnEofOnly;
  }

  /** 
   * Report if close will called only if ERROR_EOF is returned from
   * {@link #readPacket}.  Otherwise close is called when any error
   * value is returned.  The default value for this is false.
   * 
   * @return true if will close on ERROR_EOF only
   */

  public boolean willCloseOnEofOnly()
  {
    return mCloseOnEofOnly;
  }

  /** Get the correct {@link IStreamCoder} for a given stream in the
   * container.  If this is a new stream not been seen before, we record
   * it and open it before returning.
   *
   * @param streamIndex the index of the stream to be added
   */

  protected IStreamCoder getStreamCoder(int streamIndex)
  {
    // if the coder does not exists, get it

    IStreamCoder coder = mCoders.get(streamIndex);
    if (coder == null)
    {
      // test valid stream index

      int numStreams = mContainer.getNumStreams();
      if (streamIndex < 0 || streamIndex >= numStreams)
        throw new RuntimeException("invalid stream index");
    
      // estabish all streams in the container, informing listeners of
      // new streams without needing to wait for a packet for each
      // stream to show up

      for(int i = 0; i < numStreams; i++)
      {
        coder = mCoders.get(i);
        if (coder == null) 
        {
          // now get the coder for the given stream index
          
          IStream stream = mContainer.getStream(i);
          coder = stream.getStreamCoder();
          
          // put the coder into the coder map, event if it not a supported
          // type so that on further reads it will find the coder but choose
          // not decode unsupported types
          
          mCoders.put(i, coder);
          for (IMediaListener listener: getListeners())
            listener.onAddStream(this, stream);
        }
      }
    }
    coder = mCoders.get(streamIndex);
    IStream stream = mContainer.getStream(streamIndex);
    ICodec.Type type = coder.getCodecType();

    // if the coder is not open, open it
    // NOTE: MediaReader currently supports audio & video streams

    if (!coder.isOpen()
        && (type == ICodec.Type.CODEC_TYPE_AUDIO || 
          type == ICodec.Type.CODEC_TYPE_VIDEO))
    {
      if (coder.open() < 0)
        throw new RuntimeException("could not open coder for stream: "
            + streamIndex);
      mOpenedStreams.add(stream);
      for (IMediaListener listener : getListeners())
        listener.onOpenStream(this, stream);

    }
    
    return coder;
  }

  /**
   * This decodes the next packet and calls registered {@link IMediaListener}s.
   * 
   * <p>
   * 
   * If a complete {@link IVideoPicture} or {@link IAudioSamples} set are
   * decoded, it will be dispatched to the listeners added to the media reader.
   * 
   * </p>
   * 
   * @return null if there are more packets to read, otherwise return an IError
   *         instance. If {@link com.xuggle.xuggler.IError#getType()} ==
   *         {@link com.xuggle.xuggler.IError.Type#ERROR_EOF} then end of file
   *         has been reached.
   */
    
  public IError readPacket()
  {
    // if the container is not yet been opend, open it

    if (!isOpen())
      open();

    // if there is an off-nominal result from read packet, return the
    // correct error

    IPacket packet = IPacket.make();
    int rv = mContainer.readNextPacket(packet);
    if (rv < 0)
    {
      IError error = IError.make(rv);
      
      // if this is an end of file, or unknow, call close
      
      if (!mCloseOnEofOnly || IError.Type.ERROR_EOF == error.getType())
        close();

      return error;
    }

    // inform listeners that a packet was read

    for (IMediaListener l: getListeners())
      l.onReadPacket(this, packet);

    // get the coder for this packet

    IStreamCoder coder = getStreamCoder(packet.getStreamIndex());

    // decode based on type

    switch (coder.getCodecType())
    {
      // decode audio
      
    case CODEC_TYPE_AUDIO:
      decodeAudio(coder, packet);
      break;

      // decode video

    case CODEC_TYPE_VIDEO:
      decodeVideo(coder, packet);
      break;

      // all other stream types are currently ignored

    default:
    }

    // return true more packets to be read

    return null;
  }
  
  /** Decode and dispatch a video packet.
   *
   * @param videoCoder the video coder
   * @param packet the packet containing the media data
   */

  protected void decodeVideo(IStreamCoder videoCoder, IPacket packet)
  {
    // create a blank video picture
    
    IVideoPicture picture = IVideoPicture.make(videoCoder.getPixelType(),
        videoCoder.getWidth(), videoCoder.getHeight());
    
    // decode the packet into the video picture
    
    int rv = videoCoder.decodeVideo(picture, packet, 0);
    if (rv < 0)
      throw new RuntimeException("error " + IError.make(rv) + " decoding video");
    
    // if this is a complete picture, dispatch the picture
    
    if (picture.isComplete())
      dispatchVideoPicture(packet.getStreamIndex(), picture);
  }

  /** Decode and dispatch a audio packet.
   *
   * @param audioCoder the audio coder
   * @param packet the packet containing the media data
   */

  protected void decodeAudio(IStreamCoder audioCoder, IPacket packet)
  {
    // packet may contain multiple audio frames, decode audio until
    // all audio frames are extracted from the packet 
    
    int offset = 0;
    while (offset < packet.getSize())
    {
      // allocate a set of samples with the correct number of channels
      // and a stock size of 1024 (currently the buffer size will be
      // expanded to 32k to conform to ffmpeg requirements)
          
      IAudioSamples samples = IAudioSamples.make(1024, audioCoder.getChannels());

      // decode audio

      int bytesDecoded = audioCoder.decodeAudio(samples, packet, offset);
      if (bytesDecoded < 0)
        throw new RuntimeException("error " + bytesDecoded + " decoding audio");
      offset += bytesDecoded;

      // if samples are a compelete audio frame, dispatch that frame
      
      if (samples.isComplete())
        dispatchAudioSamples(packet.getStreamIndex(), samples);
    }
  }

  /**
   * Dispatch a decoded {@link IVideoPicture} to attached listeners. This is
   * called when a complete video picture has been decoded from the packet
   * stream. Optionally it will take the steps required to convert the
   * {@link IVideoPicture} to a {@link BufferedImage}. If you wanted to perform
   * a custom conversion, subclass and override this method.
   * 
   * @param streamIndex
   *          the index of the stream
   * @param picture
   *          the video picture to dispatch
   */


  protected void dispatchVideoPicture(int streamIndex, IVideoPicture picture)
  {
    BufferedImage image = null;
    
    // if should create buffered image, do so

    if (null != mConverterType)
    {
      // if the converter is not created, create one

      if (mVideoConverter == null)
        mVideoConverter = ConverterFactory.createConverter(
          mConverterType.getDescriptor(), picture);

      // create the buffered image

      image = mVideoConverter.toImage(picture);
    }
    
    // dispatch picture here

    for (IMediaListener l: getListeners())
      l.onVideoPicture(this, picture, image, streamIndex);
  }

  /**
   * Dispatch {@link IAudioSamples} to attached listeners. This is called when a
   * complete set of {@link IAudioSamples} has been decoded from the packet
   * stream. If you wanted to perform a common custom conversion, subclass and
   * override this method.
   * 
   * @param streamIndex
   *          the index of the stream
   * @param samples
   *          the audio samples to dispatch
   */
  
  protected void dispatchAudioSamples(int streamIndex, IAudioSamples samples)
  {
    for (IMediaListener l: getListeners())
      l.onAudioSamples(this, samples, streamIndex);
  }

  /** {@inheritDoc} */
  
  public void open()
  {
    if (mContainer.open(getUrl(), IContainer.Type.READ, null, 
        mStreamsCanBeAddedDynamically, mQueryStreamMetaData) < 0)
      throw new RuntimeException("could not open: " + getUrl());

    // inform listeners

    for (IMediaListener l: getListeners())
      l.onOpen(this);

    // note that we should close the container opened here

    mCloseContainer = true;
  }

  /** {@inheritDoc} */

  public void close()
  {
    int rv;

    // close the coders opened by this

    for (IStream stream: mOpenedStreams)
    {
      if ((rv = stream.getStreamCoder().close()) < 0)
        throw new RuntimeException("error " + IError.make(rv) +
          ", failed close coder " +
          stream.getStreamCoder());

      // inform listeners that the stream was closed

      for (IMediaListener listener: getListeners())
        listener.onCloseStream(this, stream);
    }
    // expunge all referneces to the coders and resamplers
    
    mCoders.clear();
    mOpenedStreams.clear();
    mVideoResamplers.clear();

    // if we're supposed to, close the container

    if (mCloseContainer)
    {
      if ((rv = mContainer.close()) < 0)
        throw new RuntimeException("error " + IError.make(rv) +
          ", failed close IContainer " +
          mContainer + " for " + getUrl());
      mCloseContainer = false;
    }

    // tell the listeners that the container is closed

    for (IMediaListener l: getListeners())
      l.onClose(this);
  }

  /** {@inheritDoc} */

  public String toString()
  {
    return "MediaReader[" + getUrl() + "]";
  }
}
