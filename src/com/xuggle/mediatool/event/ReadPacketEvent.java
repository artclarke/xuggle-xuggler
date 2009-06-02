/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaCoder;
import com.xuggle.xuggler.IPacket;

public class ReadPacketEvent extends APacketMixin implements IReadPacketEvent
{
  public ReadPacketEvent(IMediaCoder source, IPacket packet)
  {
    super(source, packet);
  }
}