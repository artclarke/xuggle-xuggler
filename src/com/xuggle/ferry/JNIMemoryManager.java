/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated. All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler. If not, see <http://www.gnu.org/licenses/>.
 */

package com.xuggle.ferry;

import java.lang.ref.ReferenceQueue;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.atomic.AtomicLong;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Manage's native memory that Ferry objects allocate and destroy.
 * <p>
 * This class is used by the Ferry system as the root for our own memory
 * management system.
 * </p>
 * <p>
 * In general you never need to use this mechanism as it's automatically used
 * behind the scenes.
 * </p>
 * <p>
 * However in some situations, especially if you are overriding the default
 * memory management used by Ferry, or are returning lots of
 * {@link java.nio.ByteBuffer} objects from {@link IBuffer} objects and not
 * keeping the corresponding {@link IBuffer} reference around, you may want to
 * force a Ferry &quot;collection&quot;. use this class to do that.
 * </p>
 * <p>
 * A Ferry collection is much cheaper than a Java {@link System#gc()} collection
 * -- it just examines a {@link ReferenceQueue} maintained internally and
 * releases any pending backing native memory.
 * </p>
 * <p>
 * How do you know if you need to explicitly call methods on this class? Well,
 * assume you don't, but if you're seeing the native heap on your program start
 * to scale <strong>and you know you're not leaking references</strong> try
 * calling {@link #collect()}. If it doesn't fix the problem, then you probably
 * are holding on to some reference in your program somewhere.
 * </p>
 * <p>
 * Or if you're running a Java Profiler and see your application
 * is spending a lot of time copying on incremental collections,
 * or in {@link JNIMemoryAllocator} methods, check out the
 * {@link #getMemoryModel()} method.
 * </p>
 * 
 * @author aclarke
 * 
 */
public class JNIMemoryManager
{
  static final private JNIMemoryManager mMgr = new JNIMemoryManager();

  /**
   * Get the global {@link JNIMemoryManager} being used.
   * 
   * @return the manager
   */
  static public JNIMemoryManager getMgr()
  {
    return mMgr;
  }

  /**
   * A convenience way to call {@link #getMgr()}.{@link #gc()}
   */
  static public void collect()
  {
    getMgr().gc();
  }

  private static final Logger log = LoggerFactory
      .getLogger(JNIMemoryManager.class);

  private final ReferenceQueue<Object> mRefQueue;

  /**
   * A thread-safe set of references to ferry and non-ferry based objects that
   * are referencing underlying Ferry native memory.
   * 
   * We have to enqueue all JNIReferences, otherwise we can't be guaranteed that
   * Java will enqueue them on a reference queue. We try to be smart and
   * efficient about this, but the fact remains that you have to pass this lock
   * for all Ferry calls.
   */
  private final ConcurrentMap<JNIReference, JNIReference> mRefList;
  private final AtomicLong mNumPinnedObjects;

  private volatile Thread mCollectionThread;

  /**
   * The constructor is package level so others can't create it.
   */
  JNIMemoryManager()
  {
    mNumPinnedObjects = new AtomicLong(0);
    mRefQueue = new ReferenceQueue<Object>();
    mRefList = new ConcurrentHashMap<JNIReference, JNIReference>();
    mCollectionThread = null;
  }

  /**
   * Get the underlying queue we track references with.
   * 
   * @return The queue.
   */
  ReferenceQueue<Object> getQueue()
  {
    return mRefQueue;
  }

  /**
   * Get the number of Ferry objects we believe are still in use.
   * <p>
   * This may be different than what you think because the Java garbage
   * collector may not have collected all objects yet.
   * </p>
   * 
   * @return number of ferry objects in use.
   */
  public long getNumPinnedObjects()
  {
    long retval = mNumPinnedObjects.get();
    return retval;
  }

  /**
   * A finalizer for the memory manager itself. It just calls internal garbage
   * collections and then exists.
   */
  public void finalize()
  {
    /**
     * 
     * This may end up "leaking" some memory if all Ferry objects have not
     * otherwise been collected, but this is not a huge problem for most
     * applications, as it's only called when the class loader is exiting.
     */
    log.trace("destroying: {}", this);
    mRefList.clear();
    mNumPinnedObjects.set(0);

    gc();
    gc();
  }

  /**
   * Add a reference to the set of references we'll collect.
   * 
   * @param ref The reference to collect.
   * @return true if already in list; false otherwise.
   */
  boolean addReference(JNIReference ref)
  {
    mNumPinnedObjects.incrementAndGet();
    return mRefList.put(ref, ref) != null;
  }

  /**
   * Remove this reference from the set of references we're tracking, and
   * collect it.
   * 
   * @param ref The reference to remove
   * @return true if the reference was in the queue; false if not.
   */
  boolean removeReference(JNIReference ref)
  {
    mNumPinnedObjects.decrementAndGet();
    return mRefList.remove(ref) != ref;
  }

  /**
   * Do a Ferry Garbage Collection.
   * 
   * This takes all Ferry objects that are no longer reachable and deletes the
   * underlying native memory. It is called every time you allocate a new Ferry
   * object to ensure we're freeing up native objects as soon as possible
   * (rather than waiting for the potentially slow finalizer thread). It is also
   * called via a finalizer on an object that is referenced by the Ferry'ed
   * object (that way, the earlier of the next Ferry allocation, or the
   * finalizer thread, frees up unused native memory). Lastly, you can use
   * {@link #startCollectionThread()} to start up a thread to call this
   * automagically for you (and that thread will exit when your JVM exits).
   */
  public void gc()
  {
    JNIReference ref = null;
    while ((ref = (JNIReference) mRefQueue.poll()) != null)
    {
      ref.delete();
    }
  }

  /**
   * Starts a new Ferry collection thread that will wake up whenever a memory
   * reference needs clean-up from native code.
   * <p>
   * This thread is not started by default as Xuggler calls {@link #gc()}
   * internally whenever a new Xuggler object is allocated. But if you're
   * caching Xuggler objects and hence avoiding new allocations, you may want to
   * call this to ensure all objects are promptly collected.
   * </p>
   * <p>
   * This call is ignored if the collection thread is already running.
   * </p>
   * <p>
   * The thread can be stopped by calling {@link #stopCollectionThread()}, and
   * will also exit if interrupted by Java.
   * </p>
   */
  public void startCollectionThread()
  {
    synchronized (this)
    {
      if (mCollectionThread != null)
        throw new RuntimeException("Thread already running");

      mCollectionThread = new Thread(new Runnable()
      {

        public void run()
        {
          JNIReference ref = null;
          try
          {
            while (true)
            {
              ref = (JNIReference) mRefQueue.remove();
              if (ref != null)
                ref.delete();
            }
          }
          catch (InterruptedException ex)
          {
            synchronized (JNIMemoryManager.this)
            {
              mCollectionThread = null;
              // reset the interruption
              Thread.currentThread().interrupt();
            }
            return;
          }

        }
      }, "Xuggle Ferry Collection Thread");
      mCollectionThread.setDaemon(true);
      mCollectionThread.start();
    }
  }

  /**
   * Stops the Ferry collection thread if running. This does nothing if no
   * collection thread is running.
   */
  public void stopCollectionThread()
  {
    synchronized (this)
    {
      if (mCollectionThread != null)
        mCollectionThread.interrupt();
    }
  }

  /**
   * Get the {@link MemoryModel} that Ferry is using for allocating large memory
   * blocks.
   * 
   * @return the memory model currently being used.
   * 
   * @see MemoryModel
   */
  public static MemoryModel getMemoryModel()
  {
    int model = 0;
    model = FerryJNI.getMemoryModel();
    MemoryModel retval = MemoryModel.JAVA_BYTE_ARRAYS;
    for (MemoryModel candidate : MemoryModel.values())
      if (candidate.getNativeValue() == model)
        retval = candidate;
    return retval;
  }

  /**
   * Sets the {@link MemoryModel}.
   * <p>
   * Only call once per process; Calling more than once has an unspecified
   * effect.
   * </p>
   * 
   * @param model The model you want to use.
   * 
   * @see #getMemoryModel()
   * @see MemoryModel
   */
  public static void setMemoryModel(MemoryModel model)
  {
    FerryJNI.setMemoryModel(model.getNativeValue());
  }

  /**
   * Internal Only.
   * 
   * Immediately frees all active objects in the system.  Do not
   * call unless you REALLY know what you're doing.
   */
  public void flush()
  {
    // Make a copy so we don't modify the real map
    // inside an iterator.
    Map<JNIReference, JNIReference> refs = 
      new HashMap<JNIReference, JNIReference>();
    
    refs.putAll(mRefList);
    for(JNIReference ref : refs.keySet())
      if (ref != null)
        ref.delete();
  }

  /**
   * The different types of native memory allocation models Ferry supports.
   * <h2>Memory Model Performance Implications</h2> Choosing the
   * {@link MemoryModel} you use in Ferry libraries can have a big effect. Some
   * models emphasize code that will work "as you expect" (Robustness), but
   * sacrifice some execution speed to make that happen. Other models value
   * speed first, and assume you know what you're doing and can manage your own
   * memory.
   * <p>
   * In our experience the set of people who need robust software is larger than
   * the set of people who need the (small) speed price paid, and so we default
   * to the most robust model.
   * </p>
   * <p>
   * Also in our experience, the set of people who really should just use the
   * robust model, but instead think they need speed is much larger than the set
   * of people who actually know what they're doing with java memory management,
   * so please, <strong>we strongly recommend you start with a robust model and
   * only change the {@link MemoryModel} if your performance testing shows you
   * need speed.</strong> Don't say we didn't warn you.
   * </p>
   * 
   * <table>
   * <tr>
   * <th>Model</th>
   * <th>Robustness</th>
   * <th>Speed</th>
   * </tr>
   * 
   * <tr>
   * <td> {@link MemoryModel#JAVA_BYTE_ARRAYS} (default)</td>
   * <td>++++</td>
   * <td>+</td>
   * </tr>
   * 
   * <tr>
   * <td> {@link MemoryModel#NATIVE_BUFFERS_WITH_JAVA_NOTIFICATION}
   * (experimental)</td>
   * <td>++</td>
   * <td>+++</td>
   * </tr>
   * 
   * <tr>
   * <td> {@link MemoryModel#NATIVE_BUFFERS}</td>
   * <td>+</td>
   * <td>++++</td>
   * </tr>
   * 
   * <tr>
   * <td> {@link MemoryModel#JAVA_DIRECT_BUFFERS} (not recommended)</td>
   * <td>+</td>
   * <td>++</td>
   * </tr>
   * 
   * 
   * 
   * </table>
   * <h2>What is &quot;Robustness&quot;?</h2>
   * <p>
   * Ferry objects have to allocate native memory to do their job -- it's the
   * reason for Ferry's existence. And native memory management is very
   * different than Java memory management (for example, native C++ code doesn't
   * have a garbage collector). To make things easier for our Java friends,
   * Ferry tries to make Ferry objects look like Java objects.
   * </p>
   * <p>
   * Which leads us to robustness. The more of these criteria we can hit with a
   * {@link MemoryModel} the more robust it is.
   * </p>
   * <ol>
   * 
   * <li><strong>Allocation</strong>: Calls to <code>make()</code> must correctly
   * allocate memory that can be accessed from native or Java code and calls to
   * <code>delete()</code> must release that memory immediately.</li>
   * 
   * <li><strong>Collection</strong>: Objects no longer referenced
   * in Java should have their underlying native memory released in a timely
   * fashion.</li>
   * 
   * <li><strong>Low Memory</strong>: New allocation in low memory conditions
   * should first have the Java garbage collector release any old objects.</li>
   * 
   * </ol>
   * <h2>What is &quot;Speed&quot;?</h2>
   * <p>
   * Speed is how fast code executes under normal operating conditions. This is
   * more subjective than it sounds, as how do you define normal operation
   * conditions?  But in general, we define it as &quot;generally plenty of heap
   * space available&quot;
   * </p>
   * <h2>What does all of this mean?</h2> Well, it means if you're first writing
   * code, don't worry about this. If you're instead trying to optimize for
   * performance, first measure where your problems are, and if fingers are
   * pointing at allocation in Ferry (really -- it's unlikely) then start
   * switching first to
   * {@link MemoryModel#NATIVE_BUFFERS_WITH_JAVA_NOTIFICATION}. If that's not
   * good enough, try {@link MemoryModel#NATIVE_BUFFERS} but see all the caveats
   * there.
   * @author aclarke
   * 
   */
  public enum MemoryModel
  {
    /**
     * Large memory blocks are allocated in Java byte[] arrays, and passed back
     * into native code. <h2>Speed</h2>
     * <p>
     * This is the slowest model available. The main decrease in speed occurs
     * for medium-life-span objects. Short life-span objects (objects that die
     * during the life-span of an incremental collection) are very efficient.
     * Once an object makes it into the Tenured generation in Java, then it's
     * also efficient as unnecessary copying stops. However while in the Eden
     * generation but surviving between incremental collections, these objects
     * may get copied many times unnecessarily.  Since the objects are,
     * by definition, large, this unnecessary copy can have a significant
     * performance impact.
     * </p>
     * <h2>Robustness</h2>
     * <ol>
     * 
     * <li><strong>Allocation</strong>: Works as expected.</li>
     * 
     * <li><strong>Collection</strong>: Released either when
     * <code>delete()</code> is called, the item is marked for collection, or
     * we're in Low Memory conditions and the item is unused.</li>
     * 
     * <li><strong>Low Memory</strong>: Very strong. In this model Java always
     * knows exactly how much native heap space is being used, and can trigger
     * collections at the right time.</li>
     * 
     * </ol>
     * 
     * <h2>Tuning Tips</h2>
     * <p>
     * When using this model, these tips may increase performance, although in
     * some situations, may instead decrease. Always measure.
     * </p>
     * <ul>
     * <li>Ensure your objects are short-lived; null out references when done.</li>
     * <li>Avoid caching objects, but if you have to, try to ensure you don't
     * start your main work until several incremental collections have occurred
     * to give the object time to move to the tenured generation.</li>
     * <li>Try the {@link MemoryModel#NATIVE_BUFFERS_WITH_JAVA_NOTIFICATION}
     * model.</li>
     * <li>Call <code>delete()</code> when done with your objects
     * to let Java know it doesn't need to copy the item across
     * a collection.</li>
     * 
     * </ul>
     */
    JAVA_BYTE_ARRAYS(0),
    /**
     * Large memory blocks are allocated as Direct {@link ByteBuffer} objects
     * (as returned from {@link ByteBuffer#allocateDirect(int)}).
     * <p>
     * This model is not recommended. It is faster than
     * {@link MemoryModel#JAVA_BYTE_ARRAYS}, but because of how Sun implements
     * direct buffers, it works poorly in low memory conditions.
     * </p>
     * <h2>Speed</h2> This is the 2nd fastest model available. It is using Java
     * to allocate direct memory, so in theory Java always knows how much memory
     * it has allocated. </p> <h2>Robustness</h2>
     * <ol>
     * 
     * <li><strong>Allocation</strong>: Works as expected.</li>
     * 
     * <li><strong>Collection</strong>: Released either when
     * <code>delete()</code> is called, the item is marked for collection, or
     * we're in Low Memory conditions and the item is unused.</li>
     * 
     * <li><strong>Low Memory</strong>: Weak. In this model Java knows how much
     * memory it has allocated, but it does not use the size of the Direct Heap
     * to influence when it collects the normal Java Heap -- and our allocation
     * scheme depends on normal Java Heap collection. Therefore it can fail to
     * run collections in time, and may cause failures.</li>
     * 
     * </ol>
     * 
     * <h2>Tuning Tips</h2>
     * <p>
     * When using this model, these tips may increase performance, although in
     * some situations, may instead decrease. Always measure.
     * </p>
     * <ul>
     * <li>Increase the size of Sun's Java's direct buffer heap. Sun's Java
     * implementation has an artificially low default separate heap for direct
     * buffers (64mb). To make it higher pass
     * <code>-XX:MaxDirectMemorySize=<i>&lt;size&gt;</i></code> to your virtual
     * machine.</li>
     * <li>Call <code>delete()</code> when done with your objects
     * to let Java know it doesn't need to copy the item across
     * a collection.</li>
     * <li>Try the {@link MemoryModel#NATIVE_BUFFERS_WITH_JAVA_NOTIFICATION}
     * model.</li>
     * 
     * </ul>
     */
    JAVA_DIRECT_BUFFERS(1),
    /**
     * Large memory blocks are allocated directly in the native memory heap of
     * this machine, and Java is not notified of this underlying storage. <h2>
     * Speed</h2>
     * <p>
     * This is the fastest model available. It is using native code to allocate
     * direct memory, but Java has no way of knowing how much memory is
     * allocated behind the scenes.
     * </p>
     * <p>
     * Memory is still available for use from Java, and this will in fact be
     * faster to allocate than Direct {@link ByteBuffer} objects.
     * </p>
     * <p>
     * In this model, Java heap size will look low, but actual committed-pages
     * in OS memory will be much higher.
     * </p>
     * <h2>Robustness</h2>
     * <ol>
     * 
     * <li><strong>Allocation</strong>: Works as expected.</li>
     * 
     * <li><strong>Collection</strong>: Very weak. Because Java doesn't know how
     * much memory is allocated when using this model, it doesn't do incremental
     * collections as often as it would do for similarly sized java obejcts.
     * Hence objects may not be collected as often as you need, leading to
     * frequent {@link OutOfMemoryError} exceptions even though you have null
     * all your references. To work around this, call <code>delete()</code> on
     * Ferry objects yourself when done.
     * 
     * <li><strong>Low Memory</strong>: Very Weak. In this model Java has no
     * idea how much memory is being used behind the scenes, and therefore
     * doesn't clean up the heap as often as it might need to.</li>
     * 
     * </ol>
     * 
     * <h2>Tuning Tips</h2>
     * <p>
     * When using this model, these tips may increase performance, although in
     * some situations, may instead decrease. Always measure.
     * </p>
     * <ul>
     * <li>Paradoxically, try decreasing the size of your Java Heap if you get
     * {@link OutOfMemoryError} exceptions. Objects that are allocated in native
     * memory have a small proxy object representing them in the Java Heap. By
     * decreasing your heap size, those proxy objects will exert more collection
     * pressure, and hopefully cause Java to do incremental collections more
     * often (and notice your unused objects).</li>
     * <li>
     * Explicitly call <code>delete</code> on every Ferry object you have
     * finished with. This by-passes the Ferry collection schemes and
     * immediately frees the underlying memory.  If you need
     * to pass an object to another thread when you don't
     * know when to call <code>delete</code>, use <code>copyReference</code>
     * to give that thread it's own reference, pass it to the thread,
     * and then call <code>delete</code> on the first thread.</li>
     * 
     * </ul>
     */
    NATIVE_BUFFERS(2),
    /**
     * This is an experimental model that the folks at
     * xuggle.com are trying.  It should be almost as
     * fast as {@link #NATIVE_BUFFERS} but more Robust.
     * <p>
     * We're still experimenting with this (and hence it's not
     * the default), but if you're curious, give us your thoughts
     * by trying it out and seeing if you code gets any faster,
     * and if you get any fewer {@link OutOfMemoryError} if you
     * were previously using the {@link #NATIVE_BUFFERS} model.
     * </p>
     * <p>
     * Here's the approach; every time we need to allocate a new
     * large buffer in native code, we first try to allocate a
     * backing byte[] array in Java.  We then use native code to
     * do the actual allocation, and release the Java byte[] array.
     * The theory is in conditions where people are not trying
     * to be smarter than us and caching Xuggler objects, the attempt
     * to allocate the byte[] array will put sufficient collection
     * pressure on Java to have it do more collections for objects.
     * Simultaneously by only really allocating on the native heap
     * we avoid the unnecessary copy performance problems in the
     * {@link #JAVA_BYTE_ARRAYS} mode.
     * </p>
     * <p>We're still measuring though, so don't consider this
     * model part of the full API yet.</p>
     */
    NATIVE_BUFFERS_WITH_JAVA_NOTIFICATION(3);

    /**
     * The integer native mode that the JNIMemoryManager.cpp file expects
     */
    private final int mNativeValue;

    /**
     * Create a {@link MemoryModel}.
     * 
     * @param nativeValue What we actually use in native code.
     */
    private MemoryModel(int nativeValue)
    {
      mNativeValue = nativeValue;
    }

    /**
     * Get the native value to pass to native code
     * 
     * @return a value.
     */
    public int getNativeValue()
    {
      return mNativeValue;
    }
    
  }

}
