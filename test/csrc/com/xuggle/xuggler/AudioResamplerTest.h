#ifndef __AUDIORESAMPLER_TEST_H__
#define __AUDIORESAMPLER_TEST_H__

#include <com/xuggle/testutils/TestUtils.h>
#include "Helper.h"
using namespace VS_CPP_NAMESPACE;

class AudioResamplerTest : public CxxTest::TestSuite
{
  public:
    AudioResamplerTest();
    ~AudioResamplerTest();
    void setUp();
    void tearDown();
    void testGetters();
    void testInvalidArguments();
    void testResamplingAudio();
    void testDifferentResampleRates();
    void testTimeStampIsAdjustedWhenResamplerEatsBytesUpsampling();
  private:
    Helper* h;
    Helper* hw;
};


#endif // __AUDIORESAMPLER_TEST_H__

