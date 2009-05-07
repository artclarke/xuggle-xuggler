package com.xuggle.xuggler;

import static org.junit.Assert.assertEquals;

import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.xuggler.video.ConverterFactory;

public class MultiThreadedWritingExhaustiveTest
{
  final private Logger log = LoggerFactory.getLogger(this.getClass());
  {
    log.trace("make warning go away");
  }

  static final int NUM_THREADS = 50;

  @Test(timeout = 30 * 60 * 1000)
  public void testMultiThreadedTest() throws InterruptedException
  {
    final boolean ADD_VIEWER = System
        .getProperty(MultiThreadedWritingExhaustiveTest.class.getName()
            + ".ShowVideo") != null;
    final Thread threads[] = new Thread[NUM_THREADS];
    final int numPackets[] = new int[NUM_THREADS];
    for (int i = 0; i < threads.length; i++)
    {
      final int index = i;
      threads[index] = new Thread(new Runnable()
      {
        public void run()
        {
          try
          {
            final MediaReader reader = new MediaReader(
                "fixtures/testfile_videoonly_20sec.flv", true,
                ConverterFactory.XUGGLER_BGR_24);
            reader.setAddDynamicStreams(false);
            reader.setQueryMetaData(true);
            if (ADD_VIEWER)
            {
              final MediaViewer viewer = new MediaViewer();
              reader.addListener(viewer);
            }

            // the writer will attach itself to the reader
            new MediaWriter(MultiThreadedWritingExhaustiveTest.class.getName()
                + "_" + index + ".flv", reader){
              long mediaDataWritten = 0; 
              @Override
              public void onAudioSamples(IAudioSamples samples, int streamIndex)
              {
                super.onAudioSamples(samples, streamIndex);
                ++mediaDataWritten;
                log.trace("wrote audio:{}", mediaDataWritten);
              }
              @Override
              public void onVideoPicture(IVideoPicture picture,
                  java.awt.image.BufferedImage image, int streamIndex)
              {
                super.onVideoPicture(picture, image, streamIndex);
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

        }
      }, "TestThread_" + index);
      threads[i].start();
    }
    for (int i = 0; i < threads.length; i++)
    {
      threads[i].join();
      if (numPackets[i] != -1)
        assertEquals(1062, numPackets[i]);
    }
  }
}
