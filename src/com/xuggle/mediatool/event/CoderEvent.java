/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaCoder;

public class CoderEvent extends Event {
  public CoderEvent(IMediaCoder source)
  {
    super(source);
  }
  @Override
  public IMediaCoder getSource()
  {
    return (IMediaCoder) super.getSource();
  }
}