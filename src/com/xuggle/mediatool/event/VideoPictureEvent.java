/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated. All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler. If not, see <http://www.gnu.org/licenses/>.
 */

package com.xuggle.mediatool.event;

import java.awt.image.BufferedImage;
import java.util.concurrent.TimeUnit;

import com.xuggle.mediatool.IMediaGenerator;
import com.xuggle.xuggler.IVideoPicture;

/**
 * An implementation of {@link IVideoPictureEvent}.
 * 
 * @author aclarke
 * 
 */
public class VideoPictureEvent extends ARawMediaMixin implements
    IVideoPictureEvent
{

  /**
   * Creates a {@link VideoPictureEvent}. If <code>image</code> is not null and
   * <code>picture</code> is not null, then <code>image</code> prevails.
   * 
   * @param source the source of this event.
   * @param picture the raw {@link IVideoPicture} for this event. Can be null if
   *        <code>image</code> is not null.
   * @param image the raw {@link BufferedImage} for this event. Can be null if
   *        <code>picture</code> is not null.
   * @param timeStamp if <code>image</code> is not null, this is the timeStamp
   *        of <code>image</code>
   * @param timeUnit if <code>image</code> is not null, this is the timeUnit of
   *        <code>timeStamp</code>
   * @param streamIndex the stream this event occurred on, or null if unknown.
   * @throws IllegalArgumentException if both <code>picture</code> and
   *         <code>image</code> are null.
   */
  public VideoPictureEvent(IMediaGenerator source, IVideoPicture picture,
      BufferedImage image, long timeStamp, TimeUnit timeUnit,
      Integer streamIndex)
  {
    super(source, picture, image, timeStamp, timeUnit, streamIndex);
  }

  /**
   * Creates a {@link VideoPictureEvent}.
   * 
   * @param source the source of this event.
   * @param picture the raw {@link IVideoPicture} for this event.
   * @param streamIndex the stream this event occurred on, or null if unknown.
   * @throws IllegalArgumentException if picture is null.
   */
  public VideoPictureEvent(IMediaGenerator source, IVideoPicture picture,
      Integer streamIndex)
  {
    this(source, picture, null, 0, null, streamIndex);
  }

  /**
   * Creates a {@link VideoPictureEvent}.
   * 
   * @param source the source of this event.
   * @param image the raw {@link BufferedImage} for this event.
   * @param timeStamp the timeStamp of <code>image</code>
   * @param timeUnit the timeUnit of <code>timeStamp</code>
   * @param streamIndex the stream this event occurred on, or null if unknown.
   * @throws IllegalArgumentException if image is null.
   */
  public VideoPictureEvent(IMediaGenerator source, BufferedImage image,
      long timeStamp, TimeUnit timeUnit, Integer streamIndex)
  {
    this(source, null, image, timeStamp, timeUnit, streamIndex);
  }

  /**
   * An implementation of {@link IVideoPictureEvent#getMediaData()}.
   * 
   * @see IVideoPictureEvent#getMediaData()
   */
  @Override
  public IVideoPicture getMediaData()
  {
    return (IVideoPicture) super.getMediaData();
  }

  /**
   * An implementation of {@link IVideoPictureEvent#getPicture()}.
   * 
   * @see IVideoPictureEvent#getPicture()
   */
  public IVideoPicture getPicture()
  {
    return getMediaData();
  }

  /**
   * An implementation of {@link IVideoPictureEvent#getImage()}.
   * 
   * @see IVideoPictureEvent#getImage()
   */
  public BufferedImage getImage()
  {
    return getJavaData();
  }

  /**
   * An implementation of {@link IVideoPictureEvent#getJavaData()}.
   * 
   * @see IVideoPictureEvent#getJavaData()
   */
  public BufferedImage getJavaData()
  {
    return (BufferedImage) super.getJavaData();
  }
}
