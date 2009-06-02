package com.xuggle.mediatool.event;

import java.awt.image.BufferedImage;
import java.util.concurrent.TimeUnit;

import com.xuggle.mediatool.IMediaGenerator;
import com.xuggle.mediatool.IMediaListener;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IVideoPicture;

/**
 * {@link Event} for {@link IMediaListener#onVideoPicture(VideoPictureEvent)}
 * 
 * @author aclarke
 *
 */
public class VideoPictureEvent extends RawMediaEvent
{

  public VideoPictureEvent(IMediaGenerator source, 
      IVideoPicture picture, BufferedImage image,
      long timeStamp, TimeUnit timeUnit,
      int streamIndex)
  {
    super(source, picture, image, timeStamp, timeUnit, streamIndex);
  }
  public VideoPictureEvent(IMediaGenerator source, IVideoPicture picture,
      int streamIndex)
  {
    this(source, picture, null, 0, null, streamIndex);
  }
  public VideoPictureEvent(IMediaGenerator source,
      BufferedImage image, long timeStamp, TimeUnit timeUnit,
      int streamIndex)
  {
    this(source, null, image, timeStamp, timeUnit, streamIndex);
  }

  /**
   * {inheritDoc}
   */
  @Override
  public IVideoPicture getMediaData()
  {
    return (IVideoPicture) super.getMediaData();
  }
  /**
   * The video picture.  May be null if {@link #getImage()}
   * is not null.
   * <p>
   * The returned {@link IAudioSamples} will only be valid for
   * the duration of the {@link IMediaListener#onVideoPicture(VideoPictureEvent)}
   * call, and {@link IMediaListener} implementations must not use it after
   * the call returns.  If you need to keep a copy of this data then
   * use {@link IAudioSamples#copyReference()} to create a reference
   * that will outlive your call.
   * </p>
   * 
   * @return the videoPicture, or null if unavailable
   */
  public IVideoPicture getPicture()
  {
    return getMediaData();
  }

  /**
   * The buffered image, if available.  If null,
   * you must use {@link #getPicture()}
   * @return the bufferedImage, or null if not available
   */
  public BufferedImage getImage()
  {
    return (BufferedImage) getJavaData();
  }
}