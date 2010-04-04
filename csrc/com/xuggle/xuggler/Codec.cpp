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

#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/ferry/RefPointer.h>
#include <com/xuggle/xuggler/Global.h>
#include <com/xuggle/xuggler/Codec.h>
#include <com/xuggle/xuggler/ContainerFormat.h>

VS_LOG_SETUP(VS_CPP_PACKAGE);

namespace com { namespace xuggle { namespace xuggler
{
  using namespace com::xuggle::ferry;

  Codec :: Codec()
  {
    VS_LOG_TRACE("Starting");
    mCodec = 0;
  }

  Codec :: ~Codec()
  {
    // no memory freeing required.
    mCodec = 0;
  }

  const char *
  Codec :: getName() {
    const char * retval = 0;
    if (mCodec)
      retval = mCodec->name;
    return retval;
  }

  int
  Codec :: getIDAsInt()
  {
    int retval = CODEC_ID_NONE;
    if (mCodec)
      retval = mCodec->id;
    return retval;
  }

  ICodec::Type
  Codec :: getType()
  {
    Type retval = (Type) AVMEDIA_TYPE_UNKNOWN;
    if (mCodec)
      retval = (Type) mCodec->type;
    return retval;
  }

  Codec *
  Codec :: make(AVCodec *aCodec)
  {
    Codec *retval = 0;
    if (aCodec)
    {
      retval = Codec::make();
      if (retval)
        retval->mCodec = aCodec;
    }
    return retval;
  }

  Codec *
  Codec :: findEncodingCodec(const ICodec::ID id)
  {
    return Codec::findEncodingCodecByIntID((const int)id);
  }
  Codec *
  Codec :: findEncodingCodecByIntID(const int id)
  {
    Codec *retval = 0;
    AVCodec *codec = 0;
    enum CodecID ffmpeg_id = (enum CodecID) id;
    Global::lock();
    codec = avcodec_find_encoder(ffmpeg_id);
    Global::unlock();
    if (codec)
      retval = Codec::make(codec);

    return retval;
  }

  Codec *
  Codec :: findEncodingCodecByName(const char* name)
  {
    Codec *retval = 0;
    AVCodec *codec = 0;
    if (name && *name)
    {
      Global::lock();
      codec = avcodec_find_encoder_by_name(name);
      Global::unlock();
      if (codec)
        retval = Codec::make(codec);
    }
    return retval;
  }

  Codec *
  Codec :: findDecodingCodec(const ICodec::ID id)
  {
    return Codec::findDecodingCodecByIntID((const int)id);
  }

  Codec *
  Codec :: findDecodingCodecByIntID(const int id)
  {
    Codec *retval = 0;
    AVCodec *codec = 0;

    Global::lock();
    codec = avcodec_find_decoder((enum CodecID) id);
    Global::unlock();
    
    if (codec)
      retval = Codec::make(codec);
    return retval;
  }

  Codec *
  Codec :: findDecodingCodecByName(const char* name)
  {
    Codec *retval = 0;
    AVCodec *codec = 0;
    if (name && *name)
    {
      Global::lock();
      codec = avcodec_find_decoder_by_name(name);
      Global::unlock();
      if (codec)
        retval = Codec::make(codec);
    }
    return retval;
  }
  bool
  Codec :: canDecode()
  {
    return mCodec ? mCodec->decode : false;
  }

  bool
  Codec :: canEncode()
  {
    return mCodec ? mCodec->encode : false;
  }

  Codec*
  Codec :: guessEncodingCodec(IContainerFormat* pFmt,
      const char* shortName,
      const char* url,
      const char* mimeType,
      ICodec::Type type)
  {
    Codec* retval = 0;
    RefPointer<ContainerFormat> fmt = 0;
    AVOutputFormat * oFmt = 0;

    // We acquire here because the RefPointer always
    // releases.
    fmt.reset(dynamic_cast<ContainerFormat*>(pFmt), true);

    if (!fmt)
    {
      fmt = ContainerFormat::make();
      if (fmt)
        fmt->setOutputFormat(shortName, url, mimeType);
    }
    if (fmt)
      oFmt = fmt->getOutputFormat();

    // Make sure at least one in put is specified.
    // The av_guess_codec function REQUIRES a
    // non null AVOutputFormat
    // It also examines url with a strcmp(), so let's
    // give it a zero-length string.
    // In reality, it ignores the other params.
    if (!url)
      url = "";

    if (oFmt)
    {
      enum CodecID id = av_guess_codec(oFmt, shortName, url,
          mimeType, (enum AVMediaType) type);
      retval = Codec::findEncodingCodecByIntID((int)id);
    }
    return retval;
  }

  int32_t
  Codec :: acquire()
  {
    int retval = 0;
    retval = RefCounted::acquire();
    VS_LOG_TRACE("Acquired %p: %d", this, retval);
    return retval;
  }
  int32_t
  Codec :: release()
  {
    int retval = 0;
    retval = RefCounted::release();
    VS_LOG_TRACE("Released %p: %d", this, retval);
    return retval;
  }
  const char *
  Codec :: getLongName()
  {
    return mCodec ? mCodec->long_name : 0;
  }

  int32_t
  Codec :: getCapabilities()
  {
    if (!mCodec)
      return 0;
    return mCodec->capabilities;
  }
  bool
  Codec :: hasCapability(Capabilities flag)
  {
    if (!mCodec)
      return false;
    return mCodec->capabilities&flag;
  }

  int32_t
  Codec :: getNumSupportedVideoFrameRates()
  {
    if (!mCodec)
      return 0;
    int count = 0;
    for(
        const AVRational* p=mCodec->supported_framerates;
        p && !(!p->den && !p->num);
        p++
    )
      ++count;
    return count;
  }
  IRational*
  Codec :: getSupportedVideoFrameRate(int32_t index)
  {
    if (!mCodec)
      return 0;
    int i = 0;
    for(
        const AVRational* p=mCodec->supported_framerates;
        p && !(!p->den && !p->num);
        p++, i++
    )
      if (index == i)
        return IRational::make(p->num,p->den);
    return 0;
  }

  int32_t
  Codec :: getNumSupportedVideoPixelFormats()
  {
    if (!mCodec)
      return 0;
    int count = 0;
    for(const enum PixelFormat* p = mCodec->pix_fmts;
    p && (*p!=-1);
    p++)
      ++count;
    return count;

  }

  IPixelFormat::Type
  Codec :: getSupportedVideoPixelFormat(int32_t index)
  {
    if (!mCodec)
      return IPixelFormat::NONE;
    int i = 0;
    for(const enum PixelFormat* p = mCodec->pix_fmts;
    p && (*p!=-1);
    p++,i++)
      if (index == i)
        return (IPixelFormat::Type)*p;
    return IPixelFormat::NONE;
  }

  int32_t
  Codec :: getNumSupportedAudioSampleRates()
  {
    if (!mCodec)
      return 0;
    int i = 0;
    for(const int *p = mCodec->supported_samplerates;
    p && *p;
    ++p, ++i)
      ;
    return i;
  }
  int32_t
  Codec :: getSupportedAudioSampleRate(int32_t index)
  {
    if (!mCodec)
      return 0;
    int i = 0;
    for(const int *p = mCodec->supported_samplerates;
    p && *p;
    p++, i++)
      if (i == index)
        return *p;
    return 0;
  }

  int32_t
  Codec :: getNumSupportedAudioSampleFormats()
  {
    if (!mCodec)
      return 0;
    int i = 0;
    for(const enum SampleFormat* p=mCodec->sample_fmts;
      p && (*p != -1);
      p++,i++)
      ;
    return i;
  }
  
  IAudioSamples::Format
  Codec :: getSupportedAudioSampleFormat(int32_t index)
  {
    if (!mCodec)
      return IAudioSamples::FMT_NONE;
    int i = 0;
    for(const enum SampleFormat* p=mCodec->sample_fmts;
      p && (*p != -1);
      p++,i++)
      if (index == i)
        return (IAudioSamples::Format)*p;
    return IAudioSamples::FMT_NONE;
    
  }

  int32_t
  Codec :: getNumSupportedAudioChannelLayouts()
  {
    if (!mCodec)
      return 0;
    int i = 0;
    for(const int64_t* p=mCodec->channel_layouts;
      p && *p;
      p++,i++)
      ;
    return i;
  }
  int64_t
  Codec :: getSupportedAudioChannelLayout(int32_t index)
  {
    if (!mCodec)
      return 0;
    int i = 0;
    for(const int64_t* p=mCodec->channel_layouts;
      p && *p;
      p++,i++)
      if (index == i)
        return *p;
    return 0;
  }

}}}
