package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaGenerator;

public interface IEvent
{

  /**
   * Get the source of this event.
   * @return the source
   */
  public abstract IMediaGenerator getSource();

}
