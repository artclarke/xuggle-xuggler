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

#include <com/xuggle/xuggler/ContainerFormat.h>

namespace com { namespace xuggle { namespace xuggler
{

  ContainerFormat :: ContainerFormat()
  {
    mInputFormat = NULL;
    mOutputFormat = NULL;
  }

  ContainerFormat :: ~ContainerFormat()
  {
  }

  int32_t
  ContainerFormat :: setInputFormat(const char *shortName)
  {
    if (shortName && *shortName)
    {
      mInputFormat = av_find_input_format(shortName);
    } else {
      mInputFormat = NULL;
    }
    return (mInputFormat == NULL ? -1 : 0);
  }
  int32_t
  ContainerFormat :: setOutputFormat(const char*shortName,
      const char*url,
      const char* mimeType)
  {
    if ((shortName && *shortName) ||
        (url && *url) ||
        (mimeType && *mimeType))
    {
      mOutputFormat = guess_format(shortName, url, mimeType);
    } else {
      mOutputFormat = NULL;
    }
    return (mOutputFormat == NULL ? -1 : 0);
  }
  const char*
  ContainerFormat :: getInputFormatShortName()
  {
    return (mInputFormat ? mInputFormat->name : 0);
  }
  const char*
  ContainerFormat :: getInputFormatLongName()
  {
    return (mInputFormat ? mInputFormat->long_name : 0);
  }
  const char *
  ContainerFormat :: getOutputFormatShortName()
  {
    return (mOutputFormat ? mOutputFormat->name : 0);
  }
  const char *
  ContainerFormat :: getOutputFormatLongName()
  {
    return (mOutputFormat ? mOutputFormat->long_name : 0);
  }
  const char *
  ContainerFormat :: getOutputFormatMimeType()
  {
    return (mOutputFormat ? mOutputFormat->mime_type : 0);
  }
  AVInputFormat*
  ContainerFormat :: getInputFormat()
  {
    return mInputFormat;
  }
  AVOutputFormat*
  ContainerFormat :: getOutputFormat()
  {
    return mOutputFormat;
  }
  void
  ContainerFormat :: setInputFormat(AVInputFormat* fmt)
  {
    mInputFormat = fmt;
  }
  void
  ContainerFormat :: setOutputFormat(AVOutputFormat* fmt)
  {
    mOutputFormat = fmt;
  }
  
  int32_t
  ContainerFormat :: getInputFlags()
  {
    return (mInputFormat ? mInputFormat->flags : 0);
  }

  void
  ContainerFormat :: setInputFlags(int32_t newFlags)
  {
    if (mInputFormat)
      mInputFormat->flags = newFlags;
  }

  bool
  ContainerFormat :: getInputFlag(IContainerFormat::Flags flag)
  {
    bool result = false;
    if (mInputFormat)
      result = mInputFormat->flags & flag;
    return result;
  }

  void
  ContainerFormat :: setInputFlag(IContainerFormat::Flags flag, bool value)
  {
    if (mInputFormat)
    {
      if (value)
      {
        mInputFormat->flags |= flag;
      }
      else
      {
        mInputFormat->flags &= (~flag);
      }
    }
  }
  
  int32_t
  ContainerFormat :: getOutputFlags()
  {
    return (mOutputFormat ? mOutputFormat->flags : 0);
  }

  void
  ContainerFormat :: setOutputFlags(int32_t newFlags)
  {
    if (mOutputFormat)
      mOutputFormat->flags = newFlags;
  }

  bool
  ContainerFormat :: getOutputFlag(IContainerFormat::Flags flag)
  {
    bool result = false;
    if (mOutputFormat)
      result = mOutputFormat->flags & flag;
    return result;
  }

  void
  ContainerFormat :: setOutputFlag(IContainerFormat::Flags flag, bool value)
  {
    if (mOutputFormat)
    {
      if (value)
      {
        mOutputFormat->flags |= flag;
      }
      else
      {
        mOutputFormat->flags &= (~flag);
      }
    }
  }
  bool
  ContainerFormat :: isOutput()
  {
    return mOutputFormat;
  }
  
  bool
  ContainerFormat :: isInput()
  {
    return mInputFormat;
  }

}}}
