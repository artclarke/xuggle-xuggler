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
 * <p>
 * A simple API for to decoding, viewing
 * and encoding media.
 * </p>
 * 
 * <h2>Sample Code</h2>
 * 
 * <p>
 * The following code snippet is all that is
 * required to decode a FLV file and encode it as a Quicktime file.
 * </p>
 * 
 * <pre>
 * MediaReader reader = new MediaReader("input.flv");
 * reader.addListener(new MediaWriter("output.mov", reader));
 * while (reader.readPacket() == null)
 *   ;
 * </pre>
 * 
 * <p>
 * And this code decodes a MPG file, and encodes it to flv, but this
 * time plays the media on screen in real-time while it's doing
 * the work.
 * </p>
 * 
 * <pre>
 * MediaReader reader = new MediaReader("input.mpg");
 * reader.addListener(new MediaViewer(true));
 * reader.addListener(new MediaWriter("output.flv", reader));
 * while (reader.readPacket() == null)
 *   ;
 * </pre>
 * 
 * <p>
 * {@link com.xuggle.mediatool.IMediaTool} objects will
 * make intelligent guesses about the parameters to decode and encode
 * with based on the file names, but you can change and override everything
 * if you want.
 * </p>
 * 
 * <h2>How Does It Work?</h2>
 * 
 * <h2>More Sample Code</h2>
 * <p>
 * 
 * For more examples of using the mediatools see the {@link
 * com.xuggle.mediatool.demos} demonstration
 * packet. */

package com.xuggle.mediatool;

