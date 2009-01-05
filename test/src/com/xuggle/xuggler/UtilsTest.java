/*
 * Copyright (c) 2008-2009 by Xuggle Inc. All rights reserved.
 *
 * It is REQUESTED BUT NOT REQUIRED if you use this library, that you let 
 * us know by sending e-mail to info@xuggle.com telling us briefly how you're
 * using the library and what you like or don't like about it.
 *
 * This library is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any later
 * version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
package com.xuggle.xuggler;

import com.xuggle.ferry.IBuffer;
import com.xuggle.test_utils.NameAwareTestClassRunner;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IPixelFormat;
import com.xuggle.xuggler.ITimeValue;
import com.xuggle.xuggler.Utils;

import org.junit.*;
import org.junit.runner.RunWith;


import static junit.framework.Assert.*;

@RunWith(NameAwareTestClassRunner.class)
public class UtilsTest
{
  @Test
  public void testGetBlankFrame()
  {
    IVideoPicture frame = null;
    byte y = 62;
    byte u = 15;
    byte v = 33;
    int w = 100;
    int h = 200;
    long pts = 102832;
    
    frame = Utils.getBlankFrame(w, h, y, u, v, pts);
    assertTrue("got a frame", frame != null);
    assertTrue("is complete", frame.isComplete());
    assertTrue("correct pts", frame.getPts() == pts);

    // now, loop through the array and ensure it is set
    // correctly.
    IBuffer data = frame.getData();
    java.nio.ByteBuffer buffer = data.getByteBuffer(0, data.getBufferSize());
    assertNotNull(buffer);
    // we have the raw data; now we set it to the specified YUV value.
    int lineLength = 0;
    int offset = 0;

    // first let's check the L
    offset = 0;
    lineLength = frame.getDataLineSize(0);
    assertTrue("incorrect format", lineLength == w);
    for(int i = offset ; i < offset + lineLength * h; i++)
    {
      byte val = buffer.get(i);
      assertTrue("color not set correctly: " + i, val == y);
    }

    // now, check the U value
    offset = (frame.getDataLineSize(0)*h);
    lineLength = frame.getDataLineSize(1);
    assertTrue("incorrect format", lineLength == w / 2);
    for(int i = offset ; i < offset + lineLength * h / 2; i++)
    {
      byte val = buffer.get(i);
      assertTrue("color not set correctly: " + i, val == u);
    }

    // and finally the V
    offset = (frame.getDataLineSize(0)*h) + (frame.getDataLineSize(1)*h/2);
    lineLength = frame.getDataLineSize(2);
    assertTrue("incorrect format", lineLength == w / 2);
    for(int i = offset; i < offset + lineLength * h / 2; i++)
    {
      byte val = buffer.get(i);
      assertTrue("color not set correctly: " + i, val == v);
    }
    
    // And check another way
    for(int i = 0; i < w; i++)
    {
      for(int j = 0; j < h; j++)
      {
        assertEquals(y, IPixelFormat.getYUV420PPixel(frame, i, j, IPixelFormat.YUVColorComponent.YUV_Y));
        assertEquals(u, IPixelFormat.getYUV420PPixel(frame, i, j, IPixelFormat.YUVColorComponent.YUV_U));
        assertEquals(v, IPixelFormat.getYUV420PPixel(frame, i, j, IPixelFormat.YUVColorComponent.YUV_V));
      }
    }
  }
  
  @Test
  public void testSamplesToTimeValue()
  {
    ITimeValue result = Utils.samplesToTimeValue(22050, 22050);
    assertEquals(1000, result.get(ITimeValue.Unit.MILLISECONDS));
  }
  
  @Test(expected=IllegalArgumentException.class)
  public void testSamplesToTimeValueNoSampleRate()
  {
    Utils.samplesToTimeValue(1024, 0); 
  }
  
  @Test
  public void testTimeValueToSamples()
  {
    ITimeValue input = ITimeValue.make(16, ITimeValue.Unit.SECONDS);
    long result = Utils.timeValueToSamples(input, 44100);
    assertEquals(44100*16, result);
  }
  
  @Test(expected=IllegalArgumentException.class)
  public void testTimeValueToSamplesNoSampleRate()
  {
    Utils.timeValueToSamples(ITimeValue.make(1, ITimeValue.Unit.SECONDS), 0);
  }

  @Test(expected=IllegalArgumentException.class)
  public void testTimeValueToSamplesNoTimeValue()
  {
    Utils.timeValueToSamples(null, 22050);
  }

}
