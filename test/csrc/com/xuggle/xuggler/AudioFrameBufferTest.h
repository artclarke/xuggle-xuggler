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

#ifndef AUDIOFRAMEBUFFERTEST_H_
#define AUDIOFRAMEBUFFERTEST_H_

#include <com/xuggle/testutils/TestUtils.h>
#include "Helper.h"
#include <com/xuggle/xuggler/AudioFrameBuffer.h>

using namespace VS_CPP_NAMESPACE;

class AudioFrameBufferTest : public CxxTest::TestSuite
{
public:
  AudioFrameBufferTest();
  virtual ~AudioFrameBufferTest();
  void setUp();
  void tearDown();
  
  /**
   * Do nothing; this is an empty test to make sure we compile
   */
  void testSuccess();
  
  /**
   * Test that we can't create objects with bad inputs
   */
  void testInvalidConstructors();
  
  /**
   * Test addSamples
   */
  void testAddSamples();
  
  /**
   * Test add samples with invalid arguments
   */
  void testAddSamplesInvalidArguments();
  
  /**
   * Test get* methods
   */
  void testGetters();
  
  /**
   * Get that getNextFrame works when passed in
   * audio samples that are exactly the same size
   * as frames.
   */
  void testGetNextFrameNoCopy();

  /**
   * Get that getNextFrame works when passed in
   * audio samples that are not exactly the same size
   * as frames.
   */
  void testGetNextFrameWithCopy();

  /**
   * Test that getNextFrame returns back the right time stamps
   * for audio computed in a frame; that is, the time stamp
   * of the earliest audio in that frame.
   */
  void testGetNextFrameReturnsCorrectTimestamps();
  
  /**
   * Do a mini-load test of the frame buffer to make sure with lots
   * of adds and delete it behaves as expected (especially under Valgrind).
   */
  void testGetNextFrameMultipleTimesMono();
  void testGetNextFrameMultipleTimesStereo();
  void helperGetNextFrameMultipleTimes(int32_t channels);

};

#endif /* AUDIOFRAMEBUFFERTEST_H_ */
