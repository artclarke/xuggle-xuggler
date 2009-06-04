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
 * <p>
 * A simple API for to decoding, viewing and encoding media -- Start with
 * {@link com.xuggle.mediatool.ToolFactory}.
 * </p>
 * 
 * <h2>Examples</h2>
 * 
 * <p>
 * The following code snippet is all that is required to decode a FLV file and
 * encode it as a Quicktime file.
 * </p>
 * 
 * <pre>
 * IMediaReader reader = ToolFactory.makeReader(&quot;input.flv&quot;);
 * reader.addListener(ToolFactory.makeWriter(&quot;output.mov&quot;, reader));
 * while (reader.readPacket() == null)
 *   ;
 * </pre>
 * 
 * <p>
 * And this code decodes a MPG file, and encodes it to flv, but this time plays
 * the media on screen in real-time while it's doing the work.
 * </p>
 * 
 * <pre>
 * IMediaReader reader = ToolFactory.makeReader(&quot;input.mpg&quot;);
 * reader.addListener(ToolFactory.makeViewer(true));
 * reader.addListener(ToolFactory.makeWriter(&quot;output.flv&quot;, reader));
 * while (reader.readPacket() == null)
 *   ;
 * </pre>
 * 
 * <p>
 * 
 * For more examples of using the mediatools see the
 * {@link com.xuggle.mediatool.demos} demonstration package.
 * </p>
 * <h2>Tutorials</h2>
 * <p>
 * Check out the <a
 * href="http://wiki.xuggle.com/MediaTool_Introduction">MediaTool Tutorial</a>
 * on our Wiki site.
 * </p>
 * 
 * <h2>How To Use</h2>
 * <p>
 * 
 * To create {@link com.xuggle.mediatool.IMediaReader},
 * {@link com.xuggle.mediatool.IMediaWriter},
 * {@link com.xuggle.mediatool.IMediaViewer} or
 * {@link com.xuggle.mediatool.IMediaDebugListener} objects, see the
 * {@link com.xuggle.mediatool.ToolFactory} class.
 * 
 * </p>
 * <p>
 * 
 * The {@link com.xuggle.mediatool.IMediaReader} and
 * {@link com.xuggle.mediatool.IMediaWriter} objects are the workhorses of this
 * package. They read and write to {@link com.xuggle.xuggler.IContainer}
 * objects, but hide the the complexity of encoding and decoding audio. Instead,
 * they generate events that they notify intertested
 * {@link com.xuggle.mediatool.IMediaListener} objects about. Interested
 * {@link com.xuggle.mediatool.IMediaListener} objects are registered through
 * the {@link com.xuggle.mediatool.IMediaGenerator} interface, which both
 * {@link com.xuggle.mediatool.IMediaReader} and
 * {@link com.xuggle.mediatool.IMediaWriter} extend.
 * 
 * </p>
 * <p>
 * 
 * {@link com.xuggle.mediatool.IMediaCoder} objects (which both
 * {@link com.xuggle.mediatool.IMediaReader} and
 * {@link com.xuggle.mediatool.IMediaWriter} are) will make intelligent guesses
 * about the parameters to decode and encode with based on the URLs or file
 * names you create the objects with, but you can change and override everything
 * if you want. To do that use the
 * {@link com.xuggle.mediatool.IMediaCoder#getContainer()} interface to get the
 * underlying {@link com.xuggle.xuggler.IContainer} object where they can then
 * query all other information. If your code is executing inside a
 * {@link com.xuggle.mediatool.IMediaListener} method, you can get the object
 * that generated that event by calling
 * {@link com.xuggle.mediatool.event.IEvent#getSource()} of an
 * {@link com.xuggle.mediatool.IMediaListener} event, and from there you can
 * query the {@link com.xuggle.xuggler.IContainer} if needed.
 * 
 * </p>
 * <p>
 * 
 * An {@link com.xuggle.mediatool.IMediaViewer} object is an
 * <strong>experimental</strong> interface that can be added to a
 * {@link com.xuggle.mediatool.IMediaGenerator} to display audio and video data
 * that the {@link com.xuggle.mediatool.IMediaReader} is generating in real
 * time. An {@link com.xuggle.mediatool.IMediaDebugListener} object can be
 * attached to {@link com.xuggle.mediatool.IMediaGenerator} objects and will log
 * the events they generate to a log file. See the <a
 * href="http://logback.qos.ch/">logback</a> logging project for information on
 * how to logback.
 * 
 * </p>
 * <p>
 * 
 * Lastly if you want to provide your own implementations of any of the
 * interfaces in this package, a series of Adaptors and Mixin classes are
 * provided.
 * 
 * </p>
 * <p>
 * 
 * Adapter classes are used when you want to implement one of the interfaces,
 * but want a lot of the implementation provided for you. For example,
 * {@link com.xuggle.mediatool.MediaListenerAdapter} provides an implementation
 * of {@link com.xuggle.mediatool.IMediaListener} with all methods implemented
 * as empty (no-op) methods. This means you can create your own
 * {@link com.xuggle.mediatool.IMediaListener} objects that only override some
 * methods.
 * 
 * </p>
 * <p>
 * 
 * Mixin classes are similar to Adapter classes, but do not declare the
 * interfaces they implement formally. In this way they can be included
 * in-sub-classes without forcing the sub-class to declare they implement a
 * method. For example, the {@link com.xuggle.mediatool.MediaToolMixin} class
 * can be useful to help implement {@link com.xuggle.mediatool.IMediaReader}
 * (and in fact, we use it for exactly that internally).
 * 
 * </p>
 * 
 * <h2>How To Make a Media Pipeline</h2>
 * 
 * <p>
 * Sometimes it can be useful to chain together a series of objects to filter
 * media and provide lots of effects. See the
 * {@link com.xuggle.mediatool.demos.ModifyAudioAndVideo} demo for an example of
 * that, but here's the basic structure of the code to make a pipeline:
 * </p>
 * 
 * <pre>
 * IMediaReader reader = ToolFactory.makeReader(inputFile.toString());
 * reader.setBufferedImageTypeToGenerate(BufferedImage.TYPE_3BYTE_BGR);
 * 
 * // create a writer and configure it's parameters from the reader
 * 
 * IMediaWriter writer = ToolFactory.makeWriter(outputFile.toString(), reader);
 * 
 * // create a tool which paints a time stamp onto the video
 * 
 * IMediaTool addTimeStamp = new TimeStampTool();
 * 
 * // create a tool which reduces audio volume to 1/10th original
 * 
 * IMediaTool reduceVolume = new VolumeAdjustTool(0.1);
 * 
 * // create a tool chain:
 * //   reader -&gt; addTimeStamp -&gt; reduceVolume -&gt; writer
 * 
 * reader.addListener(addTimeStamp);
 * addTimeStamp.addListener(reduceVolume);
 * reduceVolume.addListener(writer);
 * 
 * // add a viewer to the writer, to see the modified media
 * 
 * writer.addListener(ToolFactory.makeViewer(AUDIO_VIDEO));
 * 
 * // read and decode packets from the source file and
 * // then encode and write out data to the output file
 * 
 * while (reader.readPacket() == null)
 *   ;
 * </pre>
 * 
 * <h2>Package Use Conventions</h2>
 * 
 * <p>
 * 
 * When using this package you should be aware of the following code
 * conventions:
 * 
 * </p>
 * <ul>
 * <li>All interfaces begin with the letter &quot;I&quot;. For example:
 * {@link com.xuggle.mediatool.IMediaListener}.</li>
 * <li>All abstract classes begin with the latter &quot;A&quot;. For example:
 * {@link com.xuggle.mediatool.AMediaListenerMixin}.</li>
 * <li>Event interfaces and classes can be found in
 * {@link com.xuggle.mediatool.event} and end with &quot;Event;&quot;. For
 * example: {@link com.xuggle.mediatool.event.AddStreamEvent}.</li>
 * <li>Mixin classes will implement all methods suggested by their name, but
 * will not declare the corresponding interface. For example:
 * {@link com.xuggle.mediatool.AMediaListenerMixin}.</li>
 * <li>Adapter classes will provide an implementation of all methods suggested
 * by their name, and also will declare the corresponding interface. For
 * example: {@link com.xuggle.mediatool.MediaListenerAdapter}.</li>
 * </ul>
 */

package com.xuggle.mediatool;
