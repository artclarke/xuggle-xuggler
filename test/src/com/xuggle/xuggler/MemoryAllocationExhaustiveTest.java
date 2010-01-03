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

import static org.junit.Assert.*;

import java.nio.ByteBuffer;
import java.util.Collection;
import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.atomic.AtomicReference;

import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IPixelFormat;
import com.xuggle.ferry.IBuffer;
import com.xuggle.ferry.JNIMemoryManager;
import com.xuggle.ferry.JNIReference;
import com.xuggle.ferry.MemoryTestHelper;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@RunWith(Parameterized.class)
public class MemoryAllocationExhaustiveTest
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());

  @Parameters
  public static Collection<Object[]> getModels()
  {
    Collection<Object[]> retval = new LinkedList<Object[]>();
    // add all the models.
    final boolean TEST_ALL=true;
    if (TEST_ALL) {
      for (JNIMemoryManager.MemoryModel model :
        JNIMemoryManager.MemoryModel.values())
        retval.add(new Object[]
        {
          model
        });
    }
    else {
      retval.add(new Object[]{
          JNIMemoryManager.MemoryModel.JAVA_DIRECT_BUFFERS
      });
    }
    return retval;
  }

  public MemoryAllocationExhaustiveTest(JNIMemoryManager.MemoryModel model)
  {
    log.debug("Testing model: {}", model);
    JNIMemoryManager.getMgr().flush();
    assertEquals("objects already allocated?",
        0, JNIMemoryManager.getMgr().getNumPinnedObjects());

    JNIMemoryManager.setMemoryModel(model);
  }

  private boolean mSuccess;

  @Before
  public void setUp()
  {
    log.debug("------ START ------");
    mSuccess = false;
  }

  @After
  public void tearDown() throws InterruptedException
  {
    // Always flush when done; this needs to happen BEFORE
    // we unload this class, or else the next test might fail.
    JNIMemoryManager.getMgr().flush();
    log.debug("------ END (Success={}) ------", mSuccess);
  }

  @Test(timeout = 5 * 60 * 1000)
  public void testCorrectRefCounting() throws InterruptedException
  {
    log.debug("Testing {}", "testCorrectRefCounting");
    assertEquals("objects already allocated?",
        0, JNIMemoryManager.getMgr().getNumPinnedObjects());
        
    log.trace("create a frame");
    IVideoPicture obj = IVideoPicture.make(IPixelFormat.Type.YUV420P, 2000,
        2000);
    log.trace("force frame data allocation");
    IBuffer buffer = obj.getData(); // Force an allocation
    buffer.delete(); // (and delete the reference)
    log.trace("copy frame reference");
    IVideoPicture copy = obj.copyReference();
    log.trace("do ref count check");
    assertEquals(2, copy.getCurrentRefCount());
    assertEquals(2, obj.getCurrentRefCount());
    log.trace("delete object");
    obj.delete();
    log.trace("do second ref count check");
    assertEquals(1, copy.getCurrentRefCount());
    copy.delete();
    assertEquals("Looks like at least one object is pinned", 0,
        JNIMemoryManager.getMgr().getNumPinnedObjects());
    mSuccess = true;
  }

  /**
   * Tests that we appear to correctly release memory when we explicitly tell
   * Ferry we're done with it.
   * 
   * @throws InterruptedException if it feels like it
   * 
   */
  @Test(timeout = 600*1000)
  public void testExplicitDeletionWorks() throws InterruptedException
  {
    log.debug("Testing {}", "testExplicitDeletionWorks");
    assertEquals("objects already allocated?",
        0, JNIMemoryManager.getMgr().getNumPinnedObjects());
        
    int numLoops = 100;
    for (int i = 0; i < numLoops; i++)
    {
      IVideoPicture obj = IVideoPicture.make(IPixelFormat.Type.YUV420P, 2000,
          2000); // that's 2000x2000x3 bytes... if we're not freeing right it'll
                 // show up quick.
      assertNotNull("could not allocate object", obj);
      IBuffer buffer = obj.getData();
      assertNotNull("could not allocate underlying buffer", buffer);
      // log.trace("allocated frame: {} @ time: {}", i,
      buffer.delete();
      obj.delete();
    }
    assertEquals("objects already allocated?",
        0, JNIMemoryManager.getMgr().getNumPinnedObjects());
    mSuccess = true;
  }

  /**
   * Tests that we appear to correctly release memory when we just let objects
   * fall out of scope and let the GC take over without telling it to. Note this
   * test may cause swapping on smaller machines, but that's intended.
   * 
   * @throws InterruptedException If it wants to
   */
  @Test(timeout = 10 * 60 * 1000)
  public void testImplicitReleasingWithNoExplicitGCWorks()
      throws InterruptedException
  {
    log.debug("Testing {}", "testImplicitReleasingWithNoExplicitGCWorks");
    JNIMemoryManager.MemoryModel model = JNIMemoryManager.getMemoryModel();
    if (
        model == JNIMemoryManager.MemoryModel.NATIVE_BUFFERS ||
        model == JNIMemoryManager.MemoryModel.JAVA_DIRECT_BUFFERS)
    {
      log.debug("Skipping test; explicitly won't work with this model: {}",
          JNIMemoryManager.getMemoryModel());
      return;
    }

    assertEquals("objects left over from prior test?",
        0, JNIMemoryManager.getMgr().getNumPinnedObjects());
        
    int numLoops = 1000;
    for (int i = 0; i < numLoops; i++)
    {
      // that's 2000x2000x3 bytes... if we're not freeing right it'll
      // show up quick.
      IVideoPicture obj = IVideoPicture.make(
          IPixelFormat.Type.YUV420P, 2000, 2000);
      assertNotNull("could not allocate frame", obj);
      assertNotNull("could not allocate underlying data", obj.getData());
      obj = null;
    }

    // everything should be 
    log.debug("Test finished; cleaning up: {}",
        JNIMemoryManager.getMgr().getNumPinnedObjects());
    mSuccess = true;
  }

  /**
   * Private var for testLeakLargeFrame()
   */
  private static IVideoPicture mLeakedLargeFrame = null;
  private static Byte[] mLargeLeakedArray = null;

  /**
   * This one is a strange one. We deliberately leak a large frame by assigning
   * it to a static reference. That way we can see if it shows up in our memory
   * leak detection tools.
   * 
   * @throws InterruptedException if it feels like it
   */
  @Test
  public void testLeakLargeFrame() throws InterruptedException
  {
    log.debug("Testing {}", "testLeakLargeFrame");

    log.trace("create a frame");
    // that's 2000x2000x3 bytes... if we're not freeing right it'll
    // show up quick.
    IVideoPicture obj = IVideoPicture.make(
        IPixelFormat.Type.YUV420P, 2000, 2000);
    assertNotNull("could not allocate frame", obj);
    log.trace("force frame data allocation");
    
    // Force an allocation
    assertNotNull("could not allocate underlying data", obj.getData()); 

    log.trace("create java byte array for fun");
    Byte[] arrayToLeak = new Byte[1000000]; // Who'd miss 1 Meg?
    assertNotNull("could not allocate large array to leak", arrayToLeak);
    log.trace("leak java byte array");
    mLargeLeakedArray = arrayToLeak;
    mLargeLeakedArray[0] = 0;
    // Here's the leak
    log.trace("Deliberately leaking a large frame");
    mLeakedLargeFrame = obj;
    obj = null;

    // let's confirm there is actually data there
    log.trace("set the pts");
    mLeakedLargeFrame.setPts(0);

    log.trace("try setting pixels");
    IPixelFormat.YUVColorComponent colors[] =
    {
        IPixelFormat.YUVColorComponent.YUV_Y,
        IPixelFormat.YUVColorComponent.YUV_U,
        IPixelFormat.YUVColorComponent.YUV_V
    };
    for (int x = 100; x < 200; x++)
      for (int y = 100; y < 100; y++)
        for (IPixelFormat.YUVColorComponent c : colors)
        {
          final short val = 100;
          IPixelFormat.setYUV420PPixel(mLeakedLargeFrame, x, y, c, val);
          assertEquals(val, IPixelFormat.getYUV420PPixel(mLeakedLargeFrame, x,
              y, c));
        }

    // now we're going to force a GC to occur by overallocating
    // memory and catching the exception.  This is to test that
    // as many weak references as could be were cleared.
    MemoryTestHelper.forceJavaHeapWeakReferenceClear();
    
    assertTrue("Looks like we didn't leak the large frame????",
        0 < JNIMemoryManager.getMgr().getNumPinnedObjects());

    // and free it so that we can move on with our tests.
    mLeakedLargeFrame = null;
    mSuccess = true;
  }

  /**
   * It's been pointed out to us that sometimes we core-dump if we run out of
   * memory. So this method tests that for packets.
   */
  
  private interface IMediaAllocator {
    IMediaData getMedia();
  }
  private void testOutOfMemoryHelper(IMediaAllocator allocator)
  {
    String osName = System.getProperty("os.name", "Linux");
    if (osName != null && osName.length() > 0 &&
        osName.startsWith("Mac"))
      // turns out the Apple JVM will let a virtual machine
      // allocate memory until the cows come home and uses
      // swap space.  this will therefore KILL the machine
      // it runs on.
      return;
    
    class Tuple {
      private final ByteBuffer mBuffer;
      private final JNIReference mReference;

      public Tuple(ByteBuffer buffer, JNIReference reference)
      {
        mBuffer = buffer;
        mReference = reference;
      }

      public JNIReference getReference()
      {
        return mReference;
      }

      public ByteBuffer getBuffer()
      {
        return mBuffer;
      }
    };

    List<Tuple> leakyMedia = null;
    long bytesBeforeFailure = 0;
    int i = 0;
    try
    {
      leakyMedia = new LinkedList<Tuple>();
      while (true)
      {
        
        IMediaData media = null;
        IBuffer buffer = null;
        AtomicReference<JNIReference> reference
        = new AtomicReference<JNIReference>(null);
        try {
          media = allocator.getMedia();
  
          buffer = media.getData();
          if (buffer == null) {
            log.debug("Could not get IBuffer");
            throw new OutOfMemoryError();
          }
  
          ByteBuffer bBuf = buffer.getByteBuffer(0,
              media.getSize(), reference);
          if (bBuf == null) {
            log.debug("Could not get ByteBuffer");
            throw new OutOfMemoryError();
          }
          // some OSes will cheat and not actually commit the 
          // pages to memory unless you use them; so let's do
          // exactly that.
          for(int j = 0; i < media.getSize(); i++)
            bBuf.put(j, (byte) 0);
          Tuple tuple = new Tuple(bBuf, reference.get());
          leakyMedia.add(tuple);
          assertNotNull(tuple.getBuffer());
          assertNotNull(tuple.getReference());
          bytesBeforeFailure += media.getSize();
          
          if (bytesBeforeFailure > 4*1024*1024*1024)
          {
            // this test causes too much swapping on 64 bit machines, so
            // if we get this far, just return
            mSuccess = true;
            return;
          }
          
        } finally {
          if (media != null)
            media.delete();
          if (buffer != null)
            buffer.delete();
          if (i %100 == 0) {
            log.trace(
                "iteration: {}; allocated {} bytes; pinned: {}; holding: {}",
                new Object[]{i,
                bytesBeforeFailure,
                JNIMemoryManager.getMgr().getNumPinnedObjects(),
                leakyMedia.size()
                }
            );
          }
          i++;
        }
      }
    }
    catch (OutOfMemoryError e)
    {
      int size = leakyMedia.size();
      leakyMedia.clear();
      //e.printStackTrace();
      assertTrue("didn't allocate anything", size > 0);
      log.debug("Bytes before failure: {}; Failure: {}", bytesBeforeFailure, e);
      mSuccess = true;
    }
  }

  @Test
  public void testOutOfMemoryIPacket()
  {
    IMediaAllocator allocator = new IMediaAllocator(){
      public IMediaData getMedia()
      {
        IPacket media = IPacket.make(1024*1024);
        if (media == null)
          throw new OutOfMemoryError();
        media.setComplete(true, 1024 * 1024);
        return media;
      }
    };
    log.debug("Testing Packet Out Of Memory Handling");
    testOutOfMemoryHelper(allocator);
  }
  
  /**
   * It's been pointed out to us that sometimes we core-dump if we run out of
   * memory. So this method tests that for samples.
   */
  @Test
  public void testOutOfMemoryIAudioSamples()
  {
    IMediaAllocator allocator = new IMediaAllocator(){
      public IMediaData getMedia()
      {
        IAudioSamples media = IAudioSamples.make(1024 * 1024, 1);
        if (media == null)
          throw new OutOfMemoryError();
        media.setComplete(true, 1024 * 1024, 22050, 1,
            IAudioSamples.Format.FMT_S16, 0);
        return media;
      }
    };
    log.debug("Testing Audio Samples Out Of Memory Handling");
    testOutOfMemoryHelper(allocator);
  }

  /**
   * It's been pointed out to us that sometimes we core-dump if we run out of
   * memory. So this method tests that for samples.
   */
  @Test
  public void testOutOfMemoryIVideoPicture()
  {
    IMediaAllocator allocator = new IMediaAllocator(){
      public IMediaData getMedia()
      {
        IVideoPicture media = IVideoPicture.make(IPixelFormat.Type.YUV420P,
            1024, 1024);
        if (media == null)
          throw new OutOfMemoryError();
        media.setComplete(true,
            IPixelFormat.Type.YUV420P, 1024, 1024, 0);
        return media;
      }
    };
    log.debug("Testing Video Picture Out Of Memory Handling");
    testOutOfMemoryHelper(allocator);
  }
}
