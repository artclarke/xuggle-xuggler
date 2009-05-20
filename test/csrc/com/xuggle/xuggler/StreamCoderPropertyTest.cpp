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

#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/xuggler/IStreamCoder.h>
#include <com/xuggle/xuggler/Global.h>
#include "Helper.h"
#include "StreamCoderPropertyTest.h"

#include <cstring>

using namespace VS_CPP_NAMESPACE;

VS_LOG_SETUP(VS_CPP_PACKAGE);

StreamCoderPropertyTest :: StreamCoderPropertyTest()
{
  h = 0;
  hw = 0;
}

StreamCoderPropertyTest :: ~StreamCoderPropertyTest()
{
  tearDown();
}

void
StreamCoderPropertyTest :: setUp()
{
  coder = 0;
  h = new Helper();
  hw = new Helper();
}

void
StreamCoderPropertyTest :: tearDown()
{
  coder = 0;
  if (h)
    delete h;
  h = 0;
  if (hw)
    delete hw;
  hw = 0;
}

void
StreamCoderPropertyTest :: testSetProperty()
{
  int retval = 0;
  h->setupReading(h->SAMPLE_FILE);
  
  VS_TUT_ENSURE("need at least one stream", h->num_streams > 0);
  
  int32_t bitrate = h->coders[0]->getBitRate();
  bitrate += 20000; // add something
  // set to a value we know
  h->coders[0]->setBitRate(bitrate);
  
  // now try setting the b option
  retval = h->coders[0]->setProperty("b", "500");
  VS_TUT_ENSURE("could not set option", retval >= 0);
  
  int32_t newbitrate = h->coders[0]->getBitRate();
  
  VS_TUT_ENSURE_EQUALS("option didn't work", newbitrate, 500);
  
  VS_TUT_ENSURE("and should not be old bit rate", newbitrate != bitrate);
  
  // now test that we get errors when we set bad properties
  {
    // turn down logging
    LoggerStack stack;
    stack.setGlobalLevel(Logger::LEVEL_ERROR, false);

    retval = h->coders[0]->setProperty("flags", "+not_a_valid_setting");
    VS_TUT_ENSURE("did not get error when expected", retval < 0);
  }

  {
    // turn down logging
    LoggerStack stack;
    stack.setGlobalLevel(Logger::LEVEL_ERROR, false);

    retval = h->coders[0]->setProperty("b", "not_a_valid_setting");
    VS_TUT_ENSURE("did not get error when expected", retval < 0);
  }

}

void
StreamCoderPropertyTest :: testGetProperty()
{
  int retval = 0;
  h->setupReading(h->SAMPLE_FILE);
  
  VS_TUT_ENSURE("need at least one stream", h->num_streams > 0);
  
  int32_t bitrate = h->coders[0]->getBitRate();
  bitrate += 20000; // add something
  // set to a value we know
  h->coders[0]->setBitRate(bitrate);
  
  // now try setting the b option
  retval = h->coders[0]->setProperty("b", "500");
  VS_TUT_ENSURE("could not set option", retval >= 0);
 
  int32_t newbitrate = h->coders[0]->getBitRate();
  
  VS_TUT_ENSURE_EQUALS("option didn't work", newbitrate, 500);
  
  VS_TUT_ENSURE("and should not be old bit rate", newbitrate != bitrate);
  
  char* val=h->coders[0]->getPropertyAsString("b");
  VS_TUT_ENSURE("should be 500", strcmp(val, "500")==0);

  // now we must delete val;
  delete [] val;
}

void
StreamCoderPropertyTest :: testGetNumProperties()
{
  h->setupReading(h->SAMPLE_FILE);
  
  VS_TUT_ENSURE("need at least one stream", h->num_streams > 0);

  int32_t numProperties = h->coders[0]->getNumProperties();
  VS_TUT_ENSURE("should get at least 100 properties", numProperties > 100);
}

void
StreamCoderPropertyTest :: testGetPropertyMetaData()
{
  h->setupReading(h->SAMPLE_FILE);
  
  VS_TUT_ENSURE("need at least one stream", h->num_streams > 0);

  int32_t numProperties = h->coders[0]->getNumProperties();
  VS_TUT_ENSURE("should get at least 100 properties", numProperties > 100);
  for(int32_t i = 0; i < numProperties; i++)
  {
    RefPointer<IProperty> metaData = h->coders[0]->getPropertyMetaData(i);
    VS_TUT_ENSURE("got no option", metaData);
    const char* optionName = metaData->getName();
    VS_TUT_ENSURE("got no option name", optionName && *optionName);
    char *optionValue = h->coders[0]->getPropertyAsString(optionName);
//    const char *optionHelp = metaData->getHelp();
//    const char *optionUnit = metaData->getUnit();
//    VS_LOG_DEBUG("Option [%s:%d] = [%s]; unit=[%s]; help=[%s]; flags=%d;",
//        optionName,
//        metaData->getType(),
//        optionValue ? optionValue : "(null)",
//        optionUnit ? optionUnit : "(null)",
//        optionHelp ? optionHelp : "(null)",
//        metaData->getFlags()
//    );
    delete [] optionValue;
  }
  // and check that out of range values fail
  VS_TUT_ENSURE("should get null", !h->coders[0]->getPropertyMetaData(-1));
  VS_TUT_ENSURE("should get null", !h->coders[0]->getPropertyMetaData(numProperties));
}

void
StreamCoderPropertyTest :: testGetNumFlagSettings()
{
  h->setupReading(h->SAMPLE_FILE);
  
  VS_TUT_ENSURE("need at least one stream", h->num_streams > 0);
  
  RefPointer<IProperty> metaData = h->coders[0]->getPropertyMetaData("flags");
  VS_TUT_ENSURE("a codec context should have a flags setting", metaData);

  int numConstants = metaData->getNumFlagSettings();
  VS_TUT_ENSURE("num flag settings should be greater than 0", numConstants >0);
  
  for(int i = 0; i < numConstants; i++)
  {
    RefPointer<IProperty> constMetaData = metaData->getFlagConstant(i);
    VS_TUT_ENSURE("should find const", constMetaData);
    VS_TUT_ENSURE_EQUALS("should be const", constMetaData->getType(),
        IProperty::PROPERTY_CONST);
//    VS_LOG_DEBUG("flag [%s]; constant [%s]",
//        metaData->getName(),
//        constMetaData->getName());
  }
  // and check bounds
  VS_TUT_ENSURE("should fail", !metaData->getFlagConstant(-1));
  VS_TUT_ENSURE("should fail", !metaData->getFlagConstant(numConstants));    
}

void
StreamCoderPropertyTest :: testGetPropertyAsDouble()
{
  int32_t retval = -1;
  h->setupReading(h->SAMPLE_FILE);

  VS_TUT_ENSURE("need at least one stream", h->num_streams > 0);

  int32_t numProperties = h->coders[0]->getNumProperties();
  VS_TUT_ENSURE("should get at least 100 properties", numProperties > 100);
  
  // This is actually a int type
  retval = h->coders[0]->setProperty("ab", 5.4);
  double value = h->coders[0]->getPropertyAsDouble("ab");
  VS_TUT_ENSURE_DISTANCE("unexpected", value, 5.0, 0.00001);
}

