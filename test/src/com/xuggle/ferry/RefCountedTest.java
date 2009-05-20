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

package com.xuggle.ferry;

import static org.junit.Assert.*;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Test;
import org.junit.Ignore;

import com.xuggle.ferry.RefCountedTester;
import com.xuggle.ferry.JNIWeakReference;

public class RefCountedTest
{
  
  @After
  public void tearDown()
  {
    // Always do a collection on tear down, as we want to find leaked objects
    // and don't want things easily collectable screwing up hprof if running
    System.gc();
  }
  
  @AfterClass
  public static void tearDownClass() throws InterruptedException
  {
    System.gc();
    Thread.sleep(1000);
    JNIWeakReference.getMgr().gc();
  }
  
  @Test
  public void testCorrectStartingRefCount()
  {
    RefCountedTester obj = RefCountedTester.make();
    assertEquals("starting ref count", 1, obj.getCurrentRefCount());
  }
  
  @Test
  public void testJavaCopyKeepsRefcountConstant()
  {
    RefCountedTester obj = RefCountedTester.make();
    assertEquals("starting ref count", 1, obj.getCurrentRefCount());
    RefCountedTester javaCopy = obj;
    assertEquals("java copy should keep ref the same", 1, javaCopy.getCurrentRefCount());
  }
  
  @Test(timeout=2000)
  public void testNativeCopyRefcountIncrement() throws InterruptedException
  {
    RefCountedTester obj = RefCountedTester.make();
    assertEquals("starting ref count", 1, obj.getCurrentRefCount());
    RefCountedTester nativeCopy = RefCountedTester.make(obj);
    assertEquals("native copy should increment", 2, obj.getCurrentRefCount());
    assertEquals("native copy should increment", 2, nativeCopy.getCurrentRefCount());
    nativeCopy.delete();
    nativeCopy = null;
    assertEquals("native copy should be decremented", 1, obj.getCurrentRefCount());
    obj.delete();
    obj = null;
    System.gc();
    Thread.sleep(1000);
    assertEquals("should be no objects for collection", 0, JNIWeakReference.getMgr().getNumPinnedObjects());
  }
  
  @Test(timeout=2000)
  public void testCopyByReference() throws InterruptedException
  {
    RefCountedTester obj1 = RefCountedTester.make();
    
    RefCountedTester obj2 = obj1.copyReference();
    
    assertTrue("should look like different objects", obj1 != obj2);
    assertTrue("should be equal though", obj1.equals(obj2));
    assertEquals("should have same ref count", obj1.getCurrentRefCount(), obj2.getCurrentRefCount());
    assertEquals("should have ref count of 2", 2, obj2.getCurrentRefCount());
    
    obj1.delete(); obj1 = null;
    assertEquals("should now have refcount of 1", 1, obj2.getCurrentRefCount());
    obj2.delete();
    obj2 = null;
    System.gc();
    Thread.sleep(1000);
    assertEquals("should be no objects for collection", 0, JNIWeakReference.getMgr().getNumPinnedObjects());    
  }
  
  /**
   * This test tries to make sure the Java gargage collector
   * eventually forces a collection of RefCounted objects, even
   * if we don't call our own mini-gc methods.
   * <p>
   * It's disabled though because it can occasionally fail due
   * to Java just deciding it doesn't need to collect yet.  But
   * the fact that it passes sometimes does show that a collection
   * will occur.
   * @throws InterruptedException if interrupted
   */
  @Test(timeout=4000)
  @Ignore
  public void testGarbageCollectionDoesEventuallyReleaseNativeReferences() throws InterruptedException
  {
    RefCountedTester obj1 = RefCountedTester.make();
    
    RefCountedTester obj2 = obj1.copyReference();
    
    assertTrue("should look like different objects", obj1 != obj2);
    assertTrue("should be equal though", obj1.equals(obj2));
    assertEquals("should have same ref count", obj1.getCurrentRefCount(), obj2.getCurrentRefCount());
    assertEquals("should have ref count of 2", 2, obj2.getCurrentRefCount());
    
    obj1 = null;
    
    // obj1 should now be unreachable, so if we try a Garbage collection it should get caught.
    System.gc();
    Thread.sleep(1500);
    System.gc();
    Thread.sleep(1500);
    // at this point the Java proxy object will be unreachable, but should be sitting in the
    // reference queue and also awaiting finalization.  The finalization should have occurred by now.
    assertEquals("should be only the first object for collection", 1, JNIWeakReference.getMgr().getNumPinnedObjects());        
    assertEquals("should have a ref refcount of 1", 1, obj2.getCurrentRefCount());
  }
  
  @Test
  public void testJNIWeakReferenceFlushQueue() throws InterruptedException
  {
    RefCountedTester obj1 = RefCountedTester.make();
    
    RefCountedTester obj2 = obj1.copyReference();
    
    assertTrue("should look like different objects", obj1 != obj2);
    assertTrue("should be equal though", obj1.equals(obj2));
    assertEquals("should have same ref count", obj1.getCurrentRefCount(), obj2.getCurrentRefCount());
    assertEquals("should have ref count of 2", 2, obj2.getCurrentRefCount());
    
    obj1 = null;
    
    // obj1 should now be unreachable, so if we try a Garbage collection it should get caught.
    System.gc();
    Thread.sleep(100); // need this time to have obj1 either finalized, or added to the weak ref queue
    JNIWeakReference.getMgr().gc();
    assertEquals("should now have a ref refcount of 1", 1, obj2.getCurrentRefCount());
    assertEquals("should be only the first object for collection", 1, JNIWeakReference.getMgr().getNumPinnedObjects());        
  }
  
  @Test(timeout=5000)
  public void testReferenceCountingLoadTestOfDeath() throws InterruptedException
  {
    RefCountedTester obj = RefCountedTester.make();
    for(int i = 0; i < 10000; i++)
    {
      RefCountedTester copy = obj.copyReference();
      assertNotNull("could not copy reference", copy);
      copy = null;
    }
    obj=null;
    System.gc();
    Thread.sleep(1000);
    JNIWeakReference.getMgr().gc();
    assertEquals("Looks like we leaked an object", 0, JNIWeakReference.getMgr().getNumPinnedObjects());        

  }

  /**
   * Tests that calling .delete() on a RefCounted object and
   * then derefing it raises an exception, but does not crash
   * the JVM
   */
  @Test(expected=NullPointerException.class)
  public void testDeleteThenCallRaisesException()
  {
    RefCountedTester obj = RefCountedTester.make();
    obj.delete();
    // this should raise an exception
    obj.getCurrentRefCount();
  }
 
}
