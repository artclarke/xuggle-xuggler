#ifndef __LOGGER_TEST_H__
#define __LOGGER_TEST_H__

#include <com/xuggle/testutils/TestUtils.h>

class LoggerTestSuite : public CxxTest::TestSuite
{
  public:
  void testOutputToStandardError();
};


#endif // __LOGGER_TEST_H__

