/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaGenerator;

public abstract class AStreamMixin extends AEventMixin
{
  private final Integer mStreamIndex;

  public AStreamMixin(IMediaGenerator source, Integer streamIndex)
  {
    super(source);
    mStreamIndex = streamIndex;
  }

  /* (non-Javadoc)
   * @see com.xuggle.mediatool.event.IStreamEvent#getStreamIndex()
   */
  public Integer getStreamIndex()
  {
    return mStreamIndex;
  }
}