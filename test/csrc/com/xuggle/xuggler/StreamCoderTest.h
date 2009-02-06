#ifndef __STREAMCODER_TEST_H__
#define __STREAMCODER_TEST_H__

#include <com/xuggle/testutils/TestUtils.h>
#include "Helper.h"
using namespace VS_CPP_NAMESPACE;

class StreamCoderTest : public CxxTest::TestSuite
{
  public:
    StreamCoderTest();
    virtual ~StreamCoderTest();
    void setUp();
    void tearDown();
    void testGetters();
    void testSetCodec();
    void testSetProperty();
    void testGetProperty();
    void testOpenAndClose();
    void testOpenButNoClose();
    void testCloseButNoOpen();
    void testDecodingAndEncodingFullyInterleavedFile();
    void testTimestamps();
    void testGetFrameSize();
    void disabled_testDecodingAndEncodingNellymoserAudio();
  private:
    Helper* h;
    Helper* hw;

    RefPointer<IStreamCoder> coder;
};


#endif // __STREAMCODER_TEST_H__

