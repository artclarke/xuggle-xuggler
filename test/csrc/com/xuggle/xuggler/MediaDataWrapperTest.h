#ifndef __MEDIADATAWRAPPER_TEST_H__
#define __MEDIADATAWRAPPER_TEST_H__

#include <com/xuggle/testutils/TestUtils.h>
#include "Helper.h"
using namespace VS_CPP_NAMESPACE;

class MediaDataWrapperTest : public CxxTest::TestSuite
{
  public:
    void testCreationAndDestruction();
    void getDefaults();
    void testSetters();
    void testNullTimeBase();
    void testWrapping();
    void testUnwrapping();
  private:
};


#endif // __MEDIADATAWRAPPER_TEST_H__

