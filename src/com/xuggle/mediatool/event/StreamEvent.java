/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaGenerator;

public class StreamEvent extends Event
{
  private final Integer mStreamIndex;

  public StreamEvent(IMediaGenerator source, Integer streamIndex)
  {
    super(source);
    mStreamIndex = streamIndex;
  }

  /**
   * Get the stream index.
   * @return the stream index if known, or null if not
   */
  public Integer getStreamIndex()
  {
    return mStreamIndex;
  }
}