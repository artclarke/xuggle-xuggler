/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaCoder;
import com.xuggle.xuggler.IPacket;

public abstract class APacketMixin extends AStreamCoderMixin
{
  private final IPacket mPacket;
  public APacketMixin(IMediaCoder source, 
      IPacket packet)
  {
    super(source, packet == null ? null : packet.getStreamIndex());
    mPacket = packet;
  }
  /* (non-Javadoc)
   * @see com.xuggle.mediatool.event.IPacketEvent#getPacket()
   */
  public IPacket getPacket()
  {
    return mPacket;
  }
  
}