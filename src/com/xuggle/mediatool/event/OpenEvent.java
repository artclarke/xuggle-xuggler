/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaCoder;

public class OpenEvent extends CoderEvent {

  public OpenEvent(IMediaCoder source)
  {
    super(source);
  }
}