/*
 * StreamCoderX264Test.h
 *
 *  Created on: Feb 3, 2009
 *      Author: aclarke
 */

#ifndef STREAMCODERX264TEST_H_
#define STREAMCODERX264TEST_H_

#include <com/xuggle/testutils/TestUtils.h>
#include "Helper.h"
using namespace VS_CPP_NAMESPACE;

class StreamCoderX264Test : public CxxTest::TestSuite
{
public:
  StreamCoderX264Test();
  virtual ~StreamCoderX264Test();
  void setUp();
  void tearDown();

  // empty test
  void testSuccess();

  void testDecodingAndEncodingH264Video();

private:
  Helper* h;
  Helper* hw;

  RefPointer<IStreamCoder> coder;

};

#endif /* STREAMCODERX264TEST_H_ */
