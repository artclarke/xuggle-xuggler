#ifndef __PACKET_TEST_H__
#define __PACKET_TEST_H__

#include <com/xuggle/testutils/TestUtils.h>
#include "Helper.h"
using namespace VS_CPP_NAMESPACE;

class PacketTest : public CxxTest::TestSuite
{
  public:
    PacketTest();
    virtual ~PacketTest();
    void setUp();
    void tearDown();
    void testCreationAndDestruction();
    void testGetDefaults();
  private:
    RefPointer<IPacket> packet;
};


#endif // __PACKET_TEST_H__

