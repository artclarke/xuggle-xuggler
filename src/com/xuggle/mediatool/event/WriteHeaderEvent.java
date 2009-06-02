/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaCoder;

public class WriteHeaderEvent extends ACoderMixin implements IWriteHeaderEvent
{
  public WriteHeaderEvent(IMediaCoder source)
  {
    super(source);
  }
}