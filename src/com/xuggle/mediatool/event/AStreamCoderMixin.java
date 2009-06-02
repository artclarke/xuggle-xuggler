package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaCoder;

public abstract class AStreamCoderMixin extends AStreamMixin
{

  public AStreamCoderMixin(IMediaCoder source, Integer streamIndex)
  {
    super(source, streamIndex);
  }
  @Override
  public IMediaCoder getSource()
  {
    return (IMediaCoder) super.getSource();
  }

}
