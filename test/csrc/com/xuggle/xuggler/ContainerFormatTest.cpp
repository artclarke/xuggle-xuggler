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

void 
ContainerFormatTest :: testGetOutputNumCodecsSupported()
{
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_INFO, false);

  int retval = -1;
  format = IContainerFormat::make();
  VS_TUT_ENSURE("could not allocate container", format);
  
  // Valid formats
  retval = format->setOutputFormat("mov", 0, 0);
  VS_TUT_ENSURE("could not set format to mov", retval >= 0);
  
  VS_TUT_ENSURE("no default audio codec",
      format->getOutputDefaultAudioCodec() != ICodec::CODEC_ID_NONE);
  VS_TUT_ENSURE("no default audio codec",
      format->getOutputDefaultVideoCodec() != ICodec::CODEC_ID_NONE);
  VS_TUT_ENSURE("should have non null extensions",
      format->getOutputExtensions());
  int numCodecsSupported = format->getOutputNumCodecsSupported();
  VS_LOG_DEBUG("Num codecs supported: %ld", numCodecsSupported);
  VS_TUT_ENSURE("should have more than 1 codec supported",
      numCodecsSupported > 1);
  for(int i = 0; i < numCodecsSupported; i++)
  {
    ICodec::ID id = format->getOutputCodecID(i);
    VS_TUT_ENSURE("should not be none", id != ICodec::CODEC_ID_NONE);
    RefPointer<ICodec> codec = ICodec::findEncodingCodec(id);
    if (!codec) {
      VS_LOG_DEBUG("Codec not supported: %ld", id);
    } else {
      int tag = format->getOutputCodecTag(i);
      VS_LOG_DEBUG("Codec: %s; Tag: %ld; ID: %ld",
          codec->getName(),
          tag,
          id);
      VS_TUT_ENSURE("tag should be non zero", tag != 0);
      VS_TUT_ENSURE("should say we support this",
          format->isCodecSupportedForOutput(id));
      VS_TUT_ENSURE_EQUALS("tags should be the same",
          tag,
          format->getOutputCodecTag(i));
    }
  }
}

void
ContainerFormatTest :: testGetInstalledInputFormats()
{
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_INFO, false);
  int32_t count = IContainerFormat::getNumInstalledInputFormats();
  VS_TUT_ENSURE("should be more than one", count > 1);
  
  for(int i=0; i < count; i++)
  {
    RefPointer<IContainerFormat> fmt =
      IContainerFormat::getInstalledInputFormat(i);
    VS_TUT_ENSURE("should be valid", fmt);
    if (fmt) {
      VS_LOG_DEBUG("%s; %s",
          fmt->getInputFormatShortName(),
          fmt->getInputFormatLongName());
      VS_TUT_ENSURE("is input", fmt->isInput());
    }
  }
}

void
ContainerFormatTest :: testGetInstalledOutputFormats()
{
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_INFO, false);
  int32_t count = IContainerFormat::getNumInstalledOutputFormats();
  VS_TUT_ENSURE("should be more than one", count > 1);
  
  for(int i=0; i < count; i++)
  {
    RefPointer<IContainerFormat> fmt =
      IContainerFormat::getInstalledOutputFormat(i);
    VS_TUT_ENSURE("should be valid", fmt);
    if (fmt) {
      VS_LOG_DEBUG("%s; %s",
          fmt->getOutputFormatShortName(),
          fmt->getOutputFormatLongName());
      VS_TUT_ENSURE("is output", fmt->isOutput());
    }
  }
}

