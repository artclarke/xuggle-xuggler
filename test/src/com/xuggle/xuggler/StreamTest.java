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
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IRational;
import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IStreamCoder;

import junit.framework.TestCase;

public class StreamTest extends TestCase
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());
  private final String sampleFile = "fixtures/testfile.flv";

  private IStream mStream=null;
  private IContainer mContainer=null;

  @Before
  public void setUp()
  {
    log.debug("Executing test case: {}", this.getName());
    if (mContainer != null)
      mContainer.delete();
    mContainer = IContainer.make();
    if (mStream != null)
      mStream.delete();
    mStream = null;
  }
  
  @Test
  public void testGetStream()
  {
    helperGetStream(1);
  }
  
  @Test
  public void testStreamGetters()
  {
    helperGetStream(0);
    IRational rational = null;
    long val=0;
    
    val = mStream.getIndex();
    log.debug("Index: {}", val);
    
    val = mStream.getId();
    log.debug("ID: {}", val);
    
    rational = mStream.getFrameRate();
    log.debug("Frame Rate: {}", rational.getDouble());
    
    rational = mStream.getTimeBase();
    log.debug("Time Base: {}", rational.getDouble());
    
    val = mStream.getStartTime();
    log.debug("Start Time: {}", val);
    val = mStream.getDuration();
    log.debug("Duration: {}", val);
    val = mStream.getCurrentDts();
    log.debug("Current Dts: {}", val);
    val = mStream.getNumFrames();
    log.debug("Number of Frames: {}", val);
    val = mStream.getNumIndexEntries();
    log.debug("Number of Index Entries: {}", val);
    
  }
  
  public void testGetStreamCoder()
  {
    helperGetStream(0);
    
    IStreamCoder coder = null;
    coder = mStream.getStreamCoder();
    assertTrue(coder != null);
  }

  private void helperGetStream(int index)
  {
    int retval = -1;
    retval = mContainer.open(sampleFile, IContainer.Type.READ, null);
    assertTrue(retval >= 0);
    
    assertTrue(mContainer.getNumStreams() == 2);
    
    mStream = mContainer.getStream(index);
    assertTrue(mStream != null);
    assertTrue(mStream.getIndex() == index);
  }
}
