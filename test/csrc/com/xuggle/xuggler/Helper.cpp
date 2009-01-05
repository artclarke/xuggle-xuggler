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
#include <string>
#include <stdexcept>
// For getenv()
#include <stdlib.h>

#include <com/xuggle/testutils/TestUtils.h>
#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/xuggler/Global.h>
#include "Helper.h"

VS_LOG_SETUP(VS_CPP_PACKAGE);

namespace com { namespace xuggle { namespace xuggler {

  Helper :: Helper()
  {
    VS_LOG_TRACE("Helper created");
    char *fixtureDirectory = getenv("VS_TEST_FIXTUREDIR");
    if (fixtureDirectory)
      snprintf(mFixtureDir, sizeof(mFixtureDir), "%s", fixtureDirectory);
    else
      snprintf(mFixtureDir, sizeof(mFixtureDir), ".");
    FIXTURE_DIRECTORY = mFixtureDir;
    SAMPLE_FILE = "testfile.flv";
    snprintf(mSampleFile, sizeof(mSampleFile), "%s/%s", mFixtureDir, SAMPLE_FILE);

    // change these to zero if you don't want
    // tests to check 'em.
    expected_num_streams = 2;
    expected_audio_packets = 5714;
    expected_video_packets = 2236;
    // 1 frame per packet
    expected_video_frames = expected_video_packets;
    expected_video_key_frames = 270;
    expected_audio_samples = 3291264;
    expected_packets = expected_audio_packets + expected_video_packets;
    expected_frame_rate = 15;
    expected_time_base = .0010; // This is the default BEFORE we read any packets
    expected_width = 424;
    expected_height=176;
    expected_gops=12;
    expected_pixel_format=IPixelFormat::YUV420P;
    expected_bit_rate = 64000;
    expected_sample_rate = 22050;
    expected_channels = 1;
    expected_codec_types = new ICodec::Type[expected_num_streams];
    if (expected_codec_types) {
      expected_codec_types[0] = ICodec::CODEC_TYPE_VIDEO;
      expected_codec_types[1] = ICodec::CODEC_TYPE_AUDIO;
    }
    expected_codec_ids = new ICodec::ID[expected_num_streams];
    if (expected_codec_ids) {
      expected_codec_ids[0] = ICodec::CODEC_ID_FLV1;
      expected_codec_ids[1] = ICodec::CODEC_ID_MP3;
    }

    container_isopen = false;
    need_trailer_write = false;
    
    streams = 0;
    num_streams = 0;
    codecs = 0;
    num_codecs = 0;
    coders = 0;
    num_coders = 0;
    vresamplers = 0;
    num_vresamplers = 0;
    aresamplers = 0;
    num_aresamplers = 0;

    first_output_video_stream = -1;
    first_output_audio_stream = -1;
    first_input_video_stream = -1;
    first_input_audio_stream = -1;
    
    first_output_video_coder_open=false;
    first_output_audio_coder_open=false;
    first_input_video_coder_open=false;
    first_input_audio_coder_open=false;

    packetOffset = 0;
  }

  Helper :: ~Helper()
  {
    reset();
    if (expected_codec_ids)
      delete [] expected_codec_ids;
    if (expected_codec_types)
      delete [] expected_codec_types;
    VS_LOG_TRACE("Helper destroyed");
  }

  void
  Helper :: reset()
  {
    int retval=-1;

    resetStreams();
    
    if (container_isopen)
    {
      VS_TUT_ENSURE("we think container is open but have no container",
          container);
      if (need_trailer_write)
      {
        retval = container->writeTrailer();
        VS_TUT_ENSURE("could not write trailer", retval >= 0);
      }
      retval = container->close();
      VS_TUT_ENSURE("could not close container", retval == 0);
      container_isopen = false;
    }
    container.reset();
    format.reset();
    packet.reset();

    VS_LOG_TRACE("Helper reset");
  }
  void
  Helper :: resetStreams()
  {
    VS_LOG_TRACE("Resetting streams");
    int i = 0;
    if (streams) {
      for(i = 0; i < num_streams; i++)
      {
        if (streams[i])
          // container and me
          VS_TUT_ENSURE_EQUALS("wrong ref count",
              streams[i]->getCurrentRefCount(),
              2);
      }
      delete [] streams;
      streams = 0;
      num_streams = 0;
    }
    if (coders) {
      if (first_output_audio_coder_open)
        coders[first_output_audio_stream]->close();
      if (first_output_video_coder_open)
        coders[first_output_video_stream]->close();
      if (first_input_audio_coder_open)
        coders[first_input_audio_stream]->close();
      if (first_input_video_coder_open)
        coders[first_input_video_stream]->close();
      
      for(i = 0; i < num_coders; i++)
      {
        if (coders[i])
          // should just be the helper and the stream
          VS_TUT_ENSURE_EQUALS("wrong ref count",
              coders[i]->getCurrentRefCount(),
              2);
      }
      delete [] coders;
      coders = 0;
      num_coders = 0;
    }
    if (codecs) {
      for(i = 0; i < num_codecs; i++)
      {
        // should just be the helper and the coder,
        // but it's possible the coder doesn't hold
        // this anymore because someone set a different
        // codec.
        if (codecs[i])
          VS_TUT_ENSURE("wrong ref count",
              codecs[i]->getCurrentRefCount() >= 1 &&
              codecs[i]->getCurrentRefCount() <= 2);
      }
      delete [] codecs;
      codecs = 0;
      num_codecs = 0;
    }
    if (vresamplers)
    {
      delete [] vresamplers;
      vresamplers=0;
      num_vresamplers = 0;
    }
    if (aresamplers)
    {
      delete [] aresamplers;
      aresamplers=0;
      num_aresamplers = 0;
    }
    first_output_video_stream = -1;
    first_output_audio_stream = -1;
    first_input_video_stream = -1;
    first_input_audio_stream = -1;
    VS_LOG_TRACE("Helper Streams reset");
  }
  void
  Helper :: setupReading(const char* url)
  {
    int retval = -1;
    char actualUrl[4096];
    VS_TUT_ENSURE("invalid url", url);
    VS_TUT_ENSURE("invalid url", *url != 0);
    if (*url == '/' || *url == '\\')
    {
      // assume this is an absolute path
      snprintf(actualUrl, sizeof(actualUrl), "%s", url);
    } else {
      snprintf(actualUrl, sizeof(actualUrl), "%s/%s", FIXTURE_DIRECTORY, url);
    }

    std::string errorStr("");

    // free any old helper values
    reset();

    format = IContainerFormat::make();
    VS_TUT_ENSURE("could not get format", format);

    packet = IPacket::make();
    VS_TUT_ENSURE("could not get packet", packet);

    container = IContainer::make();
    VS_TUT_ENSURE("could not get container", container);
    VS_TUT_ENSURE("no one else has this container",
        container->getCurrentRefCount() == 1);

    {
      LoggerStack stack;
      stack.setGlobalLevel(Logger::LEVEL_DEBUG, false);
      retval = container->open(actualUrl, IContainer::READ, 0);
    }
    errorStr="could not open url: ";
    errorStr.append(actualUrl);
    VS_TUT_ENSURE(errorStr, retval >= 0);
    container_isopen = true;

    errorStr="no streams in url: ";
    errorStr.append(actualUrl);
    VS_TUT_ENSURE(errorStr, container->getNumStreams() > 0);
    allocateStreams(container->getNumStreams());
  }
  void
  Helper :: allocateStreams(int aNumStreams)
  {
    VS_LOG_TRACE("Allocating streams: %d streams", aNumStreams);
    resetStreams();
    int i = 0;

    VS_TUT_ENSURE("no streams",aNumStreams > 0);
    streams = new RefPointer<IStream>[aNumStreams];
    VS_TUT_ENSURE("could not allocate streams", streams);
    num_streams = aNumStreams;

    coders = new RefPointer<IStreamCoder>[aNumStreams];
    VS_TUT_ENSURE("could not allocate coders", coders);
    num_coders = aNumStreams;

    codecs = new RefPointer<ICodec>[aNumStreams];
    VS_TUT_ENSURE("could not allocate codecs", codecs);
    num_codecs = aNumStreams;


    // Before we start allocating streams and friends,
    // the container should ONLY have one person referring
    // to it... me
    // and it better finish that way!
    VS_TUT_ENSURE_EQUALS("invalid ref count for container",
        container->getCurrentRefCount(),
        1);
    for(i = 0; i<aNumStreams; i++)
    {
      VS_LOG_TRACE("Getting the stream: %d", i);
      streams[i] = container->getStream(i);
      VS_TUT_ENSURE("could not get stream", streams[i]);

      // container and me
      VS_TUT_ENSURE_EQUALS("invalid ref count",
          streams[i].value()->getCurrentRefCount(), 2);

      VS_LOG_TRACE("Getting the coder: %d", i);

      coders[i] = streams[i]->getStreamCoder();
      VS_TUT_ENSURE("could not get stream coder", coders[i]);

      // container and me
      VS_TUT_ENSURE_EQUALS("invalid ref count: streams",
          streams[i].value()->getCurrentRefCount(), 2);

      // stream and me
      VS_TUT_ENSURE_EQUALS("invalid ref count: coders",
          coders[i].value()->getCurrentRefCount(), 2);

      VS_LOG_TRACE("Getting the codec: %d", i);
      codecs[i] = coders[i]->getCodec();
      if (coders[i]->getCodecType() == ICodec::CODEC_TYPE_AUDIO ||
          coders[i]->getCodecType() == ICodec::CODEC_TYPE_VIDEO)
      {
        VS_TUT_ENSURE("could not get codec", codecs[i]);

        // container and me
        VS_TUT_ENSURE_EQUALS("invalid ref count: streams",
            streams[i].value()->getCurrentRefCount(), 2);

        // stream and me
        VS_TUT_ENSURE_EQUALS("invalid ref count: coders",
            coders[i].value()->getCurrentRefCount(), 2);

        // coder and me
        VS_TUT_ENSURE_EQUALS("invalid ref count: codecs",
            codecs[i].value()->getCurrentRefCount(), 2);

        if (codecs[i]->getType() == ICodec::CODEC_TYPE_AUDIO)
        {
          if (streams[i]->getDirection() == IStream::INBOUND)
          {
            if (first_input_audio_stream < 0)
              first_input_audio_stream = i;
          } else
          {
            if (first_output_audio_stream < 0)
              first_output_audio_stream = i;

          }
        } else if (codecs[i]->getType() == ICodec::CODEC_TYPE_VIDEO)
        {
          if (streams[i]->getDirection() == IStream::INBOUND)
          {
            if (first_input_video_stream < 0)
              first_input_video_stream = i;
          } else
          {
            if (first_output_video_stream < 0)
              first_output_video_stream = i; 
          }        
        }
      }
    }
    // The streams, coders and codecs must NOT hold
    // references to the container or a ref-counting
    // loop results and memory is leaked.
    // this checks for that loop.
    VS_TUT_ENSURE_EQUALS("invalid ref count for container",
        container->getCurrentRefCount(),
        1);
    // error check here
    VS_TUT_ENSURE_EQUALS("forgot to set num_streams", aNumStreams, num_streams);
    VS_TUT_ENSURE_EQUALS("forgot to set num_coders", aNumStreams, num_coders);
    VS_LOG_TRACE("Streams allocated: %d streams", aNumStreams);
  }

  void
  Helper :: setupWriting(
      const char* url,
      const char* videoCodec,
      const char* audioCodec,
      const char* formatStr,
      IPixelFormat::Type pixelformat,
      int width,
      int height,
      bool allocateVideoResamplers,
      int sampleRate,
      int channels,
      bool allocateAudioResamplers)
  {
    // Turn down logging because Ffmpeg has gotten noisy...
    LoggerStack stack;
    stack.setGlobalLevel(Logger::LEVEL_DEBUG, false);

    int retval = -1;
    std::string errorStr("");
    int numStreamsAdded = 0;
    int i = 0;

    // free any old helper values
    reset();

    format = IContainerFormat::make();
    VS_TUT_ENSURE("could not get format", format);

    retval = format->setOutputFormat(formatStr, url, 0);
    VS_TUT_ENSURE("Could not find output format", retval >= 0);

    packet = IPacket::make();
    VS_TUT_ENSURE("could not get packet", packet);

    container = IContainer::make();
    VS_TUT_ENSURE("could not get container", container);
    VS_TUT_ENSURE("no one else has this container",
        container->getCurrentRefCount() == 1);

    retval = container->open(url, IContainer::WRITE, format.value());
    errorStr="could not open url: ";
    errorStr.append(url);
    VS_TUT_ENSURE(errorStr, retval >= 0);
    container_isopen = true;

    VS_TUT_ENSURE_EQUALS("streams already?", container->getNumStreams(),
        0);
    const char * codecStrs[2];
    codecStrs[0] = videoCodec;
    codecStrs[1] = audioCodec;
    for (i = 0; i < 2; i++)
    {
      const char* codecStr = codecStrs[i];
      if (codecStr && *codecStr)
      {
        RefPointer<IStream> newstream;
        RefPointer<IStreamCoder> newcoder;
        RefPointer<ICodec> newcodec;
        RefPointer<IRational> timebase;
        RefPointer<IRational> framerate;

        newcodec = ICodec::findEncodingCodecByName(codecStr);
        VS_TUT_ENSURE("could not find codec", newcodec);

        newstream = container->addNewStream(i);
        VS_TUT_ENSURE("could not add stream", newstream);

        newcoder = newstream->getStreamCoder();
        VS_TUT_ENSURE("could not get a coder", newcoder);

        // And put some default settings into the coder
        newcoder->setCodec(newcodec.value());
        if (newcodec->getType() == ICodec::CODEC_TYPE_VIDEO)
        {
          if (width > 0)
            newcoder->setWidth(width);
          if (height > 0)
            newcoder->setHeight(height);
          if (pixelformat != IPixelFormat::NONE)
            newcoder->setPixelType(pixelformat);

          // technically these should be passed into the writer.  Fix
          // this later if needed.
          timebase = IRational::make(expected_time_base);
          framerate = IRational::make(expected_frame_rate);
          newcoder->setNumPicturesInGroupOfPictures(expected_gops);
          newcoder->setFrameRate(framerate.value());
          newcoder->setTimeBase(timebase.value());
          // Set a very high quality bit_rate for the tests.
          newcoder->setBitRate(720000);
          newcoder->setFlags(newcoder->getFlags() | IStreamCoder::FLAG_QSCALE);

          // Set the quality of the output to be as good
          // as the input
          newcoder->setGlobalQuality(0);
          
        } else if (newcodec->getType() == ICodec::CODEC_TYPE_AUDIO)
        {
          if (channels > 0)
            newcoder->setChannels(channels);
          if (sampleRate > 0)
            newcoder->setSampleRate(sampleRate);
        }

        // We can let all references drop because the container
        // should be keeping them alive now.  Let's check for that.

        // me and container
        VS_TUT_ENSURE_EQUALS("bad ref count",
            newstream->getCurrentRefCount(),
            2);

        // me and stream
        VS_TUT_ENSURE_EQUALS("bad ref count",
            newcoder->getCurrentRefCount(),
            2);

        // me and coder
        VS_TUT_ENSURE_EQUALS("bad ref count",
            newcodec->getCurrentRefCount(),
            2);

        numStreamsAdded++;
      }
    }
    // now, allocate the streams
    if (container->getNumStreams())
      allocateStreams(container->getNumStreams());

    if (videoCodec && *videoCodec)
      VS_TUT_ENSURE("no video stream", first_output_video_stream >= 0);
    if (audioCodec && *audioCodec)
      VS_TUT_ENSURE("no audio stream", first_output_audio_stream >= 0);

    // and make sure we have the right number of streams.
    VS_TUT_ENSURE_EQUALS("wrong # of streams",
        container->getNumStreams(),
        numStreamsAdded);
    VS_TUT_ENSURE_EQUALS("wrong # of streams",
        num_streams,
        numStreamsAdded);
    VS_TUT_ENSURE_EQUALS("wrong # of coders",
        num_coders,
        numStreamsAdded);
    VS_TUT_ENSURE_EQUALS("wrong # of codecs",
        num_codecs,
        numStreamsAdded);

    if (allocateVideoResamplers && numStreamsAdded)
    {
      vresamplers = new RefPointer<IVideoResampler>[numStreamsAdded];
      VS_TUT_ENSURE("alloc failure", vresamplers);
      num_vresamplers = numStreamsAdded;
    }
    if (allocateAudioResamplers && numStreamsAdded)
    {
      aresamplers = new RefPointer<IAudioResampler>[numStreamsAdded];
      VS_TUT_ENSURE("alloc failure", aresamplers);
      num_aresamplers = numStreamsAdded;
    }
    for(i=0; i < numStreamsAdded; i++)
    {
      RefPointer<IStreamCoder> oc(0);

      oc = coders[i];
      if (oc->getCodecType() == ICodec::CODEC_TYPE_AUDIO)
      {
        if (vresamplers) vresamplers[i] = 0;
        if (aresamplers)
          aresamplers[i] = IAudioResampler::make(channels,
              expected_channels, sampleRate, expected_sample_rate);
        VS_TUT_ENSURE("", !vresamplers || !vresamplers[i]);
        VS_TUT_ENSURE("", !aresamplers || aresamplers[i]);
      }
      else if (oc->getCodecType() == ICodec::CODEC_TYPE_VIDEO)
      {
        if (vresamplers)
          vresamplers[i] = IVideoResampler::make(
              width,
              height,
              pixelformat,
              expected_width,
              expected_height,
              expected_pixel_format);
        if (aresamplers)
          aresamplers[i] = 0;        
        VS_TUT_ENSURE("", !vresamplers || vresamplers[i]);
        VS_TUT_ENSURE("", !aresamplers || !aresamplers[i]);
      }
      else
      {
        if (vresamplers)
          vresamplers[i] = 0;
        if (aresamplers)
          aresamplers[i] = 0;
      }
    }
  }
  void
  Helper :: setupWriting(const char*url,
      const char* videoCodec,
      const char* audioCodec,
      const char* formatStr)
  {
    setupWriting(url,
        videoCodec,
        audioCodec,
        formatStr,
        expected_pixel_format,
        expected_width,
        expected_height,
        false,
        expected_sample_rate,
        expected_channels,
        false);
  }

  void
  Helper :: decodeAndEncode(
      Helper & h,
      Helper & hw,
      int maxSecondsToEncode,
      void (*frameListener)(void* context, IVideoPicture* pOutFrame, IVideoPicture* pInFrame),
      void *frameListenerCtx,
      void (*sampleListener)(void* context, IAudioSamples* pOutSamples, IAudioSamples* pInSamples),
      void *sampleListenerCtx
  )
  {
    RefPointer<IAudioSamples> samples = 0;
    RefPointer<IAudioSamples> outSamples = 0;
    RefPointer<IVideoPicture> frame = 0;
    RefPointer<IVideoPicture> outFrame = 0;
    RefPointer<IRational> num(0);
    RefPointer<IStreamCoder> ic(0);
    RefPointer<IStreamCoder> oc(0);
    RefPointer<IAudioResampler> ar(0);
    RefPointer<IVideoResampler> vr(0);

    int numSamples = 0;
    int numPackets = 0;
    int numFrames = 0;
    int numKeyFrames = 0;

    int maxFrames = 0;
    int maxSamples = 0;

    int retval = -1;

    VS_TUT_ENSURE("it appears setupReading and setupWriting were not called",
        h.streams &&
        h.coders &&
        h.codecs &&
        hw.streams &&
        hw.coders &&
        hw.codecs);

    RefPointer<IPacket> packet = IPacket::make();

    RefPointer<IPacket> opacket = IPacket::make();
    VS_TUT_ENSURE("! opacket", opacket);

    if (h.first_input_audio_stream >= 0 &&
        hw.first_output_audio_stream >= 0) {
      // Let's set up audio first.
      ic = h.coders[h.first_input_audio_stream];
      oc = hw.coders[hw.first_output_audio_stream];

      // Set the output coder correctly.
      oc->setBitRate(ic->getBitRate());

      samples = IAudioSamples::make(1024, ic->getChannels());
      VS_TUT_ENSURE("got no samples", samples);
      outSamples = IAudioSamples::make(1024, oc->getChannels());
      VS_TUT_ENSURE("got no samples", outSamples);
      retval = ic->open();
      VS_TUT_ENSURE("Could not open input coder", retval >= 0);
      retval = oc->open();
      VS_TUT_ENSURE("Could not open output coder", retval >= 0);

      if (maxSecondsToEncode > 0)
      {
        maxSamples = maxSecondsToEncode * ic->getSampleRate();
      }
    }
    if (h.first_input_video_stream >= 0 &&
        hw.first_output_video_stream >= 0){
      // now, let's set up video.
      ic = h.coders[h.first_input_video_stream];
      oc = hw.coders[hw.first_output_video_stream];

      num = ic->getFrameRate();
      oc->setFrameRate(num.value());
      if (maxSecondsToEncode > 0)
      {
        maxFrames = (int)(maxSecondsToEncode * num->getDouble());
      }
      num = ic->getTimeBase();
      oc->setTimeBase(num.value());
      oc->setNumPicturesInGroupOfPictures(
          ic->getNumPicturesInGroupOfPictures()
      );

      frame = IVideoPicture::make(
          ic->getPixelType(),
          ic->getWidth(),
          ic->getHeight());
      VS_TUT_ENSURE("got no frame", frame);

      outFrame = IVideoPicture::make(
          oc->getPixelType(),
          oc->getWidth(),
          oc->getHeight());
      VS_TUT_ENSURE("got no frame", outFrame);

      retval = ic->open();
      VS_TUT_ENSURE("Could not open input coder", retval >= 0);
      retval = oc->open();
      VS_TUT_ENSURE("Could not open output coder", retval >= 0);
    }

    // write header
    retval = hw.container->writeHeader();
    VS_TUT_ENSURE("could not write header", retval >= 0);

    while (h.container->readNextPacket(packet.value()) == 0)
    {
      if (packet->getStreamIndex() == h.first_input_audio_stream
          && hw.first_output_audio_stream >= 0)
      {
        int offset = 0;
        int streamIndex = packet->getStreamIndex();
        ic = h.coders[streamIndex];
        oc = hw.coders[hw.first_output_audio_stream];
        if (hw.aresamplers)
          ar = hw.aresamplers[hw.first_output_audio_stream];
        if (hw.vresamplers)
          vr = hw.vresamplers[hw.first_output_audio_stream];


        numPackets++;

        while (offset < packet->getSize())
        {
          retval = ic->decodeAudio(
              samples.value(),
              packet.value(),
              offset);
          VS_TUT_ENSURE("could not decode any audio",
              retval > 0);
          offset += retval;
          numSamples += samples->getNumSamples();

          if (maxSamples > 0 && numSamples > maxSamples)
            goto done_encoding;

          // now, write out the packets.
          RefPointer<IAudioSamples> encodeSamples(0);

          if (sampleListener)
          {
            // create a new outSamples
            sampleListener(sampleListenerCtx,
                outSamples.value(), samples.value());
            encodeSamples = outSamples;
          } else if (ar)
          {
            retval = ar->resample(outSamples.value(), samples.value(), 0);
            VS_TUT_ENSURE("could not resample audio", retval >= 0);
            encodeSamples = outSamples;
          }
          else
          {
            // Use the insamples
            encodeSamples = samples;
          }
          unsigned int numSamplesConsumed = 0;
          unsigned int numSamplesAvailable = encodeSamples->getNumSamples();
          
          while (numSamplesConsumed < numSamplesAvailable)
          {

            
            retval = oc->encodeAudio(opacket.value(), encodeSamples.value(),
                numSamplesConsumed);
            VS_TUT_ENSURE("Could not encode any audio", retval > 0);
            numSamplesConsumed += retval;

            if (opacket->isComplete())
            {
              VS_TUT_ENSURE("could not encode audio", opacket->getSize() > 0);
              RefPointer<IBuffer> encodedBuffer = opacket->getData();
              VS_TUT_ENSURE("no encoded data", encodedBuffer);
              VS_TUT_ENSURE("less data than there should be",
                  encodedBuffer->getBufferSize() >=
                  opacket->getSize());

              // now, write the packet to disk.
              retval = hw.container->writePacket(opacket.value());
              VS_TUT_ENSURE("could not write packet", retval >= 0);
            }
          }
        }
      } else if (packet->getStreamIndex() == h.first_input_video_stream &&
          hw.first_output_video_stream >= 0)
      {
        int offset = 0;
        int streamIndex = packet->getStreamIndex();
        ic = h.coders[streamIndex];
        oc = hw.coders[hw.first_output_video_stream];
        if (hw.aresamplers)
          ar = hw.aresamplers[hw.first_output_video_stream];
        if (hw.vresamplers)
          vr = hw.vresamplers[hw.first_output_video_stream];
        numPackets++;

        VS_LOG_TRACE("packet: pts: %ld; dts: %ld", packet->getPts(),
            packet->getDts());
        while (offset < packet->getSize())
        {

          retval = ic->decodeVideo(
              frame.value(),
              packet.value(),
              offset);
          VS_TUT_ENSURE("could not decode any video", retval>0);
          num = ic->getTimeBase();
          VS_TUT_ENSURE_EQUALS("time base changed", num->getDouble(), h.expected_time_base);
          offset += retval;
          if (frame->isComplete())
          {
            numFrames++;
            if (frame->isKeyFrame())
              numKeyFrames++;

            if (maxFrames > 0 && numFrames > maxFrames)
              goto done_encoding;

            RefPointer<IVideoPicture> encodeFrame(0);

            if (frameListener)
            {
              // create a new outSamples
              frameListener(frameListenerCtx,
                  outFrame.value(), frame.value());
              encodeFrame = outFrame;
            } else if (vr)
            {
              retval = vr->resample(outFrame.value(), frame.value());
              VS_TUT_ENSURE("could not resample video", retval >= 0);
              encodeFrame = outFrame;
            }
            else
            {
              encodeFrame = frame;
            }


            encodeFrame->setQuality(0);
            retval = oc->encodeVideo(
                opacket.value(),
                encodeFrame.value(),
                0);

            VS_TUT_ENSURE("could not encode video", retval >= 0);
            VS_TUT_ENSURE("could not encode video", opacket->isComplete());

            VS_TUT_ENSURE("could not encode video", opacket->getSize() > 0);

            RefPointer<IBuffer> encodedBuffer = opacket->getData();
            VS_TUT_ENSURE("no encoded data", encodedBuffer);
            VS_TUT_ENSURE("less data than there should be",
                encodedBuffer->getBufferSize() >=
                opacket->getSize());

            // now, write the packet to disk.
            retval = hw.container->writePacket(opacket.value());
            VS_TUT_ENSURE("could not write packet", retval >= 0);
          }
        }
      }
    }
    done_encoding:

    if (hw.first_output_audio_stream >= 0)
    {
      oc = hw.coders[hw.first_output_audio_stream];
      ic = h.coders[h.first_input_audio_stream];

      // sigh; it turns out that to flush the encoding buffers you need to
      // ask the encoder to encode a NULL set of samples.  So, let's do that.
      retval = oc->encodeAudio(opacket.value(), 0, 0);
      VS_TUT_ENSURE("Could not encode any audio", retval >= 0);
      if (retval > 0)
      {
        retval = hw.container->writePacket(opacket.value());
        VS_TUT_ENSURE("could not write packet", retval >= 0);
      }
      retval = ic->close();
      VS_TUT_ENSURE("! close", retval >= 0);
      retval = oc->close();
      VS_TUT_ENSURE("! close", retval >= 0);
    }
    if (hw.first_output_video_stream >= 0)
    {
      oc = hw.coders[hw.first_output_video_stream];
      ic = h.coders[h.first_input_video_stream];

      retval = oc->encodeVideo(opacket.value(), 0, 0);
      VS_TUT_ENSURE("Could not encode any video", retval >= 0);
      if (retval > 0)
      {
        retval = hw.container->writePacket(opacket.value());
        VS_TUT_ENSURE("could not write packet", retval >= 0);
      }
      retval = ic->close();
      VS_TUT_ENSURE("! close", retval >= 0);
      retval = oc->close();
      VS_TUT_ENSURE("! close", retval >= 0);
    }
    retval = hw.container->writeTrailer();
    VS_TUT_ENSURE("! writeTrailer", retval >= 0);
  }
  
  int
  Helper :: readNextDecodedObject(IAudioSamples* pOutSamples, IVideoPicture* pOutFrame)
  {
    int retval = -1;
    VS_TUT_ENSURE("must get one of samples or frame",
        pOutSamples || pOutFrame);
    
    VS_TUT_ENSURE("either don't ask for audio, or have a stream",
        !pOutFrame || first_input_video_stream >= 0);
    if (pOutFrame && first_input_video_stream >= 0 && !first_input_video_coder_open)
    {
      retval = coders[first_input_video_stream]->open();
      VS_TUT_ENSURE("could not open video codec", retval >= 0);
      first_input_video_coder_open = true;
    }
    VS_TUT_ENSURE("either don't ask for audio, or have a stream",
        !pOutSamples || first_input_audio_stream >= 0);
    if (pOutSamples && first_input_audio_stream >= 0 && !first_input_audio_coder_open)
    {
      retval = coders[first_input_audio_stream]->open();
      VS_TUT_ENSURE("could not open audio codec", retval >= 0);
      first_input_audio_coder_open = true;
    }
    
    bool gotValue = false;
    bool isAudio = false;
    RefPointer<IStreamCoder> ic;
    
    // Reuse the packet if we still have data in it, otherwise get a new one. 
    while (!gotValue && 
        (packetOffset < packet->getSize() || 
            container->readNextPacket(packet.value()) == 0)
        )
    {
      int streamIndex = packet->getStreamIndex();
      ic = coders[streamIndex];

      if (streamIndex == first_input_audio_stream &&
          pOutSamples)
      {
        while (packetOffset < packet->getSize() && !gotValue)
        {
          retval = ic->decodeAudio(
              pOutSamples,
              packet.value(),
              packetOffset);
          VS_TUT_ENSURE("could not decode any audio",
              retval > 0);
          packetOffset += retval;
          if (pOutSamples->isComplete())
          {
            gotValue = 1;
            isAudio = true;
          }
        }
      }
      else if (streamIndex == first_input_video_stream &&
          pOutFrame)
      {
        while (packetOffset < packet->getSize() && !gotValue)
        {
          retval = ic->decodeVideo(
              pOutFrame,
              packet.value(),
              packetOffset);
          VS_TUT_ENSURE("could not decode any video",
              retval >= 0);
          if (retval == 0)
          {
            // for some reason we couldn't deocde a frame;
            // just break and try again.
            packetOffset = 0;
            packet->reset();
          }
          else
          {
            packetOffset += retval;
            if (pOutFrame->isComplete())
            {
              pOutFrame->setQuality(0); // set this to the best quality for now.
              gotValue = 1;
              isAudio = false;
            }
          }
        }
      }
      else
      {
        // skip it.
        packetOffset = 0;
        packet->reset();
      }

      if (packetOffset >= packet->getSize())
      {
        // we've gone over the end; start at the begining
        packetOffset = 0;
        packet->reset();
      }
      

    }
    if (!gotValue) {
      // assume end of file
      retval = 0;
    } else {
      if (isAudio)
        retval = 1;
      else
        retval = 2;
    }

    return retval;
  }
  
  int
  Helper :: writeFrame(IVideoPicture* pInFrame)
  {
    int retval = -1;
    VS_TUT_ENSURE("must pass value to write", pInFrame);
    
    VS_TUT_ENSURE("no video stream to write to",
        first_output_video_stream >= 0);

    if (!first_output_video_coder_open)
    {
      retval = coders[first_output_video_stream]->open();
      VS_TUT_ENSURE("could not open video codec", retval >= 0);
      first_output_video_coder_open = true;
      if (!first_output_audio_coder_open)
      {
        // We're first coder to open; write a header.
        retval = container->writeHeader();
        VS_TUT_ENSURE("could not write header", retval >= 0);
        need_trailer_write = true;
      }
    }
    
    // Now, we should be good to go.
    RefPointer<IStreamCoder> oc = coders[first_output_video_stream];

    retval = oc->encodeVideo(
        packet.value(),
        pInFrame,
        0);

    VS_TUT_ENSURE("could not encode video", retval >= 0);
    VS_TUT_ENSURE("could not encode video", packet->isComplete());

    VS_TUT_ENSURE("could not encode video", packet->getSize() > 0);

    RefPointer<IBuffer> encodedBuffer = packet->getData();
    VS_TUT_ENSURE("no encoded data", encodedBuffer);
    VS_TUT_ENSURE("less data than there should be",
        encodedBuffer->getBufferSize() >=
        packet->getSize());

    // now, write the packet to disk.
    retval = container->writePacket(packet.value());
    VS_TUT_ENSURE("could not write packet", retval >= 0);
    
    return retval;
  }
  
  int
  Helper :: writeSamples(IAudioSamples *pInSamples)
  {
    int retval = -1;
    VS_TUT_ENSURE("must pass value to write", pInSamples);
    
    VS_TUT_ENSURE("no audio stream to write to",
        first_output_audio_stream >= 0);

    if (!first_output_audio_coder_open)
    {
      retval = coders[first_output_audio_stream]->open();
      VS_TUT_ENSURE("could not open audio codec", retval >= 0);
      first_output_audio_coder_open = true;
      if (!first_output_video_coder_open)
      {
        // We're first coder to open; write a header.
        retval = container->writeHeader();
        VS_TUT_ENSURE("could not write header", retval >= 0);
        need_trailer_write = true;
      }
    }
    
    // Now, we should be good to go.
    RefPointer<IStreamCoder> oc = coders[first_output_audio_stream];

    unsigned int numSamplesConsumed = 0;
    unsigned int numSamplesAvailable = pInSamples->getNumSamples();
    
    while (numSamplesConsumed < numSamplesAvailable)
    {
      retval = oc->encodeAudio(packet.value(), pInSamples,
          numSamplesConsumed);
      VS_TUT_ENSURE("Could not encode any audio", retval > 0);
      numSamplesConsumed += retval;

      if (packet->isComplete())
      {
        VS_TUT_ENSURE("could not encode audio", packet->getSize() > 0);
        RefPointer<IBuffer> encodedBuffer = packet->getData();
        VS_TUT_ENSURE("no encoded data", encodedBuffer);
        VS_TUT_ENSURE("less data than there should be",
            encodedBuffer->getBufferSize() >=
            packet->getSize());

        // now, write the packet to disk.
        retval = container->writePacket(packet.value());
        VS_TUT_ENSURE("could not write packet", retval >= 0);
      }
    }
    return retval;
  }
}}}
