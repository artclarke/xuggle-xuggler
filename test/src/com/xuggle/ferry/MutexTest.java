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

import org.junit.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.ferry.Mutex;

import junit.framework.TestCase;

public class MutexTest extends TestCase
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());
  private Mutex mMutex=null;
  private boolean mTestFail = false;
  @Before
  public void setUp()
  {
    log.debug("Executing test case: {}", this.getName());
    if (mMutex != null)
      mMutex.delete();
    mMutex = Mutex.make();
    assertTrue("didn't get a mutex", mMutex != null);
    mTestFail = false;
  }

  @Test
  public void testUncompetitiveLockAndUnlock()
  {
    long before = -1;
    long after = -1;
    // this should be VERY quick...
    before = System.currentTimeMillis();
    for (int i = 0; i<10;i++)
    {
      mMutex.lock();
      mMutex.unlock();
    }
    after = System.currentTimeMillis();
    assertTrue("not really fast", after-before < 100);    
  }
  
  @Test
  public void testCompetitiveLockAndUnlock()
  {
    long before = -1;
    long after = -1;
    Thread competitor = new Thread(new Runnable() {
      public void run()
      {
        log.debug("Competitor is running");
        mMutex.lock();
        log.debug("Got the lock");

        synchronized(mMutex) {
          // let the other thread know we have the lock
          mMutex.notifyAll();
          log.debug("Notified other thread");
        }

        try {
          log.debug("Going to sleep");

          Thread.sleep(1000);
        }
        catch (InterruptedException ex)
        {
          log.error("interrupted while sleeping");
          mTestFail = true;
        }
        mMutex.unlock();
        log.debug("And exiting the thread");

      }
    }, "Competitor");
    // this should be VERY slow...
    before = System.currentTimeMillis();
    synchronized(mMutex) {
      try {
        log.debug("Starting the competitor");
        competitor.start();
        log.debug("Waiting for the competitor to get the lock");
        mMutex.wait();
      }
      catch (InterruptedException ex)
      {
        // ignore
      }
    }
    // now we've been signaled that the other thread
    // has the lock. try to take it (should block).
    log.debug("Trying the lock... should wait...");
    mMutex.lock();
    log.debug("Got the lock.");
    after = System.currentTimeMillis();
    assertTrue("too fast", after-before > 500);
    try {
      competitor.join();
    }
    catch (InterruptedException ex)
    {
      // ignore
    }
    log.debug("releasing the lock");
    mMutex.unlock();
    assertTrue("we appear to have failed in the other thread", !mTestFail);
  }
}
