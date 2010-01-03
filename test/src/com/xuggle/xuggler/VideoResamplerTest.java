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

import static org.junit.Assert.*;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.xuggle.test_utils.NameAwareTestClassRunner;

@RunWith(NameAwareTestClassRunner.class)
public class VideoResamplerTest
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());
  private String mTestName = null;

  @Before
  public void setUp()
  {
    mTestName = NameAwareTestClassRunner.getTestMethodName();
    log.debug("-----START----- {}", mTestName);
  }
  @After
  public void tearDown()
  {
    log.debug("----- END ----- {}", mTestName);
  }
  
  @Test
  public void testGetAndSetProperties()
  {
    // don't run this test if we can't get resamplers
    if (!IVideoResampler.isSupported(IVideoResampler.Feature.FEATURE_COLORSPACECONVERSION))
      return;
    
    IVideoResampler resampler =
      IVideoResampler.make(10, 10, IPixelFormat.Type.BGR24,
          20, 20, IPixelFormat.Type.YUV420P);

    assertTrue("should have some properties", resampler.getNumProperties()>0);
    
    long sws_flags = resampler.getPropertyAsLong("sws_flags");
    
    assertTrue("should be non zero to start with", sws_flags>0);
    
    assertTrue("should succeed",
        resampler.setProperty("sws_flags", "+bicubic") >= 0);
    
    assertTrue("should be changed now",
        sws_flags != resampler.getPropertyAsLong("sws_flags"));
    
    assertNull("should fail",
        resampler.getPropertyAsString("xuggle_notanoption"));
    
    assertTrue("should fail",
        resampler.setProperty("xuggle_notanoption", 15) < 0);
    
    assertTrue("should fail",
        resampler.setProperty("sws_flags", "+notaflag") < 0);
    
  }
}
