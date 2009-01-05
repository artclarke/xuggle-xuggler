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
package com.xuggle.ferry;

import org.junit.Before;
import org.junit.Test;

import com.xuggle.ferry.Logger;

import junit.framework.TestCase;

public class LoggerTest extends TestCase
{
  private Logger mLog=null;

  @Before
  public void setUp()
  {
    mLog = null;
  }

  @Test
  public void testLogger()
  {
    boolean retval = false;
    mLog = Logger.getLogger(this.getClass().getName() + ".native");
    retval = mLog.log("Java", 0, Logger.Level.LEVEL_ERROR, "This is an error msg");
    assertTrue(retval);
    retval = mLog.log("Java", 0, Logger.Level.LEVEL_WARN, "This is a warn msg");
    assertTrue(retval);
    retval = mLog.log("Java", 0, Logger.Level.LEVEL_INFO, "This is an info msg");
    assertTrue(retval);    
    retval = mLog.log("Java", 0, Logger.Level.LEVEL_DEBUG, "This is a debug msg");
    assertTrue(retval);
    retval = mLog.log("Java", 0, Logger.Level.LEVEL_TRACE, "This is a trace msg");
    assertTrue("shouldn't log trace messages by default", !retval);
    assertTrue("all tests seemed to pass without crashes", true);
  }

  @Test
  public void testManyThreadsLogging()
  {
    final int numThreads=100;
    final int numMsgs=100;
    Thread threads[]=null;
    int i=0;

    mLog = Logger.getLogger(this.getClass().getName() + ".native");

    threads = new Thread[numThreads];
    assertTrue("couldn't allocate the threads", threads != null);
    
    // Allocate the threads and start 'em
    for(i = 0; i < numThreads; i++)
    {
      threads[i] = new Thread( new Runnable() {
        public void run() {
          int j = 0;
          for (j = 0; j < numMsgs; j++)
          {
            mLog.error("Competitor", 0, "Look at me; I'm a logging message: " + j);
          }
        }
      }, "logging thread " + i);
      threads[i].start();
    }

    for (i = 0; i< numThreads; i++)
    {
      // and wait for them to finish
      try
      {
        threads[i].join();
      } catch (InterruptedException ex)
      {
        fail("got interrupted");
      }
    }
    assertTrue("well, didn't crash which is what we're hoping to avoid", true);
  }

}
