#ifndef __CONTAINERFORMAT_TEST_H__
#define __CONTAINERFORMAT_TEST_H__

#include <com/xuggle/testutils/TestUtils.h>
#include "Helper.h"
using namespace VS_CPP_NAMESPACE;

class ContainerFormatTest : public CxxTest::TestSuite
{
  public:
    ContainerFormatTest();
    virtual ~ContainerFormatTest();
    void setUp();
    void tearDown();
    void testCreationAndDestruction();
    void testSetInputFormat();
    void testSetOutputFormat();
  private:
    Helper* h;
    RefPointer<IContainerFormat> format;
};


#endif // __CONTAINERFORMAT_TEST_H__

