package com.xuggle.ferry;

import org.junit.Test;

public class JNIMemoryAllocatorTest
{

  /**
   * Tests: http://code.google.com/p/xuggle/issues/detail?id=163
   */
  @Test
  public void testRegression163()
  {
    JNIMemoryAllocator allocator = new JNIMemoryAllocator();
    byte[] mem = allocator.malloc(2000);
    allocator.free(mem);
  }
}
