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
 * TimeValueTest.cpp
 *
 *  Created on: Sep 19, 2008
 *      Author: aclarke
 */
#include <climits>

#include "TimeValueTest.h"

using namespace VS_CPP_NAMESPACE;
using namespace com::xuggle::ferry;

void
TimeValueTest :: setUp()
{
  mObj = 0;
}

void
TimeValueTest :: testCreationAndDestruction()
{
  mObj = ITimeValue::make(15, ITimeValue::NANOSECONDS);
  VS_TUT_ENSURE("", mObj);
}

void
TimeValueTest :: testGetViaDowngrade()
{
  mObj = ITimeValue::make(1, ITimeValue::DAYS);
  VS_TUT_ENSURE("", mObj);
  long long conversion = 1;
  VS_TUT_ENSURE_EQUALS("days", mObj->get(ITimeValue::DAYS), conversion);
  conversion *= 24;
  VS_TUT_ENSURE_EQUALS("hours", mObj->get(ITimeValue::HOURS), conversion);
  conversion *= 60;
  VS_TUT_ENSURE_EQUALS("minutes", mObj->get(ITimeValue::MINUTES), conversion);
  conversion *= 60;
  VS_TUT_ENSURE_EQUALS("seconds", mObj->get(ITimeValue::SECONDS), conversion);
  conversion *= 1000;
  VS_TUT_ENSURE_EQUALS("milliseconds", mObj->get(ITimeValue::MILLISECONDS), conversion);
  conversion *= 1000;
  VS_TUT_ENSURE_EQUALS("microseconds", mObj->get(ITimeValue::MICROSECONDS), conversion);
  conversion *= 1000;
  VS_TUT_ENSURE_EQUALS("nanoseconds", mObj->get(ITimeValue::NANOSECONDS), conversion);
}

void
TimeValueTest :: testGetViaUpgrade()
{
  // we compute x days in nanoseconds
  long long numDays = 2;
  long long nanoValue = numDays;
  nanoValue *= 24;   // Hours
  nanoValue *= 60;   // Minutes
  nanoValue *= 60;   // Seconds
  nanoValue *= 1000; // Milliseconds
  nanoValue *= 1000; // Microseconds
  nanoValue *= 1000; // Nanoseconds

  mObj = ITimeValue::make(nanoValue, ITimeValue::NANOSECONDS);
  VS_TUT_ENSURE("", mObj);

  VS_TUT_ENSURE_EQUALS("nanoseconds", mObj->get(ITimeValue::NANOSECONDS), nanoValue);
  nanoValue /= 1000;
  VS_TUT_ENSURE_EQUALS("microseconds", mObj->get(ITimeValue::MICROSECONDS), nanoValue);
  nanoValue /= 1000;
  VS_TUT_ENSURE_EQUALS("milliseconds", mObj->get(ITimeValue::MILLISECONDS), nanoValue);
  nanoValue /= 1000;
  VS_TUT_ENSURE_EQUALS("seconds", mObj->get(ITimeValue::SECONDS), nanoValue);
  nanoValue /= 60;
  VS_TUT_ENSURE_EQUALS("minutes", mObj->get(ITimeValue::MINUTES), nanoValue);
  nanoValue /= 60;
  VS_TUT_ENSURE_EQUALS("hours", mObj->get(ITimeValue::HOURS), nanoValue);
  nanoValue /= 24;
  VS_TUT_ENSURE_EQUALS("days", mObj->get(ITimeValue::DAYS), nanoValue);
  VS_TUT_ENSURE_EQUALS("make sure the test conversion is right as well", nanoValue, numDays);
}

void
TimeValueTest :: testCompareTo()
{
  RefPointer<ITimeValue> obj1 = ITimeValue::make(-1, ITimeValue::NANOSECONDS);
  RefPointer<ITimeValue> obj2 = ITimeValue::make(0, ITimeValue::NANOSECONDS);
  RefPointer<ITimeValue> obj1Copy = ITimeValue::make(obj1.value());


  VS_TUT_ENSURE_EQUALS("1 < 2", obj1->compareTo(obj2.value()), -1);
  VS_TUT_ENSURE_EQUALS("1 == 1", obj1->compareTo(obj1Copy.value()), 0);

  long long val1 = LLONG_MAX;
  long long val2 = LLONG_MIN;
  obj1 = ITimeValue::make(val1, ITimeValue::SECONDS);
  obj2 = ITimeValue::make(val2, ITimeValue::SECONDS);

  VS_TUT_ENSURE("raw 1 > 2", val1 > val2);
  // the TimeValue class should assume the value has looped around because the
  // distance between the two values is relatively LARGE
  VS_TUT_ENSURE_EQUALS("timevalue 1 < 2", obj1->compareTo(obj2.value()), -1);
}

