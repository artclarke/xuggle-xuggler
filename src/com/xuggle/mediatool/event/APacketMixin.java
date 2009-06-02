/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated.  All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any
 * later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

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