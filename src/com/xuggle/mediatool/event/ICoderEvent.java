package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaCoder;

public interface ICoderEvent extends IEvent
{
  public abstract IMediaCoder getSource();

}
