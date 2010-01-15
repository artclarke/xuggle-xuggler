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
 * IndexEntryTest.cpp
 *
 *  Created on: Jan 14, 2010
 *      Author: aclarke
 */
#include "IndexEntryTest.h"
#include <com/xuggle/xuggler/IIndexEntry.h>
#include <com/xuggle/xuggler/IContainer.h>
#include <com/xuggle/xuggler/IStream.h>
IndexEntryTest :: IndexEntryTest()
{

}

IndexEntryTest :: ~IndexEntryTest()
{

}

void
IndexEntryTest :: setUp()
{

}

void
IndexEntryTest :: tearDown()
{

}

void
IndexEntryTest :: testCreation()
{
  int64_t pos = 1;
  int64_t ts = 2;
  int32_t flags = IIndexEntry::IINDEX_FLAG_KEYFRAME;
  int32_t size = 4;
  int32_t minDistance = 5;
  RefPointer<IIndexEntry> entry = IIndexEntry::make(
      pos,
      ts,
      flags,
      size,
      minDistance);
  VS_TUT_ENSURE("should exist", entry);
  if (!entry)
    return;
  VS_TUT_ENSURE_EQUALS("", pos, entry->getPosition());
  VS_TUT_ENSURE_EQUALS("", ts, entry->getTimeStamp());
  VS_TUT_ENSURE_EQUALS("", flags, entry->getFlags());
  VS_TUT_ENSURE_EQUALS("", size, entry->getSize());
  VS_TUT_ENSURE_EQUALS("", minDistance, entry->getMinDistance());
}

void
IndexEntryTest :: testAddIndexEntry()
{
  RefPointer<IContainer> container = IContainer::make();
  int retval = container->open("IIndexEntry_1.mov", IContainer::WRITE, 0);
  VS_TUT_ENSURE("", retval >= 0);
  RefPointer<IStream> stream = container->addNewStream(0);
  int64_t pos = 1;
  int64_t ts = 2;
  int32_t flags = IIndexEntry::IINDEX_FLAG_KEYFRAME;
  int32_t size = 4;
  int32_t minDistance = 5;
  RefPointer<IIndexEntry> entry = IIndexEntry::make(
      pos,
      ts,
      flags,
      size,
      minDistance);
  VS_TUT_ENSURE_EQUALS("", 0, stream->getNumIndexEntries());
  VS_TUT_ENSURE("should exist", entry);
  retval = stream->addIndexEntry(entry.value());
  VS_TUT_ENSURE("", retval >= 0);
  VS_TUT_ENSURE_EQUALS("", 1, stream->getNumIndexEntries());

  RefPointer<IIndexEntry> foundEntry = stream->getIndexEntry(0);
  VS_TUT_ENSURE("", foundEntry);
  VS_TUT_ENSURE_EQUALS("", entry->getPosition(), foundEntry->getPosition());
  VS_TUT_ENSURE_EQUALS("", entry->getTimeStamp(), foundEntry->getTimeStamp());
  VS_TUT_ENSURE_EQUALS("", entry->getFlags(), foundEntry->getFlags());
  VS_TUT_ENSURE_EQUALS("", entry->getSize(), foundEntry->getSize());
  VS_TUT_ENSURE_EQUALS("", entry->getMinDistance(), foundEntry->getMinDistance());

  foundEntry = stream->findTimeStampEntryInIndex(ts, 0);
  VS_TUT_ENSURE("", foundEntry);
  VS_TUT_ENSURE_EQUALS("", entry->getPosition(), foundEntry->getPosition());
  VS_TUT_ENSURE_EQUALS("", entry->getTimeStamp(), foundEntry->getTimeStamp());
  VS_TUT_ENSURE_EQUALS("", entry->getFlags(), foundEntry->getFlags());
  VS_TUT_ENSURE_EQUALS("", entry->getSize(), foundEntry->getSize());
  VS_TUT_ENSURE_EQUALS("", entry->getMinDistance(), foundEntry->getMinDistance());

  retval = container->close();
  VS_TUT_ENSURE("", retval >= 0);
}
