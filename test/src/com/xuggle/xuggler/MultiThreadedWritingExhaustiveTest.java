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

import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.xuggler.mediatool.IMediaTool;
import com.xuggle.xuggler.mediatool.MediaReader;
import com.xuggle.xuggler.mediatool.MediaViewer;
import com.xuggle.xuggler.mediatool.MediaWriter;
import com.xuggle.xuggler.video.ConverterFactory;

public class MultiThreadedWritingExhaustiveTest
{
  final private Logger log = LoggerFactory.getLogger(this.getClass());
  {
    log.trace("make warning go away");
  }

  static final int NUM_THREADS = 25;

  @Test(timeout = 30 * 60 * 1000)
  public void testMultiThreadedTest() throws InterruptedException
  {
    if (System.getProperty("os.name", "Linux").startsWith("Windows"))
      // doesn't run on Windows
      return;
    
    log.debug("------ START -----: {}", this.getClass().getName());
    final boolean ADD_VIEWER = System
        .getProperty(MultiThreadedWritingExhaustiveTest.class.getName()
            + ".ShowVideo") != null;
    final Thread threads[] = new Thread[NUM_THREADS];
    final int numPackets[] = new int[NUM_THREADS];
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
          final MediaReader reader = new MediaReader(
              "fixtures/testfile_videoonly_20sec.flv", true,
              ConverterFactory.XUGGLER_BGR_24);
          try
          {
            reader.setAddDynamicStreams(false);
            reader.setQueryMetaData(true);
            if (ADD_VIEWER)
            {
              final MediaViewer viewer = new MediaViewer();
              reader.addListener(viewer);
            }

            // the writer will attach itself to the reader
            new MediaWriter(MultiThreadedWritingExhaustiveTest.class.getName()
                + "_" + index + ".flv", reader)
            {
              long mediaDataWritten = 0;

              @Override
              public void onAudioSamples(IMediaTool tool,
                  IAudioSamples samples, int streamIndex)
              {
                super.onAudioSamples(tool, samples, streamIndex);
                ++mediaDataWritten;
                log.trace("wrote audio:{}", mediaDataWritten);
              }

              @Override
              public void onVideoPicture(IMediaTool tool,
                  IVideoPicture picture, java.awt.image.BufferedImage image,
                  int streamIndex)
              {
                super.onVideoPicture(tool, picture, image, streamIndex);
                ++mediaDataWritten;
                log.trace("wrote video:{}", mediaDataWritten);
              };
            };
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
            reader.close();
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
              }
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
        + " ran without memory errors", numSuccess, NUM_THREADS);
    log.debug("------  END  -----: {}", this.getClass().getName());
  }
}
