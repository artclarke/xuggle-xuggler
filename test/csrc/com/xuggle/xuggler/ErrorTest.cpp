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

/*
 * ErrorTest.cpp
 *
 *  Created on: Mar 20, 2009
 *      Author: aclarke
 */

#include "ErrorTest.h"
#include <com/xuggle/xuggler/FfmpegIncludes.h>
#include <com/xuggle/ferry/RefPointer.h>
#include <com/xuggle/ferry/Logger.h>

#include <com/xuggle/xuggler/IError.h>
#include <cstring>

using namespace com::xuggle::ferry;

using namespace VS_CPP_NAMESPACE;

VS_LOG_SETUP(VS_CPP_PACKAGE);

ErrorTest :: ErrorTest()
{
}

ErrorTest :: ~ErrorTest()
{
}

void
ErrorTest :: testErrorCreation()
{
  RefPointer<IError> error = IError::make(AVERROR_IO);
  
  VS_TUT_ENSURE("made error", error);
  
  const char* description = error->getDescription();
  
  VS_TUT_ENSURE("got description", description && *description);

  VS_TUT_ENSURE_EQUALS("should map correctly", 
      IError::ERROR_IO,
      error->getType());
  
  error = IError::make(IError::ERROR_IO);
  VS_TUT_ENSURE("made error", error);
    
  description = error->getDescription();

  VS_TUT_ENSURE("got description", description && *description);

  VS_TUT_ENSURE_EQUALS("should map correctly", 
      AVERROR_IO,
      error->getErrorNumber());
  
}
