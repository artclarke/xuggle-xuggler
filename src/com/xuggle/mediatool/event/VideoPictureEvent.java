package com.xuggle.mediatool.event;

import java.awt.image.BufferedImage;
import java.util.concurrent.TimeUnit;

import com.xuggle.mediatool.IMediaGenerator;
import com.xuggle.mediatool.IMediaListener;
import com.xuggle.xuggler.IVideoPicture;

/**
 * {@link AEventMixin} for {@link IMediaListener#onVideoPicture(IVideoPictureEvent)}
 * 
 * @author aclarke
 *
 */
public class VideoPictureEvent extends ARawMediaMixin implements IVideoPictureEvent
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

  /* (non-Javadoc)
   * @see com.xuggle.mediatool.event.IVideoPictureEvent#getMediaData()
   */
  @Override
  public IVideoPicture getMediaData()
  {
    return (IVideoPicture) super.getMediaData();
  }
  /* (non-Javadoc)
   * @see com.xuggle.mediatool.event.IVideoPictureEvent#getPicture()
   */
  public IVideoPicture getPicture()
  {
    return getMediaData();
  }

  /* (non-Javadoc)
   * @see com.xuggle.mediatool.event.IVideoPictureEvent#getImage()
   */
  public BufferedImage getImage()
  {
    return (BufferedImage) getJavaData();
  }
}