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

package com.xuggle.xuggler.mediatool;

import java.util.Map;
import java.util.HashMap;

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
 * An {@link IMediaTool} listener which counts and logs {@link
 * IMediaListener} events to a log file.
 */

public class DebugListener implements IMediaListener
{
  final private Logger log = LoggerFactory.getLogger(this.getClass());

  // flag values

  /** show video events */

  public static final int VIDEO         = 0x001;

  /** show audio events */

  public static final int AUDIO         = 0x002;

  /** show open events */

  public static final int OPEN          = 0x004;

  /** show close events */

  public static final int CLOSE         = 0x008;

  /** show add stream events */

  public static final int ADD_STREAM    = 0x010;

  /** show open stream events */

  public static final int OPEN_STREAM   = 0x020;

  /** show close stream events */

  public static final int CLOSE_STREAM  = 0x040;

  /** show read packet events */

  public static final int READ_PACKET   = 0x080;

  /** show write packet events */

  public static final int WRITE_PACKET  = 0x100;

  /** show header write events */

  public static final int HEADER        = 0x200;

  /** show trailer write events */

  public static final int TRAILER       = 0x400;

  /** show flush events */

  public static final int FLUSH         = 0x800;

  /** show all events */

  public static final int ALL           = 0xfff;

  /** show no events */

  public static final int NONE          = 0x000;

  /** show {@link #VIDEO}, {@link #AUDIO}, {@link #READ_PACKET} and
   * {@link #WRITE_PACKET} events */

  public static final int DATA          = VIDEO | AUDIO | READ_PACKET | WRITE_PACKET;

  /** show all events except {@link #VIDEO}, {@link #AUDIO}, {@link
   * #READ_PACKET} and {@link #WRITE_PACKET} events */

  public static final int META_DATA     = (ALL & ~DATA);

  // the flags

  private int mFlags;
  
  // the event counts
  
  private Map<Integer, Integer> mEventCounts = new HashMap<Integer, Integer>();
  
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

  // increment count for specific event type

  private void incrementCount(int flag)
  {
    Integer value = mEventCounts.get(flag);

    if (null == value)
      value = 0;

    mEventCounts.put(flag, value + 1);
  }

  /** 
   * Get count of events of a particular type.
   *
   * @param flag the flag for the specified event type.
   * 
   * @return the number of events of the specified type which have been
   *         transpired
   */

  public int getCount(int flag)
  {
    Integer value = mEventCounts.get(flag);
    return (null == value) ? 0 : value;
  }

  /** 
   * Reset all the event counts.
   */

  public void resetCounts()
  {
    mEventCounts.clear();
  }

  /** 
   * Set the flags which specify which events will be logs.
   *
   * @param flags the flags
   * 
   * @return old flag values.
   */

  public int setFlags(int flags)
  {
    int oldFlags = mFlags;
    mFlags = flags;
    return oldFlags;
  }

  /** 
   * Get the flags which specify which events will be logs.
   * 
   * @return the flags.
   */

  public int getFlags()
  {
    return mFlags;
  }

  /** {@inheritDoc} */

  public void onVideoPicture(IMediaTool tool, IVideoPicture picture, 
    BufferedImage image, int streamIndex)
  {
    incrementCount(VIDEO);
    if ((mFlags & VIDEO) != 0)
      log.debug("onVideoPicture({}, {}, " + image + ", " + streamIndex + ")",
        tool, picture);
  }
  
  /** {@inheritDoc} */

  public void onAudioSamples(IMediaTool tool, IAudioSamples samples,
    int streamIndex)
  {
    incrementCount(AUDIO);
    if ((mFlags & AUDIO) != 0)
      log.debug("onAudioSamples({}, {}, " + streamIndex + ")", tool, samples);
  }
  
  /** {@inheritDoc} */

  public void onOpen(IMediaTool tool)
  {
    incrementCount(OPEN);
    if ((mFlags & OPEN) != 0)
      log.debug("onOpen({})", tool);
  }
  
  /** {@inheritDoc} */

  public void onClose(IMediaTool tool)
  {
    incrementCount(CLOSE);
    if ((mFlags & CLOSE) != 0)
      log.debug("onClose({})", tool);
  }
  
  /** {@inheritDoc} */

  public void onAddStream(IMediaTool tool, IStream stream)
  {
    incrementCount(ADD_STREAM);
    if ((mFlags & ADD_STREAM) != 0)
      log.debug("onAddStream({}, {})", tool, stream);
  }
  
  /** {@inheritDoc} */

  public void onOpenStream(IMediaTool tool, IStream stream)
  {
    incrementCount(OPEN_STREAM);
    if ((mFlags & OPEN_STREAM) != 0)
      log.debug("onOpenStream({}, {})", tool, stream);
  }
  
  /** {@inheritDoc} */

  public void onCloseStream(IMediaTool tool, IStream stream)
  {
    incrementCount(CLOSE_STREAM);
    if ((mFlags & CLOSE_STREAM) != 0)
      log.debug("onCloseStream({}, {})", tool, stream);
  }
  
  /** {@inheritDoc} */

  public void onReadPacket(IMediaTool tool, IPacket packet)
  {
    incrementCount(READ_PACKET);
    if ((mFlags & READ_PACKET) != 0)
      log.debug("onReadPacket({}, {})", tool, packet);
  }
  
  /** {@inheritDoc} */

  public void onWritePacket(IMediaTool tool, IPacket packet)
  {
    incrementCount(WRITE_PACKET);
    if ((mFlags & WRITE_PACKET) != 0)
      log.debug("onWritePacket({}, {})", tool, packet);
  }

  /** {@inheritDoc} */

  public void onWriteHeader(IMediaTool tool)
  {
    incrementCount(HEADER);
    if ((mFlags & HEADER) != 0)
      log.debug("onWriteHeader({})", tool);
  }
  
  /** {@inheritDoc} */

  public void onFlush(IMediaTool tool)
  {
    incrementCount(FLUSH);
    if ((mFlags & FLUSH) != 0)
      log.debug("onFlush({})", tool);
  }

  /** {@inheritDoc} */

  public void onWriteTrailer(IMediaTool tool)
  {
    incrementCount(TRAILER);
    if ((mFlags & TRAILER) != 0)
      log.debug("onWriteTrailer({})", tool);
  }
}
