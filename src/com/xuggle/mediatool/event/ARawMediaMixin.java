/**
 * 
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