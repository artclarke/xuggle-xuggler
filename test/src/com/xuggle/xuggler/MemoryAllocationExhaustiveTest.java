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

package com.xuggle.xuggler;

import static org.junit.Assert.*;

import java.util.LinkedList;
import java.util.List;

import com.xuggle.test_utils.NameAwareTestClassRunner;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IPixelFormat;
import com.xuggle.ferry.IBuffer;
import com.xuggle.ferry.JNIMemoryManager;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@RunWith(NameAwareTestClassRunner.class)
public class MemoryAllocationExhaustiveTest
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());
  private String mTestName;
  
  @Before
  public void setUp()
  {
    mTestName = NameAwareTestClassRunner.getTestMethodName();
    log.debug("-----START----- {}", mTestName);
  }
  
  @After
  public void tearDown() throws InterruptedException
  {
    // Always do a collection on tear down, as we want to find leaked objects
    // and don't want things easily collectable screwing up hprof if running
    System.gc();
    Thread.sleep(100);
    JNIMemoryManager.getMgr().gc();
    log.debug("----- END ----- {}", mTestName);
  }

  @AfterClass
  public static void tearDownClass() throws InterruptedException
  {
    System.gc();
    Thread.sleep(1000);
    JNIMemoryManager.getMgr().gc();
  }
  
  @Test
  public void testCorrectRefCounting() throws InterruptedException
  {
    log.debug("create a frame");
    IVideoPicture obj = IVideoPicture.make(IPixelFormat.Type.YUV420P,
        2000, 2000);
    log.debug("force frame data allocation");
    obj.getData(); // Force an allocation
    log.debug("copy frame reference");
    IVideoPicture copy = obj.copyReference();
    log.debug("do ref count check");
    assertEquals(2, copy.getCurrentRefCount());
    assertEquals(2, obj.getCurrentRefCount());
    log.debug("delete object");
    obj.delete();
    log.debug("do second ref count check");
    assertEquals(1, copy.getCurrentRefCount());
    copy.delete();
    
    // now make sure no leaks
    System.gc();
    Thread.sleep(100);
    JNIMemoryManager.getMgr().gc();
    assertEquals("Looks like at least one object is pinned",
        0, JNIMemoryManager.getMgr().getNumPinnedObjects());
    
  }
  /**
   * Tests that we appear to correctly release memory when we explicitly
   * tell SWIG we're done with it.
   * @throws InterruptedException if it feels like it
   * 
   */
  @Test(timeout=600000)
  public void testExplicitDeletionWorks() throws InterruptedException
  {
    int numLoops = 100;
    for(int i = 0; i < numLoops; i++)
    {
      IVideoPicture obj = IVideoPicture.make(IPixelFormat.Type.YUV420P,
          2000, 2000); // that's 2000x2000x3 bytes... if we're not freeing right it'll show up quick.
      assertNotNull("could not allocate object", obj);
      assertNotNull("could not allocate underlying buffer", obj.getData()); // Force an allocation
      //log.debug("allocated frame: {} @ time: {}", i, System.currentTimeMillis());
      obj.delete();
    }

    while(JNIMemoryManager.getMgr().getNumPinnedObjects()>0)
    {
      System.gc();
      JNIMemoryManager.getMgr().gc();
    }
  }

  /**
   * Tests that we appear to correctly release memory when we
   * just let objects fall out of scope and let the GC take over
   * without telling it to.  Note this test may cause
   * swapping on smaller machines, but that's intended.
   * 
   * @throws InterruptedException If it wants to
   */
  @Test(timeout=60000)
  public void testImplicitReleasingWithNoExplicitGCWorks() throws InterruptedException
  {
    int numLoops = 100;
    for(int i = 0; i < numLoops; i++)
    {
      //log.debug("object alloc: {}", i);
      IVideoPicture obj = IVideoPicture.make(IPixelFormat.Type.YUV420P,
          2000, 2000); // that's 2000x2000x3 bytes... if we're not freeing right it'll show up quick.
      assertNotNull("could not allocate frame", obj);
      assertNotNull("could not allocate underlying data", obj.getData()); // Force an allocation
      //log.debug("allocated frame: {} @ time: {}", i, System.currentTimeMillis());
      obj = null;
      //System.gc();

    }

    while(JNIMemoryManager.getMgr().getNumPinnedObjects()>0)
    {
      System.gc();
      JNIMemoryManager.getMgr().gc();
    }
  }

  /**
   * Private var for testLeakLargeFrame()
   */
  private static IVideoPicture mLeakedLargeFrame = null;
  private static Byte[] mLargeLeakedArray = null;
  /**
   * This one is a strange one.  We deliberately leak a large frame by assigning
   * it to a static reference.  That way we can see if it shows up in our
   * memory leak detection tools.  
   * @throws InterruptedException if it feels like it
   */
  @Test
  public void testLeakLargeFrame() throws InterruptedException
  {
    log.debug("create a frame");    
    IVideoPicture obj = IVideoPicture.make(IPixelFormat.Type.YUV420P,
        2000, 2000); // that's 2000x2000x3 bytes... if we're not freeing right it'll show up quick.
    assertNotNull("could not allocate frame", obj);
    log.debug("force frame data allocation");
    assertNotNull("could not allocate underlying data", obj.getData()); // Force an allocation

    log.debug("create java byte array for fun");
    Byte[] arrayToLeak = new Byte[1000000]; // Who'd miss 1 Meg?
    assertNotNull("could not allocate large array to leak", arrayToLeak);
    log.debug("leak java byte array");
    mLargeLeakedArray = arrayToLeak;
    mLargeLeakedArray[0] = 0;
    // Here's the leak
    log.debug("Deliberately leaking a large frame");
    mLeakedLargeFrame = obj;
    obj = null;
    
    // let's confirm there is actually data there
    log.debug("set the pts");
    mLeakedLargeFrame.setPts(0);
    
    log.debug("try setting pixels");
    IPixelFormat.YUVColorComponent colors[] = {
        IPixelFormat.YUVColorComponent.YUV_Y,
        IPixelFormat.YUVColorComponent.YUV_U,
        IPixelFormat.YUVColorComponent.YUV_V
        };
    for(int x = 100; x < 200; x++)
      for(int y = 100; y < 100; y++)
        for(IPixelFormat.YUVColorComponent c : colors)
        {
          final short val = 100;
          IPixelFormat.setYUV420PPixel(mLeakedLargeFrame, x, y, c, val);
          assertEquals(val, IPixelFormat.getYUV420PPixel(mLeakedLargeFrame, x, y, c));
        }
    
    System.gc();
    Thread.sleep(100);
    JNIMemoryManager.getMgr().gc();
    assertTrue("Looks like we didn't leak the large frame????",
        0 < JNIMemoryManager.getMgr().getNumPinnedObjects());
  }
  
  /**
   * It's been pointed out to us that sometimes we core-dump
   * if we run out of memory.  So this method tests that
   * for packets.
   */
  @Test(expected=OutOfMemoryError.class)
  public void testOutOfMemoryIPacket()
  {
    List<IPacket> leakyPackets = new LinkedList<IPacket>();
    while(true)
    {
      IBuffer buf = IBuffer.make(null, 1024*1024);
      if (buf == null)
        throw new OutOfMemoryError();
      IPacket pkt = IPacket.make(buf);
      if (pkt == null)
        throw new OutOfMemoryError();
      pkt.setComplete(true, 1024*1024);
      
      java.nio.ByteBuffer bBuf = pkt.getByteBuffer();
      if (bBuf == null)
        throw new OutOfMemoryError();
      bBuf.put(0, (byte)0);
      leakyPackets.add(pkt);
      log.trace("allocated media: {}", pkt);
    }
  }

  /**
   * It's been pointed out to us that sometimes we core-dump
   * if we run out of memory.  So this method tests that
   * for samples.
   */
  @Test(expected=OutOfMemoryError.class)
  public void testOutOfMemoryIAudioSamples()
  {
    List<IAudioSamples> leakyMedia = new LinkedList<IAudioSamples>();
    while(true)
    {
      IAudioSamples media = IAudioSamples.make(1024*1024, 1);
      if (media == null)
        throw new OutOfMemoryError();
      media.setComplete(true,
          1024*1024, 22050, 1, IAudioSamples.Format.FMT_S16, 0);
      java.nio.ByteBuffer bBuf = media.getByteBuffer();
      if (bBuf == null)
        throw new OutOfMemoryError();
      bBuf.put(0, (byte)0);
      leakyMedia.add(media);
      log.trace("allocated media: {}", media);
    }
  }

  /**
   * It's been pointed out to us that sometimes we core-dump
   * if we run out of memory.  So this method tests that
   * for samples.
   */
  @Test(expected=OutOfMemoryError.class)
  public void testOutOfMemoryIVideoPicture()
  {
    List<IVideoPicture> leakyMedia = new LinkedList<IVideoPicture>();
    while(true)
    {
      IVideoPicture media = IVideoPicture.make(
          IPixelFormat.Type.YUV420P,
          1024,
          1024);
      if (media == null)
        throw new OutOfMemoryError();
      java.nio.ByteBuffer bBuf = media.getByteBuffer();
      if (bBuf == null)
        throw new OutOfMemoryError();
      bBuf.put(0, (byte)0);
      leakyMedia.add(media);
      log.trace("allocated media: {}", media);
    }
  }

}
