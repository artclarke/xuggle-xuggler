package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaGenerator;
import com.xuggle.mediatool.IMediaListener;

/**
 * Base class for all IMediaEvents that {@link IMediaListener}s
 * can listen for.
 * @author aclarke
 *
 */
public abstract class AEventMixin
{
  private final IMediaGenerator mSource;
  public AEventMixin(IMediaGenerator source)
  {
    mSource = source;
  }
  /* (non-Javadoc)
   * @see com.xuggle.mediatool.event.IEvent#getSource()
   */
  public IMediaGenerator getSource()
  {
    return mSource;
  }
  
}