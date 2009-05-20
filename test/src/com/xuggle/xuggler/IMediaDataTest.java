/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated.  All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any
 * later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.xuggle.xuggler;

import org.junit.Test;

import java.nio.ByteBuffer;

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
}
