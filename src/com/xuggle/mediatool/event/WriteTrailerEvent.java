/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaCoder;

public class WriteTrailerEvent extends CoderEvent
{
  public WriteTrailerEvent(IMediaCoder source)
  {
    super(source);
  }
}