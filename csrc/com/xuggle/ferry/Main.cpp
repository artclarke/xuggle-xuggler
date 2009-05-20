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
