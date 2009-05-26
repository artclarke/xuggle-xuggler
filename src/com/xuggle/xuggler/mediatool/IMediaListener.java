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

package com.xuggle.xuggler.mediatool;

import java.awt.image.BufferedImage;

import com.xuggle.xuggler.IPacket;
import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IVideoPicture;

/** 
 * The IMediaListener receives events which occure in {@link
 * IMediaTool}, specifically {@link MediaReader} and {@link
 * MediaWriter}.
 *
 * <p>
 * 
 * These methods block the calling {@link IMediaTool} while they process so
 * try to return quickly.  If you have long running actions to perform, use
 * a separate thread.
 * 
 * </p>
 */

public interface IMediaListener
{
  /**
   * Called after a video frame has been decoded by a {@link
   * MediaReader} or encoded by a {@link MediaWriter}.  Optionally a
   * BufferedImage version of the frame may be passed.
   *
   * @param tool the tool that generated this event
   * @param picture a raw video picture
   * @param image the buffered image, which may be null if conversion was
   *   not asked for.
   * @param streamIndex the index of the stream this object was decoded from.
   */

  public void onVideoPicture(IMediaTool tool, IVideoPicture picture,
    BufferedImage image, int streamIndex);
  
  /**
   * Called after audio samples have been decoded by a {@link
   * MediaReader} or encoded by a {@link MediaWriter}.
   *
   * @param tool the tool that generated this event
   * @param samples a set of audio samples
   * @param streamIndex the index of the stream
   */

  public void onAudioSamples(IMediaTool tool, IAudioSamples samples, 
    int streamIndex);

  /**
   * Called after an {@link com.xuggle.xuggler.IContainer} is opened.
   *
   * @param tool the tool that generated this event
   */

  public void onOpen(IMediaTool tool);

  /**
   * Called after an {@link com.xuggle.xuggler.IContainer} is closed.
   * 
   * @param tool the tool that generated this event
   */

  public void onClose(IMediaTool tool);

  /**
   * Called after an {@link com.xuggle.xuggler.IStream} is added to an
   * {@link com.xuggle.xuggler.mediatool.IMediaTool}.  This occurs when
   * a new stream is added or encountered by the tool.  If the {@link
   * com.xuggle.xuggler.IStream} is not already been opened, then {@link
   * #onOpenStream} will be called at some later point.
   *
   * @param tool the tool that generated this event
   * @param stream the stream opened
   */

  public void onAddStream(IMediaTool tool, IStream stream);

  /**
   * Called after an {@link com.xuggle.xuggler.IStream} is opened.
   *
   * @param tool the tool that generated this event
   * @param stream the stream opened
   */

  public void onOpenStream(IMediaTool tool, IStream stream);

  /**
   * Called after an {@link com.xuggle.xuggler.IStream} is closed.
   *
   * @param tool the tool that generated this event
   * @param stream the stream just closed
   */

  public void onCloseStream(IMediaTool tool, IStream stream);

  /**
   * Called after a {@link com.xuggle.xuggler.IPacket} has been read by
   * a {@link MediaReader}.
   *
   * @param tool the tool that generated this event
   * @param packet the packet just read
   */

  public void onReadPacket(IMediaTool tool, IPacket packet);

  /**
   * Called after a {@link com.xuggle.xuggler.IPacket} has been written
   * by a {@link MediaWriter}.
   *
   * @param tool the tool that generated this event
   * @param packet the packet just written
   */

  public void onWritePacket(IMediaTool tool, IPacket packet);

  /**
   * Called after a {@link MediaWriter} writes the header.
   *
   * @param tool the tool that generated this event
   */

  public void onWriteHeader(IMediaTool tool);

  /**
   * Called after a {@link MediaWriter} has flushed its buffers.
   *
   * @param tool the tool that generated this event
   */

  public void onFlush(IMediaTool tool);

  /**
   * Called after a {@link MediaWriter} writes the trailer.
   *
   * @param tool the tool that generated this event
   */

  public void onWriteTrailer(IMediaTool tool);
}
