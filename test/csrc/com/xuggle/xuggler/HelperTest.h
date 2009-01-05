#ifndef __HELPER_TEST_H__
#define __HELPER_TEST_H__

#include <com/xuggle/testutils/TestUtils.h>
#include "Helper.h"
using namespace VS_CPP_NAMESPACE;

class HelperTest : public CxxTest::TestSuite
{
  public:
    void testCreationAndDestruction();
    void testSetupReading();
    void testSetupWriting();
    void testDecodeAndEncode();
    void testReadNextDecodedObject();
    void testWriteFrameAndWriteSamples();
  private:
};


#endif // __HELPER_TEST_H__

