package com.xuggle.ferry;

import org.junit.Test;

import com.xuggle.xuggler.Xuggler;

public class JNILibraryTest
{
  @Test
  public void testLoadFerry()
  {
    final JNILibrary library = new JNILibrary("xuggle",
        new Long(com.xuggle.xuggler.Version.MAJOR_VERSION), null);
    JNILibrary.load("xuggle-xuggler", library);
  }
  
  @Test
  public void testWithDependencies()
  {
    // this takes advantage of the fact that Xuggler is a bitch of dependencies.
    Xuggler.load();
  }
}
