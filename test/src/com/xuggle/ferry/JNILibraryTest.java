package com.xuggle.ferry;

import static org.junit.Assert.fail;

import org.junit.Test;

public class JNILibraryTest
{
  @Test
  public void testLoadSuccess()
  {
    final JNILibrary library = new JNILibrary("xuggle",
        new Long(com.xuggle.xuggler.Version.MAJOR_VERSION));
    JNILibrary.load("xuggle-xuggler", library);
  }
  @Test
  public void testLoadFail()
  {
    final JNILibrary library = new JNILibrary("xuggle-notavalidlibrary",
        new Long(com.xuggle.xuggler.Version.MAJOR_VERSION));
    try {
      JNILibrary.load("xuggle-xuggler", library);
      fail("library should not load");
    } catch (UnsatisfiedLinkError e) {
      // this is success; any other exception should bubble out and fail
    }
  }
  
}
