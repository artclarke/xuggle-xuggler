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
#include "AtomicInteger.h"
#include "Logger.h"
#include "Mutex.h"
using namespace com::xuggle::ferry;

VS_LOG_SETUP(VS_CPP_PACKAGE);

int
main(int, const char **)
{
  int retval = 0;
  AtomicInteger ai;
  Mutex* mutex=0;

  for (unsigned int i = 0; i < 10; i++)
  {
    int val = ai.getAndIncrement();
    VS_LOG_DEBUG("Atomic Integer value: %d", val);
  }
  VS_LOG_DEBUG("Final Atomic Integer value: %d", ai.get());

  mutex = Mutex::make();
  if (mutex) {
    mutex->release();
    mutex = 0;
    VS_LOG_ERROR("Got a mutex value, but we're not in Java so null should be returned");
  } else {
    VS_LOG_INFO("got no mutex as expected");
  }
  VS_ASSERT(true, "this should never fail");
  return retval;
}
