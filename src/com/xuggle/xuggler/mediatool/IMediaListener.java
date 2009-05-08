package com.xuggle.xuggler.mediatool;

import java.awt.image.BufferedImage;

import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IVideoPicture;

/** Media listener which receives events when audio and video content
 * is extracted from an IContainer. */

public interface IMediaListener
{
  /**
   * Called after a video frame has been decoded from a media stream.
   * Optionally a BufferedImage version of the frame may be passed
   * if the calling {@link MediaReader} instance was configured to
   * create BufferedImages.
   * 
   * This method blocks, so return quickly.
   * @param tool the tool that generated this event
   * @param picture a raw video picture
   * @param image the buffered image, which will be null if buffered
   *        image creation is de-selected for this MediaReader.
   * @param streamIndex the index of the stream this object was decoded from.
   */

  public void onVideoPicture(IMediaTool tool, IVideoPicture picture,
    BufferedImage image, int streamIndex);
  
  /**
   * Called after audio samples have been decoded from a media
   * stream.  This method blocks, so return quickly.
   * @param tool the tool that generated this event
   * @param samples a audio samples
   * @param streamIndex the index of the stream
   */

  public void onAudioSamples(IMediaTool tool, IAudioSamples samples, int streamIndex);

  /**
   * Called after the underlying container is opened, it will not be
   * called if an open container is passed into the MediaReader.
   *
   * @param tool the MediaReader which was just opened
   */

  public void onOpen(IMediaTool tool);

  /**
   * Called after the underlying container and all it's associated
   * streams are closed.
   * 
   * @param tool the MediaReader which was just closed
   */

  public void onClose(IMediaTool tool);
}