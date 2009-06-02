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

import java.util.concurrent.TimeUnit;

import com.xuggle.mediatool.IMediaGenerator;
import com.xuggle.xuggler.Global;
import com.xuggle.xuggler.IMediaData;

public abstract class ARawMediaMixin extends AStreamMixin
{
  private final IMediaData mMediaData;
  private final Object mJavaData;
  private final long mTimeStamp;
  private final TimeUnit mTimeUnit;

  public ARawMediaMixin(IMediaGenerator source, 
      IMediaData picture, Object image,
      long timeStamp, TimeUnit timeUnit,
      int streamIndex)
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

  /* (non-Javadoc)
   * @see com.xuggle.mediatool.event.IRawMediaEvent#getMediaData()
   */
  public IMediaData getMediaData()
  {
    return mMediaData;
  }

  /* (non-Javadoc)
   * @see com.xuggle.mediatool.event.IRawMediaEvent#getJavaData()
   */
  public Object getJavaData()
  {
    return mJavaData;
  }

  /* (non-Javadoc)
   * @see com.xuggle.mediatool.event.IRawMediaEvent#getTimeStamp()
   */
  public Long getTimeStamp()
  {
    return getTimeStamp(TimeUnit.MICROSECONDS);
  }
  /* (non-Javadoc)
   * @see com.xuggle.mediatool.event.IRawMediaEvent#getTimeStamp(java.util.concurrent.TimeUnit)
   */
  public Long getTimeStamp(TimeUnit unit)
  {
    if (unit == null)
      throw new IllegalArgumentException();
    if (mTimeStamp == Global.NO_PTS)
      return null;
    return unit.convert(mTimeStamp, mTimeUnit);
  }

  /* (non-Javadoc)
   * @see com.xuggle.mediatool.event.IRawMediaEvent#getTimeUnit()
   */
  public TimeUnit getTimeUnit()
  {
    return mTimeUnit;
  }

}