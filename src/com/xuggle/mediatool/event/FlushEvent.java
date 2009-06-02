/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaCoder;

public class FlushEvent extends ACoderMixin implements IFlushEvent
{
  public FlushEvent(IMediaCoder source)
  {
    super(source);
  }
}