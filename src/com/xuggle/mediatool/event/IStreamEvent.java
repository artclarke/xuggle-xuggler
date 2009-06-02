package com.xuggle.mediatool.event;

public interface IStreamEvent extends IEvent
{

  /**
   * Get the stream index.
   * @return the stream index if known, or null if not
   */
  public abstract Integer getStreamIndex();

}
