#ifndef __VIDEOPICTURE_TEST_H__
#define __VIDEOPICTURE_TEST_H__

#include <com/xuggle/testutils/TestUtils.h>
#include "Helper.h"
using namespace VS_CPP_NAMESPACE;

class VideoPictureTest : public CxxTest::TestSuite
{
  public:
    VideoPictureTest();
    virtual ~VideoPictureTest();
    void setUp();
    void tearDown();
    void testCreationAndDestruction();
    void testDecodingIntoReusedFrame();
    void testDecodingAndEncodingIntoFrame();
    void testDecodingAndEncodingIntoFrameByCopyingData();
    void testDecodingAndEncodingIntoAFrameByCopyingDataInPlace();
    void testGetAndSetPts();
  private:
    Helper* hr; //reading helper
    Helper* hw; // writing helper
};


#endif // __VIDEOPICTURE_TEST_H__

