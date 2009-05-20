/*
 * This file is part of Xuggler.
 * 
 * Xuggler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public
 * License along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
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
