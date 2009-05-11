/*
 * MemoryAllocationTest.cpp
 *
 *  Created on: May 8, 2009
 *      Author: aclarke
 */

#include "MemoryAllocationTest.h"
#include <stdexcept>
#include<cstdlib>

#include <list>

#include <cstdio>

MemoryAllocationTest :: MemoryAllocationTest()
{

}

MemoryAllocationTest :: ~MemoryAllocationTest()
{
}


void
MemoryAllocationTest :: testCanCatchStdBadAlloc()
{
  // This test deliberately leaks memory, so let's not
  // have valgrind worry about it.
  char *memcheck = getenv("VS_TEST_MEMCHECK");
  if (memcheck)
    return;

  const int CHUNK_SIZE_BYTES=10*1024*1024;
  std::list<int*> leakedChunks;
  
  int64_t bytesAllocated = 0;
  try {
    // deliberately allocate a lot of memory but hang
    // on to it
    while(true)
    {
      //fprintf(stderr, "allocated %lld bytes\n", bytesAllocated);
      leakedChunks.push_back(new int[CHUNK_SIZE_BYTES]);
      bytesAllocated+=CHUNK_SIZE_BYTES;
    }
    // fail if we get here.
    VS_TUT_ENSURE("we should have caused a std::bad_alloc", false);
  }
  catch (std::bad_alloc & e)
  {
//    fprintf(stderr, "Ran out of memory after %lld bytes\n", bytesAllocated);
    std::list<int*>::iterator iter = leakedChunks.begin();
    std::list<int*>::iterator end = leakedChunks.end();
    for(; iter != end; ++iter)
    {
      int* leak = *iter;
      delete [] leak;
    }
  }
}
