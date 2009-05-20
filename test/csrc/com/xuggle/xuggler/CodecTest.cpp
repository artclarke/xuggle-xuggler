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

