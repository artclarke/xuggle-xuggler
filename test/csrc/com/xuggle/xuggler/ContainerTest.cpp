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
#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/xuggler/IContainer.h>
#include <com/xuggle/xuggler/Global.h>
#include "Helper.h"
#include "ContainerTest.h"

using namespace VS_CPP_NAMESPACE;

VS_LOG_SETUP(VS_CPP_PACKAGE);

ContainerTest :: ContainerTest()
{
  h = 0;
}

ContainerTest :: ~ContainerTest()
{
  tearDown();
}

void
ContainerTest :: setUp()
{
  h = new Helper();
  container = 0;
}

void
ContainerTest :: tearDown()
{
  if (h)
    delete h;
  h = 0;
  container = 0;
}

void
ContainerTest :: testCreationAndDescruction()
{
  container = IContainer::make();
  VS_TUT_ENSURE("could not allocate container", container);
  VS_TUT_ENSURE("no one else owns this container", container->getCurrentRefCount() == 1);
}

void
ContainerTest :: testOpenForWriting()
{
  container = IContainer::make();
  VS_TUT_ENSURE("could not allocate container", container);
  
  int retval = -1;
  std::string filename;
  filename.append("ContainerTest2");
  filename.append("_output.flv");
  
  retval = container->open(filename.c_str(),
      IContainer::WRITE,
      0);
  VS_TUT_ENSURE("could not open file for write", retval >= 0);
  int numStreams = container->getNumStreams();
  VS_TUT_ENSURE("unexpected number of streams", numStreams == 0);
  retval = container->close();
  VS_TUT_ENSURE("could not close file for write", retval >= 0);
}

void
ContainerTest :: testOpenForReading()
{
  container = IContainer::make();
  VS_TUT_ENSURE("could not allocate container", container);
  
  int retval = -1;
  {
    LoggerStack stack;
    stack.setGlobalLevel(Logger::LEVEL_DEBUG, false);

    retval = container->open(h->getSamplePath(),
        IContainer::READ,
        0);
  }
  VS_TUT_ENSURE("could not open file for read", retval >= 0);
  int numStreams = container->getNumStreams();
  VS_TUT_ENSURE("unexpected number of streams", numStreams == 2);
  retval = container->close();
  VS_TUT_ENSURE("could not close file for read", retval >= 0);
}

void
ContainerTest :: testInvalidOpenForWriting()
{
  // Temporarily turn down logging
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_ERROR, false);

  container = IContainer::make();
  VS_TUT_ENSURE("could not allocate container", container);
  
  int retval = -1;
  std::string filename;
  filename.append("thisisnot//afile");
  retval = container->open(filename.c_str(),
      IContainer::WRITE,
      0);
  VS_TUT_ENSURE("could open file for write", retval < 0);
  VS_LOG_TRACE("got here");
  // This should also fail, but without printing a second
  // log warning about not finding an output format.
  filename ="";
  filename.append("thisisnot//afile.flv");
  retval = container->open(filename.c_str(),
      IContainer::WRITE,
      0);
  VS_LOG_TRACE("got here");

  VS_TUT_ENSURE("could open file for write", retval < 0);

}

void
ContainerTest :: testInvalidOpenForReading()
{
  // Temporarily turn down logging
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_ERROR, false);

  container = IContainer::make();
  VS_TUT_ENSURE("could not allocate container", container);
  
  int retval = -1;
  std::string filename;
  filename.append("/notbloodylike;badfilewith&illegalchars");
  retval = container->open(filename.c_str(),
      IContainer::READ,
      0);
  VS_TUT_ENSURE("could open file for read", retval < 0);
}

void
ContainerTest :: testNullArguments()
{
  container = IContainer::make();
  VS_TUT_ENSURE("could not allocate container", container);
  
  int retval = -1;
  retval = container->open(0,
      IContainer::READ,
      0);
  VS_TUT_ENSURE("could open file for read", retval < 0);
  retval = container->open("",
      IContainer::READ,
      0);
  VS_TUT_ENSURE("could open file for read", retval < 0);
}

void
ContainerTest :: testReadFromFileReusingPackets()
{
  int numPackets = 0;
  // change this value to 0 if you don't want the check
  // or to the # of packets expected for SAMPLE_FILE
  int retval = -1;
  h->setupReading(h->SAMPLE_FILE);
  
  do {
    retval = h->container->readNextPacket(h->packet.value());
    if (retval == 0)
    {
      VS_TUT_ENSURE("packet from unknown stream", 
          h->packet->getStreamIndex() < h->container->getNumStreams());
      numPackets++;
    }
  } while (retval >= 0);
  VS_TUT_ENSURE("no packets in file", numPackets > 0);
  if (h->expected_packets > 0)
    VS_TUT_ENSURE_EQUALS("unexpected number of packets",
        h->expected_packets,
        numPackets);
}

void
ContainerTest :: testReadFromFileWithNewPackets()
{
  int numPackets = 0;
  // change this value to 0 if you don't want the check
  // or to the # of packets expected for SAMPLE_FILE
  int retval = -1;
  h->setupReading(h->SAMPLE_FILE);
  
  do {
    RefPointer<IPacket> packet;
    packet = IPacket::make();
    VS_TUT_ENSURE("couldn't allocate a packet", packet);
    retval = h->container->readNextPacket(packet.value());
    if (retval == 0)
    {
      VS_TUT_ENSURE("packet from unknown stream", 
          packet->getStreamIndex() < h->container->getNumStreams());
      numPackets++;
    }
  } while (retval >= 0);

  VS_TUT_ENSURE("no packets in file", numPackets > 0);
  if (h->expected_packets > 0)
    VS_TUT_ENSURE_EQUALS("unexpected number of packets",
        h->expected_packets,
        numPackets);
}

void
ContainerTest :: testOpenContainerWithoutClosing()
{
  // Temporarily turn down logging
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_WARN, false);
  {
    RefPointer<IContainer> cont;
    cont = IContainer::make();
    VS_TUT_ENSURE("could not allocate container", cont);

    int retval = -1;
    retval = cont->open(h->getSamplePath(),
        IContainer::READ,
        0);
    VS_TUT_ENSURE("could not open file for read", retval >= 0);
    int numStreams = cont->getNumStreams();
    VS_TUT_ENSURE("unexpected number of streams", numStreams == 2);

    // Now don't close, and make sure the destructoor
    // closes; this will only show up when memory checking.
  }
}

void
ContainerTest :: testCloseContainerWithoutOpening()
{
  container = IContainer::make();
  VS_TUT_ENSURE("could not allocate container", container);
  
  int retval = -1;
  retval = container->close();
  VS_TUT_ENSURE("could not close file for read", retval < 0);
}

void
ContainerTest :: testAddStreams()
{
  RefPointer<IStream> stream;
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_DEBUG, false);

  h->setupWriting("ContainerTest_11_output.flv", 0, 0, "flv");
  for(int i = 0; i < 2; i++)
  {
    stream = h->container->addNewStream(i);
    VS_TUT_ENSURE("could not add stream", stream);
  }
}

void
ContainerTest :: testReadASmallNumberOfSamplesFromFile()
{
  int numPackets = 0;
  int numKeyPackets = 0;
  // change this value to 0 if you don't want the check
  // or to the # of packets expected for SAMPLE_FILE
  int retval = -1;
  h->setupReading(h->SAMPLE_FILE);
  int maxPacketsToRead = 100;
  
  do {
    retval = h->container->readNextPacket(h->packet.value());
    if (retval == 0)
    {
      VS_TUT_ENSURE("packet from unknown stream", 
          h->packet->getStreamIndex() < h->container->getNumStreams());
      numPackets++;
      if (h->packet->isKeyPacket())
        numKeyPackets ++;
    }
  } while (retval >= 0 && numPackets < maxPacketsToRead);
  
  VS_TUT_ENSURE_EQUALS("no packets in file", numPackets, maxPacketsToRead);
  VS_TUT_ENSURE("should get at least one key packet",  
    numKeyPackets > 0);
  VS_TUT_ENSURE("should be some non key packets",  
    numKeyPackets < numPackets);
    
}

void
ContainerTest :: testWriteHeader()
{
  int retval = -1;
  
  container = IContainer::make();
  VS_TUT_ENSURE("couldn't get container", container);
  
  retval = container->open("ContainerTest_13_output.flv", IContainer::WRITE, 0);
  VS_TUT_ENSURE("could not open container", retval >= 0);

  // turn off warning about unopened codecs
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_WARN, false);

  RefPointer<IStream> stream = container->addNewStream(0);
  VS_TUT_ENSURE("got stream", stream);
  
  RefPointer<IStreamCoder> coder = stream->getStreamCoder();
  VS_TUT_ENSURE("got coder", coder);
  
  coder->setCodec(ICodec::CODEC_ID_MP3);
  coder->setSampleRate(22050);
  coder->setChannels(1);
  VS_TUT_ENSURE("couldn't open", coder->open() >= 0);
  
  retval = container->writeHeader();
  VS_TUT_ENSURE("could not write header", retval >= 0);
  
  retval = container->writeTrailer();
  VS_TUT_ENSURE("could not write trailer", retval >= 0);
  
  VS_TUT_ENSURE("could not close coder", coder->close() >= 0);
  
  retval = container->close();
  VS_TUT_ENSURE("could not close container", retval >= 0);
}

void
ContainerTest :: testWriteToFileFromFile()
{
  int numPackets = 0;
  int retval = -1;
  h->setupReading(h->SAMPLE_FILE);

  RefPointer<IContainer> outContainer = IContainer::make();
  VS_TUT_ENSURE("no container", outContainer);
  
  retval = outContainer->open("ContainerTest_testWriteToFileFromFile.flv",
      IContainer::WRITE, 0);
  VS_TUT_ENSURE("couldn't write", retval >=0);

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

void
ContainerTest :: testIssue97Regression()
{
  int retval = -1;
  
  container = IContainer::make();
  VS_TUT_ENSURE("couldn't get container", container);
  
  retval = container->open("ContainerTest_testIssue97Regression.mp3",
      IContainer::WRITE,
      0);
  VS_TUT_ENSURE("could not open container", retval >= 0);

  // turn off warning about unopened codecs
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_ERROR, false);

  retval = container->writeHeader();
  VS_TUT_ENSURE("header write should fail, not crash", retval < 0);
  
  retval = container->close();
  VS_TUT_ENSURE("could not close container", retval >= 0);
}
