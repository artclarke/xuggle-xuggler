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

#include "ICodec.h"
#include "Global.h"
#include "Codec.h"

#include "FfmpegIncludes.h"

namespace com { namespace xuggle { namespace xuggler
{

  ICodec :: ICodec()
  {
  }

  ICodec :: ~ICodec()
  {
  }

  ICodec *
  ICodec :: findEncodingCodec(ICodec::ID id)
  {
    Global::init();
    return Codec::findEncodingCodec(id);
  }

  ICodec *
  ICodec :: findEncodingCodecByIntID(int id)
  {
    Global::init();
    return Codec::findEncodingCodecByIntID(id);
  }
  
  ICodec *
  ICodec :: findEncodingCodecByName(const char *name)
  {
    Global::init();
    return Codec::findEncodingCodecByName(name);
  }

  ICodec *
  ICodec :: findDecodingCodec(ICodec::ID id)
  {
    Global::init();
    return Codec::findDecodingCodec(id);
  }
  
  ICodec *
  ICodec :: findDecodingCodecByIntID(int id)
  {
    Global::init();
    return Codec::findDecodingCodecByIntID(id);
  }
  
  ICodec*
  ICodec :: findDecodingCodecByName(const char* name)
  {
    Global::init();
    return Codec::findDecodingCodecByName(name);
  }

  ICodec*
  ICodec :: guessEncodingCodec(IContainerFormat* fmt,
      const char* shortName,
      const char* url,
      const char* mimeType,
      ICodec::Type type)
  {
    Global::init();
    return Codec::guessEncodingCodec(fmt, shortName, url, mimeType, type);
  }

  int32_t
  ICodec :: getNumInstalledCodecs()
  {
    Global::init();
    int32_t numInstalledCodecs = 0;
    AVCodec * codec = 0;
    while((codec = av_codec_next(codec))!=0)
      ++numInstalledCodecs;
    return numInstalledCodecs;
  }
  
  ICodec*
  ICodec :: getInstalledCodec(int32_t index)
  {
    Global::init();
    if (index < 0)
      return 0;
    
    AVCodec * codec = 0;
    for(int32_t i = 0; (codec = av_codec_next(codec))!=0; i++)
    {
      if (i == index)
        return Codec::make(codec);
    }
    return 0;
  }

}}}
