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
#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__

#include <cxxtest/TestSuite.h>

// These macros exists for backwards comability reasons; we used
//  to use the Template Unit Testing framework and old code
//  used this form.  This is no deprecated and you should
//  use the TS_* macros.
#define VS_TUT_ENSURE(__msg, __expr) \
  TS_ASSERT(__expr)

#define VS_TUT_ENSURE_EQUALS(msg, __a, __b) \
  TS_ASSERT_EQUALS(__a, __b)

#define VS_TUT_ENSURE_DISTANCE(msg, __a, __b, __delta) \
  TS_ASSERT_DELTA(__a, __b, __delta)

#endif // ! __TEST_UTILS_H__
