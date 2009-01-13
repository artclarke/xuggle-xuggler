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
#include <com/xuggle/xuggler/ICodec.h>
#include <com/xuggle/xuggler/Global.h>
#include "Helper.h"
#include "CodecTest.h"

using namespace VS_CPP_NAMESPACE;

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
  codec = ICodec::findEncodingCodec(ICodec::CODEC_ID_AAC);
  VS_TUT_ENSURE("could find AAC encoder", !codec);

}

void
CodecTest :: testFindByName()
{
  codec = ICodec::findDecodingCodecByName("nellymoser");
  VS_TUT_ENSURE("could not find NELLYMOSER decoder", codec);

  codec = ICodec::findEncodingCodecByName("libmp3lame");
  VS_TUT_ENSURE("could not find MP3 encoder", codec);


  // should not find these.
  codec = ICodec::findEncodingCodecByName("aac");
  VS_TUT_ENSURE("could find AAC encoder", !codec);

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

