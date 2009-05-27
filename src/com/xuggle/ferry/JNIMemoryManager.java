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
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.locks.ReentrantLock;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Manages the native memory that Ferry objects allocate and destroy.
 * <p>
 * Native memory isn't nicely garbage collected like Java memory is, and
 * managing it can be challenging to those not familiar with it. Ferry is all
 * about making native objects behave nicely in Java, and in order to have Ferry
 * make objects <i>look</i> like Java objects, the {@link JNIMemoryManager} does
 * some black magic to ensure native memory is released behind the scenes.
 * </p>
 * <p>
 * To do this by default Ferry uses a Robust mechanism for ensuring native
 * memory is released, but that comes at the expense of some Speed. This
 * approach is tunable though.
 * </p>
 * <p>
 * if you run a Java Profiler and see your application is spending a lot of time
 * copying on incremental collections, or you need to eke out a few more
 * microseconds of speed, or you're bored, then it's time to experiment with
 * different {@link MemoryModel} configurations that Ferry supports by calling
 * {@link #setMemoryModel(MemoryModel)}. This is pretty advanced stuff though,
 * so be warned.
 * </p>
 * <p>
 * Read {@link MemoryModel} for more.
 * </p>
 * 
 * @see MemoryModel
 * @author aclarke
 * 
 */
public class JNIMemoryManager
{
  /**
   * The different types of native memory allocation models Ferry supports. <h2>
   * Memory Model Performance Implications</h2> Choosing the {@link MemoryModel}
   * you use in Ferry libraries can have a big effect. Some models emphasize
   * code that will work "as you expect" (Robustness), but sacrifice some
   * execution speed to make that happen. Other models value speed first, and
   * assume you know what you're doing and can manage your own memory.
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
   * <td> {@link #JAVA_STANDARD_HEAP} (default)</td>
   * <td>+++++</td>
   * <td>+</td>
   * </tr>
   * 
   * <tr>
   * <td> {@link #JAVA_DIRECT_BUFFERS_WITH_STANDARD_HEAP_NOTIFICATION}</td>
   * <td>+++</td>
   * <td>++</td>
   * </tr>
   * 
   * <tr>
   * <td> {@link #NATIVE_BUFFERS_WITH_STANDARD_HEAP_NOTIFICATION}</td>
   * <td>+++</td>
   * <td>+++</td>
   * </tr>
   * 
   * <tr>
   * <td> {@link #JAVA_DIRECT_BUFFERS} (not recommended)</td>
   * <td>+</td>
   * <td>++++</td>
   * </tr>
   * 
   * <tr>
   * <td> {@link #NATIVE_BUFFERS}</td>
   * <td>+</td>
   * <td>+++++</td>
   * </tr>
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
   * <li><strong>Allocation</strong>: Calls to <code>make()</code> must
   * correctly allocate memory that can be accessed from native or Java code and
   * calls to <code>delete()</code> must release that memory immediately.</li>
   * 
   * <li><strong>Collection</strong>: Objects no longer referenced in Java
   * should have their underlying native memory released in a timely fashion.</li>
   * 
   * <li><strong>Low Memory</strong>: New allocation in low memory conditions
   * should first have the Java garbage collector release any old objects.</li>
   * 
   * </ol>
   * <h2>What is &quot;Speed&quot;?</h2>
   * <p>
   * Speed is how fast code executes under normal operating conditions. This is
   * more subjective than it sounds, as how do you define normal operation
   * conditions? But in general, we define it as &quot;generally plenty of heap
   * space available&quot;
   * </p>
   * 
   * <h2>How Does JNIMemoryManager Work?</h2>
   * <p>
   * Every object that is exposed from native code inherits from
   * {@link com.xuggle.ferry.RefCounted}.
   * </p>
   * <p>
   * Ferry works by implementing a reference-counted memory management scheme
   * in native code that is then manipulated from Java so you don't have to
   * (usually) think about when to release native memory. Every time an object
   * is created in native memory it has its reference count incremented by one;
   * and everywhere inside the code we take care to release a reference when
   * we're done.
   * </p>
   * <p>
   * This maps nicely to the Java model of memory management, but with the
   * benefit that Java does all the releasing behind the scenes. When you pass
   * an object from Native code to Java, Ferry makes sure it has a reference
   * count incremented, and then when the Java Virtual Machine collects the
   * instance, Ferry automatically decrements the reference it in native code.
   * </p>
   * <p>
   * In fact, in theory all you need to do is make a finalize() method on the
   * Java object that decrements the reference count in the native code and
   * everyone goes home happy.
   * </p>
   * <p>
   * So far so good, but it brings up a big problem:
   * <ul>
   * <li>
   * Turns out that video, audio and packets can be fairly large objects. For
   * example, a 640x480 YUV420P decoded video frame object will take up around
   * 500K of memory. If those are allocated from native code, Java has no idea
   * it got allocated; in fact the corresponding Java object will seem to only
   * take up a few bytes of memory. Keep enough video frames around, and your
   * Java process (that you expect to run in 64 Megs of heap memory) starts to
   * consume large amounts of native memory. Not good.</li>
   * <li>
   * The Java virtual machine only collects garbage when it thinks it needs the
   * space. However, because native code allocated the large chunks of memory,
   * Java doesn't know that memory is being used. So it doesn't collect unused
   * references, which if Ferry just used "finalize()" would mean that lots of
   * unused memory might exist that clog up your system.</li>
   * <li>
   * Lastly, even if Java does do a garbage collection, it must make sure that
   * all methods that have a finalize() method are first collected and put in a
   * "safe-area" that awaits a second collection. On the second collection call,
   * it starts calling finalize() on all those objects, but (don't ask why just
   * trust us) if needs to dedicate a separate finalizer thread to this process.
   * The result of this is if you allocate a lot of objects quickly, the
   * finalizer thread can start to fall very far behind.</li>
   * </ul>
   * Now, aren't you sorry you asked. Here's the good news; The
   * {@link com.xuggle.ferry.RefCounted} implementation solves all these
   * problems for you.
   * <p>
   * How you ask:
   * </p>
   * <ul>
   * <li>
   * We use Java Weak References to determine if a native object is no longer
   * used in Java. Ferry objects allocated from native code do not finalizers.
   * </li>
   * <li>
   * Then every-time you create a new Ferry object, we first make sure we do a
   * mini-collection of all unused Ferry objects and release that native
   * memory.</li>
   * <li>
   * Then, each Ferry object also maintains a reference to another object that
   * DOES have a finalize() method and the only thing that method does is make
   * sure another mini-collection is done. That way we can make sure memory is
   * freed even if you never do another Ferry allocation.</li>
   * <li>
   * Lastly, we make sure that whenever we need large chunks of memory (for
   * IPacket, IFrame and IAudioSamples interfaces) we can allocate those objects
   * from Java, so Java ALWAYS knows just how much memory it's using.</li>
   * </ul>
   * The end result: you usually don't need to worry. </p>
   * <p>
   * In the event you need to manage memory more expicitly, every Ferry object
   * has a "copyReference()" method that will create a new Java object that
   * points to the same underlying native object.
   * <p>
   * And In the unlikely event you want to control EXACTLY when a native object
   * is released, each Ferry object has a {@link RefCounted#delete()} method
   * that you can use. Once you call "delete()", you must ENSURE your object is
   * never referenced again from that Java object -- Ferry tries to help you
   * avoid crashes if you accidentally use an object after deletion but on this
   * but we cannot offer 100% protection (specifically if another thread is
   * accessing that object EXACTLY when you {@link RefCounted#delete()} it). If
   * you don't call {@link RefCounted#delete()}, we will call it at some point
   * in the future, but you can't depend on when (and depending on the
   * {@link MemoryModel} you are using, we may not be able to do it promptly).
   * </p>
   * <h2>What does all of this mean?</h2>
   * <p>
   * Well, it means if you're first writing code, don't worry about this. If
   * you're instead trying to optimize for performance, first measure where your
   * problems are, and if fingers are pointing at allocation in Ferry then start
   * trying different models.
   * </p>
   * <p>
   * But before you switch models, be sure to read the caveats and restrictions
   * on each of the non {@link #JAVA_STANDARD_HEAP} models, and make sure you
   * have a good understanding of how <a
   * href="http://java.sun.com/docs/hotspot/gc5.0/gc_tuning_5.html"> Java
   * Garbage Collection</a> works.
   * </p>
   * 
   * @author aclarke
   * 
   */
  public enum MemoryModel
  {
    /**
     * <p>
     * Large memory blocks are allocated in Java byte[] arrays, and passed back
     * into native code. Releasing of underlying native resources happens behind
     * the scenes with no management required on the programmer's part.
     * </p>
     * </p> <h2>Speed</h2>
     * <p>
     * This is the slowest model available.
     * </p>
     * <p>
     * The main decrease in speed occurs for medium-life-span objects. Short
     * life-span objects (objects that die during the life-span of an
     * incremental collection) are relatively efficient. Once an object makes it
     * into the Tenured generation in Java, then unnecessary copying stops until
     * the next full collection.
     * </p>
     * <p>
     * However while in the Eden generation but surviving between incremental
     * collections, large native buffers may get copied many times
     * unnecessarily. This copying can have a significant performance impact.
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
     * some situations, may instead decrease your performance. Always measure.
     * </p>
     * <ul>
     * <li>Try different garbage collectors in Java. To try the parallel
     * incremental collector, start your Java process with: these options:
     * 
     * <pre>
     * -XX:+UseParallelGC
     * </pre>
     * 
     * The concurrent garbage collector works well too. To use that pass these
     * options to java on startup:
     * 
     * <pre>
     * -XX:+UseConcMarkSweepGC -XX:+UseParNewGC
     * </pre>
     * 
     * </li>
     * <li>If you are not re-using objects across Ferry calls, ensure your
     * objects are short-lived; null out references when done.</li>
     * <li>Potentially try caching objects and reusing large objects across
     * multiple calls -- this may give those objects time to move into the
     * Tenured generation and reduce the copying overhead.</li>
     * <li>Explicitly manage Ferry memory yourself by calling
     * <code>delete()</code> on every {@link RefCounted} object when done with
     * your objects to let Java know it doesn't need to copy the item across a
     * collection. You can also use <code>copyReference()</code> to get a new
     * Java version of the same Ferry object that you can pass to another thread
     * if you don't know when <code>delete()</code> can be safely called.</li>
     * 
     * <li>Try a different {@link MemoryModel}.</li>
     * </ul>
     */
    JAVA_STANDARD_HEAP(0),

    /**
     * Large memory blocks are allocated as Direct {@link ByteBuffer} objects
     * (as returned from {@link ByteBuffer#allocateDirect(int)}).
     * <p>
     * This model is not recommended. It is faster than
     * {@link #JAVA_STANDARD_HEAP}, but because of how Sun implements direct
     * buffers, it works poorly in low memory conditions. This model has all the
     * caveats of the {@link #NATIVE_BUFFERS} model, but allocation is slightly
     * slower.
     * </p>
     * <h2>Speed</h2>
     * <p>
     * This is the 2nd fastest model available. In tests it is generally 20-30%
     * faster than the {@link #JAVA_STANDARD_HEAP} model.
     * </p>
     * 
     * </p>
     * <p>
     * It is using Java to allocate direct memory, which is slightly slower than
     * using {@link #NATIVE_BUFFERS}, but much faster than using the
     * {@link #JAVA_STANDARD_HEAP} model.
     * </p>
     * <p>
     * The downside is that for high-performance applications, you may need to
     * explicitly manage {@link RefCounted} object life-cycles with
     * {@link RefCounted#delete()} to ensure direct memory is released in a
     * timely manner.
     * </p>
     * <h2>Robustness</h2>
     * <ol>
     * 
     * <li><strong>Allocation</strong>: Weak. Java controls allocations of
     * direct memory from a separate heap (yet another one), and has an
     * additional tuning option to set that. By default on most JVMs, this heap
     * size is set to 64mb which is very low for video processing (queue up 100
     * images and see what we mean).</li>
     * 
     * <li><strong>Collection</strong>: Released either when
     * <code>delete()</code> is called, or when the item is marked for
     * collection</li>
     * 
     * <li><strong>Low Memory</strong>: Weak. In this model Java knows how much
     * <strong>direct</strong> memory it has allocated, but it does not use the
     * size of the Direct Heap to influence when it collects the normal
     * non-direct Java Heap -- and our allocation scheme depends on normal Java
     * Heap collection. Therefore it can fail to run collections in a timely
     * manner because it thinks the standard heap has plenty of space to grow.
     * This may cause failures.</li>
     * 
     * </ol>
     * 
     * <h2>Tuning Tips</h2>
     * <p>
     * When using this model, these tips may increase performance, although in
     * some situations, may instead decrease performance. Always measure.
     * </p>
     * <ul>
     * <li>Increase the size of Sun's Java's direct buffer heap. Sun's Java
     * implementation has an artificially low default separate heap for direct
     * buffers (64mb). To make it higher pass this option to Java at startup:
     * 
     * <pre>
     * -XX:MaxDirectMemorySize=&lt;i&gt;&lt;size&gt;&lt;/i&gt;
     * </pre>
     * 
     * </li>
     * <li>Paradoxically, try decreasing the size of your Java Heap if you get
     * {@link OutOfMemoryError} exceptions. Objects that are allocated in native
     * memory have a small proxy object representing them in the Java Heap. By
     * decreasing your heap size, those proxy objects will exert more collection
     * pressure, and hopefully cause Java to do incremental collections more
     * often (and notice your unused objects). To set the maximum size of your
     * java heap, pass this option to java on startup:
     * 
     * <pre>
     * -Xmx&lt;i&gt;&lt;size&gt;&lt;/i&gt;
     * </pre>
     * 
     * To change the minimum size of your java heap, pass this option to java on
     * startup:
     * 
     * <pre>
     * -Xms&lt;i&gt;&lt;size&gt;&lt;/i&gt;
     * </pre>
     * 
     * </li>
     * <li>
     * <li>Try different garbage collectors in Java. To try the parallel
     * incremental collector, start your Java process with: these options:
     * 
     * <pre>
     * -XX:+UseParallelGC
     * </pre>
     * 
     * The concurrent garbage collector works well too. To use that pass these
     * options to java on startup:
     * 
     * <pre>
     * -XX:+UseConcMarkSweepGC -XX:+UseParNewGC
     * </pre>
     * 
     * </li>
     * <li>If you are not re-using objects across Ferry calls, ensure your
     * objects are short-lived; null out references when done.</li>
     * <li>Potentially try caching objects and reusing large objects across
     * multiple calls -- this may give those objects time to move into the
     * Tenured generation and reduce the copying overhead.</li>
     * <li>Explicitly manage Ferry memory yourself by calling
     * <code>delete()</code> on every {@link RefCounted} object when done with
     * your objects to let Java know it doesn't need to copy the item across a
     * collection. You can also use <code>copyReference()</code> to get a new
     * Java version of the same Ferry object that you can pass to another thread
     * if you don't know when <code>delete()</code> can be safely called.</li>
     * 
     * <li>Try the
     * {@link MemoryModel#JAVA_DIRECT_BUFFERS_WITH_STANDARD_HEAP_NOTIFICATION}
     * model.</li>
     * 
     * </ul>
     */
    JAVA_DIRECT_BUFFERS(1),

    /**
     * Large memory blocks are allocated as Direct {@link ByteBuffer} objects
     * (as returned from {@link ByteBuffer#allocateDirect(int)}), but the Java
     * standard-heap is <i>informed</i> of the allocation by also attempting to
     * quickly allocate (and release) a buffer of the same size on the standard
     * heap..
     * <p>
     * This model can work well if your application is mostly single-threaded,
     * and your Ferry application is doing most of the memory allocation in your
     * program. The trick of <i>informing</i> Java will put pressure on the JVM
     * to collect appropriately, but by not keeping the references we avoid
     * unnecessary copying for objects that survive collections.
     * </p>
     * <p>
     * This heuristic is not failsafe though, and can still lead to collections
     * not occurring at the right time for some applications.
     * </p>
     * <p>
     * It is similar to the
     * {@link #NATIVE_BUFFERS_WITH_STANDARD_HEAP_NOTIFICATION} model and in
     * general we recommend that model over this one.
     * </p>
     * <h2>Speed</h2>
     * <p>
     * This model trades off some robustness for some speed. In tests it is
     * generally 10-20% faster than the {@link #JAVA_STANDARD_HEAP} model.
     * </p>
     * <p>
     * It is worth testing as a way of avoiding the explicit memory management
     * needed to effectively use the {@link #JAVA_DIRECT_BUFFERS} model.
     * However, the heuristic used is not fool-proof, and therefore may
     * sometimes lead to unnecessary collection or {@link OutOfMemoryError}
     * because Java didn't collect unused references in the standard heap in
     * time (and hence did not release underlying native references).
     * </p>
     * <h2>Robustness</h2>
     * <ol>
     * 
     * <li><strong>Allocation</strong>: Good. Java controls allocations of
     * direct memory from a separate heap (yet another one), and has an
     * additional tuning option to set that. By default on most JVMs, this heap
     * size is set to 64mb which is very low for video processing (queue up 100
     * images and see what we mean). With this option though we <i>inform</i>
     * Java of the allocation in the Direct heap, and this will often encourage
     * Java to collect memory on a more timely basis.</li>
     * 
     * <li><strong>Collection</strong>: Good. Released either when
     * <code>delete()</code> is called, or when the item is marked for
     * collection. Collections happen more frequently than under the
     * {@link #JAVA_DIRECT_BUFFERS} model due to <i>informing</i> the standard
     * heap at allocation time.</li>
     * 
     * <li><strong>Low Memory</strong>: Good. Especially for mostly
     * single-threaded applications, the collection pressure introduced on
     * allocation will lead to more timely collections to avoid
     * {@link OutOfMemoryError} errors on the Direct heap.</li>
     * </ol>
     * 
     * <h2>Tuning Tips</h2>
     * <p>
     * When using this model, these tips may increase performance, although in
     * some situations, may instead decrease performance. Always measure.
     * </p>
     * <ul>
     * <li>Increase the size of Sun's Java's direct buffer heap. Sun's Java
     * implementation has an artificially low default separate heap for direct
     * buffers (64mb). To make it higher pass this option to Java at startup:
     * 
     * <pre>
     * -XX:MaxDirectMemorySize=&lt;i&gt;&lt;size&gt;&lt;/i&gt;
     * </pre>
     * 
     * </li>
     * <li>Paradoxically, try decreasing the size of your Java Heap if you get
     * {@link OutOfMemoryError} exceptions. Objects that are allocated in native
     * memory have a small proxy object representing them in the Java Heap. By
     * decreasing your heap size, those proxy objects will exert more collection
     * pressure, and hopefully cause Java to do incremental collections more
     * often (and notice your unused objects). To set the maximum size of your
     * java heap, pass this option to java on startup:
     * 
     * <pre>
     * -Xmx&lt;i&gt;&lt;size&gt;&lt;/i&gt;
     * </pre>
     * 
     * To change the minimum size of your java heap, pass this option to java on
     * startup:
     * 
     * <pre>
     * -Xms&lt;i&gt;&lt;size&gt;&lt;/i&gt;
     * </pre>
     * 
     * </li>
     * <li>
     * <li>Try different garbage collectors in Java. To try the parallel
     * incremental collector, start your Java process with: these options:
     * 
     * <pre>
     * -XX:+UseParallelGC
     * </pre>
     * 
     * The concurrent garbage collector works well too. To use that pass these
     * options to java on startup:
     * 
     * <pre>
     * -XX:+UseConcMarkSweepGC -XX:+UseParNewGC
     * </pre>
     * 
     * </li>
     * <li>If you are not re-using objects across Ferry calls, ensure your
     * objects are short-lived; null out references when done.</li>
     * <li>Potentially try caching objects and reusing large objects across
     * multiple calls -- this may give those objects time to move into the
     * Tenured generation and reduce the copying overhead.</li>
     * <li>Explicitly manage Ferry memory yourself by calling
     * <code>delete()</code> on every {@link RefCounted} object when done with
     * your objects to let Java know it doesn't need to copy the item across a
     * collection. You can also use <code>copyReference()</code> to get a new
     * Java version of the same Ferry object that you can pass to another thread
     * if you don't know when <code>delete()</code> can be safely called.</li>
     * 
     * <li>Try the {@link #JAVA_STANDARD_HEAP} model.</li>
     * 
     * </ul>
     */
    JAVA_DIRECT_BUFFERS_WITH_STANDARD_HEAP_NOTIFICATION(2),

    /**
     * Large memory blocks are allocated in native memory, completely bypassing
     * the Java heap.
     * <p>
     * It is <strong>much</strong> faster than the {@link #JAVA_STANDARD_HEAP},
     * but much less robust.
     * </p>
     * <h2>Speed</h2>
     * <p>
     * This is the fastest model available. In tests it is generally 30-40%
     * faster than the {@link #JAVA_STANDARD_HEAP} model.
     * </p>
     * 
     * </p>
     * <p>
     * It is using the native operating system to allocate direct memory, which
     * is slightly faster than using {@link #JAVA_DIRECT_BUFFERS}, and much
     * faster than using the {@link #JAVA_STANDARD_HEAP} model.
     * </p>
     * <p>
     * The downside is that for high-performance applications, you may need to
     * explicitly manage {@link RefCounted} object life-cycles with
     * {@link RefCounted#delete()} to ensure native memory is released in a
     * timely manner.
     * </p>
     * <h2>Robustness</h2>
     * <ol>
     * 
     * <li><strong>Allocation</strong>: Weak. Allocations using
     * <code>make</code> and releasing objects with {@link RefCounted#delete()}
     * works like normal, but because Java has no idea of how much space is
     * actually allocated in native memory, it may not collect
     * {@link RefCounted} objects as quickly as you need it to (it will
     * eventually collect and free all references though).</li>
     * 
     * <li><strong>Collection</strong>: Released either when
     * <code>delete()</code> is called, or when the item is marked for
     * collection</li>
     * 
     * <li><strong>Low Memory</strong>: Weak. In this model Java has no idea how
     * much native memory is allocated, and therefore does not use that
     * knowledge in its determination of when to collect. This can lead to
     * {@link RefCounted} objects you created surviving longer than you want to,
     * and therefore not releasing native memory in a timely fashion.</li>
     * </ol>
     * 
     * <h2>Tuning Tips</h2>
     * <p>
     * When using this model, these tips may increase performance, although in
     * some situations, may instead decrease performance. Always measure.
     * </p>
     * <ul>
     * <li>Paradoxically, try decreasing the size of your Java Heap if you get
     * {@link OutOfMemoryError} exceptions. Objects that are allocated in native
     * memory have a small proxy object representing them in the Java Heap. By
     * decreasing your heap size, those proxy objects will exert more collection
     * pressure, and hopefully cause Java to do incremental collections more
     * often (and notice your unused objects). To set the maximum size of your
     * java heap, pass this option to java on startup:
     * 
     * <pre>
     * -Xmx&lt;i&gt;&lt;size&gt;&lt;/i&gt;
     * </pre>
     * 
     * To change the minimum size of your java heap, pass this option to java on
     * startup:
     * 
     * <pre>
     * -Xms&lt;i&gt;&lt;size&gt;&lt;/i&gt;
     * </pre>
     * 
     * </li>
     * <li>
     * <li>Try different garbage collectors in Java. To try the parallel
     * incremental collector, start your Java process with: these options:
     * 
     * <pre>
     * -XX:+UseParallelGC
     * </pre>
     * 
     * The concurrent garbage collector works well too. To use that pass these
     * options to java on startup:
     * 
     * <pre>
     * -XX:+UseConcMarkSweepGC -XX:+UseParNewGC
     * </pre>
     * 
     * </li>
     * <li>Use the {@link JNIMemoryManager#startCollectionThread()} method to
     * start up a thread dedicated to releasing objects as soon as they are
     * enqued in a {@link ReferenceQueue}, rather than (the default) waiting for
     * the next Ferry allocation or {@link JNIMemoryManager#collect()} explicit
     * call. Or periodically call {@link JNIMemoryManager#collect()} yourself.</li>
     * <li>Cache long lived objects and reuse them across calls to avoid
     * allocations.</li>
     * <li>Explicitly manage Ferry memory yourself by calling
     * <code>delete()</code> on every {@link RefCounted} object when done with
     * your objects to let Java know it doesn't need to copy the item across a
     * collection. You can also use <code>copyReference()</code> to get a new
     * Java version of the same Ferry object that you can pass to another thread
     * if you don't know when <code>delete()</code> can be safely called.</li>
     * 
     * <li>Try the
     * {@link MemoryModel#NATIVE_BUFFERS_WITH_STANDARD_HEAP_NOTIFICATION} model.
     * </li>
     * 
     * </ul>
     */
    NATIVE_BUFFERS(3),
    /**
     * Large memory blocks are allocated in native memory, completely bypassing
     * the Java heap, but Java is <i>informed</i> of the allocation by briefly
     * creating (and immediately releasing) a Java standard heap byte[] array of
     * the same size.
     * <p>
     * It is faster than the {@link #JAVA_STANDARD_HEAP}, but less robust.
     * </p>
     * <p>
     * This model can work well if your application is mostly single-threaded,
     * and your Ferry application is doing most of the memory allocation in your
     * program. The trick of informing Java will put pressure on the JVM to
     * collect appropriately, but by not keeping the references to the byte[]
     * array we temporarily allocate, we avoid unnecessary copying for objects
     * that survive collections.
     * </p>
     * <p>
     * This heuristic is not failsafe though, and can still lead to collections
     * not occurring at the right time for some applications.
     * </p>
     * <p>
     * It is similar to the
     * {@link #JAVA_DIRECT_BUFFERS_WITH_STANDARD_HEAP_NOTIFICATION} model.
     * </p>
     * <h2>Speed</h2>
     * <p>
     * In tests this model is generally 25-30% faster than the
     * {@link #JAVA_STANDARD_HEAP} model.
     * </p>
     * </p>
     * <p>
     * It is using the native operating system to allocate direct memory, which
     * is slightly faster than using
     * {@link #JAVA_DIRECT_BUFFERS_WITH_STANDARD_HEAP_NOTIFICATION}, and much
     * faster than using the {@link #JAVA_STANDARD_HEAP} model.
     * </p>
     * <p>
     * It is worth testing as a way of avoiding the explicit memory management
     * needed to effectively use the {@link #NATIVE_BUFFERS} model. However, the
     * heuristic used is not fool-proof, and therefore may sometimes lead to
     * unnecessary collection or {@link OutOfMemoryError} because Java didn't
     * collect unused references in the standard heap in time (and hence did not
     * release underlying native references).
     * </p>
     * <h2>Robustness</h2>
     * <ol>
     * 
     * <li><strong>Allocation</strong>: Good. With this option we allocate
     * large, long-lived memory from the native heap, but we <i>inform</i> Java
     * of the allocation in the Direct heap, and this will often encourage Java
     * to collect memory on a more timely basis.</li>
     * 
     * <li><strong>Collection</strong>: Good. Released either when
     * <code>delete()</code> is called, or when the item is marked for
     * collection. Collections happen more frequently than under the
     * {@link #NATIVE_BUFFERS} model due to <i>informing</i> the standard heap
     * at allocation time.</li>
     * 
     * <li><strong>Low Memory</strong>: Good. Especially for mostly
     * single-threaded applications, the collection pressure introduced on
     * allocation will lead to more timely collections to avoid
     * {@link OutOfMemoryError} errors on the native heap.</li>
     * </ol>
     * 
     * <h2>Tuning Tips</h2>
     * <p>
     * When using this model, these tips may increase performance, although in
     * some situations, may instead decrease performance. Always measure.
     * </p>
     * <ul>
     * <li>Paradoxically, try decreasing the size of your Java Heap if you get
     * {@link OutOfMemoryError} exceptions. Objects that are allocated in native
     * memory have a small proxy object representing them in the Java Heap. By
     * decreasing your heap size, those proxy objects will exert more collection
     * pressure, and hopefully cause Java to do incremental collections more
     * often (and notice your unused objects). To set the maximum size of your
     * java heap, pass this option to java on startup:
     * 
     * <pre>
     * -Xmx&lt;i&gt;&lt;size&gt;&lt;/i&gt;
     * </pre>
     * 
     * To change the minimum size of your java heap, pass this option to java on
     * startup:
     * 
     * <pre>
     * -Xms&lt;i&gt;&lt;size&gt;&lt;/i&gt;
     * </pre>
     * 
     * </li>
     * <li>
     * <li>Try different garbage collectors in Java. To try the parallel
     * incremental collector, start your Java process with: these options:
     * 
     * <pre>
     * -XX:+UseParallelGC
     * </pre>
     * 
     * The concurrent garbage collector works well too. To use that pass these
     * options to java on startup:
     * 
     * <pre>
     * -XX:+UseConcMarkSweepGC -XX:+UseParNewGC
     * </pre>
     * 
     * </li>
     * <li>Use the {@link JNIMemoryManager#startCollectionThread()} method to
     * start up a thread dedicated to releasing objects as soon as they are
     * enqued in a {@link ReferenceQueue}, rather than (the default) waiting for
     * the next Ferry allocation or {@link JNIMemoryManager#collect()} explicit
     * call. Or periodically call {@link JNIMemoryManager#collect()} yourself.</li>
     * <li>Cache long lived objects and reuse them across calls to avoid
     * allocations.</li>
     * <li>Explicitly manage Ferry memory yourself by calling
     * <code>delete()</code> on every {@link RefCounted} object when done with
     * your objects to let Java know it doesn't need to copy the item across a
     * collection. You can also use <code>copyReference()</code> to get a new
     * Java version of the same Ferry object that you can pass to another thread
     * if you don't know when <code>delete()</code> can be safely called.</li>
     * 
     * <li>Try the
     * {@link MemoryModel#NATIVE_BUFFERS_WITH_STANDARD_HEAP_NOTIFICATION} model.
     * </li>
     * 
     * </ul>
     */
    NATIVE_BUFFERS_WITH_STANDARD_HEAP_NOTIFICATION(4);

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

  /**
   * Our singleton (classloader while) manager.
   */
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

  /**
   * The reference queue that {@link RefCounted} objects will eventually find
   * their way to.
   */
  private final ReferenceQueue<Object> mRefQueue;

  /**
   * <p>
   * A thread-safe set of references to ferry and non-ferry based objects that
   * are referencing underlying Ferry native memory.
   * </p>
   * <p>
   * We have to enqueue all JNIReferences, otherwise we can't be guaranteed that
   * Java will enqueue them on a reference queue. We try to be smart and
   * efficient about this, but the fact remains that you have to pass this lock
   * for all Ferry calls.
   * </p>
   * <p>
   * Also, we don't use a ConcurrentHashMap here because the overhead of
   * allocating buckets in performance testing outweighed the competition for
   * the lock.
   * </p>
   */
  private final Set<JNIReference> mRefList;
  /**
   * The number of objects we think are awaiting collection; only an estimate.
   */
  private final AtomicLong mNumPinnedObjects;

  /**
   * A lock used for access to mRefList.
   */
  private final ReentrantLock mLock;

  private boolean addToBuffer(JNIReference ref)
  {
    boolean result = false;
    mLock.lock();
    try
    {
      mNumPinnedObjects.incrementAndGet();
      result = mRefList.add(ref);
    }
    finally
    {
      mLock.unlock();
    }
    return result;
  }

  private boolean removeFromBuffer(JNIReference ref)
  {
    boolean result;
    mLock.lock();
    try
    {
      mNumPinnedObjects.decrementAndGet();
      result = mRefList.remove(ref);
    }
    finally
    {
      mLock.unlock();
    }
    return result;
  }

  private void clearBuffer()
  {
    mLock.lock();
    try
    {
      mRefList.clear();
      mNumPinnedObjects.set(0);
    }
    finally
    {
      mLock.unlock();
    }
  }

  private void flushBuffer()
  {
    // Make a copy so we don't modify the real map
    // inside an iterator.
    Set<JNIReference> refs = new HashSet<JNIReference>();
    mLock.lock();
    try
    {
      refs.addAll(mRefList);
    }
    finally
    {
      mLock.unlock();
    }
    for (JNIReference ref : refs)
      if (ref != null)
        ref.delete();
  }

  /**
   * The collection thread if running.
   */
  private volatile Thread mCollectionThread;

  /**
   * The constructor is package level so others can't create it.
   */
  JNIMemoryManager()
  {
    mNumPinnedObjects = new AtomicLong(0);
    mRefQueue = new ReferenceQueue<Object>();
    mRefList = new HashSet<JNIReference>();
    mLock = new ReentrantLock();
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
    clearBuffer();

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
    return addToBuffer(ref);
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
    return removeFromBuffer(ref);
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
   * This thread is not started by default as Ferry calls {@link #gc()}
   * internally whenever a new Ferry object is allocated. But if you're
   * caching Ferry objects and hence avoiding new allocations, you may want to
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
    MemoryModel retval = MemoryModel.JAVA_STANDARD_HEAP;
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
   * Immediately frees all active objects in the system. Do not call unless you
   * REALLY know what you're doing.
   */
  public void flush()
  {
    flushBuffer();
  }

}
