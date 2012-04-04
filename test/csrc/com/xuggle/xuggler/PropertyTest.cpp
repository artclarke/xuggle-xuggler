/*******************************************************************************
 * Copyright (c) 2012 Xuggle Inc.  All rights reserved.
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
 * PropertyTest.cpp
 *
 *  Created on: Feb 2, 2012
 *      Author: aclarke
 */

#include <cstdlib>
#include <cstring>
#include "PropertyTest.h"

#include <com/xuggle/xuggler/IProperty.h>
#include <com/xuggle/xuggler/ICodec.h>
#include <com/xuggle/xuggler/IStreamCoder.h>

using namespace VS_CPP_NAMESPACE;

VS_LOG_SETUP(VS_CPP_PACKAGE);


PropertyTest :: PropertyTest()
{

}

PropertyTest :: ~PropertyTest()
{
}

void
PropertyTest :: setUp()
{
  
}

void
PropertyTest :: tearDown()
{
  
}

void
PropertyTest :: testCreation()
{
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_WARN, false);
  RefPointer<IStreamCoder> coder = IStreamCoder::make(IStreamCoder::ENCODING,
    ICodec::CODEC_ID_H264);
  RefPointer <IProperty> property =  coder->getPropertyMetaData("b");
  VS_LOG_DEBUG("Name: %s", property->getName());
  VS_LOG_DEBUG("Description: %s", property->getHelp());
  VS_TUT_ENSURE("should exist", property);
}

void
PropertyTest :: testIteration()
{
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_WARN, false);

  RefPointer<IStreamCoder> coder = IStreamCoder::make(IStreamCoder::ENCODING,
    ICodec::CODEC_ID_H264);

  int32_t numProperties = coder->getNumProperties();
  VS_TUT_ENSURE("", numProperties > 0);

  for(int32_t i = 0; i < numProperties; i++)
  {
    RefPointer <IProperty> property =  coder->getPropertyMetaData(i);
    VS_LOG_DEBUG("Name: %s", property->getName());
    VS_LOG_DEBUG("Description: %s", property->getHelp());
    VS_LOG_DEBUG("Default: %lld", property->getDefault());
    VS_LOG_DEBUG("Current value (boolean) : %d", (int32_t)coder->getPropertyAsBoolean(property->getName()));
    VS_LOG_DEBUG("Current value (double)  : %f", coder->getPropertyAsDouble(property->getName()));
    VS_LOG_DEBUG("Current value (long)    : %lld", coder->getPropertyAsLong(property->getName()));
    RefPointer<IRational> rational = coder->getPropertyAsRational(property->getName());
    VS_LOG_DEBUG("Current value (rational): %f", rational->getValue());
    char* value=coder->getPropertyAsString(property->getName());
    VS_LOG_DEBUG("Current value (string)  : %s", value);
    if (value) free(value);
  }
}

void
PropertyTest :: testSetMetaData()
{
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_WARN, false);

  RefPointer<IStreamCoder> coder = IStreamCoder::make(IStreamCoder::ENCODING,
    ICodec::CODEC_ID_H264);
  RefPointer<IMetaData> dict = IMetaData::make();
  RefPointer<IMetaData> unset = IMetaData::make();
  const char* realKey = "b";
  const char* fakeKey = "not-a-valid-key-no-way-all-hail-zod";
  const char* realValue = "1000";
  const char* fakeValue = "1025";
  dict->setValue(realKey, realValue);
  dict->setValue(fakeKey, fakeValue);

  VS_TUT_ENSURE("", dict->getNumKeys() == 2);
  VS_TUT_ENSURE("", unset->getNumKeys() == 0);

  VS_TUT_ENSURE("", coder->setProperty(dict.value(), unset.value()) == 0);

  VS_TUT_ENSURE("", coder->getPropertyAsLong(realKey) == 1000);

  // make sure the fake isn't there.
  RefPointer<IProperty> fakeProperty = coder->getPropertyMetaData(fakeKey);
  VS_TUT_ENSURE("", !fakeProperty);

  // now make sure the returned dictionary only had the fake in it.
  VS_TUT_ENSURE("", unset->getNumKeys() == 1);

  VS_TUT_ENSURE("", strcmp(unset->getValue(fakeKey, IMetaData::METADATA_NONE), fakeValue) == 0);
}

