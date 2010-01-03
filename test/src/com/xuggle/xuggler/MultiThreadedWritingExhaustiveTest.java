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
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

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

@RunWith(Parameterized.class)
public class MultiThreadedWritingExhaustiveTest
{
  final private Logger log = LoggerFactory.getLogger(this.getClass());
  
  @Parameters
  public static Collection<Object[]> getModels()
  {
    Collection<Object[]> retval = new LinkedList<Object[]>();
    // add all the models.
    int i = 0;
    for(MemoryModel model: JNIMemoryManager.MemoryModel.values())
    {
      for(int j = 0; j < 5; j++) {
        retval.add(new Object[]{
            model, 4*(j+1), i
        });
        ++i;
      }
    }
    return retval;
  }

  private final int mThreads;
  private final int mTestNumber;
  private final MemoryModel mModel;
  public MultiThreadedWritingExhaustiveTest(
      JNIMemoryManager.MemoryModel model,
      int numThreads, int testNo)
  {
    log.debug("Testing model: {}; Threads: {}", model, numThreads);
    mModel = model;
    mThreads = numThreads;
    mTestNumber = testNo;
    JNIMemoryManager.setMemoryModel(model);
  }

  @Test(timeout = 30 * 60 * 1000)
  public void testMultiThreadedTest() throws InterruptedException
  {
    if (System.getProperty("os.name", "Linux").startsWith("Windows"))
      // doesn't run on Windows
      return;
    
    final boolean ADD_VIEWER = System
        .getProperty(MultiThreadedWritingExhaustiveTest.class.getName()
            + ".ShowVideo") != null;
    final Thread threads[] = new Thread[mThreads];
    final int numPackets[] = new int[mThreads];
    final AtomicInteger uncaughtExceptions = new AtomicInteger();
    final AtomicReference<Throwable> lastUncaughtException = new AtomicReference<Throwable>(
        null);

    for (int i = 0; i < threads.length; i++)
    {
      final int index = i;
      threads[index] = new Thread(new Runnable()
      {
        public void run()
        {
          final IMediaReader reader = ToolFactory.makeReader(
              "fixtures/testfile_videoonly_20sec.flv");
          try
          {
            reader.setBufferedImageTypeToGenerate(BufferedImage.TYPE_3BYTE_BGR);
            reader.setAddDynamicStreams(false);
            reader.setQueryMetaData(true);
            if (ADD_VIEWER)
            {
              final IMediaViewer viewer = ToolFactory.makeViewer();
              reader.addListener(viewer);
            }

            reader.addListener(ToolFactory.makeWriter(
                MultiThreadedWritingExhaustiveTest.class.getName()
                + "_" + mModel.toString()
                + "_" + mTestNumber + "_" + index + ".flv", reader));
            while (reader.readPacket() == null)
              ++numPackets[index];
          }
          catch (OutOfMemoryError e)
          {
            // This test will cause this error on small JVMs, and that's OK
            // we'll just let this thread abort and keep going. There are other
            // tests in this suite that look at memory allocation errors.
            log.debug("Thread {} exited with memory exception WHICH IS OK: {}",
                index, e);
            numPackets[index] = -1;
          }
          finally
          {
            try {
              reader.close();
            } catch (RuntimeException e) {
              log.debug("Got and ignoring: {}", e);
            }
            log.debug("thread exited with {} packets processed",
                numPackets[index]);
          }

        }
      }, "TestThread_" + index);
      threads[i]
          .setUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler()
          {
            public void uncaughtException(Thread t, Throwable e)
            {
              if (!(e instanceof OutOfMemoryError))
              {
                log.debug(
                    "Uncaught exception leaked out of thread: {}; {}",
                    e,
                    t);
                e.printStackTrace();
                uncaughtExceptions.incrementAndGet();
                lastUncaughtException.set(e);
              } else
                numPackets[index]=-1;

            }
          });
      threads[i].start();
    }
    int numSuccess = 0;
    for (int i = 0; i < threads.length; i++)
    {
      threads[i].join();
    }
    assertEquals("got uncaught exception: " + lastUncaughtException.get(), 0,
        uncaughtExceptions.get());
    for (int i = 0; i < threads.length; i++)
    {
      if (numPackets[i] != -1)
      {
        assertEquals(1062, numPackets[i]);
        ++numSuccess;
      }
    }
    log.debug("Test completed successfully: {} of {} threads"
        + " ran without memory errors", numSuccess, mThreads);
  }
}
