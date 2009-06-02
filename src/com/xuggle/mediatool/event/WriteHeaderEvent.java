/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaCoder;

public class WriteHeaderEvent extends CoderEvent
{
  public WriteHeaderEvent(IMediaCoder source)
  {
    super(source);
  }
}