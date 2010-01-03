/*******************************************************************************
 * Copyright (c) 2008, 2010 Xuggle Inc.  All rights reserved.
 *  
 * This file is part of Xuggle-Xuggler-Main.
 *
 * Xuggle-Xuggler-Main is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xuggle-Xuggler-Main is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Xuggle-Xuggler-Main.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/

package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaCoder;
import com.xuggle.xuggler.IPacket;

/**
 * An abstract implementation of {@link IPacketEvent}, but does
 * not declare {@link IPacketEvent}.
 * 
 * @author aclarke
 *
 */
public abstract class APacketMixin extends AStreamCoderMixin
{
  private final IPacket mPacket;
  
  /**
   * Create an {@link APacketMixin}.
   * @param source the source
   * @param packet a packet.  This event will <strong>not</strong>
   *   call {@link IPacket#copyReference()} on this packet.
   */
  public APacketMixin(IMediaCoder source, 
      IPacket packet)
  {
    super(source, packet == null ? null : packet.getStreamIndex());
    mPacket = packet;
  }
  /**
   * Implementation of {@link IPacketEvent#getPacket()}.
   * @see com.xuggle.mediatool.event.IPacketEvent#getPacket()
   */
  public IPacket getPacket()
  {
    return mPacket;
  }
  
}