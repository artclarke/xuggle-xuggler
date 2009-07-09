package com.xuggle.ferry;

import java.util.LinkedList;

public class MemoryTestHelper
{

  public static void forceJavaHeapWeakReferenceClear()
  {
    LinkedList<byte[]> leakedBytes = new LinkedList<byte[]>();
    try
    {
      while(true) {
        JNIMemoryManager.collect();
        leakedBytes.add(new byte[1024*1024]);
      }
    } catch (OutOfMemoryError e) {
    }
    leakedBytes.clear();
    // and try one more allocation
    byte[] lastAlloc = new byte[1024];
    lastAlloc[0] = 9;
    JNIMemoryManager.collect();
  }
}
