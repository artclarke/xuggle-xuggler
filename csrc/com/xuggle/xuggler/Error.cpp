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
/*
 * Error.cpp
 *
 *  Created on: Mar 20, 2009
 *      Author: aclarke
 */

#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/xuggler/Error.h>
#include <com/xuggle/xuggler/FfmpegIncludes.h>

#include <cstring>
#include <stdexcept>

VS_LOG_SETUP(VS_CPP_PACKAGE);

namespace com { namespace xuggle { namespace xuggler
{

Error :: Error()
{
  mType = IError::ERROR_UNKNOWN;
  mErrorNo = 0;
  *mErrorStr = 0;
}

Error :: ~Error()
{
  
}

const char*
Error :: getDescription()
{
  const char* retval = 0;
  if (!*mErrorStr && mErrorNo != 0)
  {
    retval = strerror_r(AVUNERROR(mErrorNo), mErrorStr, sizeof(mErrorStr));
    if (retval != (const char*) mErrorStr)
      strncpy(mErrorStr, retval, sizeof(mErrorStr));
  }
  return *mErrorStr ? mErrorStr : 0;
}

int32_t
Error :: getErrorNumber()
{
  return mErrorNo;
}
IError::Type
Error :: getType()
{
  return mType;
}

Error*
Error :: make(int32_t aErrorNo)
{
  Error* retval = 0;
  try
  {
    if (aErrorNo >= 0)
      throw std::runtime_error("error number must be < 0");
    
    retval = make();
    if (!retval)
      throw std::bad_alloc();
    retval->mErrorNo = aErrorNo;
    retval->mType = errorNumberToType(aErrorNo);
    // null out and don't fill unless description is asked for
    *(retval->mErrorStr) = 0;
  }
  catch (std::exception & e)
  {
    VS_LOG_TRACE("Error: %s", e.what());
    VS_REF_RELEASE(retval);
  }
  return retval;
}

IError::Type
Error :: errorNumberToType(int32_t errNo)
{
  IError::Type retval = IError::ERROR_UNKNOWN;
  switch(errNo)
  {
    case AVERROR_IO:
      retval = IError::ERROR_IO;
      break;
    case AVERROR_NUMEXPECTED:
      retval = IError::ERROR_NUMEXPECTED;
      break;
    case AVERROR_INVALIDDATA:
      retval = IError::ERROR_INVALIDDATA;
      break;
    case AVERROR_NOMEM:
      retval = IError::ERROR_NOMEM;
      break;
    case AVERROR_NOFMT:
      retval = IError::ERROR_NOFMT;
      break;
    case AVERROR_NOTSUPP:
      retval = IError::ERROR_NOTSUPPORTED;
      break;
    case AVERROR_NOENT:
      retval = IError::ERROR_NOENT;
      break;
    case AVERROR_EOF:
      retval = IError::ERROR_EOF;
      break;
    case AVERROR_PATCHWELCOME:
      retval = IError::ERROR_PATCHWELCOME;
      break;
    case AVERROR(EAGAIN):
      retval = IError::ERROR_AGAIN;
      break;
    case AVERROR(ERANGE):
      retval = IError::ERROR_RANGE;
      break;
    default:
      retval = IError::ERROR_UNKNOWN;
      break;
  }
  return retval;
}

}}}
