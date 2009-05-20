/*
 * This file is part of Xuggler.
 * 
 * Xuggler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public
 * License along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
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
  int64_t position = packet->getPosition();
  VS_TUT_ENSURE("position was not set to -1", position == -1);

}

void
PacketTest :: testCopyPacket()
{
  packet = IPacket::make();
  VS_TUT_ENSURE("was able to allocate packet", packet);

  // everything else should be garbage.
  int64_t position = packet->getPosition();
  VS_TUT_ENSURE("position was not set to -1", position == -1);

  position = 4;
  packet->setPosition(position);
  int64_t dts = 28349762;
  packet->setDts(dts);
  int64_t pts = 82729373;
  packet->setPts(pts);
  RefPointer<IRational> timeBase = IRational::make(3,28972);
  packet->setTimeBase(timeBase.value());
  int32_t streamIndex = 8;
  packet->setStreamIndex(streamIndex);
  int64_t duration = 28387728;
  packet->setDuration(duration);
  int64_t convergenceDuration = 283;
  packet->setConvergenceDuration(convergenceDuration);
  
  // Now, make a copy
  RefPointer<IPacket> newPacket = IPacket::make(packet.value(), false);
  VS_TUT_ENSURE("should not be empty", newPacket);
  
  VS_TUT_ENSURE_EQUALS("should equal", position, newPacket->getPosition());
  VS_TUT_ENSURE_EQUALS("should equal", pts, newPacket->getPts());
  VS_TUT_ENSURE_EQUALS("should equal", dts, newPacket->getDts());
  VS_TUT_ENSURE_EQUALS("should equal", streamIndex, newPacket->getStreamIndex());
  VS_TUT_ENSURE_EQUALS("should equal", duration, newPacket->getDuration());
  VS_TUT_ENSURE_EQUALS("should equal", convergenceDuration,
      newPacket->getConvergenceDuration());
  RefPointer<IRational> newBase = newPacket->getTimeBase();
  VS_TUT_ENSURE("should be equal", newBase->compareTo(timeBase.value()) == 0);
}
