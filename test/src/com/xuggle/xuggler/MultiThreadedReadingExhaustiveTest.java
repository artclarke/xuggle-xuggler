package com.xuggle.xuggler;

import static org.junit.Assert.assertEquals;

import java.util.concurrent.BrokenBarrierException;
import java.util.concurrent.CyclicBarrier;

import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.xuggler.video.ConverterFactory;

public class MultiThreadedReadingExhaustiveTest
{
  final private Logger log = LoggerFactory.getLogger(this.getClass());
  { log.trace("make warning go away"); }

  static final int NUM_THREADS=200;
  
  @Test(timeout = 30 * 60 * 1000)
  public void testMultiThreadedTest() throws InterruptedException
  {
    final boolean ADD_VIEWER = System.getProperty(
        MultiThreadedReadingExhaustiveTest.class.getName()+".ShowVideo") != null;
    final CyclicBarrier barrier = new CyclicBarrier(NUM_THREADS);
    final Thread threads[] = new Thread[NUM_THREADS];
    final int numPackets[] = new int[NUM_THREADS];
    
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
          try {
          final MediaReader reader = 
            new MediaReader("fixtures/testfile_videoonly_20sec.flv",
                true,
                ConverterFactory.XUGGLER_BGR_24);
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
            log.debug("Thread {} exited with memory exception WHICH IS OK: {}",
                index, e);
            numPackets[index]=-1;
          }
        }}, "TestThread_"+index);
    }
    // start all the threads
    for(int i = 0; i < threads.length; i++)
    {
      threads[i].start();
    }
    // join all the threads
    for(int i = 0; i < threads.length; i++)
    {
      threads[i].join();
      if (numPackets[i] != -1)
        assertEquals(1062, numPackets[i]);
    }    
  }
}
