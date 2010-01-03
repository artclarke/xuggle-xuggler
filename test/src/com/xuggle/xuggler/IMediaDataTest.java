/*******************************************************************************
 * Copyright (c) 2008, 2010 Xuggle Inc.  All rights reserved.
 *  
 * This file is part of Xuggle-Xuggler-Main.
 *
 * Xuggle-Xuggler-Main is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xuggle-Xuggler-Main is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Xuggle-Xuggler-Main.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/

package com.xuggle.xuggler;

import org.junit.Test;

import java.nio.ByteBuffer;

import com.xuggle.ferry.IBuffer;
import com.xuggle.ferry.JNIMemoryManager;
import com.xuggle.xuggler.IAudioSamples;

import static junit.framework.Assert.*;

public class IMediaDataTest
{
  // test that getByteBuffer() returns well formed ByteBuffers,
  // specifically that the limit is properly set to the actual amount of
  // contained data

  @Test
  public void testGetByteBuffer()
  {
    int sampleCount = 1000;
    IAudioSamples mediaData = IAudioSamples.make(sampleCount, 1);
    mediaData.setComplete(true, sampleCount, 44000, 1, 
      IAudioSamples.Format.FMT_S16, 0);
    int byteCount = mediaData.getSize();

    ByteBuffer byteBuffer = mediaData.getByteBuffer();

    assertEquals("Position should be zero:", byteBuffer.position(), 0);
    assertEquals("Limit should be " + byteCount + ":", 
      byteBuffer.limit(), byteCount);
  }

  @Test
  public void testByteGetPut()
  {
    // free up any references from other tests
    JNIMemoryManager.getMgr().flush();
    byte[] in = new byte[]{ 0x38, 0x2C, 0x18, 0x7F };
    byte[] out = new byte[in.length];
    int sampleCount = 1000;
    IAudioSamples buf = IAudioSamples.make(sampleCount, 1);
    buf.setComplete(true, sampleCount, 44000, 1, 
      IAudioSamples.Format.FMT_S16, 0);
    buf.put(in, 0, 0, in.length);
    buf.get(0, out, 0, in.length);
    for(int i = 0; i < in.length; i++)
      assertEquals("mismatched bytes at " + i,
          in[i], out[i]);
    buf.delete();
    assertEquals("more objects around than expected",
        0, JNIMemoryManager.getMgr().getNumPinnedObjects());
  }

  @Test
  public void testShortGetPut()
  {
    // free up any references from other tests
    JNIMemoryManager.getMgr().flush();
    short[] in = new short[]{ 0x38, 0x2C, 0x18, 0x7F };
    short[] out = new short[in.length];
    int sampleCount = 1000;
    IAudioSamples buf = IAudioSamples.make(sampleCount, 1);
    buf.setComplete(true, sampleCount, 44000, 1, 
      IAudioSamples.Format.FMT_S16, 0);
    buf.put(in, 0, 0, in.length);
    buf.get(0, out, 0, in.length);
    for(int i = 0; i < in.length; i++)
      assertEquals("mismatched bytes at " + i,
          in[i], out[i]);
    buf.delete();
    assertEquals("more objects around than expected",
        0, JNIMemoryManager.getMgr().getNumPinnedObjects());
  }

  @Test
  public void testIntGetPut()
  {
    // free up any references from other tests
    JNIMemoryManager.getMgr().flush();
    int[] in = new int[]{ 0x38, 0x2C, 0x18, 0x7F };
    int[] out = new int[in.length];
    int sampleCount = 1000;
    IAudioSamples buf = IAudioSamples.make(sampleCount, 1);
    buf.setComplete(true, sampleCount, 44000, 1, 
      IAudioSamples.Format.FMT_S16, 0);
    buf.put(in, 0, 0, in.length);
    buf.get(0, out, 0, in.length);
    for(int i = 0; i < in.length; i++)
      assertEquals("mismatched bytes at " + i,
          in[i], out[i]);
    buf.delete();
    assertEquals("more objects around than expected",
        0, JNIMemoryManager.getMgr().getNumPinnedObjects());
  }

  @Test
  public void testCharGetPut()
  {
    // free up any references from other tests
    JNIMemoryManager.getMgr().flush();
    char[] in = new char[]{ 0x38, 0x2C, 0x18, 0x7F };
    char[] out = new char[in.length];
    int sampleCount = 1000;
    IAudioSamples buf = IAudioSamples.make(sampleCount, 1);
    buf.setComplete(true, sampleCount, 44000, 1, 
      IAudioSamples.Format.FMT_S16, 0);
    buf.put(in, 0, 0, in.length);
    buf.get(0, out, 0, in.length);
    for(int i = 0; i < in.length; i++)
      assertEquals("mismatched bytes at " + i,
          in[i], out[i]);
    buf.delete();
    assertEquals("more objects around than expected",
        0, JNIMemoryManager.getMgr().getNumPinnedObjects());
  }

  @Test
  public void testLongGetPut()
  {
    // free up any references from other tests
    JNIMemoryManager.getMgr().flush();
    long[] in = new long[]{ 0x38, 0x2C, 0x18, 0x7F };
    long[] out = new long[in.length];
    int sampleCount = 1000;
    IAudioSamples buf = IAudioSamples.make(sampleCount, 1);
    buf.setComplete(true, sampleCount, 44000, 1, 
      IAudioSamples.Format.FMT_S16, 0);
    buf.put(in, 0, 0, in.length);
    buf.get(0, out, 0, in.length);
    for(int i = 0; i < in.length; i++)
      assertEquals("mismatched bytes at " + i,
          in[i], out[i]);
    buf.delete();
    assertEquals("more objects around than expected",
        0, JNIMemoryManager.getMgr().getNumPinnedObjects());
  }

  @Test
  public void testDoubleGetPut()
  {
    // free up any references from other tests
    JNIMemoryManager.getMgr().flush();
    double[] in = new double[]{ 0x38, 0x2C, 0x18, 0x7F };
    double[] out = new double[in.length];
    int sampleCount = 1000;
    IAudioSamples buf = IAudioSamples.make(sampleCount, 1);
    buf.setComplete(true, sampleCount, 44000, 1, 
      IAudioSamples.Format.FMT_S16, 0);
    buf.put(in, 0, 0, in.length);
    buf.get(0, out, 0, in.length);
    for(int i = 0; i < in.length; i++)
      assertEquals("mismatched bytes at " + i,
          in[i], out[i]);
    buf.delete();
    assertEquals("more objects around than expected",
        0, JNIMemoryManager.getMgr().getNumPinnedObjects());
  }

  @Test
  public void testFloatGetPut()
  {
    // free up any references from other tests
    JNIMemoryManager.getMgr().flush();
    float[] in = new float[]{ 0x38, 0x2C, 0x18, 0x7F };
    float[] out = new float[in.length];
    int sampleCount = 1000;
    IAudioSamples buf = IAudioSamples.make(sampleCount, 1);
    buf.setComplete(true, sampleCount, 44000, 1, 
      IAudioSamples.Format.FMT_S16, 0);
    buf.put(in, 0, 0, in.length);
    buf.get(0, out, 0, in.length);
    for(int i = 0; i < in.length; i++)
      assertEquals("mismatched bytes at " + i,
          in[i], out[i]);
    buf.delete();
    assertEquals("more objects around than expected",
        0, JNIMemoryManager.getMgr().getNumPinnedObjects());
  }
  
  @Test
  public void testToStringDoesNotLeak()
  {
    JNIMemoryManager.getMgr().flush();
    assertEquals("more objects around than expected",
        0, JNIMemoryManager.getMgr().getNumPinnedObjects());
    IAudioSamples samples = IAudioSamples.make(1024, 2);
    samples.toString();
    IVideoPicture picture = Utils.getBlankFrame(100, 100, 0);
    picture.toString();
    IPacket packet = IPacket.make(1024);
    packet.toString();
    packet.getFormattedTimeStamp();
    IRational timeBase = IRational.make(1,25);
    packet.setTimeBase(timeBase);
    timeBase.delete();
    packet.getFormattedTimeStamp();
    packet.delete();
    samples.delete();
    picture.delete();
    assertEquals("more objects around than expected",
        0, JNIMemoryManager.getMgr().getNumPinnedObjects());
  }
  
  @Test
  public void testSetData()
  {
    JNIMemoryManager.getMgr().flush();
    assertEquals("more objects around than expected",
        0, JNIMemoryManager.getMgr().getNumPinnedObjects());
    int bufSize = 100;
    byte[] inData = new byte[bufSize];
    byte[] outData = new byte[inData.length];
    for(int i = 0; i < inData.length; i++)
      inData[i] = (byte) i;
    IBuffer buffer = IBuffer.make(null, inData, 0, inData.length);
    IAudioSamples samples = IAudioSamples.make(1024,2);
    samples.setData(buffer);
    
    IBuffer outBuffer = samples.getData();
    outBuffer.get(0, outData, 0, outData.length);
    for(int i = 0; i < inData.length; i++)
      assertEquals(inData[i], outData[i]);
    outBuffer.delete();
    
    samples.toString();
    IVideoPicture picture = IVideoPicture.make(IPixelFormat.Type.YUV420P,
        4,4);
    picture.setData(buffer);

    outBuffer = picture.getData();
    outBuffer.get(0, outData, 0, outData.length);
    for(int i = 0; i < inData.length; i++)
      assertEquals(inData[i], outData[i]);
    outBuffer.delete();
    
    IPacket packet = IPacket.make(1024);
    packet.setData(buffer);
    
    outBuffer = packet.getData();
    outBuffer.get(0, outData, 0, outData.length);
    for(int i = 0; i < inData.length; i++)
      assertEquals(inData[i], outData[i]);
    outBuffer.delete();
    
    
    packet.delete();
    samples.delete();
    picture.delete();
    buffer.delete();
    assertEquals("more objects around than expected",
        0, JNIMemoryManager.getMgr().getNumPinnedObjects());
  }
}
