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

#include <com/xuggle/xuggler/ContainerFormat.h>

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
    
#ifdef VS_FFMPEG_GOT_OFF_THEIR_BUTT_AND_EXPOSED_THIS
    // Now, this is NOT fast, and we could make it faster
    // if FFMPEG exposed the AVCodecTag structure, which
    // they don't.  Here's the code that does that:
    int numCodecs = 0;
    for(const struct AVCodecTag * tag =
      (mOutputFormat->codec_tag ? *mOutputFormat->codec_tag : 0);
      tag && tag->id != ICodec::CODEC_ID_NONE;
      tag++)
    {
      ++numCodecs;
    }
    return numCodecs;
#else
    // iterate through all codecs
    int32_t numSupportedCodecs = 0;
    AVCodec *codec = 0;
    while((codec = av_codec_next(codec))!= 0)
    {
      if (codec->id != CODEC_ID_NONE) {
        int tag = av_codec_get_tag(mOutputFormat->codec_tag,
            codec->id);
        if (tag)
          ++numSupportedCodecs;
      }
    }
    return numSupportedCodecs;
#endif
  }
  ICodec::ID
  ContainerFormat :: getOutputCodecID(int32_t index)
  {
    if (index < 0)
      return ICodec::CODEC_ID_NONE;
#ifdef VS_FFMPEG_GOT_OFF_THEIR_BUTT_AND_EXPOSED_THIS
    int codecNum= 0;
    for(const struct AVCodecTag * tag =
      (mOutputFormat->codec_tag ? *mOutputFormat->codec_tag : 0);
      tag && tag->id != ICodec::CODEC_ID_NONE;
      tag++, codecNum++)
    {
      if (codecNum == index)
        return (ICodec::ID)tag->id;
    }
    return ICodec::CODEC_ID_NONE;
#else
    // iterate through all codecs
    int32_t numSupportedCodecs = 0;
    AVCodec *codec = 0;
    while((codec = av_codec_next(codec))!= 0)
    {
      if (codec->id != CODEC_ID_NONE) {
        int tag = av_codec_get_tag(mOutputFormat->codec_tag,
            codec->id);
        if (tag) {
          if (numSupportedCodecs == index) {
            return (ICodec::ID)codec->id;
          }
          ++numSupportedCodecs;
        }
      }
    }
    return ICodec::CODEC_ID_NONE;

#endif
  }
  int32_t
  ContainerFormat :: getOutputCodecTag(int32_t index)
  {
    if (index < 0)
      return 0;
#ifdef VS_FFMPEG_GOT_OFF_THEIR_BUTT_AND_EXPOSED_THIS
    int codecNum= 0;
    for(const struct AVCodecTag * tag =
      (mOutputFormat->codec_tag ? *mOutputFormat->codec_tag : 0);
      tag && tag->id != ICodec::CODEC_ID_NONE;
      tag++, codecNum++)
    {
      if (codecNum == index)
        return tag->tag;
    }
    return 0;
#else
    // iterate through all codecs
    int32_t numSupportedCodecs = 0;
    AVCodec *codec = 0;
    while((codec = av_codec_next(codec))!= 0)
    {
      if (codec->id != CODEC_ID_NONE) {
        int tag = av_codec_get_tag(mOutputFormat->codec_tag,
            codec->id);
        if (tag) {
          if (numSupportedCodecs == index) {
            return (int32_t)tag;
          }
          ++numSupportedCodecs;
        }
      }
    }
    return 0;
#endif
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
