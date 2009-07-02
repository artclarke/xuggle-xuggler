/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated.  All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any
 * later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

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
