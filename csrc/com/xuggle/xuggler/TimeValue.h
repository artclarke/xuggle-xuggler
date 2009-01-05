/*
 * Copyright (c) 2008 by Vlideshow Inc. (a.k.a. The Yard).  All rights reserved.
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
 * TimeValue.h
 *
 *  Created on: Sep 19, 2008
 *      Author: aclarke
 */

#ifndef TIMEVALUE_H_
#define TIMEVALUE_H_

#include <climits>
#include <com/xuggle/xuggler/ITimeValue.h>

namespace com { namespace xuggle { namespace xuggler {

class TimeValue: public ITimeValue
{
  VS_JNIUTILS_REFCOUNTED_OBJECT_PRIVATE_MAKE(TimeValue);
public:
  virtual int64_t get(Unit unit);
  virtual int32_t compareTo(ITimeValue* other);

  Unit getNativeUnit();

  static TimeValue* make(int64_t value, Unit unit);
  static TimeValue* make(TimeValue *src);
  static inline int32_t compare(int64_t thisValue, int64_t thatValue)
  {
    int64_t retval;
    int64_t adjustment = 1;
    const static int64_t sMaxDistance = LLONG_MAX/2;
    if ((thisValue > sMaxDistance && thatValue <= -sMaxDistance) ||
        (thatValue > sMaxDistance && thisValue <= -sMaxDistance))
      // in these  cases we assume the time value has looped over
      // a int64_t value, and we really want to invert the diff.
      adjustment = -1;

    if (thisValue < thatValue)
      retval = -adjustment;
    else if (thisValue > thatValue)
      retval = adjustment;
    else
      retval = 0;
    return retval;
  }

protected:
  TimeValue();
  virtual ~TimeValue();
private:
  void set(int64_t value, Unit unit);
  int64_t mValue;
  Unit mUnit;
};

}}}

#endif /* TIMEVALUE_H_ */
