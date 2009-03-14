package com.xuggle.xuggler;

import static org.junit.Assert.*;

import com.xuggle.test_utils.NameAwareTestClassRunner;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IPixelFormat;
import com.xuggle.ferry.JNIWeakReference;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@RunWith(NameAwareTestClassRunner.class)
public class MemoryAllocationTest
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
    JNIWeakReference.getMgr().gc();
    log.debug("----- END ----- {}", mTestName);
  }

  @AfterClass
  public static void tearDownClass() throws InterruptedException
  {
    System.gc();
    Thread.sleep(1000);
    JNIWeakReference.getMgr().gc();
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
    JNIWeakReference.getMgr().gc();
    assertEquals("Looks like at least one object is pinned",
        0, JNIWeakReference.getMgr().getNumPinnedObjects());
    
  }
  /**
   * Tests that we appear to correctly release memory when we explicitly
   * tell SWIG we're done with it.
   * @throws InterruptedException if it feels like it
   * 
   */
  @Test(timeout=60000)
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

    while(JNIWeakReference.getMgr().getNumPinnedObjects()>0)
    {
      System.gc();
      JNIWeakReference.getMgr().gc();
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

    while(JNIWeakReference.getMgr().getNumPinnedObjects()>0)
    {
      System.gc();
      JNIWeakReference.getMgr().gc();
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
    JNIWeakReference.getMgr().gc();
    assertTrue("Looks like we didn't leak the large frame????",
        0 < JNIWeakReference.getMgr().getNumPinnedObjects());

  }
}
