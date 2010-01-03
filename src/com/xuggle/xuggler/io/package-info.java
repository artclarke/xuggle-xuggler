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

/**
 * An API for extending Xuggler to read and write from arbitrary data sources.
 * <p>
 * Xuggler needs to process raw media file data in order to decode and encode
 * those files. What's worse, is the only input Xuggler can give to FFMPEG is a
 * URL.
 * </p>
 * <p>
 * By default, Xuggler can read any "file:" URL. In fact if you don't specify a
 * protocol, we will assume "file:". But what happens if you want to read data
 * from an arbitrary source (like FMS, Red5, Wowza, etc)?
 * </p>
 * <p>
 * That's where this package comes in. Using the IO package you can implement a
 * {@link com.xuggle.xuggler.io.IURLProtocolHandler} that can respond to FFMPEG
 * requests to read or write buffers. From there any type of integration is
 * possible.
 * </p>
 * <br/>
 * Here's an architecture diagram for how it works: <img
 * src="xugglerio_architecture.png" alt="architecture diagram"/> <br/>
 * And here's a details call flow diagram showing how we move from Java to
 * native code to FFMPEG and back: <img src="xugglerio_callflow.png"
 * alt="call flow diagram"/> <br/>
 * <p>
 * To begin with this class, see
 * {@link com.xuggle.xuggler.io.URLProtocolManager} and take a look at the
 * example source code for {@link com.xuggle.xuggler.io.FileProtocolHandler} and
 * {@link com.xuggle.xuggler.io.FileProtocolHandlerFactory} classes.
 * </p>
 */
package com.xuggle.xuggler.io;

