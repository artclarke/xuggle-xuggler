/*
 * MemoryAllocationTest.h
 *
 *  Created on: May 8, 2009
 *      Author: aclarke
 */

#ifndef MEMORYALLOCATIONTEST_H_
#define MEMORYALLOCATIONTEST_H_

#include <com/xuggle/testutils/TestUtils.h>

class MemoryAllocationTest : public CxxTest::TestSuite
{
public:
  MemoryAllocationTest();
  virtual
  ~MemoryAllocationTest();
  
  void testCanCatchStdBadAlloc();
};

#endif /* MEMORYALLOCATIONTEST_H_ */
