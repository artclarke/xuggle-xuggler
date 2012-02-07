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
 * PropertyTest.h
 *
 *  Created on: Feb 2, 2012
 *      Author: aclarke
 */

#ifndef PROPERTYTEST_H_
#define PROPERTYTEST_H_

#include <com/xuggle/testutils/TestUtils.h>
#include "Helper.h"
using namespace VS_CPP_NAMESPACE;

class PropertyTest: public CxxTest::TestSuite
{
public:
  PropertyTest();
  virtual
  ~PropertyTest();
  void setUp();
  void tearDown();
  void testCreation();
  void testIteration();
  void testSetMetaData();
};

#endif /* PROPERTYTEST_H_ */
