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
#include <com/xuggle/ferry/Logger.h>
#include "com/xuggle/xuggler/OptionHelper.h"

extern "C" {
#include <libavcodec/opt.h>
}

#include <stdexcept>
#include <cstring>

VS_LOG_SETUP(VS_CPP_PACKAGE);

namespace com { namespace xuggle { namespace xuggler {

  OptionHelper ::OptionHelper()
  {
  }

  OptionHelper :: ~OptionHelper()
  {
  }
  
  int32_t
  OptionHelper :: setProperty(void *aContext, const char* aName, const char *aValue)
  {
    int32_t retval = -1;

    try
    {
      if (!aContext)
        throw std::runtime_error("no context passed in");
      
      if (!aName  || !*aName)
        throw std::runtime_error("empty property name passed to setProperty");

      retval = av_set_string3(aContext, aName, aValue, 1, 0);
      
    }
    catch (std::exception & e)
    {
      VS_LOG_DEBUG("Error: %s", e.what());
      retval = -1;
    }

    return retval;
  }

  char*
  OptionHelper :: getPropertyAsString(void *aContext, const char *aName)
  {
    char* retval = 0;

    try
    {
      if (!aContext)
        throw std::runtime_error("no context passed in");
      
      if (!aName  || !*aName)
        throw std::runtime_error("empty property name passed to setProperty");

      // we don't allow a string value longer than this.  This is
      // actually safe because this buffer is only used for non-string options
      char tmpBuffer[512];
      int32_t bufLen = sizeof(tmpBuffer)/sizeof(char);
      
      const char* value = av_get_string(aContext, aName,0,tmpBuffer,bufLen);
      
      if (value)
      {
        // let's make a copy of the data
        int32_t valLen = strlen(value);
        if (valLen > 0)
        {
          retval = new char[valLen+1];
          if (!retval)
            throw std::bad_alloc();
          
          // copy the data
          strncpy(retval, value, valLen+1);
        }
      }
    }
    catch (std::exception & e)
    {
      VS_LOG_DEBUG("Error: %s", e.what());
      if (retval)
        delete [] retval;
      retval = 0;
    }

    // NOTE: Caller must call delete[] on returned value; we mean it!
    return retval;

  }


}}}
