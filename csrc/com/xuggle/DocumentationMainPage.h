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

/*! \mainpage The Xuggler Open Source Libraries
 *
 * \section intro_sec Introduction
 *
 * This libraries contains a collection of C++ (and sometimes C) libraries that
 * Xuggle is letting loose on the world.  
 * <p>
 * Our intention in open sourcing these libraries is to break down the walls
 * of communication between people.  To that end, a lot of these libraries deal
 * with getting at raw data for different systems, and for helping people to share
 * and create cool stuff using that data.
 * </p>
 * <p>
 * That said, it is not our intent to encourage people to use these libraries for
 * illegal purposes, and we explicitly encourage people to respect the rights of others
 * when they use these libraries.
 * </p>
 * \section license_sec Licensing
 * All Xuggle Open Source Libraries are released under the
 * <a href="http://www.fsf.org/licensing/licenses/agpl-3.0.html"> GNU
 * Affero General Public License v3 (AGPL)</a>.
 * See the COPYING file in each library for details.  Non-AGPL licenses
 * are available for a fee.  Send e-mail to info@xuggle.com if interested.
 * \section libraries_sec Libraries
 * Here is a brief summary of the current libraries in this release:
 *
 * \subsection xuggler_lib Xuggle's Xuggler
 * A C++ Library for encoding and decoding pretty much any Internet available audio or video file.  See com::xuggle::xuggler.
 * <p>
 * This C++ library is a slightly simpler and safer C++ wrapper around the excellent <a href="http://ffmpeg.org">
 * FFMPEG</a> libav* libraries.  It also adds a reference-counted memory model so that ownership of
 * objects can be shared.
 * </p>
 * <p>
 * It's main raison d'etre is to be plugged into other languages (e.g. Java, Python, etc.)
 * and allow relatively safe encoding and decoding of video.   It ships today with a Java wrapper (using the
 * also excellent <a href="http://www.swig.org/">SWIG</a> system).
 * </p>
 * <p>
 * We don't expect a lot of people to use the C++ interface directly (instead we expect people to use the Java
 * interface), but we're not going to stop people either.  The installation scripts should install all the
 * header files you need, and the Java documentation is really just generated from the C++ documentation anyway,
 * so have fun.  Bear in mind that if you're #1 concern is performance, you should just use the libav interfaces
 * in C directly.  Why?  Because we slightly favor protecting the programmer from themselves and ease of use
 * over performance (the opposite of
 * what FFMPEG favors; they slightly favor performance over protection and ease of use).  What do we mean:
 * <ul>
 * <li>We copy data out of video frames upon a decode, and
 * data out of packets on an encode into our own structures
 * because FFMPEG will re-use the AVPacket and AVFrame buffers out from under you, and most other folks will be surprised if
 * it just disappears (particularly true if a Java object passes a com::xuggle::xuggler::IVideoPicture across
 * a thread boundary). 
 * </li>
 * <li>We do error checking of arguments, whereas FFMPEG assumes you know what you're doing.
 * </li>
 * <li>
 * When encoding audio we'll queue up enough data for a audio frame to be encoded, whereas FFMPEG requires you
 * to do that.  (note: if you care about this, always pass in eactly one audio frame's worth of data for your
 * given encoder and we won't do this).
 * </li>
 * </ul>
 * All these things take CPU time and some memory space.  In short, FFMPEG is a faster sharper knife,
 * where as Xuggler is a slower but safer knife.  Still, you can
 * make some real fancy dishes with either.
 * </p>
 *
 * \subsection xugglerio_lib Xuggle's Xuggler IO
 * A C++ Library for allowing people to plug in custom data sources for FFMPEG to use.  See com::xuggle::xuggler::io.
 * <p>
 * By default FFMPEG can read files, and some other protocols (like http).  Sometimes it can be useful to extend
 * that, and the Xuggler IO library allows that.  Again, it's main raison d'etre is to allow other languages (e.g.
 * Java) to extend the data sources that can feed FFMPEG.  See the Java FileProtocolHandler object for an example,
 * or download the xuggle-xuggler-red5 library to see an example where we use this to encode and deocde lvie
 * video in a <a href="red5.googlecode.com">Red5</a> media server.
 * </p>
 * \subsection ferry_lib Xuggle's Ferry
 * A set of C++ and other language (e.g. Java) classes for <i>ferrying</i> data from other languages to C/C++ and back.
 * See com::xuggle::ferry.
 * <p>
 * <a href="http://www.swig.org/">SWIG</a>, an excellent utility for wrapping C/C++ code for other languages
 * does a lot of great things.  But sometimes it can use some help, and that's where this library comes in.
 * This library implements the reference-counting memory scheme (used by the com::xuggle::xuggler package)
 * to allow C++ to pass objects in to other languages and relatively seemlessly integrate with their garbage collection
 * systems.  It also provides mechanism for allocating memory from the other language (instead of the C++ heap)
 * to help that language keep track of memory, mechanisms for logging in the other language, and mechanisms
 * for allowing the other language to directly modify C++/C memory (if possible).  Most people won't use this
 * outside of the Xuggler library, but if you're curious, go digging.  It relies HEAVILY on SWIG for a lot of the
 * heavy lifting.
 *
 * \section notes_sec Developer Notes
 *
 * \subsection threading_lib Thread Safety
 * The Xuggler library is not synchronized and callers must ensure multiple threads
 * are not using the same object at the same time.  For example, only one thread may
 * encodeAudio on an IStreamCoder instance at a time.
 * <p>
 * For C++ we do not by
 * default include any thread-locking library.  If internal library synchronization is
 * needed, we rely on the 
 * facilities of the runtime environment it's running in.  That means
 * in Java it uses the Java thread management libraries to ensure safety.
 * <p/>
 * However if you're just running raw C++, then we don't use any threading
 * library.  In general that's fine.  You need to follow these rules:
 *   1) Ensure that only one Xuggler method is working on a Xuggler object
 *      at a time.  So for example, if you're using IStreamCoder to
 *      decodeVideo into a IVideoPicture, make sure no other thread is
 *      using the IStreamCoder or the IVideoPicture at the time.
 *   2) Unfortunately there is one area where you must provide a global lock.
 *      You must make sure that IStreamCoder#open on any IStreamCoder object
 *      is locked (FFMPEG requires this).
 * <p/>
 * As mentioned earlier, we don't expect the C++ api to be used on its own,
 * but if this proves wrong, we will consider adding threading (e.g. pthreads
 * on Linux, w32threads on Windows) support if not running inside a 
 * Virtual Machine.  Let us know.
 *
 */
