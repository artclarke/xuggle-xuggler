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

package com.xuggle.mediatool;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.atomic.AtomicLong;


import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.mediatool.IMediaListener;
import com.xuggle.mediatool.IMediaGenerator;
import com.xuggle.xuggler.IPacket;

import com.xuggle.xuggler.IAudioSamples;


import static com.xuggle.mediatool.IMediaDebugListener.Event.*;
import static com.xuggle.mediatool.IMediaDebugListener.Mode.*;

/**
 * An {@link IMediaListener} that counts {@link IMediaGenerator}
 * events and optionally logs the events specified in {@link MediaDebugListener.Event}.
 * <p>
 * This object can be handy for debugging a media listener to see when
 * and where events are occurring.
 * </p>
 * <p>
 * Event counts can be queried, and {@link #toString} will
 * return an event count summary.  The details in the log can be
 * controlled by {@link MediaDebugListener.Mode}.
 * </p>
 * <p>
 * A {@link MediaDebugListener} can be attached to multiple {@link IMediaGenerator}
 * simultaneously, even if they are on different threads, in which case
 * it will return aggregate counts.
 * </p>
 */

class MediaDebugListener extends MediaListenerAdapter implements IMediaDebugListener
{
  final private Logger log = LoggerFactory.getLogger(this.getClass());
  // update max name length

  // max event name length
  private static final int mMaxNameLength;
  static {
    int maxNameLen = 0;
    for (Event event : Event.values())
      maxNameLen = Math.max(event.name().length(), maxNameLen);
    mMaxNameLength = maxNameLen;
  }
  
  /**
   * Get the maximum length of an event name.
   * @return the maximum length.
   */

  private static int getMaxNameLength()
  {
    return mMaxNameLength;
  }
  
  
  // log modes

  // the flags
  
  volatile private int mFlags;
  
  // log mode

  volatile private Mode mMode;

  // the event counts
  
  final private ConcurrentMap<Event, AtomicLong> mEventCounts = 
    new ConcurrentHashMap<Event, AtomicLong>();

  /** 
   * Construct a debug listener which logs all event types.
   */

  MediaDebugListener()
  {
    this(PARAMETERS, ALL);
  }

  /** 
   * Construct a debug listener with custom set of event types to log.
   *
   * @param events the event types which will be logged
   */
  
  MediaDebugListener(Event... events)
  {
    this(PARAMETERS, events);
  }

  /** 
   * Construct a debug listener with custom set of event types to log.
   *
   * @param mode log mode, see {@link MediaDebugListener.Mode}
   * @param events the event types which will be logged
   */
  
  MediaDebugListener(Mode mode, Event... events)
  {
    mMode = mode;
    setLogEvents(events);
  }

  // increment count for specific event type

  private void incrementCount(Event event)
  {
    for (Event candidate: Event.values())
      if ((candidate.getFlag() & event.getFlag()) != 0)
      {
        AtomicLong newValue = new AtomicLong(0);
        AtomicLong count = mEventCounts.putIfAbsent(candidate, newValue);
        if (count == null)
          count = newValue;
        count.incrementAndGet();
      }
  }

  /** 
   * Get the current count of events of a particular type.
   *
   * @param event the specified event type
   * 
   * @return the number of events of the specified type which have been
   *         transpired
   */

  public long getCount(Event event)
  {
    AtomicLong value = mEventCounts.get(event);
    return (null == value) ? 0 : value.get();
  }

  /** 
   * Reset all the event counts.
   */

  public void resetCounts()
  {
    mEventCounts.clear();
  }

  /** 
   * Set the event types which will be logged.
   *
   * @param events the events which will be logged
   */

  public void setLogEvents(Event... events)
  {
    mFlags = 0;
    for (Event event: events)
      mFlags |= event.getFlag();
  }

  /** 
   * Get the flags which specify which events will be logged.
   * 
   * @return the flags.
   */

  public int getFlags()
  {
    return mFlags;
  }

  // handle an event 

  private void handleEvent(Event event, IMediaGenerator tool, Object... args)
  {
    incrementCount(event);
    if ((mFlags & event.getFlag()) != 0 && mMode != NOTHING)
    {
      StringBuilder string = new StringBuilder();
      string.append(event.getMethod() + "(");
      switch (mMode)
      {
      case URL:
        if (tool instanceof IMediaCoder)
          string.append(((IMediaCoder)tool).getUrl());
        break;
      case PARAMETERS:
        for (Object arg: args)
          string.append(arg + (arg == args[args.length - 1] ? "" : ", "));
        break;
      }
      string.append(")");
      log.debug(string.toString());
    }
  }

  /** {@inheritDoc} */

  public void onVideoPicture(MediaVideoPictureEvent event)
  {
    handleEvent(VIDEO, event.getSource(), new Object[] {event.getVideoPicture(),
        event.getBufferedImage(),
        event.getStreamIndex()});
  }
  
  /** {@inheritDoc} */

  public void onAudioSamples(IMediaGenerator tool, IAudioSamples samples,
    int streamIndex)
  {
    handleEvent(AUDIO, tool, new Object[] {samples, streamIndex});
  }
  
  /** {@inheritDoc} */

  public void onOpen(IMediaGenerator tool)
  {
    handleEvent(OPEN, tool, new Object[] {});
  }
  
  /** {@inheritDoc} */

  public void onClose(IMediaGenerator tool)
  {
    handleEvent(CLOSE, tool, new Object[] {});
  }
  
  /** {@inheritDoc} */

  public void onAddStream(IMediaGenerator tool, int streamIndex)
  {
    handleEvent(ADD_STREAM, tool, new Object[] {streamIndex});
  }
  
  /** {@inheritDoc} */

  public void onOpenCoder(IMediaGenerator tool, Integer stream)
  {
    handleEvent(OPEN_STREAM, tool, new Object[] {stream});
  }
  
  /** {@inheritDoc} */

  public void onCloseCoder(IMediaGenerator tool, Integer stream)
  {
    handleEvent(CLOSE_STREAM, tool, new Object[] {stream});
  }
  
  /** {@inheritDoc} */

  public void onReadPacket(IMediaGenerator tool, IPacket packet)
  {
    handleEvent(READ_PACKET, tool, new Object[] {packet});
  }
  
  /** {@inheritDoc} */

  public void onWritePacket(IMediaGenerator tool, IPacket packet)
  {
    handleEvent(WRITE_PACKET, tool, new Object[] {packet});
  }

  /** {@inheritDoc} */

  public void onWriteHeader(IMediaGenerator tool)
  {
    handleEvent(HEADER, tool, new Object[] {});
  }
  
  /** {@inheritDoc} */

  public void onFlush(IMediaGenerator tool)
  {
    handleEvent(FLUSH, tool, new Object[] {});
  }

  /** {@inheritDoc} */

  public void onWriteTrailer(IMediaGenerator tool)
  {
    handleEvent(TRAILER, tool, new Object[] {});
  }

  /** {@inheritDoc}
   * Returns a string with event counts formatted nicely.
   * @return a formatted string.
   */

  public String toString()
  {
    StringBuilder sb = new StringBuilder();
    
    sb.append("event counts: ");
    for (Event event: Event.values())
      sb.append(String.format("\n  %" + getMaxNameLength() + "s: %d",
          event.name(), getCount(event)));

    return sb.toString();
  }

}
