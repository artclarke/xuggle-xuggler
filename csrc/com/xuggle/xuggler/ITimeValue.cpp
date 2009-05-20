/*
 * This file is part of Xuggler.
 * 
 * Xuggler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public
 * License along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * ITimeValue.cpp
 *
 *  Created on: Sep 19, 2008
 *      Author: aclarke
 */

#include "ITimeValue.h"
#include "TimeValue.h"

namespace com { namespace xuggle { namespace xuggler
{

ITimeValue::ITimeValue()
{
}

ITimeValue::~ITimeValue()
{
}

ITimeValue*
ITimeValue::make(int64_t aValue, Unit aUnit)
{
  return TimeValue::make(aValue, aUnit);
}

ITimeValue*
ITimeValue::make(ITimeValue *src)
{
  return TimeValue::make(dynamic_cast<TimeValue*>(src));
}

int32_t
ITimeValue::compare(ITimeValue *a, ITimeValue*b)
{
  int32_t retval = 0;
  if (a)
  {
    retval = a->compareTo(b);
  }
  else
  {
    if (b)
      // null is always less than any non-null value
      retval = 1;
    else // both are null
      retval = 0;
  }

  return retval;
}

int32_t
ITimeValue::compare(int64_t a, int64_t b)
{
  return TimeValue::compare(a, b);
}
}}}
