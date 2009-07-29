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

/**
    An advanced API for reading, decoding, re-sampling, encoding and writing
    of most media files.
    <p>
    Xuggler is a library that wraps the
    <a href="http://ffmpeg.org/">FFMPEG</a> C library with a slightly simpler interface designed
    to be used safely from within Java (and eventually any garbage-collected.
    language).
    </p><p>
    <h2>Goals</h2>
    The library's goals are:
    <ul>
    <li>Ease of Use: Provide the power of
    <a href="http://ffmpeg.org/">FFMPEG</a> with an easier learning curve for Java developers.</li>
    <li>Safety: Run natively inside a Java Virtual
    Machine process, and minimize the odds of incorrect coding
    of native <a href="http://ffmpeg.org/">FFMPEG</a> code causing crashes</li>
    <li>Portability: Write portable code that should run most places
    where <a href="http://ffmpeg.org/">FFMPEG</a> can run</li>
    </ul>
    <h2>Features</h2>
    And so we set out to do that with this package.
    As a result you'll find the following major features:
        <table>
         <tr>
           <th scope="col">Goal</th>
           <th scope="col">Feature</th>
           <th scope="col">Description</th>
         </tr>
         <tr>
           <td>Ease of Use</td>
           <td>Simplied Interface</td>
           <td>Xuggler introduces an object-oriented interface
           that is based on the
           <a href="http://cekirdek.pardus.org.tr/~ismail/ffmpeg-docs/">FFMPEG interfaces</a>,
           but with more self-documenting methods and full
           documentation.</td>
           </tr>
         <tr>
           <td>&nbsp;</td>
           <td>Java Memory Management</td>
           <td>Xuggler lets you pass objects to and from native code,
           but removes
        the need for you to worry about allocating and freeing memory.
        Instead, we let the Java Virtual machine do that*.</td>
         </tr>
         <tr>
           <td>&nbsp;</td>
           <td>Allow <a href="http://ffmpeg.org/">FFMPEG</a> to read data from Java objects</td>
           <td>Through the <code>com.xuggle.xuggler.io</code> package you can extend
        Xuggler to read raw bytes to, and write raw bytes to, any Java object
        you want. That way you can directly integrate with anything, be in
        <a href="http://red5.googlecode.com/">Red5</a>, Wowza, Adobe FMS... you name it.</td>
         </tr>
         <tr>
           <td>Safety</td>
           <td>Crash Protection</td>
           <td>Our goal is to get to a stage where it is next to impossible
        for a developer using Xuggler to crash the JVM through incorrect use
        of <a href="http://ffmpeg.org/">FFMPEG</a> native code. To that end, we do lots of error checking of
        calls to make sure you don't accidentally do something
        to make <a href="http://ffmpeg.org/">FFMPEG</a> mad.</td>
         </tr>
         <tr>
           <td>&nbsp;</td>
           <td>Full Test Suite</td>
           <td>We developed Xuggler with a Test-Driven methodology, so in
        version one we come with over 200 different tests programs and
        hundreds more assertions.</td>
         </tr>
         <tr>
           <td>&nbsp;</td>
           <td>Memory Leak and Error Testing</td>
           <td>We also developed Xuggler with a full fledged memory leaking
        framework (sorry, runs on Linux only).</td>
         </tr>
         <tr>
           <td>&nbsp;</td>
           <td>Automated Building</td>
           <td>And to keep us honest, all of our tests are run on every
        checkin, and memory tests are done at least once per day.</td>
         </tr>
         <tr>
           <td>Portability</td>
           <td>Linux 32 and 64 bit support</td>
           <td>We're tested 32-bit but the code should work for 64-bit as well</td>
         </tr>
         <tr>
           <td>&nbsp;</td>
           <td>Mac OS X Support.</td>
           <td>If you're using 1.6 though
             you need to make sure you build a 64-bit version of
             Xuggler.  We've tested 1.6 with a 64-bit Xuggler</td>
         </tr>
         <tr>
           <td>&nbsp;</td>
           <td>Windows 32 and 64 bit support</td>
           <td>We're tested 32-bit but the code should work for 64-bit as well</td>
         </tr>
        </table>
    </p><p>
    <h2>Creating Xuggler Objects</h2>
    In general very few Xuggler Interfaces provide pure Java constructors (i.e. you can't create one using the
    Java "new" operator).  Instead, they will provide a "make" method
    that you can use  For example: {@link com.xuggle.xuggler.IContainer#make()}
    <h2>How To Learn More</h2>
    There's a lot to this library, and we hope to add more tutorials.  But if you're itching to get
    started, see the source code for {@link com.xuggle.xuggler.Converter} for an example program that reads
    files in one media format and converts to a new format, and the source code for
    {@link com.xuggle.xuggler.io.FileProtocolHandler} for an example of an example Java object
    that allows Xuggler (and FFMPEG) to read data to and from
    arbitrary data sources.
    </p>
    <p>
    Or, check out the contents of the <code>com.xuggle.xuggler.demos</code> package
    for some cool demos that show the Xuggler in action.
    </p>
    Or, if you have other questions, check out <a href="http://www.xuggle.com/">Xuggle</a>.  We'd love to hear from you.
    <h2>What if Xuggler Crashes the Virtual Machine?</h2>
    We try really hard to make it hard for you to crash the
    Java virtual machine using Xuggler, but it can happen.
    <p>
    If for some reason you are able to crash the JVM,
    we want to hear about it (even if it's your error).
    Send us mail: bugs (at) xuggler.com
    </p>
    <p>
      In general, when we have time to work on open source projects,
      fixing bugs that crash the JVM will take priority.
    </p>
    <h2>Java Memory Management</h2>
    Even though you are actually accessing native objects from Java
    code, you can use these objects just like you would Java objects.
    The garbage collector will clean up for you.
    <p>
    That said, we do advise you to use close() methods if provided
    (for example {@link com.xuggle.xuggler.IContainer#close()}).  These
     methods can free up resources (like file handles) for other
     parts of the system.
    </p>
    <p>
    In general, treat Xuggler objects like you would Hibernate
    data-base connections... use them, but close them when
    you don't need them.
    </p>
    <h2>Java Memory Management For Those Who Can't Mind Their Own Business</h2>
    This section is for people who can't just take our word for something, and instead start to poke around
    and notice that everything derives from {@link com.xuggle.ferry.RefCounted}.
    <p>For those people, let's just say that you should effectively ignore that, and just use the objects like
    you would any other Java object.
    </p>
    <p>
    Really.  We mean that.
    </p>
*/
package com.xuggle.xuggler;
