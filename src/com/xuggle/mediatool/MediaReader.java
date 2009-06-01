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
import com.xuggle.xuggler.IContainerFormat;
import com.xuggle.xuggler.video.IConverter;
import com.xuggle.xuggler.video.ConverterFactory;

/**
 * Opens a media container, and decodes all audio and video in the container.
 *
 * <p>
 *
 * The MediaReader class is a simplified interface to the Xuggler
 * library that opens up an {@link IContainer} object, and then every
 * time you call {@link MediaReader#readPacket()}, attempts to decode
 * the packet and call any registered {@link IMediaPipeListener} objects for
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

class MediaReader extends AMediaTool implements IMediaReader
{
  final private Logger log = LoggerFactory.getLogger(this.getClass());
  { log.trace("<init>"); }

  // a map between stream IDs and coders

  private Map<Integer, IStreamCoder> mCoders = 
    new HashMap<Integer, IStreamCoder>();
  
  // a map between stream IDs and resamplers

  // all the coders opened by this MediaReader which are candidates for
  // closing

  private final Collection<IStream> mOpenedStreams = new Vector<IStream>();

  // the type of converter to use, NULL if no conversion should occur

  private ConverterFactory.Type mConverterType;

  // true if new streams can may appear during a read packet at any time

  private boolean mStreamsCanBeAddedDynamically = false;

  //  will reader attempt to establish meta data when container opened

  private boolean mQueryStreamMetaData = true;

  // video picture converter

  private IConverter mVideoConverter;

  // close on EOF only

  private boolean mCloseOnEofOnly = false;

  // What buffered image type do people want us to produce
  // -1 disables
  
  private int mBufferedImageType = -1;

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

  MediaReader(String url)
  {
    super(url, IContainer.make());
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

  MediaReader(IContainer container)
  {
    super(container.getURL(), container);

    // get dynamic stream add ability

    mStreamsCanBeAddedDynamically = container.canStreamsBeAddedDynamically();
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
   * Asks the {@link IMediaReader} to generate {@link BufferedImage}
   * images when calling {@link IMediaPipeListener#onVideoPicture(IMediaPipe, IVideoPicture, BufferedImage, long, TimeUnit, int)}.
   * 
   * @param bufferedImageType The buffered image type (e.g.
   *   {@link BufferedImage#TYPE_3BYTE_BGR}).  Set to -1 to disable
   *   this feature.
   *   
   * @see BufferedImage
   * @throws RuntimeException if the media container has been opened you
   *   cannot change this setting.
   */

  public void setBufferedImageTypeToGenerate(int bufferedImageType)
  {
    if (isOpen())
      throw new RuntimeException("media container is already open");
    if (bufferedImageType >= 0 &&
        bufferedImageType != BufferedImage.TYPE_3BYTE_BGR)
      // can remove this once the any converter is created
      throw new RuntimeException("Only BufferedImage.TYPE_3BYTE_BGR supported: "+
          bufferedImageType);
    mBufferedImageType = bufferedImageType;
  }
  
  /**
   * Get the {@link BufferedImage} type this {@link IMediaReader} will
   * generate.
   * @return the type, or -1 if disabled.
   * 
   * @see #getBufferedImageTypeToGenerate()
   */
  public int getBufferedImageTypeToGenerate()
  {
    return mBufferedImageType;
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

  private IStreamCoder getStreamCoder(int streamIndex)
  {
    // if the coder does not exists, get it

    IStreamCoder coder = mCoders.get(streamIndex);
    if (coder == null)
    {
      // test valid stream index

      int numStreams = getContainer().getNumStreams();
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
          
          IStream stream = getContainer().getStream(i);
          try
          {
            coder = stream.getStreamCoder();

            // put the coder into the coder map, event if it not a supported
            // type so that on further reads it will find the coder but choose
            // not decode unsupported types

            mCoders.put(i, coder);
            // and release our coder to the list
            coder = null;
            super.onAddStream(this, i);
          }
          finally
          {
            if (coder != null) coder.delete();
            if (stream != null)
              stream.delete();
          }
        }
      }
    }
    coder = mCoders.get(streamIndex);
    IStream stream = getContainer().getStream(streamIndex);
    try
    {
      ICodec.Type type = coder.getCodecType();

      // if the coder is not open, open it
      // NOTE: MediaReader currently supports audio & video streams

      if (!coder.isOpen()
          && (type == ICodec.Type.CODEC_TYPE_AUDIO || type == ICodec.Type.CODEC_TYPE_VIDEO))
      {
        if (coder.open() < 0)
          throw new RuntimeException("could not open coder for stream: "
              + streamIndex);
        mOpenedStreams.add(stream);
        super.onOpenCoder(this, stream.getIndex());
        stream = null;
      }
    } finally {
      if (stream != null)
        stream.delete();
    }
    // return back the reference we had
    return coder;
  }

  /**
   * This decodes the next packet and calls registered {@link IMediaPipeListener}s.
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
    try
    {
      int rv = getContainer().readNextPacket(packet);
      if (rv < 0)
      {
        IError error = IError.make(rv);

        // if this is an end of file, or unknow, call close

        if (!mCloseOnEofOnly || IError.Type.ERROR_EOF == error.getType())
          close();

        return error;
      }

      // inform listeners that a packet was read

      super.onReadPacket(this, packet);

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

      
    }
    finally
    {
      if (packet != null)
        packet.delete();
    }

    // return true more packets to be read

    return null;
  }
  
  /** Decode and dispatch a video packet.
   *
   * @param videoCoder the video coder
   * @param packet the packet containing the media data
   */

  private void decodeVideo(IStreamCoder videoCoder, IPacket packet)
  {
    // create a blank video picture
    
    IVideoPicture picture = IVideoPicture.make(videoCoder.getPixelType(),
        videoCoder.getWidth(), videoCoder.getHeight());
    try {
      // decode the packet into the video picture

      int rv = videoCoder.decodeVideo(picture, packet, 0);
      if (rv < 0)
        throw new RuntimeException("error " + getErrorMessage(rv)
            + " decoding video");

      // if this is a complete picture, dispatch the picture

      if (picture.isComplete())
        dispatchVideoPicture(packet.getStreamIndex(), picture);
    } finally {
      if (picture != null) picture.delete();
    }
  }

  /** Decode and dispatch a audio packet.
   *
   * @param audioCoder the audio coder
   * @param packet the packet containing the media data
   */

  private void decodeAudio(IStreamCoder audioCoder, IPacket packet)
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
      try {
        if (samples.isComplete())
          dispatchAudioSamples(packet.getStreamIndex(), samples);
      } finally {
        if (samples != null)
          samples.delete();
      }
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


  private void dispatchVideoPicture(int streamIndex, IVideoPicture picture)
  {
    BufferedImage image = null;
    
    // if should create buffered image, do so

    if (mBufferedImageType >= 0)
    {
      if (mConverterType == null) {
        mConverterType = ConverterFactory 
        .findRegisteredConverter(ConverterFactory.XUGGLER_BGR_24);
      if (mConverterType == null)
        throw new UnsupportedOperationException(
          "No converter \"" + ConverterFactory.XUGGLER_BGR_24 + "\" found.");
      }
        // if the converter is not created, create one

      if (mVideoConverter == null)
        mVideoConverter = ConverterFactory.createConverter(mConverterType
            .getDescriptor(), picture);

      // create the buffered image

      image = mVideoConverter.toImage(picture);
    } else {
      // reset it for next time someone calls.
      mConverterType = null;
      mVideoConverter = null;
    }
    
    // dispatch picture here

    super.onVideoPicture(this, picture, image, picture.getTimeStamp(),
        TimeUnit.MICROSECONDS, streamIndex);
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
  
  private void dispatchAudioSamples(int streamIndex, IAudioSamples samples)
  {
    super.onAudioSamples(this, samples, streamIndex);
  }

  /** {@inheritDoc} */
  
  public void open()
  {
    if (getContainer().open(getUrl(), IContainer.Type.READ, null, 
        mStreamsCanBeAddedDynamically, mQueryStreamMetaData) < 0)
      throw new RuntimeException("could not open: " + getUrl());

    // inform listeners

    super.onOpen(this);
    // note that we should close the container opened here

    setShouldCloseContainer(true);
  }

  /** {@inheritDoc} */

  public void close()
  {
    int rv;

    // close the coders opened by this

    for (IStream stream: mOpenedStreams)
    {
      IStreamCoder coder = stream.getStreamCoder();
      try {
        if ((rv = coder.close()) < 0)
        {
          String errorString = getErrorMessage(rv);
          throw new RuntimeException("error " + errorString
              + ", failed close coder " + coder);
        }
        // inform listeners that the stream was closed
        super.onCloseCoder(this,stream.getIndex());
      } finally {
        coder.delete();
        stream.delete();
      }
    }
    // expunge all referneces to the coders and resamplers
    for(IStreamCoder coder : mCoders.values())
      coder.delete();
    mCoders.clear();
    mOpenedStreams.clear();

    // if we're supposed to, close the container

    if (getShouldCloseContainer())
    {
      if ((rv = getContainer().close()) < 0)
        throw new RuntimeException("error " + getErrorMessage(rv) +
          ", failed close IContainer " +
          getContainer() + " for " + getUrl());
      setShouldCloseContainer(false);
    }

    // tell the listeners that the container is closed

    super.onClose(this);
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
   * Prints the {@link MediaReader} class,
   * object id, and URL if known.
   * {@inheritDoc}
   * @return a nicely formatted string.
   */

  public String toString()
  {
    StringBuilder builder = new StringBuilder();
    builder.append(super.toString());
    builder.append("[");
    builder.append(getContainer() != null ? getContainer().getURL() :"");
    builder.append("]");
    return builder.toString();
  }
}
