/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaCoder;

public class OpenEvent extends ACoderMixin implements IOpenEvent {

  public OpenEvent(IMediaCoder source)
  {
    super(source);
  }
}