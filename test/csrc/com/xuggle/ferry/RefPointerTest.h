#ifndef __REFPOINTER_TEST_H__
#define __REFPOINTER_TEST_H__

#include <com/xuggle/testutils/TestUtils.h>

class RefPointerTestSuite : public CxxTest::TestSuite
{
  public:
  void testCreateAndDestroy();
  void testPointerOperations();
  void testDefaultConstructor();
  void testRefPointerCopy();
};


#endif // __REFPOINTER_TEST_H__

