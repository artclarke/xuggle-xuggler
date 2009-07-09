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

#include <com/xuggle/xuggler/ICodec.h>
#include <com/xuggle/xuggler/Global.h>
#include "Helper.h"
#include "CodecTest.h"

using namespace VS_CPP_NAMESPACE;

VS_LOG_SETUP(VS_CPP_PACKAGE);


void
CodecTest :: setUp()
{
  codec = 0;
}

void
CodecTest :: tearDown()
{
  codec = 0;
}

void
CodecTest :: testCreationAndDescruction()
{
  codec = ICodec::findDecodingCodec(ICodec::CODEC_ID_NELLYMOSER);
  VS_TUT_ENSURE("could not find NELLYMOSER decoder", codec);

  codec = ICodec::findEncodingCodec(ICodec::CODEC_ID_MP3);
  VS_TUT_ENSURE("could not find MP3 encoder", codec);

  // should not find these.
  codec = ICodec::findEncodingCodec(ICodec::CODEC_ID_RV40);
  VS_TUT_ENSURE("could find Real Audio encoder, but we thought FFMPEG couldn't support that", !codec);

}

void
CodecTest :: testFindByName()
{
  codec = ICodec::findDecodingCodecByName("nellymoser");
  VS_TUT_ENSURE("could not find NELLYMOSER decoder", codec);

  codec = ICodec::findEncodingCodecByName("libmp3lame");
  VS_TUT_ENSURE("could not find MP3 encoder", codec);


  // should not find these.
  codec = ICodec::findEncodingCodecByName("xan_wc3");
  VS_TUT_ENSURE("could find xan_wc3 encoder", !codec);

  codec = ICodec::findDecodingCodecByName("adts");
  VS_TUT_ENSURE("could find ADTS Decoder", !codec);

}

void
CodecTest :: testInvalidArguments()
{
  // should not find these.
  codec = ICodec::findEncodingCodecByName(0);
  VS_TUT_ENSURE("could find null encoder", !codec);

  codec = ICodec::findEncodingCodecByName("");
  VS_TUT_ENSURE("could find empty encoder", !codec);

  codec = ICodec::findDecodingCodecByName(0);
  VS_TUT_ENSURE("could find null Decoder", !codec);

  codec = ICodec::findDecodingCodecByName("");
  VS_TUT_ENSURE("could find empty Decoder", !codec);

}

void
CodecTest :: testGuessEncodingCodecs()
{
  // should not find these.
  codec = ICodec::guessEncodingCodec(0, 0, "file.mov", 0,
      ICodec::CODEC_TYPE_VIDEO);
  VS_TUT_ENSURE("could not find mpeg4 codec", codec);
  codec = ICodec::guessEncodingCodec(0, 0, "file.flv", 0,
      ICodec::CODEC_TYPE_VIDEO);
  VS_TUT_ENSURE("could not find flv codec", codec);
  codec = ICodec::guessEncodingCodec(0, 0, "file.flv", 0,
      ICodec::CODEC_TYPE_AUDIO);
  VS_TUT_ENSURE("could not find flv codec", codec);
  codec = ICodec::guessEncodingCodec(0, 0, 0, 0,
      ICodec::CODEC_TYPE_AUDIO);
  VS_TUT_ENSURE("could find codec", !codec);

}

void
CodecTest :: testGetInstalledCodecs()
{
  LoggerStack stack;
  stack.setGlobalLevel(Logger::LEVEL_INFO, false);

  int num = ICodec::getNumInstalledCodecs();
  VS_TUT_ENSURE("should be > 1", num > 1);
  for(int i = 0; i < num; i++)
  {
    codec = ICodec::getInstalledCodec(i);
    VS_TUT_ENSURE("should be valid", codec);
    if (codec) {
      VS_LOG_DEBUG("%s: %s",
          codec->getName(), codec->getLongName());
    }
  }
  VS_TUT_ENSURE("could fail quietly",
      0 == ICodec::getInstalledCodec(-1));
  VS_TUT_ENSURE("could fail quietly",
      0 == ICodec::getInstalledCodec(0x7FFFFFFF));
}

void
CodecTest :: testHasCapability()
{
  codec = ICodec::findEncodingCodecByName("nellymoser");
  VS_TUT_ENSURE("got codec", codec);
  int32_t capabilities = codec->getCapabilities();
  VS_TUT_ENSURE("this codec has some set", capabilities != 0);
  VS_TUT_ENSURE("should have delay set", 
      codec->hasCapability(ICodec::CAP_DELAY));
}

void
CodecTest :: testGetSupportedVideoFramRates()
{
  codec = ICodec::findEncodingCodecByName("mpeg2video");
  VS_TUT_ENSURE("got codec", codec);
  int32_t numFrameRates = codec->getNumSupportedVideoFrameRates();
  VS_TUT_ENSURE("should be more than one", numFrameRates > 0);
  for(int i = 0; i < numFrameRates; i++)
  {
    RefPointer<IRational> rate = codec->getSupportedVideoFrameRate(i);
    VS_TUT_ENSURE("should be non null", rate);
    VS_TUT_ENSURE("should be valid number", rate->getDouble());
  }
  VS_TUT_ENSURE("should fail quietly",
      0 == codec->getSupportedVideoFrameRate(-1));
  VS_TUT_ENSURE("should fail quietly",
      0 == codec->getSupportedVideoFrameRate(0x7FFFFFFF));
  
}

void
CodecTest :: testGetSupportedVideoPixelFormats()
{
  codec = ICodec::findEncodingCodecByName("ffvhuff");
  VS_TUT_ENSURE("got codec", codec);
  int32_t num= codec->getNumSupportedVideoPixelFormats();
  VS_TUT_ENSURE("should be more than one", num > 0);
  for(int i = 0; i < num; i++)
  {
    IPixelFormat::Type type = codec->getSupportedVideoPixelFormat(i);
    VS_TUT_ENSURE("should be non null", type != IPixelFormat::NONE);
  }
  VS_TUT_ENSURE("should fail quietly",
      IPixelFormat::NONE == codec->getSupportedVideoPixelFormat(-1));
  VS_TUT_ENSURE("should fail quietly",
      IPixelFormat::NONE == codec->getSupportedVideoPixelFormat(0x7FFFFFFF));
}

void
CodecTest :: testGetSupportedAudioSampleRates()
{
  codec = ICodec::findEncodingCodecByName("nellymoser");
  VS_TUT_ENSURE("got codec", codec);
  int32_t num= codec->getNumSupportedAudioSampleRates();
  VS_TUT_ENSURE("no one supports this yet", num == 0);
  VS_TUT_ENSURE("should fail quietly",
      codec->getSupportedAudioSampleRate(-1)==0);
  VS_TUT_ENSURE("should fail quietly", 
      codec->getSupportedAudioSampleRate(0x7FFFFFFF)==0);
}

void
CodecTest :: testGetSupportedAudioSampleFormats()
{
  codec = ICodec::findDecodingCodecByName("aac");
  VS_TUT_ENSURE("got codec", codec);
  int32_t num= codec->getNumSupportedAudioSampleFormats();
  VS_TUT_ENSURE("should be more than none", num > 0);
  for(int i = 0; i < num; i++)
  {
    IAudioSamples::Format fmt = codec->getSupportedAudioSampleFormat(i);
    VS_TUT_ENSURE("should be non null", fmt != IAudioSamples::FMT_NONE);
  }
  VS_TUT_ENSURE("should fail quietly",
      IAudioSamples::FMT_NONE == codec->getSupportedAudioSampleFormat(-1));
  VS_TUT_ENSURE("should fail quietly",
      IAudioSamples::FMT_NONE == codec->getSupportedAudioSampleFormat(0x7FFFFFFF));
}

void
CodecTest :: testGetSupportedAudioChannelLayouts()
{
  codec = ICodec::findEncodingCodecByName("ac3");
  VS_TUT_ENSURE("got codec", codec);
  int32_t num= codec->getNumSupportedAudioChannelLayouts();
  VS_TUT_ENSURE("should be more than none", num > 0);
  for(int i = 0; i < num; i++)
  {
    int64_t layout = codec->getSupportedAudioChannelLayout(i);
    VS_TUT_ENSURE("should be non null", 0 != layout);
  }
  VS_TUT_ENSURE("should fail quietly",
      0 == codec->getSupportedAudioChannelLayout(-1));
  VS_TUT_ENSURE("should fail quietly",
      0 == codec->getSupportedAudioChannelLayout(0x7FFFFFFF));
}
