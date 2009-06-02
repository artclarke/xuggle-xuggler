/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaCoder;

public abstract class ACoderMixin extends AEventMixin {
  public ACoderMixin(IMediaCoder source)
  {
    super(source);
  }
  @Override
  public IMediaCoder getSource()
  {
    return (IMediaCoder) super.getSource();
  }
}