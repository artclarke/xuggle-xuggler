/*******************************************************************************
 * Copyright (c) 2008, 2010 Xuggle Inc.  All rights reserved.
 *  
 * This file is part of Xuggle-Xuggler-Main.
 *
 * Xuggle-Xuggler-Main is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xuggle-Xuggler-Main is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Xuggle-Xuggler-Main.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/

package com.xuggle.mediatool.event;

import java.awt.image.BufferedImage;
import com.xuggle.xuggler.IVideoPicture;

/**
 * Dispatched by {@link com.xuggle.mediatool.IMediaListener#onVideoPicture(IVideoPictureEvent)}.
 * @author aclarke
 *
 */

public interface IVideoPictureEvent extends IRawMediaEvent
{

  /**
   * {@inheritDoc}
   */
  public abstract IVideoPicture getMediaData();

  /**
   * Forwards to {@link #getMediaData()}.
   * @see #getMediaData()
   */
  public abstract IVideoPicture getPicture();

  /**
   * The buffered image, if available.  If null,
   * you must use {@link #getPicture()}
   * @return the bufferedImage, or null if not available
   */
  public abstract BufferedImage getImage();

  /**
   * {@inheritDoc}
   */
  public abstract BufferedImage getJavaData();
}
