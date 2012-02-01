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
package com.xuggle.xuggler;

import static org.junit.Assert.*;

import java.io.DataInputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.RandomAccessFile;
import java.util.Collection;
import java.util.LinkedList;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

import com.xuggle.xuggler.io.FileProtocolHandler;

@RunWith(Parameterized.class)
public class ContainerExhaustiveTest
{

  private static final String TEST_FILE="fixtures/testfile.flv";
  private static final int TEST_PACKETS=7950;
  private static IContainer mContainer;

  private interface ITestSetup {
    public IContainer setUp() throws FileNotFoundException;
  }
  @Parameters
  public static Collection<Object[]> getHandlersToTest()
  throws FileNotFoundException
  {
    Collection<Object[]> retval = new LinkedList<Object[]>();
    retval.add(new Object[]{
        new ITestSetup() {
          public IContainer setUp() throws FileNotFoundException
          {
            IContainer retval = IContainer.make();
            assertTrue(
                "Xuggler Custom IO failed",
                retval.open(new FileInputStream(TEST_FILE), null) >=0
            );
            return retval;
          }},
    });

    retval.add(new Object[]{
        new ITestSetup() {
          public IContainer setUp() throws FileNotFoundException
          {
            IContainer retval = IContainer.make();
            assertTrue(
                retval.open(
                    new FileInputStream(TEST_FILE).getChannel(),
                    IContainer.Type.READ,
                    null) >=0
            );
            return retval;
          }},
    });

    retval.add(new Object[]{
        new ITestSetup() {
          public IContainer setUp() throws FileNotFoundException
          {
            IContainer retval = IContainer.make();
            assertTrue(
                retval.open(
                    new RandomAccessFile(TEST_FILE,"r"),
                    IContainer.Type.READ,
                    null) >=0
            );
            return retval;
          }},
    });

    retval.add(new Object[]{
        new ITestSetup() {
          public IContainer setUp() throws FileNotFoundException
          {
            IContainer retval = IContainer.make();
            assertTrue(
                "Xuggler Custom IO failed",
                retval.open(
                    new DataInputStream(new FileInputStream(TEST_FILE)),
                    null) >=0
            );
            return retval;
          }},
    });

    retval.add(new Object[]{
        new ITestSetup() {
          public IContainer setUp() throws FileNotFoundException
          {
            IContainer retval = IContainer.make();
            assertTrue("could not open container",
                retval.open(
                    new FileProtocolHandler(TEST_FILE),
                    IContainer.Type.READ,
                    null) >=0
            );
            return retval;
          }},
    });



    return retval;
  }

  public ContainerExhaustiveTest(ITestSetup setup) throws FileNotFoundException
  {
    mContainer = setup.setUp();
  }
  
  @Test
  public void testReadPackets()
  {
    long packetsRead = 0;
    IPacket packet = IPacket.make();
    while(mContainer.readNextPacket(packet) >= 0)
    {
      if (packet.isComplete())
        ++packetsRead;
    }
    assertEquals("got unexpected number of packets in file",
        TEST_PACKETS, packetsRead);
    mContainer.close();
    
  }

}
