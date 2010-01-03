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

import java.util.concurrent.TimeUnit;

import com.xuggle.mediatool.IMediaGenerator;
import com.xuggle.xuggler.Global;
import com.xuggle.xuggler.IMediaData;

/**
 * An abstract implementation of {@link IRawMediaEvent}, but does not declare
 * {@link IRawMediaEvent}.
 * 
 * @author aclarke
 * 
 */
public abstract class ARawMediaMixin extends AStreamMixin
{
  private final IMediaData mMediaData;
  private final Object mJavaData;
  private final long mTimeStamp;
  private final TimeUnit mTimeUnit;

  /**
   * Create an {@link ARawMediaMixin}.
   * 
   * @param source the source
   * @param picture a picture
   * @param image an image
   * @param timeStamp a time stamp value. If image is null this value is
   *        ignored.
   * @param timeUnit a time unit for timeStamp. If image is null this value is
   *        ignored.
   * @param streamIndex the stream index this media is associated with
   * or null if none.
   * @throws IllegalArgumentException if both picture and image are null.
   */
  public ARawMediaMixin(IMediaGenerator source, IMediaData picture,
      Object image, long timeStamp, TimeUnit timeUnit, Integer streamIndex)
  {
    super(source, streamIndex);
    if (image == null && picture == null)
      throw new IllegalArgumentException();
    mMediaData = picture;
    mJavaData = image;
    if (image == null)
    {
      timeStamp = picture.getTimeStamp();
      timeUnit = TimeUnit.MICROSECONDS;
    }
    mTimeStamp = timeStamp;
    if (timeUnit == null)
      throw new IllegalArgumentException();
    mTimeUnit = timeUnit;
  }

  /**
   * Implementation of {@link IRawMediaEvent#getMediaData()}.
   * @see com.xuggle.mediatool.event.IRawMediaEvent#getMediaData()
   */
  public IMediaData getMediaData()
  {
    return mMediaData;
  }

  /**
   * Implementation of {@link IRawMediaEvent#getJavaData()}.
   * @see com.xuggle.mediatool.event.IRawMediaEvent#getJavaData()
   */
  public Object getJavaData()
  {
    return mJavaData;
  }

  /**
   * Implementation of {@link IRawMediaEvent#getTimeStamp()}.
   * @see com.xuggle.mediatool.event.IRawMediaEvent#getTimeStamp()
   */
  public Long getTimeStamp()
  {
    return getTimeStamp(TimeUnit.MICROSECONDS);
  }

  /**
   * Implementation of {@link IRawMediaEvent#getTimeStamp(TimeUnit)}.
   * @see
   * com.xuggle.mediatool.event.IRawMediaEvent#getTimeStamp(java.util.concurrent.TimeUnit)
   */
  public Long getTimeStamp(TimeUnit unit)
  {
    if (unit == null)
      throw new IllegalArgumentException();
    if (mTimeStamp == Global.NO_PTS)
      return null;
    return unit.convert(mTimeStamp, mTimeUnit);
  }

  /**
   * Implementation of {@link IRawMediaEvent#getTimeUnit()}.
   * @see com.xuggle.mediatool.event.IRawMediaEvent#getTimeUnit()
   */
  public TimeUnit getTimeUnit()
  {
    return mTimeUnit;
  }

}
