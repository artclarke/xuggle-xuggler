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

%typemap (javacode) com::xuggle::xuggler::IMediaData,com::xuggle::xuggler::IMediaData*,com::xuggle::xuggler::IMediaData& %{

  // used to correct timezone offsets for timestamp format 

  private static final long TIME_OFFSET = -java.util.Calendar.getInstance()
    .getTimeZone().getRawOffset();

  /** The default time stamp format. */
  
  public static final String DEFAULT_TIME_STAMP_FORMAT =
    "%1$tH:%1$tM:%1$tS.%1$tL";
/**
 * Absolute bulk put method.
 * <p>
 * This method delegates to the same put method on the
 * underlying {@link com.xuggle.ferry.IBuffer} returned from {@link #getData()} ,
 * but may be more efficient in some {@link com.xuggle.ferry.JNIMemoryManager.MemoryModel}
 * configurations.
 * </p>
 * @see com.xuggle.ferry.IBuffer#put(byte[], int, int, int)
 */
public void put(byte[] src, int srcPos, int destPos, int length)
{
  com.xuggle.ferry.IBuffer buffer = getDataCached();
  if (buffer == null)
    return;
  buffer.put(src, srcPos, destPos, length);
  return;
}

/**
 * Absolute bulk get method.
 * <p>
 * This method delegates to the same get method on the
 * underlying {@link com.xuggle.ferry.IBuffer} returned from {@link #getData()} ,
 * but may be more efficient in some {@link com.xuggle.ferry.JNIMemoryManager.MemoryModel}
 * configurations.
 * </p>
 * @see com.xuggle.ferry.IBuffer#get(int, byte[], int, int)
 */
public void get(int srcPos, byte[] dest, int destPos, int length)
{
   com.xuggle.ferry.IBuffer buffer = getDataCached();
   if (buffer == null)
     return;
   buffer.get(srcPos, dest, destPos, length);
   return;
}

/**
 * Absolute bulk put method.
 * <p>
 * This method delegates to the same put method on the
 * underlying {@link com.xuggle.ferry.IBuffer} returned from {@link #getData()} ,
 * but may be more efficient in some {@link com.xuggle.ferry.JNIMemoryManager.MemoryModel}
 * configurations.
 * </p>
 * @see com.xuggle.ferry.IBuffer#put(char[], int, int, int)
 */
public void put(char[] src, int srcPos, int destPos, int length)
{
  com.xuggle.ferry.IBuffer buffer = getDataCached();
  if (buffer == null)
    return;
  buffer.put(src, srcPos, destPos, length);
  return;
}

/**
 * Absolute bulk get method.
 * <p>
 * This method delegates to the same get method on the
 * underlying {@link com.xuggle.ferry.IBuffer} returned from {@link #getData()} ,
 * but may be more efficient in some {@link com.xuggle.ferry.JNIMemoryManager.MemoryModel}
 * configurations.
 * </p>
 * @see com.xuggle.ferry.IBuffer#get(int, char[], int, int)
 */
public void get(int srcPos, char[] dest, int destPos, int length)
{
   com.xuggle.ferry.IBuffer buffer = getDataCached();
   if (buffer == null)
     return;
   buffer.get(srcPos, dest, destPos, length);
   return;
}

/**
 * Absolute bulk put method.
 * <p>
 * This method delegates to the same put method on the
 * underlying {@link com.xuggle.ferry.IBuffer} returned from {@link #getData()} ,
 * but may be more efficient in some {@link com.xuggle.ferry.JNIMemoryManager.MemoryModel}
 * configurations.
 * </p>
 * @see com.xuggle.ferry.IBuffer#put(short[], int, int, int)
 */
public void put(short[] src, int srcPos, int destPos, int length)
{
  com.xuggle.ferry.IBuffer buffer = getDataCached();
  if (buffer == null)
    return;
  buffer.put(src, srcPos, destPos, length);
  return;
}

/**
 * Absolute bulk get method.
 * <p>
 * This method delegates to the same get method on the
 * underlying {@link com.xuggle.ferry.IBuffer} returned from {@link #getData()} ,
 * but may be more efficient in some {@link com.xuggle.ferry.JNIMemoryManager.MemoryModel}
 * configurations.
 * </p>
 * @see com.xuggle.ferry.IBuffer#get(int, short[], int, int)
 */
public void get(int srcPos, short[] dest, int destPos, int length)
{
   com.xuggle.ferry.IBuffer buffer = getDataCached();
   if (buffer == null)
     return;
   buffer.get(srcPos, dest, destPos, length);
   return;
}

/**
 * Absolute bulk put method.
 * <p>
 * This method delegates to the same put method on the
 * underlying {@link com.xuggle.ferry.IBuffer} returned from {@link #getData()} ,
 * but may be more efficient in some {@link com.xuggle.ferry.JNIMemoryManager.MemoryModel}
 * configurations.
 * </p>
 * @see com.xuggle.ferry.IBuffer#put(int[], int, int, int)
 */
public void put(int[] src, int srcPos, int destPos, int length)
{
  com.xuggle.ferry.IBuffer buffer = getDataCached();
  if (buffer == null)
    return;
  buffer.put(src, srcPos, destPos, length);
  return;
}

/**
 * Absolute bulk get method.
 * <p>
 * This method delegates to the same get method on the
 * underlying {@link com.xuggle.ferry.IBuffer} returned from {@link #getData()} ,
 * but may be more efficient in some {@link com.xuggle.ferry.JNIMemoryManager.MemoryModel}
 * configurations.
 * </p>
 * @see com.xuggle.ferry.IBuffer#get(int, int[], int, int)
 */
public void get(int srcPos, int[] dest, int destPos, int length)
{
   com.xuggle.ferry.IBuffer buffer = getDataCached();
   if (buffer == null)
     return;
   buffer.get(srcPos, dest, destPos, length);
   return;
}

/**
 * Absolute bulk put method.
 * <p>
 * This method delegates to the same put method on the
 * underlying {@link com.xuggle.ferry.IBuffer} returned from {@link #getData()} ,
 * but may be more efficient in some {@link com.xuggle.ferry.JNIMemoryManager.MemoryModel}
 * configurations.
 * </p>
 * @see com.xuggle.ferry.IBuffer#put(long[], int, int, int)
 */
public void put(long[] src, int srcPos, int destPos, int length)
{
  com.xuggle.ferry.IBuffer buffer = getDataCached();
  if (buffer == null)
    return;
  buffer.put(src, srcPos, destPos, length);
  return;
}

/**
 * Absolute bulk get method.
 * <p>
 * This method delegates to the same get method on the
 * underlying {@link com.xuggle.ferry.IBuffer} returned from {@link #getData()} ,
 * but may be more efficient in some {@link com.xuggle.ferry.JNIMemoryManager.MemoryModel}
 * configurations.
 * </p>
 * @see com.xuggle.ferry.IBuffer#get(int, long[], int, int)
 */
public void get(int srcPos, long[] dest, int destPos, int length)
{
   com.xuggle.ferry.IBuffer buffer = getDataCached();
   if (buffer == null)
     return;
   buffer.get(srcPos, dest, destPos, length);
   return;
}

/**
 * Absolute bulk put method.
 * <p>
 * This method delegates to the same put method on the
 * underlying {@link com.xuggle.ferry.IBuffer} returned from {@link #getData()} ,
 * but may be more efficient in some {@link com.xuggle.ferry.JNIMemoryManager.MemoryModel}
 * configurations.
 * </p>
 * @see com.xuggle.ferry.IBuffer#put(float[], int, int, int)
 */
public void put(float[] src, int srcPos, int destPos, int length)
{
  com.xuggle.ferry.IBuffer buffer = getDataCached();
  if (buffer == null)
    return;
  buffer.put(src, srcPos, destPos, length);
  return;
}

/**
 * Absolute bulk get method.
 * <p>
 * This method delegates to the same get method on the
 * underlying {@link com.xuggle.ferry.IBuffer} returned from {@link #getData()} ,
 * but may be more efficient in some {@link com.xuggle.ferry.JNIMemoryManager.MemoryModel}
 * configurations.
 * </p>
 * @see com.xuggle.ferry.IBuffer#get(int, float[], int, int)
 */
public void get(int srcPos, float[] dest, int destPos, int length)
{
   com.xuggle.ferry.IBuffer buffer = getDataCached();
   if (buffer == null)
     return;
   buffer.get(srcPos, dest, destPos, length);
   return;
}

/**
 * Absolute bulk put method.
 * <p>
 * This method delegates to the same put method on the
 * underlying {@link com.xuggle.ferry.IBuffer} returned from {@link #getData()} ,
 * but may be more efficient in some {@link com.xuggle.ferry.JNIMemoryManager.MemoryModel}
 * configurations.
 * </p>
 * @see com.xuggle.ferry.IBuffer#put(double[], int, int, int)
 */
public void put(double[] src, int srcPos, int destPos, int length)
{
  com.xuggle.ferry.IBuffer buffer = getDataCached();
  if (buffer == null)
    return;
  buffer.put(src, srcPos, destPos, length);
  return;
}

/**
 * Absolute bulk get method.
 * <p>
 * This method delegates to the same get method on the
 * underlying {@link com.xuggle.ferry.IBuffer} returned from {@link #getData()} ,
 * but may be more efficient in some {@link com.xuggle.ferry.JNIMemoryManager.MemoryModel}
 * configurations.
 * </p>
 * @see com.xuggle.ferry.IBuffer#get(int, double[], int, int)
 */
public void get(int srcPos, double[] dest, int destPos, int length)
{
   com.xuggle.ferry.IBuffer buffer = getDataCached();
   if (buffer == null)
     return;
   buffer.get(srcPos, dest, destPos, length);
   return;
}


  /**
   * Gets the underlying {@link java.nio.ByteBuffer} for this {@link
   * IMediaData} object.  Users may modify the contents of the
   * ByteBuffer and their changes will be reflected in the underlying
   * memory.  See {@link com.xuggle.ferry.IBuffer} for the warnings
   * associated with the use of raw memory.  The buffer position and
   * mark are initialized to zero, and the limit is initialized to the
   * number of elements in the buffer.
   *
   * <p>
   *
   * This is a convenience method to allow you to avoid getting the
   * IBuffer object that has the actual data.
   *
   * </p>
   *
   * @return The underlying ByteBuffer
   * 
   * @see com.xuggle.ferry.IBuffer#getByteBuffer(int, int)
   */

  public java.nio.ByteBuffer getByteBuffer()
  {
    return getByteBuffer(null);
  }

  /**
   * Gets the underlying {@link java.nio.ByteBuffer} for this {@link
   * IMediaData} object.  Users may modify the contents of the
   * ByteBuffer and their changes will be reflected in the underlying
   * memory.  See {@link com.xuggle.ferry.IBuffer} for the warnings
   * associated with the use of raw memory.  The buffer position and
   * mark are initialized to zero, and the limit is initialized to the
   * number of elements in the buffer.
   *
   * <p>
   *
   * This is a convenience method to allow you to avoid getting the
   * IBuffer object that has the actual data.
   *
   * </p>
   *
   * @param ref A reference that on return from this method will
   *   contain a {@link com.xuggle.ferry.JNIReference} object
   *   that can be used for explicit memory management.
   * @return The underlying ByteBuffer
   * 
   * @see com.xuggle.ferry.IBuffer#getByteBuffer(int, int, java.util.concurrent.atomic.AtomicReference)
   */

  public java.nio.ByteBuffer getByteBuffer(
      java.util.concurrent.atomic.AtomicReference<com.xuggle.ferry.JNIReference> ref) {
    if (ref != null)
      ref.set(null);
      
    com.xuggle.ferry.IBuffer buf = this.getDataCached();
    if (buf == null)
      return null;
    java.nio.ByteBuffer byteBuffer = buf.getByteBuffer(0, this.getSize(), ref);
    if (byteBuffer != null)
    {
      byteBuffer.position(0);
      byteBuffer.mark();
      byteBuffer.limit(this.getSize());
    }
    return byteBuffer;
  }


  /**
   * Get a string representation of the time stamp for this {@link
   * IMediaData}.  The time is formatted as: <b>HH:MM:SS.ms</b>
   *
   * @return the printable string form of the time stamp of this media
   *
   * @see #getFormattedTimeStamp(String)
   * @see #DEFAULT_TIME_STAMP_FORMAT
   */

  public String getFormattedTimeStamp()
  {
    return getFormattedTimeStamp(DEFAULT_TIME_STAMP_FORMAT);
  }

  /**
   * Get a string representation of the time stamp for this {@link
   * IMediaData}.  The format of the resulting string is specified by
   * the format parameter.  See {@link java.util.Formatter} for 
   * details on how to specify formats, however a good place to start
   * is with the following format: <b>%1$tH:%1$tM:%1$tS.%1$tL</b>
   *
   * @param format the format for the time stamp string
   *
   * @return the printable string form of the timestamp
   * 
   * @see #getFormattedTimeStamp()
   * @see #DEFAULT_TIME_STAMP_FORMAT
   * @see java.util.Formatter
   */

  public String getFormattedTimeStamp(String format)
  {
    java.util.Formatter formatter = new java.util.Formatter();
    IRational timeBase = getTimeBase();
    if (timeBase == null)
      timeBase = IRational.make(1,(int)Global.DEFAULT_PTS_PER_SECOND);
    String retval = formatter.format(format,
      (long)(getTimeStamp() * timeBase.getDouble() * 1000) +
      TIME_OFFSET).toString();
    timeBase.delete();
    return retval;
  }
  
  /**
   * Get any underlying raw data available for this object.
   * 
   * @return The raw data, or null if not accessible.
   */
  public IBuffer getData()
  {
    IBuffer cachedData = getDataCached();
    return cachedData == null ? null : cachedData.copyReference();
  }

  private IBuffer mData;

  /**
   * Get any underlying raw data available for this object, but do
   * not create a new Java wrapping object for it.
   *
   * <p>
   * Do not call {@link com.xuggle.ferry.RefCounted#delete()} on
   * the returned value. 
   * </p>
   * 
   * @return The raw data, or null if not accessible.
   */
  public IBuffer getDataCached()
  {
    long cPtr = XugglerJNI.IMediaData_getData_internal(swigCPtr, this);
    if (mData == null)
      mData = (cPtr == 0) ? null : new IBuffer(cPtr, false);
    else {
      long cachedPtr = mData.getMyCPtr();
      if (cachedPtr == cPtr) {
        // release the ref
        com.xuggle.ferry.Ferry.release(cPtr);
      } else {
        mData.delete();
        mData = (cPtr == 0) ? null : new IBuffer(cPtr, false);
      }
    }
    return mData;
  }

  /**
   * Releases this instance of $javaclassname and frees any underlying
   * native memory.
   * <p>
   * {@inheritDoc}
   * </p> 
   */
  @Override
  public void delete()
  {
    if (mData != null) {
      mData.delete();
      mData = null;
    }
    super.delete();
  }


  
%}
%javamethodmodifiers com::xuggle::xuggler::IMediaData::getData "protected"
%rename(getData_internal) com::xuggle::xuggler::IMediaData::getData;

%include <com/xuggle/xuggler/IMediaData.h>
