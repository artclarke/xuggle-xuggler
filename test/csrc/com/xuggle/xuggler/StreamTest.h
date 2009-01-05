#ifndef __STREAM_TEST_H__
#define __STREAM_TEST_H__

#include <com/xuggle/testutils/TestUtils.h>
#include "Helper.h"
using namespace VS_CPP_NAMESPACE;

class StreamTest : public CxxTest::TestSuite
{
  public:
    StreamTest();
    virtual ~StreamTest();
    void setUp();
    void tearDown();
    void testCreationAndDestruction();
  private:
    Helper* h;
};


#endif // __STREAM_TEST_H__

