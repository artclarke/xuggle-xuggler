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
  
  // We're disabling this test for now; Mac-OS appears to have a bug
  // that lets any old process keep allocating swap space and bring
  // a machine to its knees.  It works on Windows (which was the reason
  // to add the test) and Linux.
  if (memcheck || 1)
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
