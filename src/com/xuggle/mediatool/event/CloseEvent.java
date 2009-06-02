/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaCoder;

public class CloseEvent extends ACoderMixin implements ICloseEvent
{
  public CloseEvent(IMediaCoder source)
  {
    super(source);
  }
}