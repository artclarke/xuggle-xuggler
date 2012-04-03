package com.xuggle.ferry;

import static org.junit.Assert.*;

import org.junit.Test;

public class JNIEnvTest
{

  @Test
  public void testGetEnv()
  {
    assertNotNull(JNIEnv.getEnv());
  }
  @Test
  public void testGetOSFamily()
  {
    assertNotSame(JNIEnv.OSFamily.UNKNOWN, JNIEnv.getEnv().getOSFamily());
    assertEquals(JNIEnv.OSFamily.UNKNOWN, JNIEnv.getOSFamily(null));
    assertEquals(JNIEnv.OSFamily.UNKNOWN, JNIEnv.getOSFamily(""));
    assertEquals(JNIEnv.OSFamily.LINUX, JNIEnv.getOSFamily("Linux"));
    assertEquals(JNIEnv.OSFamily.WINDOWS, JNIEnv.getOSFamily("Windows"));
    assertEquals(JNIEnv.OSFamily.MAC, JNIEnv.getOSFamily("Mac"));
  }
  @Test
  public void testGetCPUArch()
  {
    assertNotSame(JNIEnv.CPUArch.UNKNOWN, JNIEnv.getEnv().getCPUArch());
    assertEquals(JNIEnv.CPUArch.UNKNOWN, JNIEnv.getCPUArch(null));
    assertEquals(JNIEnv.CPUArch.UNKNOWN, JNIEnv.getCPUArch(""));
    assertEquals(JNIEnv.CPUArch.X86, JNIEnv.getCPUArch("i386"));
    assertEquals(JNIEnv.CPUArch.X86_64, JNIEnv.getCPUArch("amd64"));
    assertEquals(JNIEnv.CPUArch.PPC, JNIEnv.getCPUArch("PowerPC"));
    assertEquals(JNIEnv.CPUArch.PPC64, JNIEnv.getCPUArch("PowerPC64"));

  }
}
