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

import static org.junit.Assert.assertEquals;

import java.awt.image.BufferedImage;
import java.util.Collection;
import java.util.LinkedList;
import java.util.concurrent.BrokenBarrierException;
import java.util.concurrent.CyclicBarrier;
import java.util.concurrent.atomic.AtomicInteger;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.ferry.JNIMemoryManager;
import com.xuggle.ferry.JNIMemoryManager.MemoryModel;
import com.xuggle.mediatool.IMediaReader;
import com.xuggle.mediatool.ToolFactory;
import com.xuggle.mediatool.IMediaViewer;

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
@RunWith(Parameterized.class)
public class MultiThreadedReadingExhaustiveTest
{
  final private Logger log = LoggerFactory.getLogger(this.getClass());
  @Parameters
  public static Collection<Object[]> getModels()
  {
    Collection<Object[]> retval = new LinkedList<Object[]>();
    
    final boolean TEST_ALL=true;
    
    if (TEST_ALL)
    {
      // add all the models.
      for(MemoryModel model: JNIMemoryManager.MemoryModel.values())
        // ramp up in terms of multi-threading
        for(int i = 0; i < 5; i++)
          retval.add(new Object[]{
              model, 4*(i+1),
          });
    } else {
      retval.add(new Object[] {
          JNIMemoryManager.MemoryModel.NATIVE_BUFFERS_WITH_STANDARD_HEAP_NOTIFICATION, 20
      });
    }
    return retval;
  }

  final int mThreads;
  public MultiThreadedReadingExhaustiveTest(
      JNIMemoryManager.MemoryModel model,
      int numThreads)
  {
    log.debug("Testing model: {}; Threads: {}", model, numThreads);
    mThreads = numThreads;
    JNIMemoryManager.setMemoryModel(model);
  }
   

  @Test(timeout = 30 * 60 * 1000)
  public void testMultiThreadedTest() throws InterruptedException
  {
    if (System.getProperty("os.name", "Linux").startsWith("Windows"))
      // doesn't run on Windows
      return;
    
    final boolean ADD_VIEWER = System.getProperty(
        MultiThreadedReadingExhaustiveTest.class.getName()+".ShowVideo") != null;
    final CyclicBarrier barrier = new CyclicBarrier(mThreads);
    final Thread threads[] = new Thread[mThreads];
    final int numPackets[] = new int[mThreads];
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
          final IMediaReader reader = 
            ToolFactory.makeReader("fixtures/testfile_videoonly_20sec.flv");
          try {
            reader.setBufferedImageTypeToGenerate(BufferedImage.TYPE_3BYTE_BGR);

            log.debug("Created reader: {}", reader);
            if (ADD_VIEWER)
            {
              final IMediaViewer viewer = ToolFactory.makeViewer();
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
              } else {
                numPackets[index]=-1;
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
        + " ran without memory errors", numSuccess, mThreads);
  }
}
