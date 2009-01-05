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
package com.xuggle.xuggler;

import org.junit.*;

import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IContainerFormat;

import junit.framework.TestCase;

public class ContainerTest extends TestCase
{
  private final String sampleFile = "fixtures/testfile.flv";

  @Test
  public void testContainerOpenAndClose()
  {
    // Try getting a new Container
    IContainer container = null;
    IContainerFormat fmt = null;    
    int retval = -1;

    container = IContainer.make();
    fmt = IContainerFormat.make();
    
    // try opening a container format
    
    retval = fmt.setInputFormat("flv");
    assertTrue("could not set input format", retval >= 0);
    
    retval = fmt.setOutputFormat("flv", null, null);
    assertTrue("could not set output format", retval >= 0);
    
    // force collection of native resources.
    container.delete();
    container = null;
    
    // get a new container; which will deref the old one.
    container = IContainer.make();
    
    // now, try opening a container.
    retval = container.open("file:"+this.getClass().getName()+"_"+this.getName()+".flv",
        IContainer.Type.WRITE, fmt);
    assertTrue("could not open file for writing", retval >= 0);

    retval = container.close();
    assertTrue("could not close file", retval >= 0);
    
    retval = container.open("file:"+this.getClass().getName()+"_"+this.getName()+".flv",
        IContainer.Type.WRITE, null);
    assertTrue("could not open file for writing", retval >= 0);

    retval = container.close();
    assertTrue("could not close file", retval >= 0);
    
    retval = container.open("file:"+sampleFile,
        IContainer.Type.READ, fmt);
    assertTrue("could not open file for writing", retval >= 0);

    retval = container.close();
    assertTrue("could not close file", retval >= 0);
    
    retval = container.open("file:"+sampleFile,
        IContainer.Type.READ, null);
    assertTrue("could not open file for writing", retval >= 0);

    retval = container.close();
    assertTrue("could not close file", retval >= 0);
    
  }
  
  @Test
  public void testCorrectNumberOfStreams()
  {
    IContainer container = IContainer.make();
    int retval = -1;
    
    retval = container.open(sampleFile, IContainer.Type.READ, null);
    assertTrue("could not open file", retval >= 0);
    
    int numStreams = container.getNumStreams();
    assertTrue("incorrect number of streams: " + numStreams, numStreams == 2);
    
    retval = container.close();
    assertTrue("could not close the file", retval >= 0);
  }
  
  @Test
  public void testWriteHeaderAndTrailer()
  {
    IContainer container = IContainer.make();
    int retval = -1;
    
    retval = container.open(this.getClass().getName()+"_"+this.getName()+".flv",
        IContainer.Type.WRITE, null);
    assertTrue("could not open file", retval >= 0);
    
    retval = container.writeHeader();
    assertTrue("could not write header", retval >= 0);
    
    retval = container.writeTrailer();
    assertTrue("could not write header", retval >= 0);
    
    retval = container.close();
    assertTrue("could not close the file", retval >= 0);
    
  }

  @Test
  public void testWriteTrailerWithNoWriteHeaderDoesNotCrashJVM()
  {
    IContainer container = IContainer.make();
    int retval = -1;
    
    retval = container.open(this.getClass().getName()+"_"+this.getName()+".flv",
        IContainer.Type.WRITE, null);
    assertTrue("could not open file", retval >= 0);

    // Should give a warning and fail, but should NOT crash the JVM like it used to.
    retval = container.writeTrailer();
    assertTrue("could not write header", retval < 0);
    
    retval = container.close();
    assertTrue("could not close the file", retval >= 0);
    
  }
  
  @Test
  public void testOpenFileSetsInputContainer()
  {
    IContainer container = null;
    IContainerFormat fmt = null;    
    int retval = -1;

    container = IContainer.make();
    
    // now, try opening a container.
    retval = container.open("file:"+this.getClass().getName()+"_"+this.getName()+".flv",
        IContainer.Type.WRITE, null);
    assertTrue("could not open file for writing", retval >= 0);
    
    fmt = container.getContainerFormat();
    assertNotNull(fmt);
    assertEquals("flv", fmt.getOutputFormatShortName());

    retval = container.close();
    assertTrue("could not close file", retval >= 0);
    
    retval = container.open("file:"+sampleFile,
        IContainer.Type.READ, fmt);
    assertTrue("could not open file for writing", retval >= 0);

    fmt = container.getContainerFormat();
    assertNotNull(fmt);
    assertEquals("flv", fmt.getInputFormatShortName());


    retval = container.close();
    assertTrue("could not close file", retval >= 0);
    
  }


}
