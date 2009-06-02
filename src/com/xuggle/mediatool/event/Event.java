package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaGenerator;
import com.xuggle.mediatool.IMediaListener;

/**
 * Base class for all IMediaEvents that {@link IMediaListener}s
 * can listen for.
 * @author aclarke
 *
 */
public class Event
{
  private final IMediaGenerator mSource;
  public Event(IMediaGenerator source)
  {
    mSource = source;
  }
  /**
   * Get the source of this event.
   * @return the source
   */
  public IMediaGenerator getSource()
  {
    return mSource;
  }
  
}