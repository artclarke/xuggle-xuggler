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
