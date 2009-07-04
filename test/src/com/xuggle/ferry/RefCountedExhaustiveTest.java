/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated.  All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any
 * later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.xuggle.ferry;

import static org.junit.Assert.*;

import java.util.Collection;
import java.util.LinkedList;
import java.util.concurrent.atomic.AtomicBoolean;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

import com.xuggle.ferry.JNIMemoryManager.MemoryModel;

@RunWith(Parameterized.class)
public class RefCountedExhaustiveTest
{
  @Parameters
  public static Collection<Object[]> getModels()
  {
    Collection<Object[]> retval = new LinkedList<Object[]>();
    // add all the models.
    for(MemoryModel model: JNIMemoryManager.MemoryModel.values())
      retval.add(new Object[]{
          model
      });
    return retval;
  }

  public RefCountedExhaustiveTest(JNIMemoryManager.MemoryModel model)
  {
    JNIMemoryManager.setMemoryModel(model);
  }
  
  @Before
  public void setUp()
  {
    JNIMemoryManager.getMgr().flush();
  }
   
  @Test(timeout=5*60*1000)
  public void testReferenceCountingLoadTestOfDeath() throws InterruptedException
  {
    assertEquals("should be no objects for collection", 
        0, JNIReference.getMgr().getNumPinnedObjects());
    RefCountedTester obj = RefCountedTester.make();
    for(int i = 0; i < 1000; i++)
    {
      RefCountedTester copy = obj.copyReference();
      assertNotNull("could not copy reference", copy);
    }
    obj=null;
    while(JNIReference.getMgr().getNumPinnedObjects() > 0)
    {
      byte[] bytes = new byte[1024*1024];
      bytes[0] = 0;
      JNIReference.getMgr().gc();
      System.gc(); // needed on x86_64 systems because otherwise they really delay collecting
    }
    assertEquals("Looks like we leaked an object",
        0, JNIReference.getMgr().getNumPinnedObjects());        

  }

  @Test
  public void testCopyReferenceLoadTest()
  {
    assertEquals(0, JNIMemoryManager.getMgr().getNumPinnedObjects());

    RefCounted obj = RefCountedTester.make();
    
    for(int i = 0; i < 100000; i++)
    {
      RefCounted copy = obj.copyReference();
      copy.delete();
    }
    obj.delete();
    assertEquals(0, JNIMemoryManager.getMgr().getNumPinnedObjects());
  }
  
  @Test
  public void testCopyReferenceLoadTestMultiThreaded() throws InterruptedException
  {
    assertEquals(0, JNIMemoryManager.getMgr().getNumPinnedObjects());
    final RefCounted obj = RefCountedTester.make();

    final int NUM_THREADS=100;
    final int NUM_ITERS=10000;
    final AtomicBoolean start = new AtomicBoolean(false);
    
    Thread[] threads = new Thread[NUM_THREADS];
    for(int i = 0; i < threads.length; i++)
    {
      threads[i] = new Thread(
          new Runnable(){
            public void run()
            {
              synchronized(start)
              {
                while(!start.get())
                  try
                  {
                    start.wait();
                  }
                  catch (InterruptedException e)
                  {
                    Thread.currentThread().interrupt();
                  }
              }
//              System.out.println("Thread started: "+Thread.currentThread().getName());
              for(int i = 0; i < NUM_ITERS; i++)
              {
                RefCounted copy = obj.copyReference();
                copy.delete();
              }
            }},
          "thread_"+i);
    }
    for(int i = 0; i < threads.length; i++)
    {
      threads[i].start();
    }
    synchronized(start)
    {
      start.set(true);
      start.notifyAll();
    }
    for(int i = 0; i < threads.length; i++)
    {
      threads[i].join();
//      System.out.println("Thread finished: "+threads[i].getName());
    }
    obj.delete();
    assertEquals(0, JNIMemoryManager.getMgr().getNumPinnedObjects());
  }
  
}
