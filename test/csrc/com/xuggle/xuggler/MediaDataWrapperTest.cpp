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

#include <stdexcept>

#include <com/xuggle/ferry/RefPointer.h>
#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/xuggler/Global.h>
#include <com/xuggle/xuggler/IMediaDataWrapper.h>
#include <com/xuggle/xuggler/IAudioSamples.h>
#include "MediaDataWrapperTest.h"

using namespace VS_CPP_NAMESPACE;
using namespace com::xuggle::ferry;

VS_LOG_SETUP(VS_CPP_PACKAGE);

void
MediaDataWrapperTest :: testCreationAndDestruction()
{
  RefPointer<IMediaDataWrapper> obj = IMediaDataWrapper::make(0);
  VS_TUT_ENSURE("could not make", obj);
}

void
MediaDataWrapperTest :: getDefaults()
{
  RefPointer<IMediaDataWrapper> obj = IMediaDataWrapper::make(0);
  VS_TUT_ENSURE("could not make", obj);
 
  RefPointer<IRational> timeBase;
  
  timeBase = obj->getTimeBase();
  VS_TUT_ENSURE("no time base", !timeBase);
  
  long long expectedTimeStamp = Global::NO_PTS;
  long long timeStamp = obj->getTimeStamp();
  VS_TUT_ENSURE_EQUALS("unexpected time stamp", timeStamp, expectedTimeStamp);
  
  bool isKey = obj->isKey();
  VS_TUT_ENSURE_EQUALS("unexpected type", isKey, true);
  
  RefPointer<IBuffer> data = obj->getData();
  VS_TUT_ENSURE("unexpected buffer", !data);
  
  int size = obj->getSize();
  VS_TUT_ENSURE_EQUALS("unexpected size", size, -1);
}

void
MediaDataWrapperTest :: testSetters()
{
  RefPointer<IMediaDataWrapper> obj = IMediaDataWrapper::make(0);
  VS_TUT_ENSURE("could not make", obj);
 
  RefPointer<IRational> expectedTimeBase = IRational::make(1, 1000);
  obj->setTimeBase(expectedTimeBase.value());
  
  RefPointer<IRational> timeBase;
  timeBase = obj->getTimeBase();
  VS_TUT_ENSURE("no time base", timeBase);
  VS_TUT_ENSURE_EQUALS("unexpected time base numerator", timeBase->getNumerator(), expectedTimeBase->getNumerator());
  VS_TUT_ENSURE_EQUALS("unexpected time base denominator", timeBase->getDenominator(), expectedTimeBase->getDenominator());
  VS_TUT_ENSURE_EQUALS("unexpected ref count", timeBase->getCurrentRefCount(), 3);
  
  long long expectedTimeStamp = 16;
  obj->setTimeStamp(16);
  
  long long timeStamp = obj->getTimeStamp();
  VS_TUT_ENSURE_EQUALS("unexpected time stamp", timeStamp, expectedTimeStamp);
  
  bool expectedKey = false;
  obj->setKey(expectedKey);
  bool isKey = obj->isKey();
  VS_TUT_ENSURE_EQUALS("unexpected type", isKey, expectedKey);
}

void
MediaDataWrapperTest :: testNullTimeBase()
{
  RefPointer<IMediaDataWrapper> obj = IMediaDataWrapper::make(0);
  VS_TUT_ENSURE("could not make", obj);
  obj->setTimeBase(0);
  // should be no exception
}

void
MediaDataWrapperTest :: testWrapping()
{
  RefPointer<IMediaDataWrapper> obj = IMediaDataWrapper::make(0);
  VS_TUT_ENSURE("could not make", obj);
  RefPointer<IRational> objBase = IRational::make(1,10);
  obj->setTimeBase(objBase.value());
  long long objTimeStamp = 1;
  obj->setTimeStamp(objTimeStamp);
  bool objKeyStatus = false;
  obj->setKey(objKeyStatus);
  
  RefPointer<IMediaDataWrapper> wrapper = IMediaDataWrapper::make(obj.value());

  RefPointer<IRational> wrappedBase = wrapper->getTimeBase(); 
  VS_TUT_ENSURE("no time base", wrappedBase);
  VS_TUT_ENSURE_EQUALS("wrong numerator", wrappedBase->getNumerator(), objBase->getNumerator());
  VS_TUT_ENSURE_EQUALS("wrong denominator", wrappedBase->getDenominator(), objBase->getDenominator());
  VS_TUT_ENSURE_EQUALS("wrong time stamp", wrapper->getTimeStamp(), objTimeStamp);
  VS_TUT_ENSURE_EQUALS("wrong key status", wrapper->isKey(), objKeyStatus);
  
  long long newTimeStamp = 2;
  bool newKeyStatus = true;
  RefPointer<IRational> newBase = IRational::make(2,33);

  wrapper->setTimeBase(newBase.value());
  wrapper->setTimeStamp(newTimeStamp);
  wrapper->setKey(newKeyStatus);
  
  wrappedBase = wrapper->getTimeBase();
  VS_TUT_ENSURE("no time base", wrappedBase);
  VS_TUT_ENSURE_EQUALS("wrong numerator", wrappedBase->getNumerator(), newBase->getNumerator());
  VS_TUT_ENSURE_EQUALS("wrong denominator", wrappedBase->getDenominator(), newBase->getDenominator());
  VS_TUT_ENSURE_EQUALS("wrong time stamp", wrapper->getTimeStamp(), newTimeStamp);
  VS_TUT_ENSURE_EQUALS("wrong key status", wrapper->isKey(), newKeyStatus);
  
  VS_TUT_ENSURE("wrong numerator", wrappedBase->getNumerator() != objBase->getNumerator());
  VS_TUT_ENSURE("wrong denominator", wrappedBase->getDenominator() != objBase->getDenominator());
  VS_TUT_ENSURE("wrong time stamp", wrapper->getTimeStamp() != objTimeStamp);
  VS_TUT_ENSURE("wrong key status", wrapper->isKey() != objKeyStatus);
  
  // Let's try wrapping ourselves; this should result in an error and not actually change the wrapped value.
  VS_TUT_ENSURE("make sure we're null", !obj->get());
  RefPointer<IMediaData> wrappedObj = wrapper->get();
  VS_TUT_ENSURE_EQUALS("should be equal", obj.value(), wrappedObj.value());
  // have the underlying wrapper wrap itself; this would cause a wrap loop if we didn't detect it
  {
    // Turn off logging while we do this test
    LoggerStack stack;
    stack.setGlobalLevel(Logger::LEVEL_ERROR, false);
    obj->wrap(wrapper.value());
  }
  VS_TUT_ENSURE("should be null", !obj->get());    
}

void
MediaDataWrapperTest :: testUnwrapping()
{
  RefPointer<IMediaDataWrapper> obj = IMediaDataWrapper::make(0);
  VS_TUT_ENSURE("could not make", obj);
  RefPointer<IRational> objBase = IRational::make(1,10);
  obj->setTimeBase(objBase.value());
  long long objTimeStamp = 1;
  obj->setTimeStamp(objTimeStamp);
  bool objKeyStatus = false;
  obj->setKey(objKeyStatus);
  
  RefPointer<IMediaDataWrapper> wrapper = IMediaDataWrapper::make(obj.value());

  // a wrapper wrapping just a rapper should return null.
  VS_TUT_ENSURE("unwrapping should return null", !wrapper->unwrap());
 
  // let's try wrapping a non null value
  RefPointer<IMediaData> audio = IAudioSamples::make(1024, 2);
  VS_TUT_ENSURE("got audio", audio);
  
  obj->wrap(audio.value());

  RefPointer<IMediaData> unwrapped = wrapper->unwrap();
  VS_TUT_ENSURE_EQUALS("should be our friend audio", audio.value(), unwrapped.value());
  
 }
