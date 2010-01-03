/*******************************************************************************
 * Copyright (c) 2008, 2010 Xuggle Inc.  All rights reserved.
 *  
 * This file is part of Xuggle-Xuggler-Main.
 *
 * Xuggle-Xuggler-Main is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xuggle-Xuggler-Main is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Xuggle-Xuggler-Main.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/

/*
 * TimeValue.cpp
 *
 *  Created on: Sep 19, 2008
 *      Author: aclarke
 */

#include <climits>
#include "TimeValue.h"

namespace com { namespace xuggle { namespace xuggler {

static int64_t sConversionTable[7][7] =
{        /*  DY, HR,   MI,    SE,       MS,          US,               NS */
  /* DY */ {  1, 24, 1440, 86400, 86400000, 86400000000LL, 86400000000000LL },
  /* HR */ {  0,  1,   60,  3600,  3600000,  3600000000LL,  3600000000000LL },
  /* MI */ {  0,  0,    1,    60,    60000,    60000000LL,    60000000000LL },
  /* SE */ {  0,  0,    0,     1,     1000,     1000000LL,     1000000000LL },
  /* MS */ {  0,  0,    0,     0,        1,        1000LL,        1000000LL },
  /* US */ {  0,  0,    0,     0,        0,           1LL,           1000LL },
  /* NS */ {  0,  0,    0,     0,        0,           0LL,              1LL },
};

TimeValue::TimeValue()
{
  mValue = 0;
  mUnit = NANOSECONDS;
}

TimeValue::~TimeValue()
{
}

void
TimeValue::set(int64_t value, Unit unit)
{
  mValue = value;
  mUnit = unit;
}

TimeValue*
TimeValue::make(int64_t value, Unit unit)
{
  TimeValue *retval = make();
  if (retval)
    retval->set(value, unit);
  return retval;
}

TimeValue*
TimeValue::make(TimeValue * src)
{
  if (!src)
    return 0;
  return make(src->mValue, src->mUnit);
}

ITimeValue::Unit
TimeValue::getNativeUnit()
{
  return mUnit;
}

int64_t
TimeValue::get(Unit aUnit)
{
  if (mUnit == aUnit)
    return mValue;

  int64_t retval = 0;
  bool multiply = false;
  int64_t factor = sConversionTable[aUnit][mUnit];
  if (!factor)
  {
    factor = sConversionTable[mUnit][aUnit];
    multiply = true;
  }
  if (multiply)
    retval = mValue * factor;
  else
    retval = mValue / factor;

  return retval;
}

int32_t
TimeValue::compareTo(ITimeValue* aThat)
{
  int32_t retval = -1;
  TimeValue* that = dynamic_cast<TimeValue*>(aThat);
  if (that)
  {
    // convert both to NANOSECONDS
    Unit thisUnit = this->getNativeUnit();
    Unit thatUnit = that->getNativeUnit();

    // NOTE: Order of the entires in the Enum matters here.
    Unit minUnit = thisUnit > thatUnit ? thisUnit : thatUnit;

    int64_t thisValue = this->get(minUnit);
    int64_t thatValue = that->get(minUnit);
    retval = TimeValue::compare(thisValue, thatValue);
  }
  return retval;
}

}}}
