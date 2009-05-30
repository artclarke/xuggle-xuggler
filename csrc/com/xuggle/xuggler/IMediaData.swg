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
  
  public static final String DEFALUT_TIME_STAMP_FORMAT =
    "%1$tH:%1$tM:%1$tS.%1$tL";

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
      
    com.xuggle.ferry.IBuffer buf = this.getData();
    if (buf == null)
      return null;
    try
    {
      java.nio.ByteBuffer byteBuffer = buf.getByteBuffer(0, this.getSize(), ref);
      if (byteBuffer != null)
      {
        byteBuffer.position(0);
        byteBuffer.mark();
        byteBuffer.limit(this.getSize());
      }
      return byteBuffer;
    }
    finally
    {
      buf.delete();
    }
  }


  /**
   * Get a string representation of the time stamp for this {@link
   * IMediaData}.  The time is formatted as: <b>HH:MM:SS.ms</b>
   *
   * @return the printable string form of the time stamp of this media
   *
   * @see #getFormattedTimeStamp(String)
   * @see #DEFALUT_TIME_STAMP_FORMAT
   */

  public String getFormattedTimeStamp()
  {
    return getFormattedTimeStamp(DEFALUT_TIME_STAMP_FORMAT);
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
   * @see #DEFALUT_TIME_STAMP_FORMAT
   * @see java.util.Formatter
   */

  public String getFormattedTimeStamp(String format)
  {
    java.util.Formatter formatter = new java.util.Formatter();
    return formatter.format(format,
      (long)(getTimeStamp() * getTimeBase().getDouble() * 1000) +
      TIME_OFFSET).toString();
  }
%}

%include <com/xuggle/xuggler/IMediaData.h>
