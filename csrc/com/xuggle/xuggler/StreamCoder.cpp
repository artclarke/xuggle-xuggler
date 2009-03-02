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
#include <stdexcept>
#include <cstring>

#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/ferry/RefPointer.h>

#include <com/xuggle/xuggler/Global.h>
#include <com/xuggle/xuggler/StreamCoder.h>
#include <com/xuggle/xuggler/Codec.h>
#include <com/xuggle/xuggler/Rational.h>
#include <com/xuggle/xuggler/AudioSamples.h>
#include <com/xuggle/xuggler/VideoPicture.h>
#include <com/xuggle/xuggler/Packet.h>
#include <com/xuggle/xuggler/Property.h>

extern "C" {
#include <libavcodec/opt.h>
}

VS_LOG_SETUP(VS_CPP_PACKAGE);

namespace com {
namespace xuggle {
namespace xuggler {
using namespace com::xuggle::ferry;

StreamCoder :: StreamCoder() :
  mCodec(0)
{
  mCodecContext = 0;
  // default to DECODING.
  mDirection = DECODING;
  mOpened = false;
  mStream = 0;
  mAudioFrameBuffer = 0;
  mBytesInFrameBuffer = 0;
  mFakePtsTimeBase = IRational::make(1, AV_TIME_BASE);
  mFakeNextPts = 0;
  mFakeCurrPts = 0;
  mSamplesDecoded = 0;
  mLastValidAudioTimeStamp = Global::NO_PTS;
  mStartingTimestampOfBytesInFrameBuffer = Global::NO_PTS;
  mDefaultAudioFrameSize = 576;
}

StreamCoder :: ~StreamCoder()
{
  reset();
}

void
StreamCoder :: reset()
{
  // Auto-close if the caller forgot to close this
  // stream-coder
  if (mOpened)
  {
    VS_LOG_WARN("Closing dangling StreamCoder");
    (void) this->close();
  }

  mOpened = false;
  if (mCodecContext && (!mStream || mDirection != DECODING))
  {
    // Don't free if we're decoding; the Container
    // will do that.

    // This is a fix for the leak introduced here in ffmpeg:
    // http://lists.mplayerhq.hu/pipermail/ffmpeg-devel/2008-July/049805.html
    // This happened somewhere around r14487 for ffmpeg
    av_free((char*) mCodecContext->rc_eq);
    av_free(mCodecContext);
  }
  mCodecContext = 0;
  // We do not refcount the stream
  mStream = 0;
}

StreamCoder *
StreamCoder :: make(Direction direction)
{
  StreamCoder *retval = 0;
  AVCodecContext* codecCtx = avcodec_alloc_context();

  // set encoding defaults
  int32_t flags = 0;
  if (direction == IStreamCoder::ENCODING)
    flags |= AV_OPT_FLAG_ENCODING_PARAM;
  else
    flags |= AV_OPT_FLAG_DECODING_PARAM;
  
  av_opt_set_defaults2(codecCtx, flags, flags);
  
  if (codecCtx)
  {
    retval = StreamCoder::make();
    if (retval)
    {
      retval->mCodecContext = codecCtx;
      retval->mDirection = direction;
      retval->mStream = 0;
      retval->mCodecContext->codec_id = CODEC_ID_PROBE; // Tell FFMPEG we don't know, but to probe to find out

      // Set the private data to this object.
      codecCtx->opaque = retval;
    }
  }
  return retval;
}

StreamCoder *
StreamCoder :: make(Direction direction, AVCodecContext * codecCtx,
    Stream* stream)
{
  StreamCoder *retval = 0;
  if (codecCtx)
  {
    retval = StreamCoder::make();
    if (retval)
    {
      retval->mCodecContext = codecCtx;
      retval->mDirection = direction;
      retval->mStream = stream; //do not ref count.

      // Set the private data to this object.
      codecCtx->opaque = retval;
    }
  }
  return retval;
}

IStream*
StreamCoder :: getStream()
{
  // Acquire for the caller.
  VS_REF_ACQUIRE(mStream);
  return mStream;
}

ICodec *
StreamCoder :: getCodec()
{
  ICodec *retval = 0;
  if (!mCodec && mCodecContext)
  {
    // Try getting it from the context
    setCodec(mCodecContext->codec_id);
  }
  if (mCodec)
    retval = mCodec.get();

  return retval;
}

ICodec::Type
StreamCoder :: getCodecType()
{
  ICodec::Type retval = ICodec::CODEC_TYPE_UNKNOWN;
  if (mCodecContext) {
    retval = (ICodec::Type)mCodecContext->codec_type;
  } else {
    VS_LOG_WARN("Attempt to get CodecType from uninitialized StreamCoder");
  }
  return retval;
}

ICodec::ID
StreamCoder :: getCodecID()
{
  ICodec::ID retval = ICodec::CODEC_ID_NONE;
  if (mCodecContext)
  {
    retval = (ICodec::ID)mCodecContext->codec_id;
  } else {
    VS_LOG_WARN("Attempt to get CodecID from uninitialized StreamCoder");
  }
  return retval;
}

void
StreamCoder :: setCodec(ICodec * newCodec)
{
  // reset our code and acquire the one passed in
  if (mCodec.value() != newCodec)
  {
    mCodec.reset(dynamic_cast<Codec*> (newCodec), true);
    // Check that the new codec can do the right
    // thing
    if (mCodec && mCodecContext && !mOpened)
    {
      mCodecContext->codec_id = (enum CodecID) mCodec->getIDAsInt();
      mCodecContext->codec_type = (enum CodecType) mCodec->getType();
    }
    else
    {
      VS_LOG_INFO("Cannot set Codec because StreamCoder is either not initialized or already open");
      mCodec = 0;
    }
  }
}

void
StreamCoder :: setCodec(ICodec::ID id)
{
  setCodec((int32_t) id);
}

void
StreamCoder :: setCodec(int32_t id)
{
  ICodec *codec = 0;
  if (ENCODING == mDirection)
  {
    codec = Codec::findEncodingCodecByIntID(id);
  }
  else
  {
    codec = Codec::findDecodingCodecByIntID(id);
  }
  if (codec)
  {
    setCodec(codec);
    // release it because the set above should get
    // the permanent reference.
    codec->release();
    codec = 0;
  }
}

int32_t
StreamCoder :: getBitRate()
{
  return (mCodecContext ? mCodecContext->bit_rate : -1);
}
void
StreamCoder :: setBitRate(int32_t val)
{
  if (mCodecContext && !mOpened)
    mCodecContext->bit_rate = val;
}
int32_t
StreamCoder :: getBitRateTolerance()
{
  return (mCodecContext ? mCodecContext->bit_rate_tolerance : -1);
}
void
StreamCoder :: setBitRateTolerance(int32_t val)
{
  if (mCodecContext && !mOpened)
    mCodecContext->bit_rate_tolerance = val;
}
int32_t
StreamCoder :: getHeight()
{
  return (mCodecContext ? mCodecContext->height : -1);
}

void
StreamCoder :: setHeight(int32_t val)
{
  if (mCodecContext && !mOpened)
    mCodecContext->height = val;
}

int32_t
StreamCoder :: getWidth()
{
  return (mCodecContext ? mCodecContext->width : -1);
}

void
StreamCoder :: setWidth(int32_t val)
{
  if (mCodecContext && !mOpened)
    mCodecContext->width = val;
}

IRational*
StreamCoder :: getTimeBase()
{
  // we make a new value and return it; caller must
  // release.
  IRational *retval = 0;

  // annoyingly, some codec contexts will NOT have
  // a timebase... so we take it from stream then.
  if (mCodecContext && mCodecContext->time_base.den
      && mCodecContext->time_base.num)
  {
    retval = Rational::make(&mCodecContext->time_base);
  }
  else
  {
    retval = mStream ? mStream->getTimeBase() : 0;
  }

  return retval;
}

void
StreamCoder :: setTimeBase(IRational* src)
{
  if (mCodecContext && src && !mOpened)
  {
    mCodecContext->time_base.num = src->getNumerator();
    mCodecContext->time_base.den = src->getDenominator();
  }
  else
  {
    VS_LOG_INFO("Failed to setTimeBase on StreamCoder");
  }
}

int64_t
StreamCoder :: getNextPredictedPts()
{
  return mFakeNextPts;
}

IRational*
StreamCoder :: getFrameRate()
{
  return (mStream ? mStream->getFrameRate() : 0);
}

void
StreamCoder :: setFrameRate(IRational* src)
{
  if (mStream && !mOpened)
    mStream->setFrameRate(src);
}

int32_t
StreamCoder :: getNumPicturesInGroupOfPictures()
{
  return (mCodecContext ? mCodecContext->gop_size : -1);
}

void
StreamCoder :: setNumPicturesInGroupOfPictures(int32_t val)
{
  if (mCodecContext && !mOpened)
    mCodecContext->gop_size = val;
}

IPixelFormat::Type
StreamCoder :: getPixelType()
{
  IPixelFormat::Type retval = IPixelFormat::NONE;
  int32_t type = 0;
  if (mCodecContext)
  {
    retval = (IPixelFormat::Type) mCodecContext->pix_fmt;

    // little check here to see if we have an undefined int32_t.
    type = (int32_t) retval;
    if (type != mCodecContext->pix_fmt)
      retval = IPixelFormat::NONE;
  }
  return retval;
}

void
StreamCoder :: setPixelType(IPixelFormat::Type type)
{
  if (mCodecContext && !mOpened)
  {
    mCodecContext->pix_fmt = (enum PixelFormat) type;
  }
}

int32_t
StreamCoder :: getSampleRate()
{
  return (mCodecContext ? mCodecContext->sample_rate : -1);
}

void
StreamCoder :: setSampleRate(int32_t val)
{
  if (mCodecContext && !mOpened && val > 0)
    mCodecContext->sample_rate = val;
}

IAudioSamples::Format
StreamCoder :: getSampleFormat()
{
  return (IAudioSamples::Format)(mCodecContext ? mCodecContext->sample_fmt : -1);
}

void
StreamCoder :: setSampleFormat(IAudioSamples::Format val)
{
  if (mCodecContext && !mOpened && val > 0)
    mCodecContext->sample_fmt = (SampleFormat)val;
}

int32_t
StreamCoder :: getChannels()
{
  return (mCodecContext ? mCodecContext->channels : -1);
}

void
StreamCoder :: setChannels(int32_t val)
{
  if (mCodecContext && !mOpened && val > 0)
    mCodecContext->channels = val;
}

int32_t
StreamCoder :: getGlobalQuality()
{
  return (mCodecContext ? mCodecContext->global_quality : FF_LAMBDA_MAX);
}

void
StreamCoder :: setGlobalQuality(int32_t newQuality)
{
  if (newQuality < 0 || newQuality > FF_LAMBDA_MAX)
    newQuality = FF_LAMBDA_MAX;
  if (mCodecContext)
    mCodecContext->global_quality = newQuality;
}

int32_t
StreamCoder :: getFlags()
{
  return (mCodecContext ? mCodecContext->flags : 0);
}

void
StreamCoder :: setFlags(int32_t newFlags)
{
  if (mCodecContext)
    mCodecContext->flags = newFlags;
}

bool
StreamCoder :: getFlag(IStreamCoder::Flags flag)
{
  bool result = false;
  if (mCodecContext)
    result = mCodecContext->flags & flag;
  return result;
}

void
StreamCoder :: setFlag(IStreamCoder::Flags flag, bool value)
{
  if (mCodecContext)
  {
    if (value)
    {
      mCodecContext->flags |= flag;
    }
    else
    {
      mCodecContext->flags &= (~flag);
    }
  }

}

int32_t
StreamCoder :: open()
{
  int32_t retval = -1;
  try
  {
    if (!mCodecContext)
      throw std::runtime_error("no codec context");

    if (!mCodec)
    {
      RefPointer<ICodec> codec = this->getCodec();
      // This should set mCodec and then release
      // the local reference.
    }

    if (!mCodec)
      throw std::runtime_error("no codec set for coder");

    if ((mDirection == ENCODING && !mCodec->canEncode()) ||
        (mDirection == DECODING && !mCodec->canDecode()))
    {
      throw std::runtime_error("Codec not valid for direction StreamCoder is working in");
    }

    // Fix for issue #14: http://code.google.com/p/xuggle/issues/detail?id=14
    if (mStream)
    {
      RefPointer<IContainer> container = mStream->getContainer();
      if (container)
      {
        RefPointer<IContainerFormat> format = container->getContainerFormat();
        if (format && mDirection == ENCODING && format->getOutputFlag(IContainerFormat::FLAG_GLOBALHEADER))
        {
          this->setFlag(FLAG_GLOBAL_HEADER, true);
        }
      }
    }

    // Ffmpeg doesn't like it if multiple threads try to open a codec at the same time.
    Global::lock();
    retval = avcodec_open(mCodecContext, mCodec->getAVCodec());
    Global::unlock();

    if (retval < 0)
      throw std::runtime_error("could not open codec");
    mOpened = true;

    mFakeCurrPts = mFakeNextPts = mSamplesDecoded = mLastValidAudioTimeStamp
        = 0;

    // Do any post open initialization here.
    if (this->getCodecType() == ICodec::CODEC_TYPE_AUDIO)
    {
      int32_t frame_bytes = getAudioFrameSize() * getChannels()
          * IAudioSamples::findSampleBitDepth((IAudioSamples::Format) mCodecContext->sample_fmt) / 8;
      if (frame_bytes <= 0)
        frame_bytes = AVCODEC_MAX_AUDIO_FRAME_SIZE;

      if (!mAudioFrameBuffer || mAudioFrameBuffer->getBufferSize()
          < frame_bytes)
        // Re-create it.
        mAudioFrameBuffer = IBuffer::make(this, frame_bytes);
      mBytesInFrameBuffer = 0;
      mStartingTimestampOfBytesInFrameBuffer = Global::NO_PTS;
    }
  }
  catch (std::exception & e)
  {
    VS_LOG_DEBUG("Error: %s", e.what());
    retval = -1;
  }
  return retval;
}

int32_t
StreamCoder :: close()
{
  int32_t retval = -1;
  if (mCodecContext && mOpened)
  {
    // Ffmpeg doesn't like it if multiple threads try to close a codec at the same time.
    Global::lock();
    retval = avcodec_close(mCodecContext);
    Global::unlock();
    mOpened = false;
  }
  mBytesInFrameBuffer = 0;
  mStartingTimestampOfBytesInFrameBuffer = Global::NO_PTS;
  return retval;
}

int32_t
StreamCoder :: decodeAudio(IAudioSamples *pOutSamples, IPacket *pPacket,
    int32_t startingByte)
{
  int32_t retval = -1;
  AudioSamples *samples = dynamic_cast<AudioSamples*> (pOutSamples);
  Packet* packet = dynamic_cast<Packet*> (pPacket);

  if (samples && packet && mCodecContext && mOpened && mDirection == DECODING && getCodecType() == ICodec::CODEC_TYPE_AUDIO)
  {
    int outBufSize = 0;
    int32_t inBufSize = 0;

    // reset the samples
    samples->setComplete(false, 0, getSampleRate(), getChannels(),
        (IAudioSamples::Format) mCodecContext->sample_fmt, Global::NO_PTS);

    outBufSize = samples->getMaxBufferSize();
    inBufSize = packet->getSize() - startingByte;

    if (inBufSize > 0 && outBufSize > 0)
    {
      RefPointer<IBuffer> buffer = packet->getData();
      uint8_t * inBuf = 0;
      int16_t * outBuf = 0;

      VS_ASSERT(buffer, "no buffer in packet!");
      if (buffer)
        inBuf = (uint8_t*) buffer->getBytes(startingByte, inBufSize);

      outBuf = samples->getRawSamples(0);

      VS_ASSERT(inBuf, "no in buffer");
      VS_ASSERT(outBuf, "no out buffer");
      if (outBuf && inBuf)
      {
        VS_LOG_TRACE("Attempting decodeAudio(%p, %p, %d, %p, %d);",
            mCodecContext,
            outBuf,
            outBufSize,
            inBuf,
            inBufSize);
        retval = avcodec_decode_audio2(mCodecContext, outBuf, &outBufSize,
            inBuf, inBufSize);
        VS_LOG_TRACE("Finished %d decodeAudio(%p, %p, %d, %p, %d);",
            retval,
            mCodecContext,
            outBuf,
            outBufSize,
            inBuf,
            inBufSize);
      }
      if (retval >= 0)
      {
        // outBufSize is an In-Out parameter
        if (outBufSize < 0)
          // this can happen for some MPEG decoders
          outBufSize = 0;

        IAudioSamples::Format format =
            (IAudioSamples::Format) mCodecContext->sample_fmt;
        int32_t bytesPerSample = (IAudioSamples::findSampleBitDepth(format) / 8
            * getChannels());
        int32_t numSamples = outBufSize / bytesPerSample;

        // The audio decoder doesn't set a PTS, so we need to manufacture one.
        RefPointer<IRational> timeBase = this->mStream ? this->mStream->getTimeBase() : 0;
        if (!timeBase) timeBase = this->getTimeBase();

        int64_t packetTs = packet->getPts();
        if (packetTs == Global::NO_PTS)
          packetTs = packet->getDts();

        if (packetTs != Global::NO_PTS)
        {
          // The packet had a valid stream, and a valid time base
          if (timeBase->getNumerator() != 0 && timeBase->getDenominator() != 0)
          {
            int64_t tsDelta = Global::NO_PTS;
            if (mFakeNextPts != Global::NO_PTS)
            {
              int64_t fakeTsInStreamTimeBase = Global::NO_PTS;
              // rescale our fake into the time base of stream
              fakeTsInStreamTimeBase = timeBase->rescale(mFakeNextPts, mFakePtsTimeBase.value());
              tsDelta = fakeTsInStreamTimeBase - packetTs;
            }

            // now, compare it to our internal value;  if our internally calculated value
            // is within 1 tick of the packet's time stamp (in the packet's time base),
            // then we're probably right;
            // otherwise, we should reset the stream's fake time stamp based on this
            // packet
            if (mFakeNextPts != Global::NO_PTS &&
                (tsDelta <= 1 && tsDelta >= -1))
            {
              // we're the right value; keep our fake next pts
            }
            else
            {
              // rescale to our internal timebase
              int64_t packetTsInMicroseconds = mFakePtsTimeBase->rescale(packetTs, timeBase.value());
              VS_LOG_TRACE("%p Gap in audio (%lld); Resetting calculated ts from %lld to %lld",
                  this,
                  tsDelta,
                  mFakeNextPts,
                  packetTsInMicroseconds);
              mLastValidAudioTimeStamp = packetTsInMicroseconds;
              mSamplesDecoded = 0;
              mFakeNextPts = mLastValidAudioTimeStamp;
            }
          }
        }
        // Use the last value of the next pts
        mFakeCurrPts = mFakeNextPts;
        // adjust our next Pts pointer
        if (numSamples > 0)
        {
          mSamplesDecoded += numSamples;
          mFakeNextPts = mLastValidAudioTimeStamp
              + IAudioSamples::samplesToDefaultPts(mSamplesDecoded, getSampleRate());
        }

        // copy the packet PTS
        samples->setComplete(numSamples > 0, numSamples, getSampleRate(),
            getChannels(), format, mFakeCurrPts);
      }
    }
  }
  return retval;
}

int32_t
StreamCoder :: decodeVideo(IVideoPicture *pOutFrame, IPacket *pPacket,
    int32_t byteOffset)
{
  int32_t retval = -1;
  VideoPicture* frame = dynamic_cast<VideoPicture*> (pOutFrame);
  Packet* packet = dynamic_cast<Packet*> (pPacket);
  if (frame && packet && mCodecContext && mOpened && mDirection == DECODING && getCodecType() == ICodec::CODEC_TYPE_VIDEO)
  {
    // reset the frame
    frame->setComplete(false, this->getPixelType(), -1,
        -1, mFakeCurrPts);

    AVFrame *avFrame = avcodec_alloc_frame();
    VS_ASSERT(avFrame, "missing AVFrame");
    if (avFrame)
    {
      RefPointer<IBuffer> buffer = packet->getData();
      int frameFinished = 0;
      int32_t inBufSize = 0;
      uint8_t * inBuf = 0;

      inBufSize = packet->getSize() - byteOffset;

      VS_ASSERT(buffer, "no buffer in packet?");
      if (buffer)
        inBuf = (uint8_t*) buffer->getBytes(byteOffset, inBufSize);

      VS_ASSERT(inBuf, "incorrect size or no data in packet");

      if (inBufSize > 0 && inBuf)
      {
        VS_LOG_TRACE("Attempting decodeVideo(%p, %p, %d, %p, %d);",
            mCodecContext,
            avFrame,
            frameFinished,
            inBuf,
            inBufSize);
        mCodecContext->reordered_opaque = packet->getPts();
        retval = avcodec_decode_video(mCodecContext, avFrame, &frameFinished,
            inBuf, inBufSize);
        VS_LOG_TRACE("Finished %d decodeVideo(%p, %p, %d, %p, %d);",
            retval,
            mCodecContext,
            avFrame,
            frameFinished,
            inBuf,
            inBufSize);
        if (frameFinished)
        {
          // copy FFMPEG's buffer into our buffer; don't try to get efficient
          // and reuse the buffer FFMPEG is using; in order to allow our
          // buffers to be thread safe, we must do a copy here.
          frame->copyAVFrame(avFrame, getWidth(), getHeight());
          RefPointer<IRational> timeBase = 0;
          timeBase = this->mStream ? this->mStream->getTimeBase() : 0;
          if (!timeBase) timeBase = this->getTimeBase();

          int64_t packetTs = avFrame->reordered_opaque;
          if (packetTs == Global::NO_PTS)
            packetTs = packet->getDts();

          if (packetTs != Global::NO_PTS)
          {
            if (timeBase->getNumerator() != 0)
            {
              // The decoder set a PTS, so we let it override us
              mFakeNextPts = mFakePtsTimeBase->rescale(packetTs,
                  timeBase.value());
            }
          }

          // Use the last value of the next pts
          mFakeCurrPts = mFakeNextPts;
          double frameDelay = av_rescale(timeBase->getNumerator(), AV_TIME_BASE,
              timeBase->getDenominator());
          frameDelay += avFrame->repeat_pict * (frameDelay*0.5);

          // adjust our next Pts pointer
          mFakeNextPts += (int64_t)frameDelay;
//          VS_LOG_TRACE("frame complete: %s; pts: %lld; packet ts: %lld; tb: %ld/%ld",
//              frameFinished ? "yes" : "no",
//              mFakeCurrPts,
//              packet ? packet->getDts() : 0,
//                  timeBase->getNumerator(), timeBase->getDenominator()
//                  );
        }
        frame->setComplete(frameFinished, this->getPixelType(),
            this->getWidth(), this->getHeight(), mFakeCurrPts);

      }
      av_free(avFrame);
    }
  }
  else
  {
    VS_LOG_WARN("Attempting to decode when not ready");
  }

  return retval;
}

int32_t
StreamCoder :: encodeVideo(IPacket *pOutPacket, IVideoPicture *pFrame,
    int32_t suggestedBufferSize)
{
  int32_t retval = -1;
  VideoPicture *frame = dynamic_cast<VideoPicture*> (pFrame);
  Packet *packet = dynamic_cast<Packet*> (pOutPacket);
  RefPointer<IBuffer> encodingBuffer;

  try
  {
    if (packet)
      packet->reset();
    
    if (getCodecType() != ICodec::CODEC_TYPE_VIDEO)
      throw std::runtime_error("Attempting to encode video with non video coder");

    if (frame && frame->getPixelType() != this->getPixelType())
    {
      throw std::runtime_error("frame is not of the same PixelType as this Coder expected");
    }

  if (mCodecContext && mOpened && mDirection == ENCODING && packet)
  {
    uint8_t* buf = 0;
    uint32_t bufLen = 0;

    // First, get the right buffer size.
    if (suggestedBufferSize <= 0)
    {
      if (frame)
      {
        suggestedBufferSize = frame->getSize();
      }
      else
      {
        suggestedBufferSize = avpicture_get_size(getPixelType(), getWidth(),
            getHeight());
      }
    }
    VS_ASSERT(suggestedBufferSize> 0, "no buffer size in input frame");
    suggestedBufferSize = FFMAX(suggestedBufferSize, FF_MIN_BUFFER_SIZE);

    retval = packet->allocateNewPayload(suggestedBufferSize);
    if (retval >= 0)
    {
      encodingBuffer = packet->getData();
    }
    if (encodingBuffer)
    {
      buf = (uint8_t*) encodingBuffer->getBytes(0, suggestedBufferSize);
      bufLen = encodingBuffer->getBufferSize();
    }

    if (buf && bufLen)
    {
      // Change the PTS in our frame to the timebase of the encoded stream
      RefPointer<IRational> thisTimeBase = getTimeBase();

      /*
       * We make a copy of the AVFrame object and explicitly copy
       * over the values that we know encoding cares about.
       *
       * This is because often some programs decode into an VideoPicture
       * and just want to pass that to encodeVideo.  If we just
       * leave the values that Ffmpeg had in the AVFrame during
       * decoding, strange errors can result.
       *
       * Plus, this forces us to KNOW what we're passing into the encoder.
       */
      AVFrame* avFrame = 0;

      if (frame)
      {
        avFrame = avcodec_alloc_frame();
        VS_ASSERT(avFrame, "Memory allocation problem");
        frame->fillAVFrame(avFrame);

        avFrame->pict_type = 0; // let the encoder choose what pict_type to use

        // convert into the time base that this coder wants
        // to output in
        avFrame->pts = thisTimeBase->rescale(frame->getPts(),
            mFakePtsTimeBase.value());
        /*
         VS_LOG_DEBUG("Rescaling ts: %lld to %lld (from base %d/%d to %d/%d)",
         srcFrame->pts,
         avFrame->pts,
         mFakePtsTimeBase->getNumerator(),
         mFakePtsTimeBase->getDenominator(),
         thisTimeBase->getNumerator(),
         thisTimeBase->getDenominator());
         */
      }

      VS_LOG_TRACE("Attempting encodeVideo(%p, %p, %d, %p)",
          mCodecContext,
          buf,
          bufLen,
          avFrame);
      retval = avcodec_encode_video(mCodecContext, buf, bufLen, avFrame);
      VS_LOG_TRACE("Finished %d encodeVideo(%p, %p, %d, %p)",
          retval,
          mCodecContext,
          buf,
          bufLen,
          avFrame);
      if (retval >= 0)
      {
        setPacketParameters(packet, retval, frame ? frame->getTimeStamp() : Global::NO_PTS);
      }
      if (avFrame)
        av_free(avFrame);
    }
  }
  else
  {
    VS_LOG_WARN("Attempting to encode when not ready");
  }
  } catch (std::exception & e)
  {
    VS_LOG_WARN("Got error: %s", e.what());
    retval = -1;
  }

  return retval;
}

int32_t
StreamCoder :: encodeAudio(IPacket * pOutPacket, IAudioSamples* pSamples,
    uint32_t startingSample)
{
  int32_t retval = -1;
  AudioSamples *samples = dynamic_cast<AudioSamples*> (pSamples);
  Packet *packet = dynamic_cast<Packet*> (pOutPacket);
  RefPointer<IBuffer> encodingBuffer;
  bool usingInternalFrameBuffer = false;

  try
  {
    if (!packet)
      throw std::invalid_argument("Invalid packet to encode to");
    // Zero out our packet
    packet->reset();

    if (!mCodecContext)
      throw std::runtime_error("StreamCoder not initialized properly");
    if (!mOpened)
      throw std::runtime_error("StreamCoder not open");
    if (!(mDirection == ENCODING))
      throw std::runtime_error("Attempting to encode while decoding");
    if (getCodecType() != ICodec::CODEC_TYPE_AUDIO)
      throw std::runtime_error("Attempting to encode audio with non audio coder");
    if (!mAudioFrameBuffer)
      throw std::runtime_error("Audio Frame Buffer not initialized");

    // First, how many bytes do we need to encode a packet?
    int32_t frameSize = 0;
    int32_t frameBytes = 0;
    int32_t availableSamples = (samples ? samples->getNumSamples() - startingSample
        : 0);
    int32_t samplesConsumed = 0;
    int16_t *avSamples = (samples ? samples->getRawSamples(startingSample) : 0);

    if (samples)
    {
      if (samples->getChannels() != getChannels())
        throw std::invalid_argument(
            "channels in sample do not match StreamCoder");
      if (samples->getSampleRate() != getSampleRate())
        throw std::invalid_argument(
            "sample rate in sample does not match StreamCoder");
    }
    if (availableSamples < 0 || (avSamples && availableSamples == 0))
      throw std::invalid_argument(
          "no bytes in buffer at specified starting sample");

    int32_t bytesPerSample = (samples ? samples->getSampleSize()
        : IAudioSamples::findSampleBitDepth((IAudioSamples::Format)mCodecContext->sample_fmt) / 8
            * getChannels());

    /*
     * This gets tricky; There may be more audio samples passed to
     * us than can fit in an audio frame, or there may be less.
     *
     * If less, we need to cache for a future call.
     *
     * If more, we need to just use what's needed, and let the caller
     * know the number of samples we used.
     *
     * What happens when we exhaust all audio, but we still don't
     * have enough to decode a frame?  answer: the caller passes us
     * NULL as the pInSamples, and we just silently drop the incomplete
     * frame.
     *
     * To simplify coding here (and hence open for optimization if
     * this ends up being a bottleneck), I always copy audio samples
     * into a frame buffer, and then only encode from the frame buffer
     * in this class.
     */
    frameSize = getAudioFrameSize();
    frameBytes = frameSize * bytesPerSample;

    // More error checking
    VS_ASSERT(frameBytes <= mAudioFrameBuffer->getBufferSize(),
        "did frameSize change from open?");
    if (frameBytes > mAudioFrameBuffer->getBufferSize())
      throw std::runtime_error("not enough memory in internal frame buffer");

    VS_ASSERT(mBytesInFrameBuffer <= frameBytes,
        "did frameSize change from open?");
    if (frameBytes < mBytesInFrameBuffer)
      throw std::runtime_error(
          "too many bytes left over in internal frame buffer");

    unsigned char * frameBuffer = (unsigned char*) mAudioFrameBuffer->getBytes(
        0, frameBytes);
    if (!frameBuffer)
      throw std::runtime_error("could not get internal frame buffer");

    int32_t bytesToCopyToFrameBuffer = frameBytes - mBytesInFrameBuffer;
    bytesToCopyToFrameBuffer = FFMIN(bytesToCopyToFrameBuffer,
        availableSamples*bytesPerSample);

    int64_t packetStartPts = Global::NO_PTS;

    if (avSamples)
    {
      if (mBytesInFrameBuffer == 0)
        // We have nothing in the frame buffer, so the input samples
        // timestamp will be the output
        mStartingTimestampOfBytesInFrameBuffer = 
          samples->getTimeStamp()+IAudioSamples::samplesToDefaultPts(startingSample, getSampleRate());

      // Set the packet to whatever we think the start of the frame is
      packetStartPts = mStartingTimestampOfBytesInFrameBuffer;
      
      if (availableSamples >= frameSize && mBytesInFrameBuffer == 0)
      {
        VS_LOG_TRACE("audioEncode: Using passed in buffer: %d, %d, %d",
            availableSamples, frameSize, mBytesInFrameBuffer);
        frameBuffer = (unsigned char*)avSamples;
        samplesConsumed = frameSize;
        usingInternalFrameBuffer = false;
      }
      else
      {
        VS_LOG_TRACE("audioEncode: Using internal buffer: %d, %d, %d",
            availableSamples, frameSize, mBytesInFrameBuffer);
        memcpy(frameBuffer + mBytesInFrameBuffer, avSamples,
            bytesToCopyToFrameBuffer);
        mBytesInFrameBuffer += bytesToCopyToFrameBuffer;
        samplesConsumed = bytesToCopyToFrameBuffer / bytesPerSample;
        retval = samplesConsumed;
        usingInternalFrameBuffer = true;
      }
    }
    else
    {
      // drop everything in the frame buffer, and instead
      // just pass a null buffer to the encoder.

      // this should happen when the caller passes NULL for the
      // input samples
      frameBuffer = 0;
      packetStartPts = mStartingTimestampOfBytesInFrameBuffer;
    }
    if (!frameBuffer || !usingInternalFrameBuffer || mBytesInFrameBuffer >= frameBytes)
    {
      // First, get the right buffer size.
      int32_t bufferSize = frameBytes;
      if (mCodecContext->codec->id == CODEC_ID_FLAC)
      {
        // FLAC audio for some reason gives an error if your output buffer isn't 
        // over double the frame size, so we fake it here.  This could be further optimized
        // to only require an exact number, but this math is simpler and will always
        // be large enough.
        bufferSize = (64 + getAudioFrameSize()*(bytesPerSample+1))*2;
      }
      VS_ASSERT(bufferSize> 0, "no buffer size in samples");
      retval = packet->allocateNewPayload(bufferSize);
      if (retval >= 0)
      {
        encodingBuffer = packet->getData();
      }

      uint8_t* buf = 0;

      if (encodingBuffer)
      {
        buf = (uint8_t*) encodingBuffer->getBytes(0, bufferSize);
      }
      if (buf && bufferSize)
      {
        VS_LOG_TRACE("Attempting encodeAudio(%p, %p, %d, %p)",
            mCodecContext,
            buf,
            bufferSize,
            frameBuffer);

        // This hack works around the fact that PCM's codecs
        // calculate samples from buffer length, and sets
        // the wrong frame size, but in
        // reality we're always passing in 2 byte samples.
        double pcmCorrection = 1.0;
        switch (mCodecContext->codec->id)
        {
          case CODEC_ID_PCM_S32LE:
          case CODEC_ID_PCM_S32BE:
          case CODEC_ID_PCM_U32LE:
          case CODEC_ID_PCM_U32BE:
            pcmCorrection = 2.0;
            break;
          case CODEC_ID_PCM_S24LE:
          case CODEC_ID_PCM_S24BE:
          case CODEC_ID_PCM_U24LE:
          case CODEC_ID_PCM_U24BE:
          case CODEC_ID_PCM_S24DAUD:
            pcmCorrection = 1.5;
            break;
          case CODEC_ID_PCM_S16LE:
          case CODEC_ID_PCM_S16BE:
          case CODEC_ID_PCM_U16LE:
          case CODEC_ID_PCM_U16BE:
            pcmCorrection = 1.0;
            break;
          case CODEC_ID_PCM_ALAW:
          case CODEC_ID_PCM_MULAW:
          case CODEC_ID_PCM_S8:
          case CODEC_ID_PCM_U8:
          case CODEC_ID_PCM_ZORK:
            pcmCorrection = 0.5;
            break;
          default:
            pcmCorrection = 1.0;
        }
        retval = avcodec_encode_audio(mCodecContext, buf, (int32_t) (bufferSize
            * pcmCorrection), (int16_t*) frameBuffer);
        VS_LOG_TRACE("Finished %d encodeAudio(%p, %p, %d, %p)",
            retval,
            mCodecContext,
            buf,
            bufferSize,
            frameBuffer);
        // regardless of what happened, nuke any data in our frame
        // buffer.
        mBytesInFrameBuffer = 0;
        if (retval >= 0)
        {
          setPacketParameters(packet, retval,
              packetStartPts);

          retval = samplesConsumed;
        }
        else
        {
          throw std::runtime_error("avcodec_encode_audio failed");
        }
        // Increment this for next time around in case null is passed in.
        mStartingTimestampOfBytesInFrameBuffer += IAudioSamples::samplesToDefaultPts(frameSize, getSampleRate());
      }
    }
  }
  catch (std::exception& e)
  {
    VS_LOG_DEBUG("error: %s", e.what());
    retval = -1;
  }

  return retval;
}

void
StreamCoder :: setPacketParameters(Packet * packet, int32_t size, int64_t srcTimestamp)
{
  int32_t keyframe = 0;
  int64_t pts = Global::NO_PTS;
  RefPointer<IRational> streamBase = mStream->getTimeBase();

  VS_ASSERT(mCodecContext->coded_frame, "No coded frame?");
  if (mCodecContext->coded_frame)
  {
    keyframe = mCodecContext->coded_frame->key_frame;
    if (streamBase && mCodecContext->coded_frame->pts != Global::NO_PTS)
    {
      RefPointer<IRational> coderBase = this->getTimeBase();
      // rescale from the encoder time base to the stream timebase
      pts = streamBase->rescale(mCodecContext->coded_frame->pts,
          coderBase.value());
    } else {
      // assume they're equal
      pts = mCodecContext->coded_frame->pts;
    }
  }
  // If for some reason FFMPEG's encoder didn't put a PTS on this
  // packet, we'll fake one.
  if (pts == Global::NO_PTS && srcTimestamp != Global::NO_PTS && mStream)
  {
    if (streamBase)
    {
      pts = streamBase->rescale(srcTimestamp, mFakePtsTimeBase.value());
    }
  }
  packet->setKeyPacket(keyframe);
  packet->setPts(pts);
  // for now, set the dts and pts to be the same
  packet->setDts(pts);
  packet->setStreamIndex(mStream ? mStream->getIndex() : -1);
  // We will sometimes encode some data, but have zero data to send.
  // in that case, mark the packet as incomplete so people don't
  // output it.
  packet->setComplete(size > 0, size);

}

int32_t
StreamCoder :: getAudioFrameSize()
{
  int32_t retval = 0;
  if (mCodec && mCodec->getType() == ICodec::CODEC_TYPE_AUDIO)
  {
    if (mCodecContext->frame_size <= 1)
    {
      // Rats; some PCM encoders give a frame size of 1, which is too
      //small.  We pick a more sensible value.
      retval = getDefaultAudioFrameSize();
    }
    else
    {
      retval = mCodecContext->frame_size;
    }
  }
  return retval;
}

int32_t
StreamCoder :: streamClosed(Stream*stream)
{
  int32_t retval = 0;
  (void) stream; // removes a warning because in release mode the assert is ignored
  VS_ASSERT(stream == mStream, "Should be closed on the same stream");
  reset();
  return retval;
}

// For ref-count debugging purposes.
int32_t
StreamCoder :: acquire()
{
  int32_t retval = 0;
  retval = RefCounted::acquire();
  VS_LOG_TRACE("Acquired %p: %d", this, retval);
  return retval;
}

int32_t
StreamCoder :: release()
{
  int32_t retval = 0;
  retval = RefCounted::release();
  VS_LOG_TRACE("Released %p: %d", this, retval);
  return retval;
}

int32_t
StreamCoder :: getCodecTag()
{
  return (mCodecContext ? mCodecContext->codec_tag : 0);
}

void
StreamCoder :: setCodecTag(int32_t tag)
{
  if (mCodecContext)
    mCodecContext->codec_tag = tag;
}

int32_t
StreamCoder :: getNumProperties()
{
  return Property::getNumProperties(mCodecContext);
}

IProperty*
StreamCoder :: getPropertyMetaData(int32_t propertyNo)
{
  return Property::getPropertyMetaData(mCodecContext, propertyNo);
}

IProperty*
StreamCoder :: getPropertyMetaData(const char *name)
{
  return Property::getPropertyMetaData(mCodecContext, name);
}

int32_t
StreamCoder :: setProperty(const char* aName, const char *aValue)
{
  return Property::setProperty(mCodecContext, aName, aValue);
}

int32_t
StreamCoder :: setProperty(const char* aName, double aValue)
{
  return Property::setProperty(mCodecContext, aName, aValue);
}

int32_t
StreamCoder :: setProperty(const char* aName, int64_t aValue)
{
  return Property::setProperty(mCodecContext, aName, aValue);
}

int32_t
StreamCoder :: setProperty(const char* aName, bool aValue)
{
  return Property::setProperty(mCodecContext, aName, aValue);
}


int32_t
StreamCoder :: setProperty(const char* aName, IRational *aValue)
{
  return Property::setProperty(mCodecContext, aName, aValue);
}


char*
StreamCoder :: getPropertyAsString(const char *aName)
{
  return Property::getPropertyAsString(mCodecContext, aName);
}

double
StreamCoder :: getPropertyAsDouble(const char *aName)
{
  return Property::getPropertyAsDouble(mCodecContext, aName);
}

int64_t
StreamCoder :: getPropertyAsLong(const char *aName)
{
  return Property::getPropertyAsLong(mCodecContext, aName);
}

IRational*
StreamCoder :: getPropertyAsRational(const char *aName)
{
  return Property::getPropertyAsRational(mCodecContext, aName);
}

bool
StreamCoder :: getPropertyAsBoolean(const char *aName)
{
  return Property::getPropertyAsBoolean(mCodecContext, aName);
}

bool
StreamCoder :: isOpen()
{
  return mOpened;
}

int32_t
StreamCoder :: getDefaultAudioFrameSize()
{
  return mDefaultAudioFrameSize;
}

void
StreamCoder :: setDefaultAudioFrameSize(int32_t aNewSize)
{
  if (aNewSize >0)
    mDefaultAudioFrameSize = aNewSize;
}
}}}
