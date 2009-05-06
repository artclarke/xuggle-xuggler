package com.xuggle.xuggler;

import static org.junit.Assert.assertEquals;

import org.junit.Test;

import com.xuggle.xuggler.video.ConverterFactory;

public class MultiThreadedReadingExhaustiveTest
{

  static final int NUM_THREADS=100;
  
  @Test(timeout=10*60*1000)
  public void testMultiThreadedTest() throws InterruptedException
  {
    final boolean ADD_VIEWER = System.getProperty(
        MultiThreadedReadingExhaustiveTest.class.getName()+".ShowVideo") != null;
    final Thread threads[] = new Thread[NUM_THREADS];
    final int numPackets[] = new int[NUM_THREADS];
    for(int i = 0; i < threads.length; i++)
    {
      final int index = i;
      threads[index] = new Thread(new Runnable(){
        public void run()
        {
          final MediaReader reader = 
            new MediaReader("fixtures/testfile_videoonly_20sec.flv",
                true,
                ConverterFactory.XUGGLER_BGR_24);
          if (ADD_VIEWER)
          {
            final MediaViewer viewer = new MediaViewer();
            reader.addListener(viewer);
          }
          while(reader.readPacket() == null)
            ++numPackets[index];
        }}, "TestThread_"+index);
      threads[i].start();
    }
    for(int i = 0; i < threads.length; i++)
    {
      threads[i].join();
      assertEquals(1062, numPackets[i]);
    }    
  }
}
