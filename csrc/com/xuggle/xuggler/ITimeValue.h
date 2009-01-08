/*
 * Copyright (c) 2008-2009 by Xuggle Inc. All rights reserved.
 *
 * It is REQUESTED BUT NOT REQUIRED if you use this library, that you let 
 * us know by sending e-mail to info@xuggle.com telling us briefly how you're
 * using the library and what you like or don't like about it.
 *
 * This library is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any later
 * version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
/*
 * ITimeValue.h
 *
 *  Created on: Sep 19, 2008
 *      Author: aclarke
 */

#ifndef ITIMEVALUE_H_
#define ITIMEVALUE_H_

#include <com/xuggle/ferry/RefCounted.h>
#include <com/xuggle/xuggler/Xuggler.h>

namespace com { namespace xuggle { namespace xuggler
{

/**
 * Time (duration) values with units.
 * This class also has methods
 * that correctly account for long wrapping when comparing time values.
 */
class VS_API_XUGGLER ITimeValue: public com::xuggle::ferry::RefCounted
{
public:
  typedef enum
  {
    DAYS,
    HOURS,
    MINUTES,
    SECONDS,
    MILLISECONDS,
    MICROSECONDS,
    NANOSECONDS,
  } Unit;

  /**
   * Make a new time value.
   * @param value The value.
   * @param unit The unit value is expressed in.
   * @return a new time value.
   */
  static ITimeValue* make(int64_t value, Unit unit);

  /**
   * Make a copy of a time value from another time value.
   * @param src The time value to copy.  If 0 this method returns 0.
   * @return a new time value.
   */
  static ITimeValue* make(ITimeValue *src);

  /**
   * Get the value of this ITimeValue, in the specified Unit.
   * @param unit The unit you want to get a value as.
   * @return The value, converted into the appropriate Unit.
   */
  virtual int64_t get(Unit unit) = 0;

  /**
   * Compare this timeValue to another.
   * This compareTo will compare the values, but will assume that the values
   * can never be more than half of int64_t's MAX_VALUE apart.  If they are
   * it will assume long wrapping has occurred.  This is required especially
   * if you're using TimeValue's as absolute time stamps, and want to know
   * which is earlier.
   * @param other the value to compare to
   * @return -1 if this < other; +1 if this > other; 0 otherwise
   */
  virtual int32_t compareTo(ITimeValue* other)=0;

  /**
   * Convenience method that calls a.compareTo(b).
   * @see #compareTo(ITimeValue)
   * @param a first value.
   * @param b second value.
   * @return -1 if a < b; +1 if a > b; 0 otherwise
   *
   */
  static int32_t compare(ITimeValue *a, ITimeValue* b);

  /**
   * And another convenience method that deals with un-unitted long values.
   * @param a the first object.
   * @param b the second object.
   * @return #compare(ITimeValue, ITimeValue) where both a and b are assumed to be MICROSECONDS.
   */
  static int32_t compare(int64_t a, int64_t b);

protected:
  ITimeValue();
  virtual ~ITimeValue();

};

}}}
#endif /* ITIMEVALUE_H_ */
