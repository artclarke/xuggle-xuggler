/*
 * Copyright (c) 2008-2009 by Xuggle Inc. All rights reserved.
 *
 * It is REQUESTED BUT NOT REQUIRED if you use this library, that you let 
 * us know by sending e-mail to info@xuggle.com telling us briefly how you're
 * using the library and what you like or don't like about it.
 *
 * This library is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any later
 * version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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

  retval = container->writeHeader();
  VS_TUT_ENSURE("could not write header", retval >= 0);
  
  retval = container->writeTrailer();
  VS_TUT_ENSURE("could not write trailer", retval >= 0);
  
  retval = container->close();
  VS_TUT_ENSURE("could not close container", retval >= 0);
}
