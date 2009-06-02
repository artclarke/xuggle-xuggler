/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaCoder;

public class WriteTrailerEvent extends ACoderMixin implements IWriteTrailerEvent
{
  public WriteTrailerEvent(IMediaCoder source)
  {
    super(source);
  }
}