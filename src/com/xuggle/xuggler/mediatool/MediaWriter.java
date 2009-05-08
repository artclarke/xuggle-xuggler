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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;


import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IPacket;
import com.xuggle.xuggler.IStreamCoder;
import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IContainerFormat;
import com.xuggle.xuggler.mediatool.MediaReader;
import com.xuggle.xuggler.video.IConverter;
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

public class MediaWriter implements IMediaListener
{
  final private Logger log = LoggerFactory.getLogger(this.getClass());
  { log.trace("<init>"); }

  // the input container of packets
  
  protected IContainer mInputContainer;

  // the output container of packets
  
  protected IContainer mOutputContainer;

  // a map between input stream indicies to output stream indicies

  protected Map<Integer, Integer> mOutputStreamIndices = 
    new HashMap<Integer, Integer>();

  // a map between output stream indicies and coders

  protected Map<Integer, IStreamCoder> mCoders = 
    new HashMap<Integer, IStreamCoder>();
  
  // a map between output stream indicies and video converters

  protected Map<Integer, IConverter> mVideoConverters = 
    new HashMap<Integer, IConverter>();
  
  // all the coders opened by this MediaWriter which are candidates for
  // closing

  protected final Collection<IStreamCoder> mOpenedCoders 
    = new Vector<IStreamCoder>();

  // true if this media writer should close the container

  protected boolean mCloseContainer;

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

    // this writer as a listerner to the reader
    
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
   * @throws IllegalArgumentException if the specifed {@link
   *         MediaReader} is configure to allow dynamic adding of
   *         streams.
   */

  public MediaWriter(String url, IContainer inputContainer)
  {
    // verify that the input container is a readable type

    if (inputContainer.getType() != IContainer.Type.READ)
      throw new IllegalArgumentException(
        "inputContainer is improperly must be of type readable.");

    // verify that no streams will be added dynamically

    if (inputContainer.canStreamsBeAddedDynamically())
      throw new IllegalArgumentException(
        "inputContainer is improperly configured to allow " + 
        "dynamic adding of streams.");

    // record the input container

    mInputContainer = inputContainer;

    // create format 

    IContainerFormat format = IContainerFormat.make();
    format.setOutputFormat(mInputContainer.getContainerFormat().
      getInputFormatShortName(), url, null);
    
    // create the container

    mOutputContainer = IContainer.make();
    
    // open the container

    if (mOutputContainer.open(url, IContainer.Type.WRITE, 
        format, true, false) < 0)
      throw new IllegalArgumentException("could not open: " + url);

    // note that we should close the container opened here

    mCloseContainer = true;
  }

  /**
   * create a MediaWriter which reads and disptchs data from a media
   * stream for a given source URL. The media stream is opened, and
   * subsequent calls to {@link #readPacket()} will read stream content
   * and dispatch it to attached listeners. When the end of the stream
   * is encountered the media container and it's contained streams are
   * all closed.
   *
   * @param url the location of the media content, a file name will also
   *        work here
   *
   * @throws IllegalArgumentException if BufferedImage conversion is
   *         requried and the passed converter descriptor is NULL
   * @throws UnsupportedOperationException if the converter can not be
   *         found
   */

  @SuppressWarnings("unused")
  private MediaWriter(String url, IContainerFormat format)
  {
    // create the container

    mOutputContainer = IContainer.make();
    if (null == mOutputContainer)
      throw new RuntimeException("failed to create output IContainer");
    
    // open the container

    if (mOutputContainer.open(url, IContainer.Type.WRITE, format, true, false) < 0)
      throw new IllegalArgumentException("could not open: " + url);

    // note that we should close the container opened here

    mCloseContainer = true;
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

  /** Get the underlying media {@link IContainer} that this reader is
   * using to decode streams.  Listeners can use stream index values to
   * learn more about particular streams they are receiving data from.
   *
   * @return the stream container.
   */

  public IContainer getContainer()
  {
    return mOutputContainer;
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
  }
  
  /** {@inheritDoc} */
  
  public void onAudioSamples(IMediaTool tool, IAudioSamples samples, int streamIndex)
  {
    encodeAudio(getStreamCoder(streamIndex), samples);
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
        
        //addStreamFromContainer(inputStreamIndex);
      }

      // if the header has not been writen, do so now

      if (!mOutputContainer.isHeaderWritten())
      {
        int errorNo = mOutputContainer.writeHeader();
        if (0 != errorNo)
          throw new RuntimeException("Error " + errorNo +
            ", failed to write header to " + mOutputContainer.getURL());
      }
    }

    // establish the coder for the output stream index

    IStreamCoder coder = mCoders.get(getOutputStreamIndex(inputStreamIndex));
    if (null == coder)
      throw new RuntimeException("invalid input stream index: " + inputStreamIndex);
    
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
    
    IStream stream = mOutputContainer.addNewStream(
      mInputContainer.getStream(inputStreamIndex).getId());

    // create the coder
    
    IStreamCoder newCoder = IStreamCoder.make(IStreamCoder.Direction.ENCODING,
      mInputContainer.getStream(inputStreamIndex).getStreamCoder());
    stream.setStreamCoder(newCoder);

    // open the new stream

    openNewStream(stream, inputStreamIndex, stream.getIndex());
  }

  /**
   * Open a newly added stream.
   */

  protected void openNewStream(IStream stream, int inputStreamIndex, 
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
      mOpenedCoders.add(coder);
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

  /** {@inheritDoc} */

  public void onOpen(IMediaTool source)
  {
  }

  /** {@inheritDoc} */

  public void onClose(IMediaTool source)
  {
    close();
    source.removeListener(this);
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

    mOutputContainer.flushPackets();
  }

  /**
   * Write packet to the output container
   * 
   * @param packet the packet to write out
   */

  protected void writePacket(IPacket packet)
  {
    if (-1 == mOutputContainer.writePacket(packet, mForceInterleave))
      throw new RuntimeException("failed to write packet: " + packet);
  }

  /**
   * Reset this {@link MediaWriter} object, closing all elements we opened.
   * For example, if the {@link MediaWriter} opened the {@link IContainer},
   * it closes it now.
   */
  
  public void close()
  {
    int rv;

    // flush coders
    
    flush();

    // write the trailer on the output conteiner
    
    if ((rv = mOutputContainer.writeTrailer()) < 0)
      throw new RuntimeException("error " + rv + ", failed to write trailer to "
        + mOutputContainer.getURL());

    // close the coders opened by this MediaWriter

    for (IStreamCoder coder: mOpenedCoders)
    {
      if ((rv = coder.close()) < 0)
        throw new RuntimeException("error " + rv + ", failed close coder " +
          coder);
    }

    // expunge all referneces to the coders and resamplers
    
    mCoders.clear();
    mOpenedCoders.clear();
    mVideoConverters.clear();

    // if we're supposed to, close the container

    if (mCloseContainer && null != mOutputContainer)
    {
      if ((rv = mOutputContainer.close()) < 0)
        throw new RuntimeException("error " + rv + ", failed close IContainer " +
          mOutputContainer + " for " + mOutputContainer.getURL());
        
      mOutputContainer = null;
      mCloseContainer = false;
    }
  }
}
