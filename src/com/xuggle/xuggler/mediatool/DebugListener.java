/*
 * Copyright (c) 2008-2009 by Xuggle Inc. All rights reserved.
 *
 * It is REQUESTED BUT NOT REQUIRED if you use this library, that you
 * let us know by sending e-mail to info@xuggle.com telling us briefly
 * how you're using the library and what you like or don't like about
 * it.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

package com.xuggle.xuggler.mediatool;

import java.awt.image.BufferedImage;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IPacket;

import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IAudioSamples;

import com.xuggle.xuggler.mediatool.IMediaTool;
import com.xuggle.xuggler.mediatool.IMediaListener;

/**
 * An {@link IMediaTool} listener which logs variouse events.
 */

public class DebugListener implements IMediaListener
{
  final private Logger log = LoggerFactory.getLogger(this.getClass());
  { log.trace("<init>"); }

  // flag values

  /** show video events */

  public static final int VIDEO         = 0x001;

  /** show audio events */

  public static final int AUDIO         = 0x002;

  /** show open events */

  public static final int OPEN          = 0x004;

  /** show close events */

  public static final int CLOSE         = 0x008;

  /** show open stream events */

  public static final int OPEN_STREAM   = 0x010;

  /** show close stream events */

  public static final int CLOSE_STREAM  = 0x020;

  /** show read packet events */

  public static final int READ_PACKET   = 0x040;

  /** show write packet events */

  public static final int WRITE_PACKET  = 0x080;

  /** show header write events */

  public static final int HEADER        = 0x100;

  /** show trailer write events */

  public static final int TRAILER       = 0x200;

  /** show flush events */

  public static final int FLUSH         = 0x400;

  /** show all events */

  public static final int ALL           = 0x7ff;

  /** show {@link #VIDEO}, {@link #AUDIO}, {@link #READ_PACKET} and
   * {@link #WRITE_PACKET} events */

  public static final int DATA          = VIDEO | AUDIO | READ_PACKET | WRITE_PACKET;

  /** show all events except {@link #VIDEO}, {@link #AUDIO}, {@link
   * #READ_PACKET} and {@link #WRITE_PACKET} events */

  public static final int META_DATA     = (ALL & ~DATA);

  // the flags

  private final int mFlags;
  
  /** 
   * Construct a debug listener with custom display flags to log
   * specified event types.
   *
   * @param flags display flags for different event types
   */

  public DebugListener(int flags)
  {
    mFlags = flags;
  }

  /** 
   * Construct a debug listener which logs all event types.
   */

  public DebugListener()
  {
    this(ALL);
  }

  /** {@inheritDoc} */

  public void onVideoPicture(IMediaTool tool, IVideoPicture picture, 
    BufferedImage image, int streamIndex)
  {
    if ((mFlags & VIDEO) != 0)
      log.debug("onVideoPicture({}, {}, " + image + ", " + streamIndex + ")",
        tool, picture);
  }
  
  /** {@inheritDoc} */

  public void onAudioSamples(IMediaTool tool, IAudioSamples samples,
    int streamIndex)
  {
    if ((mFlags & AUDIO) != 0)
      log.debug("onAudioSamples({}, {}, " + streamIndex + ")", tool, samples);
  }
  
  /** {@inheritDoc} */

  public void onOpen(IMediaTool tool)
  {
    if ((mFlags & OPEN) != 0)
      log.debug("onOpen({})", tool);
  }
  
  /** {@inheritDoc} */

  public void onClose(IMediaTool tool)
  {
    if ((mFlags & CLOSE) != 0)
      log.debug("onClose({})", tool);
  }
  
  /** {@inheritDoc} */

  public void onOpenStream(IMediaTool tool, IStream stream)
  {
    if ((mFlags & OPEN_STREAM) != 0)
      log.debug("onOpenStream({}, {})", tool, stream);
  }
  
  /** {@inheritDoc} */

  public void onCloseStream(IMediaTool tool, IStream stream)
  {
    if ((mFlags & CLOSE_STREAM) != 0)
      log.debug("onCloseStream({}, {})", tool, stream);
  }
  
  /** {@inheritDoc} */

  public void onReadPacket(IMediaTool tool, IPacket packet)
  {
    if ((mFlags & READ_PACKET) != 0)
      log.debug("onReadPacket({}, {})", tool, packet);
  }
  
  /** {@inheritDoc} */

  public void onWritePacket(IMediaTool tool, IPacket packet)
  {
    if ((mFlags & WRITE_PACKET) != 0)
      log.debug("onWritePacket({}, {})", tool, packet);
  }

  /** {@inheritDoc} */

  public void onWriteHeader(IMediaTool tool)
  {
    if ((mFlags & HEADER) != 0)
      log.debug("onWriteHeader({})", tool);
  }
  
  /** {@inheritDoc} */

  public void onFlush(IMediaTool tool)
  {
    if ((mFlags & FLUSH) != 0)
      log.debug("onFlush({})", tool);
  }

  /** {@inheritDoc} */

  public void onWriteTrailer(IMediaTool tool)
  {
    if ((mFlags & TRAILER) != 0)
      log.debug("onWriteTrailer({})", tool);
  }
}
