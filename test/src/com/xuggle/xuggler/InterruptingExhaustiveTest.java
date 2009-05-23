package com.xuggle.xuggler;

import java.util.concurrent.atomic.AtomicBoolean;

import org.junit.Test;

import static org.junit.Assert.*;

/**
 * Tests to make sure we can interrupt long-running Xuggler calls.
 * 
 * @author aclarke
 *
 */
public class InterruptingExhaustiveTest
{
  
  // needs a long time out because if running in an exhaustive
  // test on a slow machine (oh like our build server) it can
  // sometime fail.
  @Test(timeout=120*1000)
  public void testXugglerSupportInterruptions() throws InterruptedException
  {
    final AtomicBoolean testPassed = new AtomicBoolean(false);
    Thread thread = new Thread(
        new Runnable(){
          public void run()
          {
            synchronized(this) {
              this.notifyAll();
            }
            IContainer container = IContainer.make();

            int retval = container.open(
//                "fixtures/testfile.flv",
               "udp://127.0.0.1:12345/notvalid.flv?localport=28302",
                IContainer.Type.READ,
                null, true, false);
            // we should return from this method in an
            // interrupted state.
            if (Thread.interrupted()) {
              // clear the interrupt
              synchronized(this) {
                this.notifyAll();
              }
              if (retval < 0)
                testPassed.set(true);
            }
          }},
        "xuggler thread");
    
    synchronized(thread) {
      thread.start();
      thread.wait();
    }
    synchronized(thread) {
      thread.interrupt();
      thread.wait();
    }
    thread.join();
    assertTrue("test did not pass", testPassed.get());
  }

}
