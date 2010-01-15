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
 * IndexEntry.cpp
 *
 *  Created on: Mar 20, 2009
 *      Author: aclarke
 */

#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/xuggler/IndexEntry.h>

#include <cstring>
#include <stdexcept>

VS_LOG_SETUP(VS_CPP_PACKAGE);

namespace com { namespace xuggle { namespace xuggler
{

IndexEntry :: IndexEntry()
{
}

IndexEntry :: ~IndexEntry()
{
  
}

int64_t
IndexEntry::getPosition()
{
  return mEntry.pos;
}

int64_t
IndexEntry::getTimeStamp()
{
  return mEntry.timestamp;
}

int32_t
IndexEntry::getFlags()
{
  return mEntry.flags;
}

int32_t
IndexEntry::getSize()
{
  return mEntry.size;
}

int32_t
IndexEntry::getMinDistance()
{
  return mEntry.min_distance;
}

bool
IndexEntry::isKeyFrame()
{
  return mEntry.flags & AVINDEX_KEYFRAME;
}

IndexEntry*
IndexEntry::make(int64_t position, int64_t timeStamp,
    int32_t flags, int32_t size, int32_t minDistance)
{
  IndexEntry* retval = make();
  if (retval) {
    retval->mEntry.pos = position;
    retval->mEntry.timestamp = timeStamp;
    retval->mEntry.flags = flags;
    retval->mEntry.size = size;
    retval->mEntry.min_distance = minDistance;
  }
  return retval;
}
}}}
