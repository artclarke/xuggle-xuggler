/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated.  All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any
 * later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string>
#include <com/xuggle/ferry/RefPointer.h>
#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/xuggler/IContainerFormat.h>
#include <com/xuggle/xuggler/Global.h>
#include "Helper.h"
#include "ContainerFormatTest.h"

using namespace VS_CPP_NAMESPACE;

VS_LOG_SETUP(VS_CPP_PACKAGE);

ContainerFormatTest :: ContainerFormatTest()
{
  h = 0;
}

ContainerFormatTest :: ~ContainerFormatTest()
{
  tearDown();
}

void
ContainerFormatTest :: setUp()
{
  format = 0;
  h = new Helper();
}

void
ContainerFormatTest :: tearDown()
{
  if (h)
    delete h;
  h = 0;
  format = 0;
}

void
ContainerFormatTest :: testCreationAndDestruction()
{
  format = IContainerFormat::make();
  VS_TUT_ENSURE("could not allocate container", format);
  VS_TUT_ENSURE("no one else owns this container", format->getCurrentRefCount() == 1);
}

void
ContainerFormatTest :: testSetInputFormat()
{
  int retval = -1;
  format = IContainerFormat::make();
  VS_TUT_ENSURE("could not allocate container", format);

  // Valid formats
  retval = format->setInputFormat("flv");
  VS_TUT_ENSURE("could not set format to flv", retval >= 0);
  
  // Invalid formats
  retval = format->setInputFormat("notavalidshortname");
  VS_TUT_ENSURE("could set to an invalid format", retval < 0);
  retval = format->setInputFormat("");
  VS_TUT_ENSURE("could set to an invalid format", retval < 0);
  retval = format->setInputFormat(0);
  VS_TUT_ENSURE("could set to an invalid format", retval < 0);
}

void
ContainerFormatTest :: testSetOutputFormat()
{
  int retval = -1;
  format = IContainerFormat::make();
  VS_TUT_ENSURE("could not allocate container", format);
  
  // Valid formats
  retval = format->setOutputFormat("flv", 0, 0);
  VS_TUT_ENSURE("could not set format to flv", retval >= 0);
  retval = format->setOutputFormat("flv","", "");
  VS_TUT_ENSURE("could not set format to flv", retval >= 0);    
  retval = format->setOutputFormat("flv",h->SAMPLE_FILE, "");
  VS_TUT_ENSURE("could not set format to flv", retval >= 0);
  retval = format->setOutputFormat(0,h->SAMPLE_FILE, "");
  VS_TUT_ENSURE("could not set format to flv", retval >= 0);
  retval = format->setOutputFormat("",h->SAMPLE_FILE, "");
  VS_TUT_ENSURE("could not set format to flv", retval >= 0);

  // invalid formats
  retval = format->setOutputFormat(0, 0, 0);
  VS_TUT_ENSURE("could set to an invalid format", retval < 0);
  retval = format->setOutputFormat("", "", "");
  VS_TUT_ENSURE("could set to an invalid format", retval < 0);

  // FFMPEG returns an internal format if it doesn't know
  // the short name.
  retval = format->setOutputFormat("notavalidshortname", "", "");
  VS_TUT_ENSURE("could set to an invalid format", retval == 0);
  VS_TUT_ENSURE_EQUALS("unexpected format short name",
      std::string(format->getOutputFormatShortName()),
      std::string("ffm"));
  VS_TUT_ENSURE_EQUALS("unexpected format long name",
      std::string(format->getOutputFormatLongName()),
      std::string("FFM (FFserver live feed) format"));
  VS_TUT_ENSURE("no mime type expected", format->getOutputFormatMimeType());
  
  // FFMPEG retuns an internal format if it doesn't know the
  // long name.
  retval = format->setOutputFormat("", "notavalidurl&kdjs2kd:dkjs", "");
  VS_TUT_ENSURE("could set to an invalid format", retval == 0);
  VS_TUT_ENSURE_EQUALS("unexpected format short name",
      std::string(format->getOutputFormatShortName()),
      std::string("ffm"));
  VS_TUT_ENSURE_EQUALS("unexpected format long name",
      std::string(format->getOutputFormatLongName()),
      std::string("FFM (FFserver live feed) format"));
  VS_TUT_ENSURE("no mime type expected", format->getOutputFormatMimeType());

  // But it doesn't return anything if it doesn't know the
  // mime type.
  retval = format->setOutputFormat("", "", "notavalidmeimetype/foo");
  VS_TUT_ENSURE("could set to an invalid format", retval < 0 );
}
