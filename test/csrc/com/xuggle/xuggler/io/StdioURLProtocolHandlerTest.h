/*******************************************************************************
 * Copyright (c) 2012 Xuggle Inc.  All rights reserved.
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
/*
 * StdioURLProtocolHandlerTest.h
 *
 *  Created on: Feb 6, 2012
 *      Author: aclarke
 */

#ifndef STDIOURLHANDLERTEST_H_
#define STDIOURLHANDLERTEST_H_

#include <com/xuggle/testutils/TestUtils.h>
#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/xuggler/io/StdioURLProtocolManager.h>

using namespace VS_CPP_NAMESPACE;


class StdioURLProtocolHandlerTest: public CxxTest::TestSuite
{
public:
  StdioURLProtocolHandlerTest();
  virtual
  ~StdioURLProtocolHandlerTest();
  void setUp();
  void tearDown();
  void testCreation();
  void testOpenClose();
  void testRead();
  void testReadWrite();
  void testSeek();
  void testSeekableFlags();
private:
  const char * FIXTURE_DIRECTORY;
  const char * SAMPLE_FILE;
  char mFixtureDir[4098];
  char mSampleFile[4098];
};

#endif /* STDIOURLHANDLERTEST_H_ */
