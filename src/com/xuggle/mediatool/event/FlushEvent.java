/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaCoder;

public class FlushEvent extends CoderEvent
{
  public FlushEvent(IMediaCoder source)
  {
    super(source);
  }
}