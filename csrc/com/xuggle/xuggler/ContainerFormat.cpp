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

#include <com/xuggle/xuggler/ContainerFormat.h>

#define VS_FFMPEG_GOT_OFF_THEIR_BUTT_AND_EXPOSED_THIS
#ifdef VS_FFMPEG_GOT_OFF_THEIR_BUTT_AND_EXPOSED_THIS
/*
 * Sigh; this hack is necessary because FFMPEG doesn't actually
 * expose the AVCodecTag structure.  This might break if they
 * change the structure, but by placing it here it should
 * at least cause a compiler error.
 */
struct AVCodecTag
{
  int32_t id;
  uint32_t tag;
};
#endif  
  
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
      mOutputFormat = av_guess_format(shortName, url, mimeType);
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

  const char *
  ContainerFormat :: getOutputExtensions()
  {
    if (!mOutputFormat || !mOutputFormat->extensions ||
        !*mOutputFormat->extensions)
      return 0;
    return mOutputFormat->extensions;
  }

  ICodec::ID
  ContainerFormat :: getOutputDefaultAudioCodec()
  {
    if (!mOutputFormat)
      return ICodec::CODEC_ID_NONE;
    return (ICodec::ID)mOutputFormat->audio_codec;
  }
  
  ICodec::ID
  ContainerFormat :: getOutputDefaultVideoCodec()
  {
    if (!mOutputFormat)
      return ICodec::CODEC_ID_NONE;
    return (ICodec::ID)mOutputFormat->video_codec;
  }

  ICodec::ID
  ContainerFormat :: getOutputDefaultSubtitleCodec()
  {
    if (!mOutputFormat)
      return ICodec::CODEC_ID_NONE;
    return (ICodec::ID)mOutputFormat->subtitle_codec;
  }
  
  int32_t
  ContainerFormat :: getOutputNumCodecsSupported()
  {
    if (!mOutputFormat)
      return 0;
    
    const struct AVCodecTag * const*tags = mOutputFormat->codec_tag;
    if (!tags)
      return 0;

    int numCodecs = 0;

    for(int i = 0;
        tags[i];
        i++)
    {
      for(const struct AVCodecTag * tag = tags[i];
          tag && tag->id != ICodec::CODEC_ID_NONE;
          ++tag)
      {
        ++numCodecs;
      }
    }
    return numCodecs;
  }
  ICodec::ID
  ContainerFormat :: getOutputCodecID(int32_t index)
  {
    if (index < 0)
      return ICodec::CODEC_ID_NONE;

    const struct AVCodecTag * const*tags = mOutputFormat->codec_tag;
    if (!tags)
      return ICodec::CODEC_ID_NONE;

    int numCodecs = 0;

    for(int i = 0;
        tags[i];
        i++)
    {
      for(
          const struct AVCodecTag * tag = tags[i];
          tag && tag->id != ICodec::CODEC_ID_NONE;
          ++tag, ++numCodecs)
      {
        if (numCodecs == index)
          return (ICodec::ID)tag->id;
      }
    }
    return ICodec::CODEC_ID_NONE;
  }

  int32_t
  ContainerFormat :: getOutputCodecTag(int32_t index)
  {
    if (index < 0)
      return ICodec::CODEC_ID_NONE;

    const struct AVCodecTag * const*tags = mOutputFormat->codec_tag;
    if (!tags)
      return ICodec::CODEC_ID_NONE;

    int numCodecs = 0;

    for(int i = 0;
        tags[i];
        i++)
    {
      for(
          const struct AVCodecTag * tag = tags[i];
          tag && tag->id != ICodec::CODEC_ID_NONE;
          ++tag, ++numCodecs)
      {
        if (numCodecs == index)
          return tag->tag;
      }
    }
    return ICodec::CODEC_ID_NONE;
  }

  bool
  ContainerFormat :: isCodecSupportedForOutput(ICodec::ID id)
  {
    return getOutputCodecTag(id);
  }
  
  int32_t
  ContainerFormat :: getOutputCodecTag(ICodec::ID id)
  {
    if (!mOutputFormat)
      return 0;
    return (int32_t)av_codec_get_tag(mOutputFormat->codec_tag,
        (enum CodecID)id);
    
  }

}}}
