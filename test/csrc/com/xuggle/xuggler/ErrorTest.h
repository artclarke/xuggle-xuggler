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

/*
 * ErrorTest.h
 *
 *  Created on: Mar 20, 2009
 *      Author: aclarke
 */

#ifndef ERRORTEST_H_
#define ERRORTEST_H_

#include <com/xuggle/testutils/TestUtils.h>
#include "Helper.h"
using namespace VS_CPP_NAMESPACE;

class ErrorTest : public CxxTest::TestSuite
{
public:
  ErrorTest();
  virtual ~ErrorTest();
  
  void testErrorCreation();
};

#endif /* ERRORTEST_H_ */
