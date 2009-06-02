/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaCoder;

public class CloseCoderEvent extends AStreamCoderMixin implements ICloseCoderEvent
{
  public CloseCoderEvent(IMediaCoder source, Integer streamIndex)
  {
    super(source, streamIndex);
  }
}