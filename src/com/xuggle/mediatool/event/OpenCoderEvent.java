/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaCoder;

public class OpenCoderEvent extends AStreamCoderMixin implements IOpenCoderEvent
{
  public OpenCoderEvent(IMediaCoder source, Integer streamIndex)
  {
    super(source, streamIndex);
  }
}