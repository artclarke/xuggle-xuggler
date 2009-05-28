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

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.atomic.AtomicLong;

import java.awt.image.BufferedImage;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IPacket;

import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IAudioSamples;

import com.xuggle.xuggler.mediatool.IMediaTool;
import com.xuggle.xuggler.mediatool.IMediaListener;

import static com.xuggle.xuggler.mediatool.MediaDebugListener.Event.*;
import static com.xuggle.xuggler.mediatool.MediaDebugListener.Mode.*;

/**
 * An {@link IMediaListener} that counts {@link IMediaTool}
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
 * A {@link MediaDebugListener} can be attached to multiple {@link IMediaTool}
 * simultaneously, even if they are on different threads, in which case
 * it will return aggregate counts.
 * </p>
 */

public class MediaDebugListener extends MediaAdapter implements IMediaListener
{
  final private Logger log = LoggerFactory.getLogger(this.getClass());
  
  // log modes

  /**
   * How much detail on each event you want to log.
   */
  public enum Mode {

    /** log no events */

    NOTHING, 

    /** log events without details */

    EVENT,

    /** log events with source or destination URL */

    URL, 

    /** log parameters passed to the event */

    PARAMETERS};

  // max event name length
  
  private static int mMaxNameLength = 0;
  
  /** The different type of events you'd like to print
   * data for.  */
  
  public enum Event
  {
    /** Video events */

    VIDEO         (0x001, "onVideoPicture"),

    /** Audio events */

    AUDIO       (0x002, "onAudioSamples"),

    /** Open events */

    OPEN        (0x004, "onOpen"),

    /** Close events */

    CLOSE       (0x008, "onClose"),

    /** Add stream events */

    ADD_STREAM  (0x010, "onAddStream"),

    /** Open stream events */

    OPEN_STREAM (0x020, "onOpenStream"),

    /** Close stream events */

    CLOSE_STREAM(0x040, "onCloseStream"),

    /** Read packet events */

    READ_PACKET (0x080, "onReadPacket"),

    /** Write packet events */

    WRITE_PACKET(0x100, "onWritePacket"),

    /** Write header events */

    HEADER      (0x200, "onWriteHeader"),

    /** Write trailer events */

    TRAILER     (0x400, "onWriteTrailer"),

    /** Flush events */

    FLUSH       (0x800, "onFlush"),

    /** All events */

    ALL         (0xfff, "<ALL-EVENT>"),

    /** No events */

    NONE        (0x000, "<NO-EVENTS>"),

    /**
     * {@link #VIDEO}, {@link #AUDIO}, {@link #READ_PACKET} and
     * {@link #WRITE_PACKET} events
     */

    DATA        (0x183, "<DATA-EVENTS>"),

    /**
     * All events except {@link #VIDEO}, {@link #AUDIO}, {@link
     * #READ_PACKET} and {@link #WRITE_PACKET} events
     */

    META_DATA   (0xfff & ~0x183, "<META-DATA-EVENTS>");

    // event flag

    private int mFlag;

    // name of called method

    private String mMethod;

    /**
     * Create an event.
     */
    Event(int flag, String method)
    {
      mFlag = flag;
      mMethod = method;
      updateMaxNameLength(name());
    }

    /**
     * Get the event flag.
     * @return the flag.
     */

    public int getFlag()
    {
      return mFlag;
    }

    /**
     * Get the {@link IMediaListener} event this event
     * will fire for.
     * @return The method.
     */
    public String getMethod()
    {
      return mMethod;
    }
  };

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

  public MediaDebugListener()
  {
    this(PARAMETERS, ALL);
  }

  /** 
   * Construct a debug listener with custom set of event types to log.
   *
   * @param events the event types which will be logged
   */
  
  public MediaDebugListener(Event... events)
  {
    this(PARAMETERS, events);
  }

  /** 
   * Construct a debug listener with custom set of event types to log.
   *
   * @param mode log mode, see {@link MediaDebugListener.Mode}
   * @param events the event types which will be logged
   */
  
  public MediaDebugListener(Mode mode, Event... events)
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

  private void handleEvent(Event event, IMediaTool tool, Object... args)
  {
    incrementCount(event);
    if ((mFlags & event.getFlag()) != 0 && mMode != NOTHING)
    {
      StringBuilder string = new StringBuilder();
      string.append(event.getMethod() + "(");
      switch (mMode)
      {
      case URL:
        string.append(tool.getUrl());
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

  public void onVideoPicture(IMediaTool tool, IVideoPicture picture, 
    BufferedImage image, int streamIndex)
  {
    handleEvent(VIDEO, tool, new Object[] {picture, image, streamIndex});
  }
  
  /** {@inheritDoc} */

  public void onAudioSamples(IMediaTool tool, IAudioSamples samples,
    int streamIndex)
  {
    handleEvent(AUDIO, tool, new Object[] {samples, streamIndex});
  }
  
  /** {@inheritDoc} */

  public void onOpen(IMediaTool tool)
  {
    handleEvent(OPEN, tool, new Object[] {});
  }
  
  /** {@inheritDoc} */

  public void onClose(IMediaTool tool)
  {
    handleEvent(CLOSE, tool, new Object[] {});
  }
  
  /** {@inheritDoc} */

  public void onAddStream(IMediaTool tool, IStream stream)
  {
    handleEvent(ADD_STREAM, tool, new Object[] {stream});
  }
  
  /** {@inheritDoc} */

  public void onOpenStream(IMediaTool tool, IStream stream)
  {
    handleEvent(OPEN_STREAM, tool, new Object[] {stream});
  }
  
  /** {@inheritDoc} */

  public void onCloseStream(IMediaTool tool, IStream stream)
  {
    handleEvent(CLOSE_STREAM, tool, new Object[] {stream});
  }
  
  /** {@inheritDoc} */

  public void onReadPacket(IMediaTool tool, IPacket packet)
  {
    handleEvent(READ_PACKET, tool, new Object[] {packet});
  }
  
  /** {@inheritDoc} */

  public void onWritePacket(IMediaTool tool, IPacket packet)
  {
    handleEvent(WRITE_PACKET, tool, new Object[] {packet});
  }

  /** {@inheritDoc} */

  public void onWriteHeader(IMediaTool tool)
  {
    handleEvent(HEADER, tool, new Object[] {});
  }
  
  /** {@inheritDoc} */

  public void onFlush(IMediaTool tool)
  {
    handleEvent(FLUSH, tool, new Object[] {});
  }

  /** {@inheritDoc} */

  public void onWriteTrailer(IMediaTool tool)
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

  // update max lane length
  
  private static void updateMaxNameLength(String name)
  {
    mMaxNameLength = Math.max(name.length(), mMaxNameLength);
  }
  
  /**
   * Get the maximum length of an event name.
   * @return the maximum length.
   */

  public static int getMaxNameLength()
  {
    return mMaxNameLength;
  }
}
