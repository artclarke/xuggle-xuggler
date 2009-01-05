#ifndef __CODEC_TEST_H__
#define __CODEC_TEST_H__

#include <com/xuggle/testutils/TestUtils.h>
#include "Helper.h"
using namespace VS_CPP_NAMESPACE;

class CodecTest : public CxxTest::TestSuite
{
  public:
    void setUp();
    void tearDown();
    void testCreationAndDescruction();
    void testInvalidArguments();
    void testFindByName();
    void testGuessEncodingCodecs();
  private:
    RefPointer<ICodec> codec;
};


#endif // __CODEC_TEST_H__

