#ifndef __RATIONAL_TEST_H__
#define __RATIONAL_TEST_H__

#include <com/xuggle/testutils/TestUtils.h>
#include "Helper.h"
using namespace VS_CPP_NAMESPACE;

class RationalTest : public CxxTest::TestSuite
{
  public:
    void setUp();
    void testCreationAndDestruction();
    void testReduction();
    void testGetDouble();
    void testMultiplication();
    void testAddition();
    void testSubtraction();
    void testDivision();
    void testConstructionFromNumeratorAndDenominatorPair();
    void testRescaling();
  private:
    RefPointer<IRational> num;
};


#endif // __RATIONAL_TEST_H__

