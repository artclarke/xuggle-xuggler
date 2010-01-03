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
 * MetaDataTest.cpp
 *
 *  Created on: Jun 30, 2009
 *      Author: aclarke
 */

#include <cstring>
#include "MetaDataTest.h"

#include <com/xuggle/xuggler/IMetaData.h>

using namespace VS_CPP_NAMESPACE;

VS_LOG_SETUP(VS_CPP_PACKAGE);


MetaDataTest :: MetaDataTest()
{

}

MetaDataTest :: ~MetaDataTest()
{
}

void
MetaDataTest :: setUp()
{
  
}

void
MetaDataTest :: tearDown()
{
  
}

void
MetaDataTest :: testCreation()
{
  RefPointer<IMetaData> metaData = IMetaData::make();
  VS_TUT_ENSURE_EQUALS("should be zero", metaData->getNumKeys(), 0);
  metaData->setValue("foo", "bar");
  VS_TUT_ENSURE_EQUALS("should be zero", metaData->getNumKeys(), 1);
  const char* value = metaData->getValue("foo", IMetaData::METADATA_NONE);
  VS_TUT_ENSURE("should be bar", strcmp("bar", value)==0);
  VS_TUT_ENSURE("should be empty", !metaData->getValue("none",
      IMetaData::METADATA_NONE));
}

void
MetaDataTest :: testContainerGetMetaData()
{
  Helper h;
  h.setupReading("testfile.mp3");
  RefPointer<IMetaData> meta = h.container->getMetaData();
  VS_TUT_ENSURE("got meta data", meta);
  if (meta) {
    int32_t numKeys = meta->getNumKeys();
    VS_TUT_ENSURE("should be right", numKeys >= 5);
    VS_TUT_ENSURE("should be right", numKeys <= 7);
    for(int32_t i = 0; i < numKeys; i++)
    {
      const char* key = meta->getKey(i);
      VS_TUT_ENSURE("should be found", key);
      VS_TUT_ENSURE("should be found", *key);
      const char* value = meta->getValue(key, IMetaData::METADATA_NONE);
      VS_TUT_ENSURE("should be found", value);
      VS_TUT_ENSURE("should be found", *value);
    }
  }
}
  
void
MetaDataTest :: testFLVContainerGetMetaData()
{
  Helper h;
  return;
  h.setupReading("testfile.flv");
  RefPointer<IMetaData> meta = h.container->getMetaData();
  VS_TUT_ENSURE("got meta data", meta);
  if (meta) {
    int32_t numKeys = meta->getNumKeys();
    if (numKeys > 0) { // using FFmpeg with a patch for FLV meta-data reading
      VS_TUT_ENSURE_EQUALS("should be right", numKeys, 11);
      for(int32_t i = 0; i < numKeys; i++)
      {
        const char* key = meta->getKey(i);
        VS_TUT_ENSURE("should be found", key);
        VS_TUT_ENSURE("should be found", *key);
        const char* value = meta->getValue(key, IMetaData::METADATA_NONE);
        VS_TUT_ENSURE("should be found", value);
        VS_TUT_ENSURE("should be found", *value);
      }
    }
  }
}
  
void
MetaDataTest :: testContainerSetMetaData()
{
  Helper h;
  h.setupWriting("testContainerSetMetaData.mp3",
      0, "libmp3lame", 0);
  RefPointer<IMetaData> meta = h.container->getMetaData();
  VS_TUT_ENSURE("got meta data", meta);
  meta = IMetaData::make();
  if (meta)
  {
    VS_TUT_ENSURE_EQUALS("", meta->getNumKeys(), 0);
    meta->setValue("author", "Art Clarke");
    h.container->setMetaData(meta.value());
  }
  meta = h.container->getMetaData();
  VS_TUT_ENSURE("got meta data", meta);
  if (meta)
  {
    VS_TUT_ENSURE_EQUALS("", meta->getNumKeys(), 1);
    const char* value = meta->getValue("author", IMetaData::METADATA_NONE);
    VS_TUT_ENSURE("", strcmp("Art Clarke",value)==0);
  }
}

void
MetaDataTest :: testContainerGetMetaDataIsWriteThrough()
{
  Helper h;
  h.setupWriting("testContainerSetMetaDataIsWriteThrough.mp3",
      0, "libmp3lame", 0);
  RefPointer<IMetaData> meta = h.container->getMetaData();
  VS_TUT_ENSURE("got meta data", meta);
  if (meta)
  {
    VS_TUT_ENSURE_EQUALS("", meta->getNumKeys(), 0);
    meta->setValue("author", "Art Clarke");
  }
  meta = h.container->getMetaData();
  VS_TUT_ENSURE("got meta data", meta);
  if (meta)
  {
    VS_TUT_ENSURE_EQUALS("", meta->getNumKeys(), 1);
    const char* value = meta->getValue("author", IMetaData::METADATA_NONE);
    VS_TUT_ENSURE("", strcmp("Art Clarke",value)==0);
  }
}
void
MetaDataTest :: testStreamGetMetaData()
{
  Helper h;
  h.setupReading("testfile.mp3");
  RefPointer<IStream> stream = h.container->getStream(0);
  RefPointer<IMetaData> meta = stream->getMetaData();
  VS_TUT_ENSURE("got meta data", meta);
  if (meta) {
    int32_t numKeys = meta->getNumKeys();
    VS_TUT_ENSURE_EQUALS("should be right", numKeys, 0);
    for(int32_t i = 0; i < numKeys; i++)
    {
      const char* key = meta->getKey(i);
      VS_TUT_ENSURE("should be found", key);
      VS_TUT_ENSURE("should be found", *key);
      const char* value = meta->getValue(key, IMetaData::METADATA_NONE);
      VS_TUT_ENSURE("should be found", value);
      VS_TUT_ENSURE("should be found", *value);
    }
  }
}
  
void
MetaDataTest :: testStreamSetMetaData()
{
  Helper h;
  h.setupWriting("testStreamSetMetaData.mp3",
      0, "libmp3lame", 0);
  RefPointer<IStream> stream = h.container->getStream(0);
  RefPointer<IMetaData> meta = stream->getMetaData();
  VS_TUT_ENSURE("got meta data", meta);
  meta = IMetaData::make();
  if (meta)
  {
    VS_TUT_ENSURE_EQUALS("", meta->getNumKeys(), 0);
    meta->setValue("author", "Art Clarke");
    stream->setMetaData(meta.value());
  }
  meta = stream->getMetaData();
  VS_TUT_ENSURE("got meta data", meta);
  if (meta)
  {
    VS_TUT_ENSURE_EQUALS("", meta->getNumKeys(), 1);
    const char* value = meta->getValue("author", IMetaData::METADATA_NONE);
    VS_TUT_ENSURE("", strcmp("Art Clarke",value)==0);
  }
}

void
MetaDataTest :: testStreamGetMetaDataIsWriteThrough()
{
  Helper h;
  h.setupWriting("testStreamSetMetaDataIsWriteThrough.mp3",
      0, "libmp3lame", 0);
  RefPointer<IStream> stream = h.container->getStream(0);
  RefPointer<IMetaData> meta = stream->getMetaData();
  VS_TUT_ENSURE("got meta data", meta);
  if (meta)
  {
    VS_TUT_ENSURE_EQUALS("", meta->getNumKeys(), 0);
    meta->setValue("author", "Art Clarke");
  }
  meta = stream->getMetaData();
  VS_TUT_ENSURE("got meta data", meta);
  if (meta)
  {
    VS_TUT_ENSURE_EQUALS("", meta->getNumKeys(), 1);
    const char* value = meta->getValue("author", IMetaData::METADATA_NONE);
    VS_TUT_ENSURE("", strcmp("Art Clarke",value)==0);
  }
}
