/**
 * 
 */
package com.xuggle.mediatool.event;

import java.util.concurrent.TimeUnit;

import com.xuggle.mediatool.IMediaGenerator;
import com.xuggle.mediatool.IMediaListener;
import com.xuggle.xuggler.Global;
import com.xuggle.xuggler.IMediaData;

public class RawMediaEvent extends StreamEvent
{
  private final IMediaData mMediaData;
  private final Object mJavaData;
  private final long mTimeStamp;
  private final TimeUnit mTimeUnit;

  public RawMediaEvent(IMediaGenerator source, 
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

  /**
   * The {@link IMediaData} for this object.
   * May be null if {@link #getJavaData()}
   * is not null.
   * <p>
   * The returned {@link IMediaData} will only be valid for
   * the duration of the callbackand {@link IMediaListener} implementations
   * must not use it after
   * the call returns.  If you need to keep a copy of this data then
   * use {@link IMediaData#copyReference()} to create a reference
   * that will outlive your call.
   * </p>
   * 
   * @return the media data, or null if unavailable
   */
  public IMediaData getMediaData()
  {
    return mMediaData;
  }

  /**
   * The Java object registered with this event.  If null,
   * you must use {@link #getMediaData()}
   * @return the object, or null if not available
   */
  public Object getJavaData()
  {
    return mJavaData;
  }

  /**
   * The time stamp of this media, in {@link TimeUnit#MICROSECONDS}.
   * @return the timeStamp, or null if none.
   */
  public Long getTimeStamp()
  {
    return getTimeStamp(TimeUnit.MICROSECONDS);
  }
  /**
   * Get the time stamp of this media in the specified units.
   * @param unit the time unit
   * @return the time stamp, or null if none
   * @throws IllegalArgumentException if unit is null
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
   * The time unit of {@link #getTimeStamp()}.
   * @return the timeUnit
   */
  public TimeUnit getTimeUnit()
  {
    return mTimeUnit;
  }

}