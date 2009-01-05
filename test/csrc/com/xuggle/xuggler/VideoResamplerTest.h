#ifndef __VIDEORESAMPLER_TEST_H__
#define __VIDEORESAMPLER_TEST_H__

#include <com/xuggle/testutils/TestUtils.h>
#include "Helper.h"
using namespace VS_CPP_NAMESPACE;

class VideoResamplerTest : public CxxTest::TestSuite
{
  public:
    VideoResamplerTest();
    virtual ~VideoResamplerTest();
    void setUp();
    void tearDown();
    void testGetter();
    void testInvalidConstructors();
    void testInvalidResamples();
    void testResampleAndOutput();
    void testSwitchPixelFormatsAndOutput();
    void testRescaleUpInYUV();
    void testRescaleDownInYUV();
  private:
    Helper* h;
    Helper* hw;
    RefPointer<IVideoResampler> mResampler;

    static void frameListener(void* context, IVideoPicture* pOutFrame, IVideoPicture* pInFrame);
};


#endif // __VIDEORESAMPLER_TEST_H__

