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

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertNotNull;

import java.nio.ByteBuffer;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.test_utils.NameAwareTestClassRunner;


@RunWith(NameAwareTestClassRunner.class)
public class BufferExhaustiveTest
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());

  private String mTestName = null;
  @Before
  public void setUp()
  {
    mTestName = NameAwareTestClassRunner.getTestMethodName();
    log.debug("-----START----- {}", mTestName);
  }
  @After
  public void tearDown()
  {
    log.debug("----- END ----- {}", mTestName);
  }


  /**
   * This tests tries to make sure the IBuffer correctly releases any
   * references it might hold to the underlying java.nio.ByteBuffer
   * it wraps.
   * 
   * It does this by creating a lot of large IBuffer objects.
   */
  @Test
  public void testNoLeaksWhenBufferMadeFromDirectJavaByteBuffer()
  {
    for(int i = 0; i < 100; i++)
    {
      ByteBuffer nativeBytes = ByteBuffer.allocateDirect(10*1024*1024);
      IBuffer buf = IBuffer.make(null, nativeBytes, 0, nativeBytes.capacity());
      assertNotNull(buf);
      buf.delete();
      buf = null;
      
      // We need to do three GC's because it appears Java NIO ByteBuffers sometimes rely
      // on finalizers to free memory.  The first GC will mark something for finalization
      // The second will finalize, and the third will reclaim.
      System.gc();
      System.gc();
      System.gc();
      Thread.yield();
    }
  }

  /**
   * This is a paranoia test really testing java.nio.ByteBuffers.
   * 
   * If you make a view of a ByteBuffer (ByteBuffer.asIntBuffer for example),
   * we want to make sure the resulting IntBuffer maintains a reference
   * back to the original ByteBuffer.
   * 
   */
  @Test(timeout=2*60*1000)
  public void testIntBuffersMaintainReferenceToByteBuffer()
  {
    // make sure we don't have any pinned objects lying around
    while(JNIWeakReference.getMgr().getNumPinnedObjects()>0)
    {
      System.gc();
      JNIWeakReference.getMgr().gc();
    }
    assertEquals(0, JNIWeakReference.getMgr().getNumPinnedObjects());

    // now start the test
    IBuffer buf = IBuffer.make(null, 1024*1024);
    assertNotNull(buf);

    // This will create a reference
    java.nio.ByteBuffer jbuf = buf.getByteBuffer(0, buf.getBufferSize());

    // kill the xuggler refcount in the IBuffer
    buf.delete();

    // this is the test; we want to know if creating an IntBuffer
    // ends up holding a reference to the underlying jbuf
    java.nio.IntBuffer ibuf = jbuf.asIntBuffer();
    ibuf.put(0, 15);

    // release the reference to the bytebuffer
    jbuf = null;

    assertEquals(15, ibuf.get(0));
    
    // now we're going to try for force a gc
    byte[] arr;
    for(int i = 0; i < 1000; i++)
    {
      arr = new byte[1024*1024]; 
      System.gc();
      JNIWeakReference.getMgr().gc();
      arr[0] = (byte)1;
      ibuf.put(i, 16);
    }
    
    // even though the buf and jbuf are now dead, the ibuf should
    // be forcing one buffer to be pinned.
    assertEquals(1, JNIWeakReference.getMgr().getNumPinnedObjects());
    
    // this is here to make sure the compiler doesn't optimize
    // away ibuf
    assertEquals(16, ibuf.get(999));
    // release the ibuf
    ibuf= null;
    // try to force another gc to make sure ibuf is now collected
    while(JNIWeakReference.getMgr().getNumPinnedObjects()>0)
    {
      arr = new byte[1024*1024]; 
      System.gc();
      JNIWeakReference.getMgr().gc();
      arr[0] = (byte)1;
    }

    // and make sure the ibuf is collected
    assertEquals(0, JNIWeakReference.getMgr().getNumPinnedObjects());
  }
}
