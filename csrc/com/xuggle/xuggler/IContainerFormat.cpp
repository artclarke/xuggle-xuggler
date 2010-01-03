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

#include "IContainerFormat.h"
#include "Global.h"
#include "ContainerFormat.h"

#include "FfmpegIncludes.h"

namespace com { namespace xuggle { namespace xuggler
{
  IContainerFormat :: ~IContainerFormat()
  {
  }
  
  IContainerFormat*
  IContainerFormat :: make()
  {
    Global::init();
    return ContainerFormat::make();
  }
  
  int32_t
  IContainerFormat :: getNumInstalledInputFormats()
  {
    Global::init();
    int i = 0;
    for(AVInputFormat* f = 0;
    (f = av_iformat_next(f))!=0;
    ++i)
      ;
    return i;
  }
  
  IContainerFormat*
  IContainerFormat :: getInstalledInputFormat(int32_t index)
  {
    Global::init();
    int i = 0;
    for(AVInputFormat* f = 0;
    (f = av_iformat_next(f))!=0;
    ++i)
      if (i == index) {
        ContainerFormat * retval = ContainerFormat::make();
        if (retval)
          retval->setInputFormat(f);
        return retval;
      }
    return 0;
  }
  
  int32_t
  IContainerFormat :: getNumInstalledOutputFormats()
  {
    Global::init();
    int i = 0;
    for(AVOutputFormat* f = 0;
    (f = av_oformat_next(f))!=0;
    ++i)
      ;
    return i;
  }
  
  IContainerFormat*
  IContainerFormat :: getInstalledOutputFormat(int32_t index)
  {
    Global::init();
    int i = 0;
    for(AVOutputFormat* f = 0;
    (f = av_oformat_next(f))!=0;
    ++i)
      if (i == index)
      {
        ContainerFormat* retval =ContainerFormat::make();
        if (retval)
          retval->setOutputFormat(f);
        return retval;
      }
    return 0;
  }
  
}}}
