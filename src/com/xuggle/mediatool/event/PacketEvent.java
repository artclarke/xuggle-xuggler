/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaCoder;
import com.xuggle.xuggler.IPacket;

public class PacketEvent extends StreamEvent
{
  private final IPacket mPacket;
  public PacketEvent(IMediaCoder source, 
      IPacket packet)
  {
    super(source, packet == null ? null : packet.getStreamIndex());
    mPacket = packet;
  }
  public IPacket getPacket()
  {
    return mPacket;
  }
  
  @Override
  public IMediaCoder getSource()
  {
    return (IMediaCoder) super.getSource();
  }
}