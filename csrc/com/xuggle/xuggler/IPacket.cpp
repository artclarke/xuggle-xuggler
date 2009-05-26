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

#include "IPacket.h"
#include "Global.h"
#include "Packet.h"

namespace com { namespace xuggle { namespace xuggler
{
  using namespace com::xuggle::ferry;
  
  IPacket :: IPacket()
  {
  }

  IPacket :: ~IPacket()
  {
  }

  IPacket*
  IPacket :: make()
  {
    Global::init();
    return Packet::make();
  }
  
  IPacket*
  IPacket :: make(IBuffer* buffer)
  {
    Global::init();
    if (!buffer)
      throw std::invalid_argument("no buffer");
    
    return Packet::make(buffer);
  }

  IPacket*
  IPacket :: make(IPacket* packet, bool copyData)
  {
    Global::init();
    return Packet::make(dynamic_cast<Packet*>(packet), copyData);
  }
  
  IPacket*
  IPacket :: make(int32_t size)
  {
    Global::init();
    IPacket* retval = make();
    if (retval) {
      if (retval->allocateNewPayload(size) < 0)
        VS_REF_RELEASE(retval);
    }
    return retval;
  }

}}}
