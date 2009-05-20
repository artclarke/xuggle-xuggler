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

/**
 * The media tools provide a simplified approach to decoding, viewing
 * and encoding media. The following code snippet is all that is
 * required to decode, view and encode a mdiea file.
 * thing.
 *
 * </p>
 * <pre>
 * MediaReader reader = new MediaReader(sourceUrl);
 * reader.addListener(new MediaViewer(true));
 * new MediaWriter(destinationUlr, reader);
 * while (reader.readPacket() == null)
 *   ;
 * </pre>
 * <p>
 * 
 * For a good example of using the approach see the {@link
 * com.xuggle.xuggler.mediatool.demos.DecodeAndPlayVideo} demonstration
 * application. In about 20 lines of Java code is opens up any media
 * file, finds all the video streams in that file, and displays them on
 * the screen.
 * 
 * </p>
 * <p>
 * 
 * For see the {@link com.xuggle.xuggler.mediatool.demos.DisplayWebcamVideo}
 * demonstration that displays a webcam on screen.
 * 
 * </p>
 * <p>
 *
 * Use a {@link com.xuggle.xuggler.mediatool.MediaReader} to read and
 * decode media.
 *
 * </p>
 * <p>
 *
 * Use a {@link com.xuggle.xuggler.mediatool.MediaWriter} to encode
 * media and write it out.
 *
 * </p>
 * <p>
 * 
 * A {@link com.xuggle.xuggler.mediatool.MediaWriter} can be added as a
 * {@link com.xuggle.xuggler.mediatool.IMediaListener} to a {@link
 * com.xuggle.xuggler.mediatool.MediaReader} to "close the loop" and
 * effectively transcode media.
 * 
 * </p>
 * <p>
 * 
 * To view the media a {@link com.xuggle.xuggler.mediatool.MediaViewer}
 * can be added as a listener to either a {@link
 * com.xuggle.xuggler.mediatool.MediaReader} or {@link
 * com.xuggle.xuggler.mediatool.MediaWriter}.
 * 
 * </p>
 * <p>
 * 
 * Various hooks are provided via the {@link
 * com.xuggle.xuggler.mediatool.IMediaListener} interface.
 * 
 * </p>
 * <p>
 * 
 * And the {@link com.xuggle.xuggler.mediatool.DebugListener} is an
 * implementation of {@link com.xuggle.xuggler.mediatool.IMediaListener}
 * that logs and counts media events so you can more easily debug what's
 * happening.
 * 
 * </p>
 */

package com.xuggle.xuggler.mediatool;

