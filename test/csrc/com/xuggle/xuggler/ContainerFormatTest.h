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

#ifndef __CONTAINERFORMAT_TEST_H__
#define __CONTAINERFORMAT_TEST_H__

#include <com/xuggle/testutils/TestUtils.h>
#include "Helper.h"
using namespace VS_CPP_NAMESPACE;

class ContainerFormatTest : public CxxTest::TestSuite
{
  public:
    ContainerFormatTest();
    virtual ~ContainerFormatTest();
    void setUp();
    void tearDown();
    void testCreationAndDestruction();
    void testSetInputFormat();
    void testSetOutputFormat();
    void testGetOutputNumCodecsSupported();
    void testGetInstalledInputFormats();
    void testGetInstalledOutputFormats();
  private:
    Helper* h;
    RefPointer<IContainerFormat> format;
};


#endif // __CONTAINERFORMAT_TEST_H__

