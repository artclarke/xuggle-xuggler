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
 * IIndexEntry.cpp
 *
 *  Created on: Mar 20, 2009
 *      Author: aclarke
 */

#include <com/xuggle/xuggler/IIndexEntry.h>
#include <com/xuggle/xuggler/IndexEntry.h>

namespace com { namespace xuggle { namespace xuggler
{

IIndexEntry :: IIndexEntry()
{
  
}

IIndexEntry :: ~IIndexEntry()
{

}

IIndexEntry*
IIndexEntry::make(int64_t position, int64_t timeStamp,
    int32_t flags, int32_t size, int32_t minDistance)
{
  return IndexEntry::make(position, timeStamp,
      flags, size, minDistance);
}

}}}
