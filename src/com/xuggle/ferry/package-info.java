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

/**
 * A collection of classes that ferry objects from Java to native code and back.
 * <p>
 * This package contains a variety of internal-only classes used by Xuggler to
 * manage communication between Java and C++.
 * </p>
 * <p>
 * Classes and methods marked &quot;Internal Only&quot; are not meant for usage
 * by anyone outside these libraries. They are public members only because they
 * need to be for other Xuggler packages to use them -- don't go calling methods
 * on these objects from outside Xuggler as you can quickly bring down the Java
 * virtual machine if you don't know what you're doing.
 * </p>
 * <h2>Notes on Java and Native Memory Management</h2> Every object that is
 * exposed from native code inherits from {@link com.xuggle.ferry.RefCounted}.
 * In general as a Java user you can (and should) ignore this. But if you want
 * more details, here's what's going on. </p>
 * <p>
 * Xuggler works by implementing a reference-counted memory management scheme in
 * C++ that we then manipulate from Java so you don't have to. Every time an
 * object is created in C++ it has it's reference count incremented by one; and
 * everywhere inside the code we take care to release a reference when we're
 * done.
 * </p>
 * <p>
 * This maps nicely to the Java model of memory management, but with the benefit
 * that Java does all the releasing behind the scenes. So, when you pass an
 * object from Native code to Java, we make sure it has a reference count
 * incremented, and then when the Java Virtual Machine collects the instance, we
 * decrement the reference it in native code.
 * </p>
 * <p>
 * In fact, in theory all you need to do is make a finalize() method on the Java
 * object that decrements the reference count in the native code and everyone
 * goes home happy.
 * </p>
 * <p>
 * So far so good, but it brings up a big problem:
 * <ul>
 * <li>
 * Turns out that video, audio and packets can be fairly large objects. For
 * example, a 640x480 YUV420P decoded video frame object will take up around
 * 500K of memory. If those are allocated from C++ code, Java has no idea it got
 * allocated; in fact the corresponding Java object will seem to only take up a
 * few bytes of memory. Keep enough video frames around, and your Java process
 * (that you expect to run in 64 Megs of heap memory) starts to consume large
 * amounts of native memory. Not good.</li>
 * <li>
 * Now, The Java virtual machine only collects garbage when it thinks it needs
 * the space. However, because native code allocated the large chunks of memory,
 * Java doesn't know that memory is being used. So it doesn't collect unused
 * references, which if we just used "finalize()" would mean that lots of unused
 * memory might exist that clog up your system.</li>
 * <li>
 * Lastly, even if Java does do a garbage collection, it must make sure that all
 * methods that have a finalize() method are first collected and put in a
 * "safe-area" that awaits a second collection. On the second collection call,
 * it starts calling finalize() on all those objects, but (don't ask why just
 * trust us) if needs to dedicate a separate finalizer thread to this process.
 * The result of this is if you allocate a lot of objects quickly, the finalizer
 * thread can start to fall very far behind.</li>
 * </ul>
 * Now, aren't you sorry you asked. Here's the good news; The
 * {@link com.xuggle.ferry.RefCounted} implementation solves all these problems
 * for you.
 * <p>
 * How you ask:
 * </p>
 * <ul>
 * <li>
 * We use Java Phantom References to determine if a native object is no longer
 * used in Java. Xuggler objects allocated from native code do not finalizers.</li>
 * <li>
 * Then every-time you create a new Xuggler object, we first make sure we do a
 * mini-collection of all unused Xuggler objects and release that native memory.
 * </li>
 * <li>
 * Then, each Xuggler object also maintains a reference to another object that
 * DOES have a finalize() method and the only thing that method does is make
 * sure another mini-collection is done. That way we can make sure memory is
 * freed even if you never do another Xuggler allocation.</li>
 * <li>
 * Lastly, we make sure that whenever we need large chunks of memory (for
 * IPacket, IFrame and IAudioSamples interfaces) we allocate those objects from
 * Java, so Java ALWAYS knows just how much memory it's using.</li>
 * </ul>
 * The end result: you don't need to worry. </p>
 * <p>
 * In the unlikely event you need to "copy" a reference explicitly, every
 * Xuggler object has a "copyReference()" method that will create a new Java
 * object that points to the same underlying native object.
 * <p>
 * And In the unlikely event you want to control EXACTLY when a native object is
 * released, each Xuggler object has a "delete()" method that you can
 * use. Once you call "delete()", you must ENSURE your object is never
 * referenced again from that Java object -- Ferry tries to help you avoid
 * crashes if you accidentally use an object after deletion but on this but we
 * cannot offer 100% protection (specifically if another thread is accessing
 * that object EXACTLY when you "delete()" it).
 * </p>
 */
package com.xuggle.ferry;

