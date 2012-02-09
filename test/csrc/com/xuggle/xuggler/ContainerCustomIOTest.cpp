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

#include <string>
#include <cstring>
#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/xuggler/IContainer.h>
#include <com/xuggle/xuggler/io/StdioURLProtocolManager.h>
#include <com/xuggle/xuggler/Global.h>
#include "Helper.h"
#include "ContainerCustomIOTest.h"

using namespace VS_CPP_NAMESPACE;

VS_LOG_SETUP(VS_CPP_PACKAGE);

ContainerCustomIOTest :: ContainerCustomIOTest()
{
  h = 0;
}

ContainerCustomIOTest :: ~ContainerCustomIOTest()
{
  tearDown();
}

void
ContainerCustomIOTest :: setUp()
{
  h = new Helper();
  container = 0;
}

void
ContainerCustomIOTest :: tearDown()
{
  if (h)
    delete h;
  h = 0;
  container = 0;
  com::xuggle::xuggler::io::URLProtocolManager::unregisterAllProtocols();
}

void
ContainerCustomIOTest :: testWriteToFileFromFile()
{
  int numPackets = 0;
  int retval = -1;
  const char* OUTPUT_FILE="test:ContainerCustomIOTest_testWriteToFileFromFile.flv";

  // let's register a custom protocol
  com::xuggle::xuggler::io::StdioURLProtocolManager::registerProtocol("test");

  h->setupReading("test", h->SAMPLE_FILE);

  // get an output format
  RefPointer<IContainerFormat> outputFormat =
      IContainerFormat::make();
  outputFormat->setOutputFormat("flv", OUTPUT_FILE, 0);
  VS_TUT_ENSURE("", outputFormat);

  RefPointer<IContainer> outContainer = IContainer::make(outputFormat.value());
  VS_TUT_ENSURE("no container", outContainer);
  
  // let's set some options; this helps us test for memory leaks
  RefPointer<IMetaData> options = IMetaData::make();
  options->setValue("packetsize", "0");
  options->setValue("xuggle-not-an-option", "55");

  RefPointer<IMetaData> rejectedOptions = IMetaData::make();

  retval = outContainer->open(OUTPUT_FILE,
      IContainer::WRITE, 0, false, true, options.value(), rejectedOptions.value());
  VS_TUT_ENSURE("couldn't write", retval >=0);

  for(int32_t i = rejectedOptions->getNumKeys()-1; i >= 0; i--)
  {
    const char* key=rejectedOptions->getKey(i);
    const char* value=rejectedOptions->getValue(key, IMetaData::METADATA_NONE);
    (void) key;
    (void) value;
    VS_LOG_TRACE("key: %s; value: %s;", key, value);
  }
  VS_TUT_ENSURE_EQUALS("", 1, rejectedOptions->getNumKeys());
  VS_TUT_ENSURE_EQUALS("", 0, strcmp("55", rejectedOptions->getValue("xuggle-not-an-option",
      IMetaData::METADATA_NONE)));

  int32_t numInStreams = h->container->getNumStreams();
  for(int i = 0; i < numInStreams; i++)
  {
    RefPointer<IStream> inStream = h->container->getStream(i);
    RefPointer<IStreamCoder> inCoder = inStream->getStreamCoder();
    
    VS_TUT_ENSURE("input coder not open", inCoder->open() >= 0);

    RefPointer<IStream> outStream = outContainer->addNewStream(inStream->getId());
    VS_TUT_ENSURE("couldn't add stream", outStream);

    RefPointer<IStreamCoder> outCoder = IStreamCoder::make(IStreamCoder::ENCODING,
        inCoder.value());
    VS_TUT_ENSURE("couldn't copy in coder", outStream->setStreamCoder(outCoder.value()) >= 0);
    VS_TUT_ENSURE("couldn't open out coder", outCoder->open() >= 0);
  }
  VS_TUT_ENSURE("couldn't write header",
      outContainer->writeHeader() >= 0);
  
  int32_t numPacketsWritten = 0;
  do {
    retval = h->container->readNextPacket(h->packet.value());
    if (retval == 0)
    {
      VS_TUT_ENSURE("packet from unknown stream", 
          h->packet->getStreamIndex() < h->container->getNumStreams());
      numPackets++;
      VS_TUT_ENSURE("couldn't write packet",
          outContainer->writePacket(h->packet.value(), false) >= 0);
      numPacketsWritten++;
    }
  } while (retval >= 0);
  
  VS_TUT_ENSURE("no packets in file", numPackets > 0);
  if (h->expected_packets > 0)
  {
    VS_TUT_ENSURE_EQUALS("unexpected number of packets",
        h->expected_packets,
        numPackets);
  }
  VS_TUT_ENSURE_EQUALS("same input as output",
      numPackets,
      numPacketsWritten);
  
  VS_TUT_ENSURE("couldn't write trailer",
      outContainer->writeTrailer() >= 0);
  
  int32_t outStreams = (int32_t) outContainer->getNumStreams();
  for(int i = 0; i < outStreams; i++)
  {
    RefPointer<IStream> stream = outContainer->getStream(i);
    RefPointer<IStreamCoder> coder = stream->getStreamCoder();
    VS_TUT_ENSURE("no close stream", coder->close() >= 0);
    
    stream = h->container->getStream(i);
    coder = stream->getStreamCoder();
    VS_TUT_ENSURE("no close stream", coder->close() >= 0);
  }
  VS_TUT_ENSURE("couldn't close container", outContainer->close() >= 0);
}
