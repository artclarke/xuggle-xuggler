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

import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

import org.junit.Before;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.ferry.JNIThreadProxy;
import com.xuggle.ferry.JNIThreadProxy.Interruptable;
import com.xuggle.xuggler.io.IURLProtocolHandler;

/**
 * Tests to make sure we can interrupt long-running Xuggler calls.
 * 
 * @author aclarke
 *
 */
public class InterruptingExhaustiveTest
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());
  
  @Before
  public void setUp()
  {
    // always clear the global interrupt handler so we don't interfere across
    // tests.
    JNIThreadProxy.setGlobalInterruptable(null);
  }
 
  // needs a long time out because if running in an exhaustive
  // test on a slow machine (oh like our build server) it can
  // sometime fail.
  @Test(timeout=30*1000)
  public void testXugglerSupportInterruptions() throws InterruptedException
  {
    final AtomicBoolean testStarted = new AtomicBoolean(false);
    final AtomicBoolean testPassed = new AtomicBoolean(false);
    final AtomicReference<Thread> threadInterrupted = new AtomicReference<Thread>();
    final Object lock = new Object();
    final IURLProtocolHandler handler = new IURLProtocolHandler(){
      public int close()
      {
        return 0;
      }

      public boolean isStreamed(String url, int flags)
      {
        return true;
      }

      public int open(String url, int flags)
      {
        return 0;
      }

      public int read(byte[] buf, int size)
      {
        if (Thread.interrupted()) {
          log.debug("interrupted!");
          return -1;
        } else {
          log.debug("not interrupted: {}", size);
          try
          {
            Thread.sleep(500);
          }
          catch (InterruptedException e)
          {
            log.debug("doh");
          }
          // keep returning one byte of data
          return 1;
        }
      }

      public long seek(long offset, int whence)
      {
        return -1;
      }

      public int write(byte[] buf, int size)
      {
        return size;
      }
    };
    
    final Thread thread = new Thread(
        new Runnable(){
          public void run()
          {
            final IContainer container = IContainer.make();

            synchronized(lock) {
              testStarted.set(true);
              lock.notifyAll();
            }
            log.info("About to call blocking ffmpeg method: {}", Thread.currentThread());

            container.open(
//                "fixtures/testfile.flv",
//               "udp://127.0.0.1:12345/notvalid.flv?localport=28302",
                handler,
                IContainer.Type.READ,
                null, true, false);
            log.info("ffmpeg method returned: {}", Thread.currentThread());
            // we should return from this method in an
            // interrupted state.
            final Thread thread = threadInterrupted.get();
            if (thread != null) {
              log.info("thread was interrupted: {}", thread);
              testPassed.set(true);
            } else {
              log.info("but test did not pass");
            }
          }},
        "xuggler thread");
    
    // First, let's set up a global interrupt handler because UDP handling
    // in FFmpeg starts its own threads
    final Interruptable interruptHandler = new Interruptable()
    {
      public boolean preInterruptCheck()
      {
        log.debug("forcing an interrupt");
        // we're going to interrupt the checking thread
        Thread.currentThread().interrupt();
        // and tell the global handler to keep going
        return true;
      }
      public boolean postInterruptCheck(boolean interruptStatus)
      {
        log.debug("checking that thread is interrupted");
        // because we interrupted the thread, this should be true
        assertTrue(Thread.interrupted());
        assertTrue(interruptStatus);
        threadInterrupted.set(Thread.currentThread());
        return interruptStatus;
      }
    };
    try {
      JNIThreadProxy.setGlobalInterruptable(interruptHandler);

      synchronized(lock) {
        thread.start();
        while(!testStarted.get()) {
          log.info("waiting for start");
          lock.wait();
        }
      }
      // give the thread about a half a second to get into ffmpeg land
      log.info("joining interrupted ffmpeg-calling thread: {}", thread);
      thread.join();
      assertTrue("test did not pass", testPassed.get());
    } finally {
      // and remove myself
      Interruptable oldHandler = JNIThreadProxy.setGlobalInterruptable(null);
      assertEquals(interruptHandler, oldHandler);
    }
  }

  @Test
  public void testInteruptStatusPreservedAcrossJNICalls()
  {
    final IContainer container = IContainer.make();
    
    final IURLProtocolHandler handler = new IURLProtocolHandler(){
      public int close()
      {
        return 0;
      }

      public boolean isStreamed(String url, int flags)
      {
        return true;
      }

      public int open(String url, int flags)
      {
        return 0;
      }

      public int read(byte[] buf, int size)
      {
        // interrupt the thread
        Thread.currentThread().interrupt();
        throw new RuntimeException("doh");
      }

      public long seek(long offset, int whence)
      {
        return -1;
      }

      public int write(byte[] buf, int size)
      {
        return size;
      }
    };
    
    assertTrue(!Thread.currentThread().isInterrupted());
    assertTrue(container.open(handler,
        IContainer.Type.READ, null, true, false) < 0);
    assertTrue("thread interrupt not preserved from open",
        Thread.currentThread().isInterrupted());
  }
}
