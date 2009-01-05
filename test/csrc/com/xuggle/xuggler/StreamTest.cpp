/*
 * Copyright (c) 2008 by Vlideshow Inc. (a.k.a. The Yard).  All rights reserved.
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
#include <com/xuggle/ferry/RefPointer.h>
#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/xuggler/IStream.h>
#include <com/xuggle/xuggler/IStreamCoder.h>
#include <com/xuggle/xuggler/ICodec.h>
#include <com/xuggle/xuggler/Global.h>
#include "StreamTest.h"

using namespace VS_CPP_NAMESPACE;

VS_LOG_SETUP(VS_CPP_PACKAGE);

StreamTest :: StreamTest()
{
  h = 0;
}

StreamTest :: ~StreamTest()
{
  tearDown();
}

void
StreamTest :: setUp()
{
  h=new Helper();
}

void
StreamTest :: tearDown()
{
  int i = 0;

  if (h)
  {
    for (i = 0; i < h->num_streams; i++)
    {
      IStream* stream=h->streams[i].value();
      IStreamCoder* coder=h->coders[i].value();
      ICodec* codec=h->codecs[i].value();

      VS_TUT_ENSURE("no stream", stream);
      VS_TUT_ENSURE("no coder", coder);
      VS_TUT_ENSURE("no codec", codec);

      // Helper, container.
      VS_TUT_ENSURE_EQUALS("wrong stream ref count",
          stream->getCurrentRefCount(),
          2);
      VS_TUT_ENSURE_EQUALS("wrong coder ref count",
          coder->getCurrentRefCount(),
          2);
      VS_TUT_ENSURE_EQUALS("wrong codec ref count",
          codec->getCurrentRefCount(),
          2);
    }
    if (h->container)
      // just container
      VS_TUT_ENSURE_EQUALS("wrong container ref count",
          h->container->getCurrentRefCount(),
          1);
    VS_LOG_TRACE("StreamTest destroyed");
    delete h;
  }
  h = 0;
}

  void
StreamTest :: testCreationAndDestruction()
{
  RefPointer<IStream> stream;
  RefPointer<IStreamCoder> coder;
  RefPointer<ICodec> codec;
  RefPointer<IRational> rational;

  h->setupReading(h->SAMPLE_FILE);

  for (int i = 0; i < h->num_streams; i++)
  {
    VS_LOG_TRACE("Starting test of stream: %d", i);

    VS_LOG_TRACE("Stream test: Getting stream: %d", i);
    stream = h->streams[i];
    VS_TUT_ENSURE("could not get stream", stream);

    // container, helper, and me.
    VS_TUT_ENSURE_EQUALS("stream", stream->getCurrentRefCount(), 3);

    VS_TUT_ENSURE("stream index did not match",
        stream->getIndex() == i);

    VS_LOG_TRACE("Stream test: Getting coder: %d", i);
    coder = stream->getStreamCoder();
    VS_TUT_ENSURE("could not get coder for stream", coder);

    VS_TUT_ENSURE_EQUALS("coder", coder->getCurrentRefCount(), 3);

    VS_LOG_TRACE("Stream test: Getting codec: %d", i);
    codec = coder->getCodec();
    VS_TUT_ENSURE("could not get codec", codec);

    VS_TUT_ENSURE_EQUALS("codec", codec->getCurrentRefCount(), 3);

    if (!codec->getType() == ICodec::CODEC_TYPE_AUDIO &&
        !codec->getType() == ICodec::CODEC_TYPE_VIDEO)
    {
      VS_LOG_ERROR("Unexpected type of codec");
      TS_ASSERT(false);
    }

    if (codec->getType() == ICodec::CODEC_TYPE_VIDEO)
    {
      rational = stream->getFrameRate();
      if (h->expected_frame_rate > 0)
        VS_TUT_ENSURE_DISTANCE("unexpected frame rate",
            rational->getDouble(),
            h->expected_frame_rate, 0.0001);
      rational = stream->getTimeBase();
      if (h->expected_time_base > 0)
        VS_TUT_ENSURE_DISTANCE("unexpected frame rate",
            rational->getDouble(),
            h->expected_time_base, 0.0001);
      VS_LOG_TRACE("Finished test of stream: %d", i);
    }
  }
}
