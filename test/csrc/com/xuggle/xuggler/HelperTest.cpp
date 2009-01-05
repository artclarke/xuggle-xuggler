/*
 * Copyright (c) 2008 by Vlideshow Inc. (a.k.a. The Yard).  All rights reserved.
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
#include <com/xuggle/xuggler/config.h>
#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/xuggler/Global.h>
#include "Helper.h"
#include "HelperTest.h"

using namespace VS_CPP_NAMESPACE;

VS_LOG_SETUP(VS_CPP_PACKAGE);

void
HelperTest :: testCreationAndDestruction()
{
  Helper h;
}

void
HelperTest :: testSetupReading()
{
  Helper h;
  h.setupReading(h.SAMPLE_FILE);
}

void
HelperTest :: testSetupWriting()
{
#ifdef VS_ENABLE_GPL
  Helper hw;
  hw.setupWriting("HelperTest_3_output.flv", "flv", "libmp3lame", "flv",
      hw.expected_pixel_format,
      hw.expected_width,
      hw.expected_height,
      true,
      hw.expected_sample_rate,
      hw.expected_channels,
      true);
#endif
}

void
HelperTest :: testDecodeAndEncode()
{
#ifdef VS_ENABLE_GPL
  Helper hr;
  Helper hw;
  hr.setupReading(hr.SAMPLE_FILE);
  hw.setupWriting("HelperTest_3_output.flv", "flv", "libmp3lame", "flv",
      hw.expected_pixel_format,
      hw.expected_width,
      hw.expected_height,
      true,
      hw.expected_sample_rate,
      hw.expected_channels,
      true);
  
  Helper::decodeAndEncode(
      hr, // input helper
      hw, // output helper
      5, // max number of seconds to decode for
      0,  // video frame listener
      0,  // video frame listener context
      0,
      0
      );
#endif
}

void
HelperTest :: testReadNextDecodedObject()
{
#ifdef VS_ENABLE_GPL
  Helper hr;
  hr.setupReading(hr.SAMPLE_FILE);
  RefPointer<IVideoPicture> frame = IVideoPicture::make(
      hr.expected_pixel_format,
      hr.expected_width,
      hr.expected_height);
  RefPointer<IAudioSamples> samples = IAudioSamples::make(10000,
      hr.expected_channels);
  int retval = -1;
  int numReads = 0;
  
  while ((retval = hr.readNextDecodedObject(samples.value(), frame.value())) > 0)
  {
    numReads++;
    VS_TUT_ENSURE("should only return 1 or 2", retval == 1 || retval == 2);
    
    if (retval == 1)
      VS_TUT_ENSURE("something should be complete...", samples->isComplete());

    if (retval == 2)
      VS_TUT_ENSURE("something should be complete...", frame->isComplete());
    
  }
  VS_TUT_ENSURE("should return some data",
      numReads > 0);
#endif  
}

void
HelperTest :: testWriteFrameAndWriteSamples()
{
#ifdef VS_ENABLE_GPL
  Helper hr;
  Helper hw;
  int maxReads = 1000;
  
  hr.setupReading(hr.SAMPLE_FILE);
  hw.setupWriting("HelperTest_6_output.flv", "flv", "libmp3lame", "flv",
        hw.expected_pixel_format,
        hw.expected_width,
        hw.expected_height,
        true,
        hw.expected_sample_rate,
        hw.expected_channels,
        true);
    
  RefPointer<IVideoPicture> frame = IVideoPicture::make(
      hr.expected_pixel_format,
      hr.expected_width,
      hr.expected_height);
  RefPointer<IAudioSamples> samples = IAudioSamples::make(10000,
      hr.expected_channels);
  int retval = -1;
  int numReads = 0;
  
  while ((retval = hr.readNextDecodedObject(samples.value(), frame.value())) > 0
      && numReads < maxReads)
  {
    numReads++;
    VS_TUT_ENSURE("should only return 1 or 2", retval == 1 || retval == 2);
    
    if (retval == 1)
    {
      VS_TUT_ENSURE("something should be complete...", samples->isComplete());
      retval = hw.writeSamples(samples.value());
      VS_TUT_ENSURE("could not write audio", retval >= 0);
    }

    if (retval == 2)
    {
      VS_TUT_ENSURE("something should be complete...", frame->isComplete());
      retval = hw.writeFrame(frame.value());
      VS_TUT_ENSURE("could not write video", retval >= 0);
    }
  }
  VS_TUT_ENSURE("should return some data",
      numReads > 0);

#endif
  
}
