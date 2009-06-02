/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaCoder;

public class AddStreamEvent extends AStreamCoderMixin implements IAddStreamEvent
{
  public AddStreamEvent(IMediaCoder source, Integer streamIndex)
  {
    super(source, streamIndex);
  }
}