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

import static com.xuggle.xuggler.mediatool.DebugListener.Event.*;
import static com.xuggle.xuggler.mediatool.DebugListener.Mode.*;

/**
 * An {@link IMediaTool} listener which counts {@link IMediaListener}
 * events and optinally logs specified {@link DebugListener.Event}
 * types.  Event counts can be queried, and {@link #toString} will
 * return an event count summery.  The detail in event log can ben
 * controlled by {@link DebugListener.Mode}.
 */

public class DebugListener implements IMediaListener
{
  final private Logger log = LoggerFactory.getLogger(this.getClass());
  
  // log modes

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
  
  /** The media listener event type specifiers. */
  
  public enum Event
  {
    // events
    
    /** video events */
    
    VIDEO         (0x001, "onVideoPicture"),
      
      /** audio events */
      
      AUDIO       (0x002, "onAudioSamples"),
      
      /** open events */
      
      OPEN        (0x004, "onOpen"),
      
      /** close events */
      
      CLOSE       (0x008, "onClose"),
      
      /** add stream events */
      
      ADD_STREAM  (0x010, "onAddStream"),
      
      /** open stream events */
      
      OPEN_STREAM (0x020, "onOpenStream"),
      
      /** close stream events */
      
      CLOSE_STREAM(0x040, "onCloseStream"),
      
      /** read packet events */
      
      READ_PACKET (0x080, "onReadPacket"),
      
      /** write packet events */
      
      WRITE_PACKET(0x100, "onWritePacket"),
      
      /** header write events */
      
      HEADER      (0x200, "onWriteHeader"),
      
      /** trailer write events */
      
      TRAILER     (0x400, "onWriteTrailer"),
      
      /** flush events */
      
      FLUSH       (0x800, "onFlush"),
      
      /** all events */
      
      ALL         (0xfff, "<ALL-EVENT>"),
      
      /** no events */
      
      NONE        (0x000, "<NO-EVENTS>"),
      
      /**
       * {@link #VIDEO}, {@link #AUDIO}, {@link #READ_PACKET} and
       * {@link #WRITE_PACKET} events
       */
      
      DATA        (0x183, "<DATA-EVENTS>"),
      
      /**
       * all events except {@link #VIDEO}, {@link #AUDIO}, {@link
       * #READ_PACKET} and {@link #WRITE_PACKET} events
       */
      
      META_DATA   (0xfff & ~0x183, "<META-DATA-EVENTS>");

    // event flag

    private int mFlag;

    // name of called method

    private String mMethod;

    // construct an event
    
    Event(int flag, String method)
    {
      mFlag = flag;
      mMethod = method;
      updateMaxNameLength(name());
    }

    // get event flag

    public int getFlag()
    {
      return mFlag;
    }

    // get event method

    public String getMethod()
    {
      return mMethod;
    }
  };

  // the flags
  
  private int mFlags;
  
  // log mode

  private Mode mMode;

  // the event counts
  
  private Map<Event, Integer> mEventCounts = new HashMap<Event, Integer>();

  /** 
   * Construct a debug listener which logs all event types.
   */

  public DebugListener()
  {
    this(PARAMETERS, ALL);
  }

  /** 
   * Construct a debug listener with custom set of event types to log.
   *
   * @param events the event types which will be logged
   */
  
  public DebugListener(Event... events)
  {
    this(PARAMETERS, events);
  }

  /** 
   * Construct a debug listener with custom set of event types to log.
   *
   * @param mode log mode, see {@link DebugListener.Mode}
   * @param events the event types which will be logged
   */
  
  public DebugListener(Mode mode, Event... events)
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
        Integer value = mEventCounts.get(candidate);

        if (null == value)
          value = 0;

        mEventCounts.put(candidate, value + 1);
      }
  }

  /** 
   * Get count of events of a particular type.
   *
   * @param event the specified event type
   * 
   * @return the number of events of the specified type which have been
   *         transpired
   */

  public int getCount(Event event)
  {
    Integer value = mEventCounts.get(event);
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
   * Set the event types which will be loggged.
   *
   * @param events the events wich will be logged
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

  private int getFlags()
  {
    return mFlags;
  }

  // handle an event 

  protected void handleEvent(Event event, IMediaTool tool, Object... args)
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

  /** {@inheritDoc} */

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
  
  // max event name length
  
  public static int getMaxNameLength()
  {
    return mMaxNameLength;
  }
}
