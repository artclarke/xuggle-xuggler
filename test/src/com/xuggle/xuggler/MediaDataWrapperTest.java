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

package com.xuggle.xuggler;

import static org.junit.Assert.*;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class MediaDataWrapperTest
{

  @Before
  public void setUp() throws Exception
  {
  }

  @After
  public void tearDown() throws Exception
  {
  }

  @Test
  public void testGet()
  {
    IAudioSamples samples = IAudioSamples.make(1024, 2);
    IMediaDataWrapper wrapper = IMediaDataWrapper.make(samples);
    
    // This is a non-obvious test; without special code swig objects can't
    // actually upcast correctly when crossing the Java/native boundary.  This
    // test makes sure that they can.
    assertTrue(wrapper.get() instanceof IAudioSamples);
    assertEquals(samples, wrapper.get());
    
    // test the other types
    IMediaData media;
    media = IPacket.make();
    wrapper.wrap(media);
    assertTrue(wrapper.get() instanceof IPacket);
    
    media = IVideoPicture.make(IPixelFormat.Type.YUV420P, 10, 10);
    wrapper.wrap(media);
    assertTrue(wrapper.get() instanceof IVideoPicture);
    
    media = IMediaDataWrapper.make(null);
    wrapper.wrap(media);
    assertTrue(wrapper.get() instanceof IMediaDataWrapper);
    
  }

  @Test
  public void testUnwrap()
  {
    IAudioSamples samples = IAudioSamples.make(1024,2);
    IMediaDataWrapper wrapper = IMediaDataWrapper.make(samples);
    IMediaDataWrapper wrapper2 = IMediaDataWrapper.make(wrapper);
    
    assertEquals(wrapper, wrapper2.get());
    assertEquals(samples, wrapper2.unwrap());
  }

  @Test
  public void testWrap()
  {
    IAudioSamples samples = IAudioSamples.make(1024, 2);
    IMediaDataWrapper wrapper = IMediaDataWrapper.make(null);
    
    assertNull(wrapper.get());
    
    wrapper.wrap(samples);
    
    assertTrue(wrapper.get() instanceof IAudioSamples);
    assertEquals(samples, wrapper.get());
  }

}
