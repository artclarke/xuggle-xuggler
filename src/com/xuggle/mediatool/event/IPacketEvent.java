package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaCoder;
import com.xuggle.xuggler.IPacket;

public interface IPacketEvent extends IStreamEvent, ICoderEvent
{

  public abstract IPacket getPacket();

  public abstract IMediaCoder getSource();

}
