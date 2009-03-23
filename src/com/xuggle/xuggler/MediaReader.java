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

package com.xuggle.xuggler;

import java.util.Map;
import java.util.HashMap;
import java.util.Vector;
import java.util.Collection;

import java.awt.image.BufferedImage;

import com.xuggle.xuggler.Utils;
import com.xuggle.xuggler.Global;
import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IPixelFormat;
import com.xuggle.xuggler.IPacket;
import com.xuggle.xuggler.IVideoResampler;
import com.xuggle.xuggler.IStreamCoder;
import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IError;

import static com.xuggle.xuggler.ICodec.Type.*;

/** General purpose media reader. */

public class MediaReader
{
  // defined to support object serialization

  public static final long serialVersionUID = 1;

  // a place to put packets
  
  final IPacket mPacket = IPacket.make();

  // the container of packets
  
  IContainer mContainer;

  // a map between stream IDs and coders

  Map<Integer, IStreamCoder> mCoders = 
    new HashMap<Integer, IStreamCoder>();
  
  // a map between stream IDs and resamplers

  Map<Integer, IVideoResampler> mVideoResamplers = 
    new HashMap<Integer, IVideoResampler>();
  
  // all the media reader listeners

  final Collection<IListener> mListeners = new Vector<IListener>();

  // all the coders opened by this MediaReader which are candidates for
  // closing

  final Collection<IStreamCoder> mOpenedCoders = new Vector<IStreamCoder>();

  // should this media reader create buffered images

  final boolean mCreateBufferedImages;

  // should this media reader close the container

  final boolean mCloseContainer;

  /** Read and dispatch data from media stream from a given source
   * URL. The media stream is opened, and subsequent calls to
   * readPacket() will read stream content and dispatch it to attached
   * listeners. When the end of the stream is encountered the media
   * container and it's containd streams are all closed.
   *
   * @param url the location of the media content, a file name will also
   *        work here
   * @param createBufferedImages true if BufferedImages should be
   *        created during media reading
   */

  public MediaReader(String url, boolean createBufferedImages)
  {
    // create the container

    mContainer = IContainer.make();
    
    // open the container

    if (mContainer.open(url, IContainer.Type.READ, null, true, false) < 0)
      throw new IllegalArgumentException("could not open: " + url);

    // note that we should close the container opened here

    mCloseContainer = true;

    // note that we should or should not create buffered images during
    // video decoding

    mCreateBufferedImages = createBufferedImages;
  }
  
  /** Read and dispatch data from a media container.  Calls to
   * readPacket() will read stream content and dispatch it to attached
   * listeners. If the end of the media stream is encountered, the
   * MediaReader does NOT close the container, that is left to the
   * calling context (you).  Streams opend by the MediaReader during
   * packet reading will be closed by the MediaReader, however streams
   * opened prior to MediaReader construction will not be closed.  In
   * short MediaReader closes what it opens.
   *
   * @param container on already open media container
   * @param createBufferedImages should BufferedImages be created during
   *        media reading
   */

  public MediaReader(IContainer container, boolean createBufferedImages)
  {
    // get the container

    mContainer = container;

    // note that we should not close the container opened elsewere

    mCloseContainer = false;

    // note that we should or should not create buffered images during
    // video decoding

    mCreateBufferedImages = createBufferedImages;
  }

  /** Get the underlying media container that this reader is using to
   * decode streams.  Listeners can use stream index values to learn
   * more about particular streams they are receiving data from.
   *
   * @return the stream container.
   */

  public IContainer getContainer()
  {
    return mContainer;
  }
  
  /** Get the correct coder for a given stream in the container.  If
   * this is a new stream not been seen before Add a new stream coder to
   * set of coders used to decode this container.
   *
   * @param streamIndex the index of the stream to be added
   */

  public IStreamCoder getStreamCoder(int streamIndex)
  {
    // if the coder does not exists, get it

    IStreamCoder coder = mCoders.get(streamIndex);
    if (coder == null)
    {
      // test valid stream index

      if (streamIndex < 0 || streamIndex >= mContainer.getNumStreams())
        throw new RuntimeException("invalid stream index");
    
      // now get the coder for the given stream index

      IStream stream = mContainer.getStream(streamIndex);
      coder = stream.getStreamCoder();
      ICodec.Type type = coder.getCodecType();

      // put the coder into the coder map, event if it not a supported
      // type so that on further reads it will find the coder but choose
      // not decode unsupported types

      mCoders.put(streamIndex, coder);

      // if the coder is not open, open it 
      // NOTE: MediaReader currently supports audio & video streams

      if (!coder.isOpen() && 
        (type==ICodec.Type.CODEC_TYPE_AUDIO || type==ICodec.Type.CODEC_TYPE_VIDEO))
      {
        if (coder.open() < 0)
          throw new RuntimeException(
            "could not open coder for stream: " + streamIndex);
        mOpenedCoders.add(coder);
      }
    }
      
    // return the coder, new or cached
    
    return coder;
  }

  /** This reads a single packet from the container.  If a complete
   * video frame or audio sample set are decoded, it will be dispatched
   * to the listeners added to the media reader.
   *
   * @return null if there are more packets to read, otherwise return an
   *         IError instance.
   */
    
  public IError readPacket()
  {
    // if there is an off nomial resul from read packet return the
    // correct error

    int rv = mContainer.readNextPacket(mPacket);
    if (rv < 0)
    {
      IError error = IError.make(rv);
      
      // if this is an end of file, call close
      
      if (error.getType() == IError.Type.ERROR_EOF)
        close();
      return error;
    }

    // get the coder for this packet

    IStreamCoder coder = getStreamCoder(mPacket.getStreamIndex());

    // decode based on type

    switch (coder.getCodecType())
    {
      // decode audio

    case CODEC_TYPE_AUDIO:
      decodeAudio(coder);
      break;

      // decode video

    case CODEC_TYPE_VIDEO:
      decodeVideo(coder);
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
   */

  public void decodeVideo(IStreamCoder videoCoder)
  {
    // create a blank video picture
    
    IVideoPicture picture = IVideoPicture.make(videoCoder.getPixelType(),
        videoCoder.getWidth(), videoCoder.getHeight());
    
    // decode the packet into the video picture
    
    int rv = videoCoder.decodeVideo(picture, mPacket, 0);
    if (rv < 0)
      throw new RuntimeException("error " + rv + " decoding video");
    
    // if this is a complete picture, dispatch the picture
    
    if (picture.isComplete())
      dispatchVideoPicture(mPacket.getStreamIndex(), picture);
  }

  /** Decode and dispatch a audio packet.
   *
   * @param audioCoder the audio coder
   */

  public void decodeAudio(IStreamCoder audioCoder)
  {
    // if it doesn't exist, allocate a set of samples with the same
    // number of channels as in the audio coder and a stock size of 1024
    // (currently the buffer size will be expanded to 32k to conform to
    // ffmpeg)
    
    IAudioSamples samples = IAudioSamples.make(1024, audioCoder.getChannels());
    
    // packet may contain multiple audio frames, decode audio until
    // all audio frames are extracted from the packet 
    
    for (int offset = 0; offset < mPacket.getSize(); /* in loop */)
    {
      // decode audio
      
      int bytesDecoded = audioCoder.decodeAudio(samples, mPacket, offset);
      if (bytesDecoded < 0)
        throw new RuntimeException("error " + bytesDecoded + " decoding audio");
      offset += bytesDecoded;
      
      // if samples are a complete audio frame, dispatch that frame
      
      if (samples.isComplete())
        dispatchAudioSamples(mPacket.getStreamIndex(), samples);
    }
  }

  /** Dispatch a video picture to attched listeners.  This is called when
   * a complete video picture has been decoded from the packet stream.
   * Optionally it will take the steps required to convert the
   * IVideoPicture to a BufferedImage.  If you wanted to perform a
   * custom conversion, subclass and overide this method.
   *
   * @param streamIndex the index of the stream
   * @param picture the video picture to dispatch
   */
  
  public void dispatchVideoPicture(int streamIndex, IVideoPicture picture)
  {
    BufferedImage image = null;
    
    // if should create buffered image, do so

    if (mCreateBufferedImages)
    {
      IVideoPicture argbPic = picture;

      // if the picture is not ARGB

      if (picture.getPixelType() != IPixelFormat.Type.ARGB)
      {
        // get the resampler; create it if it doesn't exist

        IVideoResampler resampler = mVideoResamplers.get(streamIndex);
        if (resampler == null)
        {
          resampler = IVideoResampler.make(
            picture.getWidth(), picture.getHeight(), 
            IPixelFormat.Type.ARGB,
            picture.getWidth(), picture.getHeight(), 
            picture.getPixelType());
          
          if (resampler == null)
            throw new RuntimeException(
              "could not create color space resampler.");
          mVideoResamplers.put(streamIndex, resampler);
        }

        // create rgb 32bit version of the image

        argbPic = IVideoPicture.make(resampler.getOutputPixelFormat(), 
          picture.getWidth(), picture.getHeight());
        if (resampler.resample(argbPic, picture) < 0)
          throw new RuntimeException("could not resample video picture.");
        if (argbPic.getPixelType() != IPixelFormat.Type.ARGB)
          throw new RuntimeException("could not decode video as RGB 32 bit data.");
      }
      
      // create the buffered image

      image = Utils.videoPictureToImage(argbPic);
    }
    
    // dispatch picture here

    for (IListener l: mListeners)
      l.onVideoPicture(picture, image, streamIndex);
  }

  /** Dispatch audio samples to attched listeners.  This is called when
   * a complete set of audio samples has been decoded from the packet
   * stream.  If you wanted to perform a common custom conversion,
   * subclass and overide this method.
   *
   * @param streamIndex the index of the stream
   * @param samples the audio samples to dispatch
   */
  
  public void dispatchAudioSamples(int streamIndex, IAudioSamples samples)
  {
    for (IListener l: mListeners)
      l.onAudioSamples(samples, streamIndex);
  }

  /** Close all elements of the media stream. */
  
  public void close()
  {
    // close the coders opened by this

    for (IStreamCoder coder: mOpenedCoders)
      coder.close();
  
    // expunge all referneces to the coders and resamplers
    
    mCoders.clear();
    mOpenedCoders.clear();
    mVideoResamplers.clear();

    // if we're supposed to, close the container

    if (mCloseContainer && mContainer !=null)
    {
      mContainer.close();
      mContainer = null;
    }
  }

  /** Add a media reader listener.
   *
   * @param listener the listener to add
   *
   * @return true if this collection changed as a result of the call
   */
  
  public boolean addListener(IListener listener)
  {
    return mListeners.add(listener);
  }

  /** Remove a media reader listener.
   *
   * @param listener the listener to remove
   *
   * @return true if this collection changed as a result of the call
   */
  
  public boolean removeListener(IListener listener)
  {
    return mListeners.remove(listener);
  }
  
  /** Media listener which receives events when audio and video content
   * is extracted from an IContainer. */

  public static interface IListener
  {
    /** Called after a video frame has been decode from a media stream.
     * Optionally a BufferedImage version of the frame will be passed.
     * This method blocks, and should execute quickly.
     *
     * @param picture a raw video picture
     * @param image the buffered image, which will be null if buffered
     *        image creation is deselected for this MediaReader.
     * @param streamIndex the index of the stream
     */

    public void onVideoPicture(IVideoPicture picture, BufferedImage image,
      int streamIndex);
    
    /** Called after audio samples have been decoded from a media
     * stream.  This method blocks, and should execute quickly.
     *
     * @param samples a audio samples
     * @param streamIndex the index of the stream
     */

    public void onAudioSamples(IAudioSamples samples, int streamIndex);
  }

  /** Media adapter which stubs methods in the Listener. */

  public static class ListenerAdapter implements IListener
  {
    /** {@inheritDoc} */

    public void onVideoPicture(IVideoPicture picture, BufferedImage image, 
      int streamIndex)
    {
    }
    
    /** {@inheritDoc} */

    public void onAudioSamples(IAudioSamples samples, int streamIndex)
    {
    }
  }
}
