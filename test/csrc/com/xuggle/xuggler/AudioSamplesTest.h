#ifndef __AUDIOSAMPLES_TEST_H__
#define __AUDIOSAMPLES_TEST_H__

#include <com/xuggle/testutils/TestUtils.h>
#include "Helper.h"
using namespace VS_CPP_NAMESPACE;

class AudioSamplesTest : public CxxTest::TestSuite
{
  public:
    AudioSamplesTest();
    virtual ~AudioSamplesTest();
    void setUp();
    void tearDown();
    void testCreationAndDestruction();
    void testDecodingToBuffer();
    void testEncodingToBuffer();
    void testSetSampleEdgeCases();
    void testSetSampleSunnyDayScenarios();
    void testGetSampleRainyDayScenarios();
  private:
    Helper* h;
    Helper* hw;
};


#endif // __AUDIOSAMPLES_TEST_H__

