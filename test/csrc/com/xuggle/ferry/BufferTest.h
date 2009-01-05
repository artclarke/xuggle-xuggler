#ifndef __BUFFER_TEST_H__
#define __BUFFER_TEST_H__

#include <com/xuggle/testutils/TestUtils.h>
#include <com/xuggle/ferry/RefPointer.h>
#include <com/xuggle/ferry/IBuffer.h>
using namespace VS_CPP_NAMESPACE;

class BufferTest : public CxxTest::TestSuite
{
  public:
    void setUp();
    void tearDown();
    void testCreationAndDestruction();
    void testReadingAndWriting();
    void testWrapping();
  private:
    RefPointer<com::xuggle::ferry::IBuffer> buffer;
    static void freeBuffer(void *buf, void*closure)
    {
      delete [] (unsigned char*)buf;
      if (closure)
        *((bool*)closure) = true;
    }
};


#endif // __BUFFER_TEST_H__

