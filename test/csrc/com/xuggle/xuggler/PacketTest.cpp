/*
 * Copyright (c) 2008 by Vlideshow Inc. (a.k.a. The Yard).  All rights reserved.
 *
 * It is REQUESTED BUT NOT REQUIRED if you use this library, that you let 
 * us know by sending e-mail to info@xuggle.com telling us briefly how you're
 * using the library and what you like or don't like about it.
 *
 * This library is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any later
 * version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include <com/xuggle/ferry/RefPointer.h>
#include <com/xuggle/xuggler/IPacket.h>
#include <com/xuggle/xuggler/Global.h>
#include "Helper.h"
#include "PacketTest.h"

using namespace VS_CPP_NAMESPACE;

PacketTest :: PacketTest()
{
}

PacketTest :: ~PacketTest()
{
  tearDown();
}

void
PacketTest :: setUp()
{
  packet = 0;
}

void
PacketTest :: tearDown()
{
  packet = 0;
}

void
PacketTest :: testCreationAndDestruction()
{
  packet = IPacket::make();
  VS_TUT_ENSURE("was able to allocate packet", packet);
}

void
PacketTest :: testGetDefaults()
{
  packet = IPacket::make();
  VS_TUT_ENSURE("was able to allocate packet", packet);

  // everything else should be garbage.
  int position = packet->getPosition();
  VS_TUT_ENSURE("position was not set to -1", position == -1);

}
