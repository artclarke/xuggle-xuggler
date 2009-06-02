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