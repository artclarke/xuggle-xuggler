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
