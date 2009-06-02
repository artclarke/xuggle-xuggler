package com.xuggle.mediatool.event;

import java.awt.image.BufferedImage;

import com.xuggle.mediatool.IMediaListener;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IVideoPicture;

public interface IVideoPictureEvent extends IRawMediaEvent
{

  /**
   * {inheritDoc}
   */
  public abstract IVideoPicture getMediaData();

  /**
   * The video picture.  May be null if {@link #getImage()}
   * is not null.
   * <p>
   * The returned {@link IAudioSamples} will only be valid for
   * the duration of the {@link IMediaListener#onVideoPicture(IVideoPictureEvent)}
   * call, and {@link IMediaListener} implementations must not use it after
   * the call returns.  If you need to keep a copy of this data then
   * use {@link IAudioSamples#copyReference()} to create a reference
   * that will outlive your call.
   * </p>
   * 
   * @return the videoPicture, or null if unavailable
   */
  public abstract IVideoPicture getPicture();

  /**
   * The buffered image, if available.  If null,
   * you must use {@link #getPicture()}
   * @return the bufferedImage, or null if not available
   */
  public abstract BufferedImage getImage();

}
