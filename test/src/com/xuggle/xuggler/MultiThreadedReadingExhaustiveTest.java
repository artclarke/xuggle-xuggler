/*
 * This file is part of Xuggler.
 * 
 * Xuggler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public
 * License along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.xuggle.xuggler;

import static org.junit.Assert.assertEquals;

import java.util.concurrent.BrokenBarrierException;
import java.util.concurrent.CyclicBarrier;
import java.util.concurrent.atomic.AtomicInteger;

import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.xuggler.mediatool.MediaReader;
import com.xuggle.xuggler.mediatool.MediaViewer;
import com.xuggle.xuggler.video.ConverterFactory;

/**
 * This test opens up a lot of threads and attempts to read from them.
 * If we run out of memory (which will happen on small, or client, 
 * JVMs), we just catch the error but continue.
 * 
 * Success is when (a) threads that have no memory errors all decode
 * the expected number of items and (b) there are no other unexplained
 * crashes.
 * 
 * @author aclarke
 *
 */
public class MultiThreadedReadingExhaustiveTest
{
  final private Logger log = LoggerFactory.getLogger(this.getClass());
  { log.trace("make warning go away"); }

  static final int NUM_THREADS=100;
  
  @Test(timeout = 30 * 60 * 1000)
  public void testMultiThreadedTest() throws InterruptedException
  {
    if (System.getProperty("os.name", "Linux").startsWith("Windows"))
      // doesn't run on Windows
      return;
    
    log.debug("------ START -----: {}", this.getClass().getName());
    final boolean ADD_VIEWER = System.getProperty(
        MultiThreadedReadingExhaustiveTest.class.getName()+".ShowVideo") != null;
    final CyclicBarrier barrier = new CyclicBarrier(NUM_THREADS);
    final Thread threads[] = new Thread[NUM_THREADS];
    final int numPackets[] = new int[NUM_THREADS];
    final AtomicInteger uncaughtExceptions = new AtomicInteger();
    
    // create all the threads
    for(int i = 0; i < threads.length; i++)
    {
      final int index = i;
      threads[index] = new Thread(new Runnable(){
        public void run()
        {
          // first let's make sure everyone has started
          log.debug("Thread {} ready to go", index);
          try
          {
            barrier.await();
          }
          catch (InterruptedException e)
          {
            return;
          }
          catch (BrokenBarrierException e)
          {
            e.printStackTrace();
            return;
          }
          final MediaReader reader = 
            new MediaReader("fixtures/testfile_videoonly_20sec.flv",
                true,
                ConverterFactory.XUGGLER_BGR_24);
          try {
            log.debug("Created reader: {}", reader);
            if (ADD_VIEWER)
            {
              final MediaViewer viewer = new MediaViewer();
              reader.addListener(viewer);
            }
            while(reader.readPacket() == null)
            {
              log.trace("read packet: {}", numPackets[index]);
              ++numPackets[index];
            }
          } catch (OutOfMemoryError e)
          {
            // This test will cause this error on small JVMs, and that's OK
            // we'll just let this thread abort and keep going.  There are other
            // tests in this suite that look at memory allocation errors.
            numPackets[index]=-1;
          } finally {
            reader.close();
            log.debug("Thread exited; memory exception: {};",
                numPackets[index]==-1 ? "yes" : "no");
          }
          
        }}, "TestThread_"+index);
      threads[i].setUncaughtExceptionHandler(
          new Thread.UncaughtExceptionHandler(){
            public void uncaughtException(Thread t, Throwable e)
            {
              if (!(e instanceof OutOfMemoryError))
              {
                log.debug("Uncaught exception leaked out of thread: {}; {}",
                    e, t);
                e.printStackTrace();
                uncaughtExceptions.incrementAndGet();
              }
            }});

    }
    // start all the threads
    for(int i = 0; i < threads.length; i++)
    {
      threads[i].start();
    }
    // join all the threads
    int numSuccess = 0;
    for(int i = 0; i < threads.length; i++)
    {
      threads[i].join();
      if (numPackets[i] != -1) {
        assertEquals(1062, numPackets[i]);
        ++numSuccess;
      }
    }
    assertEquals(0, uncaughtExceptions.get());
    log.debug("Test completed successfully: {} of {} threads"
        + " ran without memory errors", numSuccess, NUM_THREADS);
    log.debug("------  END  -----: {}", this.getClass().getName());
  }
}
