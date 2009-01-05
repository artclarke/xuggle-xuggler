#ifndef __TIMEVALUE_TEST_H__
#define __TIMEVALUE_TEST_H__

#include <com/xuggle/testutils/TestUtils.h>
#include <com/xuggle/ferry/RefPointer.h>
#include <com/xuggle/xuggler/ITimeValue.h>
using namespace com::xuggle::ferry;
using namespace VS_CPP_NAMESPACE;

class TimeValueTest : public CxxTest::TestSuite
{
  public:
    void setUp();
    void testCreationAndDestruction();
    void testGetViaDowngrade();
    void testGetViaUpgrade();
    void testCompareTo();
  private:
    RefPointer<ITimeValue> mObj;
};


#endif // __TIMEVALUE_TEST_H__

