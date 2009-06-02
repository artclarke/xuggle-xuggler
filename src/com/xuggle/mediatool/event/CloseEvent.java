/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaCoder;

public class CloseEvent extends CoderEvent
{
  public CloseEvent(IMediaCoder source)
  {
    super(source);
  }
}