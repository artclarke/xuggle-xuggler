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

// for strncpy
#include <string.h>

#include <com/xuggle/ferry/Logger.h>

#include <com/xuggle/xuggler/Container.h>
#include <com/xuggle/xuggler/ContainerFormat.h>
#include <com/xuggle/xuggler/ContainerParameters.h>
#include <com/xuggle/xuggler/Stream.h>
#include <com/xuggle/xuggler/Packet.h>
#include <com/xuggle/xuggler/Global.h>
#include <com/xuggle/xuggler/Property.h>

VS_LOG_SETUP(VS_CPP_PACKAGE);

using namespace com::xuggle::ferry;

namespace com { namespace xuggle { namespace xuggler
{

  Container :: Container()
  {
    VS_LOG_TRACE("Making container: %p", this);
    mFormatContext = 0;
    mIsOpened = false;
    mIsMetaDataQueried=false;
    mNeedTrailerWrite = false;
    mNumStreams = 0;
    mInputBufferLength = 0;
    mReadRetryCount = 1;
    mParameters = IContainerParameters::make();
  }

  Container :: ~Container()
  {
    reset();
    VS_LOG_TRACE("Destroyed container: %p", this);
  }

  void
  Container :: reset()
  {
    if (mIsOpened)
    {
      VS_LOG_WARN("Closing dangling Container");
      (void) this->close();
    }
    VS_ASSERT(!mFormatContext,
        "this should be freed by close or already zero");
  }

  AVFormatContext *
  Container :: getFormatContext()
  {
    return mFormatContext;
  }

  int32_t
  Container :: setInputBufferLength(uint32_t size)
  {
    int32_t retval = -1;
    if (mIsOpened)
    {
      VS_LOG_WARN("Attempting to set input buffer length while file is opened; ignoring");
    }
    else
    {
      mInputBufferLength = size;
      retval = size;
    }
    return retval;
  }

  uint32_t
  Container :: getInputBufferLength()
  {
    return mInputBufferLength;
  }

  bool
  Container :: isOpened()
  {
    return mIsOpened;
  }

  bool
  Container :: isHeaderWritten()
  {
    return (mIsOpened && mNeedTrailerWrite);
  }

  int32_t
  Container :: open(const char *url,Type type,
      IContainerFormat *pContainerFormat)
  {
    return open(url, type, pContainerFormat, false, true);
  }

  int32_t
  Container :: open(const char *url,Type type,
      IContainerFormat *pContainerFormat,
      bool aStreamsCanBeAddedDynamically,
      bool aLookForAllStreams)
  {
    int32_t retval = -1;

    // reset if an open is called before a close.
    reset();

    if (url && *url)
    {
      if (type == WRITE)
      {
        retval = openOutputURL(url, pContainerFormat);
      } else if (type == READ)
      {
        retval = openInputURL(url, pContainerFormat, aStreamsCanBeAddedDynamically, aLookForAllStreams);
      }
      else
      {
        VS_ASSERT(false, "Invalid type for open");
        retval = -1;
      }
    }
    return retval;
  }

  IContainerFormat*
  Container :: getContainerFormat()
  {
    ContainerFormat *retval = ContainerFormat::make();
    if (retval)
    {
      if (mFormatContext)
      {
        if (mFormatContext->iformat)
          retval->setInputFormat(mFormatContext->iformat);
        if (mFormatContext->oformat)
          retval->setOutputFormat(mFormatContext->oformat);
      }
    }
    return retval;
  }

  int32_t
  Container:: setupAllInputStreams()
  {
    // do nothing if we're already all set up.
    if (mNumStreams == mFormatContext->nb_streams)
      return 0;

    int32_t retval = -1;
    // loop through and find the first non-zero time base
    AVRational *goodTimebase = 0;
    for(uint32_t i = 0;i < mFormatContext->nb_streams;i++){
      AVStream *avStream = mFormatContext->streams[i];
      if(avStream && avStream->time_base.num && avStream->time_base.den){
        goodTimebase = &avStream->time_base;
        break;
      }
    }

    // Only look for new streams
    for (uint32_t i =mNumStreams ; i < mFormatContext->nb_streams; i++)
    {
      AVStream *avStream = mFormatContext->streams[i];
      if (avStream)
      {
        if (goodTimebase && (!avStream->time_base.num || !avStream->time_base.den))
        {
          avStream->time_base = *goodTimebase;
        }

        RefPointer<Stream>* stream = new RefPointer<Stream>(
            Stream::make(this, avStream,
                (this->getType() == READ ?
                    IStream::INBOUND : IStream::OUTBOUND
                ))
        );

        if (stream)
        {
          if (stream->value())
          {
            mStreams.push_back(stream);
            mNumStreams++;
          } else {
            VS_LOG_ERROR("Couldn't make a stream %d", i);
            delete stream;
          }
          stream = 0;
        }
        else
        {
          VS_LOG_ERROR("Could not make Stream %d", i);
          retval = -1;
        }
      } else {
        VS_LOG_ERROR("no FFMPEG allocated stream: %d", i);
        retval = -1;
      }
    }
    return retval;
  }

  int32_t
  Container :: openInputURL(const char *url,
      IContainerFormat* pContainerFormat,
      bool aStreamsCanBeAddedDynamically,
      bool aLookForAllStreams)
  {
    int32_t retval = -1;
    AVInputFormat *inputFormat = 0;
    ContainerFormat *containerFormat = dynamic_cast<ContainerFormat*>(pContainerFormat);
    AVFormatParameters* params = 0;
    ContainerParameters* cParams = dynamic_cast<ContainerParameters*>(mParameters.value());
    if (cParams)
      params = cParams->getAVParameters();

    if (containerFormat)
    {
      inputFormat = containerFormat->getInputFormat();
    } else {
      inputFormat = 0;
    }
    retval = av_open_input_file(&mFormatContext, url, inputFormat, mInputBufferLength, params);
    if (retval >= 0) {
      mIsOpened = true;
      if (aStreamsCanBeAddedDynamically)
      {
        mFormatContext->ctx_flags |= AVFMTCTX_NOHEADER;
      }

      if (aLookForAllStreams)
      {
        retval = queryStreamMetaData();
      }
    } else {
      VS_LOG_TRACE("Could not open output file: %s", url);
    }
    return retval;
  }

  int32_t
  Container :: openOutputURL(const char* url,
      IContainerFormat* pContainerFormat)
  {
    int32_t retval = -1;
    AVOutputFormat *outputFormat = 0;
    ContainerFormat *containerFormat = dynamic_cast<ContainerFormat*>(pContainerFormat);

    if (containerFormat)
    {
      // ask for it from the container
      outputFormat = containerFormat->getOutputFormat();
    } else {
      // guess it.
      outputFormat = guess_format(0, url, 0);
    }
    if (outputFormat)
    {
      mFormatContext = avformat_alloc_context();
      if (mFormatContext)
      {
        mFormatContext->oformat = outputFormat;

        retval = url_fopen(&mFormatContext->pb, url, URL_WRONLY);
        if (retval >= 0) {
          mIsOpened = true;
          strncpy(mFormatContext->filename, url, sizeof(mFormatContext->filename)-1);
        } else {
          // failed to open; kill the context.
          av_free(mFormatContext);
          mFormatContext = 0;
        }
      } else {
        VS_LOG_ERROR("Could not allocate format context");
      }
    }
    else
    {
      // bad output format
      VS_LOG_ERROR("Could not find output format");
    }
    return retval;
  }

  IContainer::Type
  Container :: getType()
  {
    return (!mFormatContext ? READ :
        (mFormatContext->oformat ? WRITE: READ));
  }

  int32_t
  Container :: getNumStreams()
  {
    int32_t retval = 0;
    if (mFormatContext)
    {
      if (mFormatContext->nb_streams != mNumStreams)
        setupAllInputStreams();
      retval = mFormatContext->nb_streams;
    }
    return retval;
  }

  IStream*
  Container :: addNewStream(int32_t id)
  {
    Stream *retval=0;
    if (mFormatContext && mIsOpened)
    {
      AVStream * stream=0;
      stream = av_new_stream(mFormatContext, id);
      if (stream)
      {
        RefPointer<Stream>* p = new RefPointer<Stream>(
            Stream::make(this, stream, IStream::OUTBOUND)
        );
        if (p)
        {
          if (*p)
          {
            mStreams.push_back(p);
            mNumStreams++;
            retval = p->get(); // acquire for caller
          }
          else
          {
            VS_LOG_ERROR("could not allocate Stream");
            delete p;
          }
          p = 0;
        }
        else
        {
          VS_LOG_ERROR("could not allocate RefPointer");
          VS_REF_RELEASE(retval);
        }
      }
      else
      {
        VS_LOG_ERROR("could not allocated Stream");
      }
    }
    return retval;
  }

  int32_t
  Container :: close()
  {
    int32_t retval = -1;
    if (mFormatContext && mIsOpened)
    {
      VS_ASSERT(mNumStreams == mStreams.size(),
          "unexpected number of streams");
      VS_ASSERT(mNumStreams == mFormatContext->nb_streams,
          "Container and FormatContext out of sync");

      if (mNeedTrailerWrite)
      {
        VS_LOG_WARN("Writing dangling trailer");
        (void) this->writeTrailer();
      }
      mOpenCoders.clear();

      while(mStreams.size() > 0)
      {
        RefPointer<Stream> * stream=mStreams.back();

        VS_ASSERT(stream && *stream, "no stream?");
        if (stream && *stream) {
          (*stream)->containerClosed(this);
          delete stream;
        }
        mStreams.pop_back();
      }
      mNumStreams = 0;

      if (this->getType() == READ)
      {
        av_close_input_file(mFormatContext);
        retval = 0;
      } else
      {
        retval = url_fclose(mFormatContext->pb);
        av_free(mFormatContext);
      }
      mFormatContext = 0;
      mIsOpened = false;
      mIsMetaDataQueried=false;
    }
    return retval;
  }

  IStream *
  Container :: getStream(uint32_t position)
  {
    Stream *retval = 0;
    if (mFormatContext)
    {
      if (mFormatContext->nb_streams != mNumStreams)
        setupAllInputStreams();

      if (position < mNumStreams)
      {
        // will acquire for caller.
        retval = (*mStreams.at(position)).get();
      }
    }
    return retval;
  }

  int32_t
  Container :: readNextPacket(IPacket * ipkt)
  {
    int32_t retval = -1;
    Packet* pkt = dynamic_cast<Packet*>(ipkt);
    if (mFormatContext && pkt)
    {
      AVPacket tmpPacket;
      AVPacket* packet=0;

      packet = &tmpPacket;
      av_init_packet(packet);
      pkt->reset();
      int32_t numReads=0;
      do
      {
        retval = av_read_frame(mFormatContext,
            packet);
        ++numReads;
      }
      while (retval == AVERROR(EAGAIN) &&
          (mReadRetryCount < 0 || numReads <= mReadRetryCount));

      if (retval >= 0)
        pkt->wrapAVPacket(packet);
      av_free_packet(packet);

      // Get a pointer to the wrapped packet
      packet = pkt->getAVPacket();
      // and dump it's contents
      VS_LOG_TRACE("read-packet: %lld, %lld, %d, %d, %d, %lld, %lld: %p",
          pkt->getDts(),
          pkt->getPts(),
          pkt->getFlags(),
          pkt->getStreamIndex(),
          pkt->getSize(),
          pkt->getDuration(),
          pkt->getPosition(),
          packet->data);
      
      // and let's try to set the packet time base if known
      if (pkt->getStreamIndex() >= 0)
      {
        RefPointer<IStream> stream = this->getStream(pkt->getStreamIndex());
        if (stream)
        {
          RefPointer<IRational> streamBase = stream->getTimeBase();
          if (streamBase)
          {
            pkt->setTimeBase(streamBase.value());
          }
        }
      }
    }
    return retval;
  }
  int32_t
  Container :: writePacket(IPacket *ipkt)
  {
    return writePacket(ipkt, true);
  }
  int32_t
  Container :: writePacket(IPacket *ipkt, bool forceInterleave)
  {
    int32_t retval = -1;
    Packet *pkt = dynamic_cast<Packet*>(ipkt);
    try
    {
      if (this->getType() != WRITE)
        throw std::runtime_error("cannot write packet to read only container");
      
      if (!mFormatContext)
        throw std::logic_error("no format context");

      if (!pkt)
        throw std::runtime_error("cannot write missing packet");

      if (!pkt->isComplete())
        throw std::runtime_error("cannot write incomplete packet");

      if (!pkt->getSize())
        throw std::runtime_error("cannot write empty packet");

      if (!mNeedTrailerWrite)
        throw std::runtime_error("container has not written header yet");

      if ((uint32_t)pkt->getStreamIndex() >= mNumStreams)
        throw std::runtime_error("packet being written to stream that doesn't exist");

      AVPacket *packet = 0;
      packet = pkt->getAVPacket();
      if (!packet || !packet->data)
        throw std::runtime_error("no data in packet");
      /*
      VS_LOG_DEBUG("write-packet: %lld, %lld, %d, %d, %d, %lld, %lld: %p",
          pkt->getDts(),
          pkt->getPts(),
          pkt->getFlags(),
          pkt->getStreamIndex(),
          pkt->getSize(),
          pkt->getDuration(),
          pkt->getPosition(),
          packet->data);
          */
      if (forceInterleave)
        retval =  av_interleaved_write_frame(mFormatContext, packet);
      else
        retval = av_write_frame(mFormatContext, packet);
    }
    catch (std::exception & e)
    {
      VS_LOG_ERROR("Error: %s", e.what());
      retval = -1;
    }
    return retval;
  }

  int32_t
  Container :: writeHeader()
  {
    int32_t retval = -1;
    try {
      if (this->getType() != WRITE)
        throw std::runtime_error("cannot write packet to read only container");

      if (!mFormatContext)
        throw std::runtime_error("no format context allocated");

#ifdef VS_DEBUG
      // for shits and giggles, dump the ffmpeg output
      // dump_format(mFormatContext, 0, (mFormatContext ? mFormatContext->filename :0), 1);
#endif // VS_DEBUG

      // This checks to make sure that parameters are set correctly.
      retval = av_set_parameters(mFormatContext, 0);
      if (retval < 0)
        throw std::runtime_error("incorrect parameters set on container");

      // now we're going to walk through and record each open stream coder.
      // this is needed to catch a potential error on writeTrailer().
      mOpenCoders.clear();
      int numStreams = getNumStreams();
      if (numStreams <= 0)
        throw std::runtime_error("no streams added to container");
      
      for(int i = 0; i < numStreams; i++)
      {
        RefPointer<IStream> stream = this->getStream(i);
        if (stream)
        {
          RefPointer<IStreamCoder> coder = stream->getStreamCoder();
          if (coder)
          {
            if (coder->isOpen())
              // add to our open list
              mOpenCoders.push_back(coder);
            else
              VS_LOG_WARN("writing Header for container, but at least one codec (%d) is not opened first", i);
          }
        }
      }
      retval = av_write_header(mFormatContext);
      if (retval < 0)
        throw std::runtime_error("could not write header for container");

      // force a flush.
      put_flush_packet(mFormatContext->pb);
      // and remember that a writeTrailer is needed
      mNeedTrailerWrite = true;
    }
    catch (std::exception & e)
    {
      VS_LOG_ERROR("Error: %s", e.what());
      retval = -1;
    }
    return retval;
  }
  int32_t
  Container :: writeTrailer()
  {
    int32_t retval = -1;
    try
    {
      if (this->getType() != WRITE)
        throw std::runtime_error("cannot write packet to read only container");

      if (!mFormatContext)
        throw std::runtime_error("no format context allocated");

      if (mNeedTrailerWrite)
      {
        while(mOpenCoders.size()>0)
        {
          RefPointer<IStreamCoder> coder = mOpenCoders.front();
          mOpenCoders.pop_front();
          if (!coder->isOpen())
          {
            mOpenCoders.clear();
            throw std::runtime_error("attempt to write trailer, but at least one used codec already closed");
          }
        }
        retval = av_write_trailer(mFormatContext);
        if (retval == 0)
        {
          put_flush_packet(mFormatContext->pb);
        }
      } else {
        VS_LOG_WARN("writeTrailer() with no matching call to writeHeader()");
      }
    }
    catch (std::exception & e)
    {
      VS_LOG_ERROR("Error: %s", e.what());
      retval = -1;
    }
    // regardless of whether or not the write trailer succeeded, since
    // an attempt has occurred, we shouldn't call it twice.
    mNeedTrailerWrite = false;

    return retval;
  }

  int32_t
  Container :: queryStreamMetaData()
  {
    int retval = -1;
    if (mIsOpened)
    {
      if (!mIsMetaDataQueried)
      {
        retval = av_find_stream_info(mFormatContext);
        // for shits and giggles, dump the ffmpeg output
        // dump_format(mFormatContext, 0, (mFormatContext ? mFormatContext->filename :0), 0);
        mIsMetaDataQueried = true;
      } else {
        retval = 0;
      }

      if (retval >= 0 && mFormatContext->nb_streams > 0)
      {
        setupAllInputStreams();
      } else {
        VS_LOG_WARN("Could not find streams in input container");
      }
    }
    else
    {
      VS_LOG_WARN("Attempt to queryStreamMetaData but container is not open");
    }
    return retval;
  }

  int32_t
  Container :: seekKeyFrame(int streamIndex, int64_t timestamp, int32_t flags)
  {
    int32_t retval = -1;

    if (mIsOpened)
    {
      if (streamIndex < 0 || (uint32_t)streamIndex >= mNumStreams)
        VS_LOG_WARN("Attempt to seek on streamIndex %d but only %d streams known about in container",
            streamIndex, mNumStreams);
      else
        retval = av_seek_frame(mFormatContext, streamIndex, timestamp, flags);
    }
    else
    {
      VS_LOG_WARN("Attempt to seekKeyFrame but container is not open");
    }
    return retval;
  }

  int64_t
  Container :: getDuration()
  {
    int64_t retval = Global::NO_PTS;
    queryStreamMetaData();
    if (mFormatContext)
      retval = mFormatContext->duration;
    return retval;
  }

  int64_t
  Container :: getStartTime()
  {
    int64_t retval = Global::NO_PTS;
    queryStreamMetaData();
    if (mFormatContext)
      retval = mFormatContext->start_time;
    return retval;
  }

  int64_t
  Container :: getFileSize()
  {
    int64_t retval = -1;
    queryStreamMetaData();
    if (mFormatContext)
      retval = mFormatContext->file_size;
    return retval;
  }

  int32_t
  Container :: getBitRate()
  {
    int32_t retval = -1;
    queryStreamMetaData();
    if (mFormatContext)
      retval = mFormatContext->bit_rate;
    return retval;
  }

  int32_t
  Container :: getNumProperties()
  {
    return Property::getNumProperties(mFormatContext);
  }

  IProperty*
  Container :: getPropertyMetaData(int32_t propertyNo)
  {
    return Property::getPropertyMetaData(mFormatContext, propertyNo);
  }

  IProperty*
  Container :: getPropertyMetaData(const char *name)
  {
    return Property::getPropertyMetaData(mFormatContext, name);
  }

  int32_t
  Container :: setProperty(const char* aName, const char *aValue)
  {
    return Property::setProperty(mFormatContext, aName, aValue);
  }

  int32_t
  Container :: setProperty(const char* aName, double aValue)
  {
    return Property::setProperty(mFormatContext, aName, aValue);
  }

  int32_t
  Container :: setProperty(const char* aName, int64_t aValue)
  {
    return Property::setProperty(mFormatContext, aName, aValue);
  }

  int32_t
  Container :: setProperty(const char* aName, bool aValue)
  {
    return Property::setProperty(mFormatContext, aName, aValue);
  }


  int32_t
  Container :: setProperty(const char* aName, IRational *aValue)
  {
    return Property::setProperty(mFormatContext, aName, aValue);
  }


  char*
  Container :: getPropertyAsString(const char *aName)
  {
    return Property::getPropertyAsString(mFormatContext, aName);
  }

  double
  Container :: getPropertyAsDouble(const char *aName)
  {
    return Property::getPropertyAsDouble(mFormatContext, aName);
  }

  int64_t
  Container :: getPropertyAsLong(const char *aName)
  {
    return Property::getPropertyAsLong(mFormatContext, aName);
  }

  IRational*
  Container :: getPropertyAsRational(const char *aName)
  {
    return Property::getPropertyAsRational(mFormatContext, aName);
  }

  bool
  Container :: getPropertyAsBoolean(const char *aName)
  {
    return Property::getPropertyAsBoolean(mFormatContext, aName);
  }

  int32_t
  Container :: getFlags()
  {
    return (mFormatContext ? mFormatContext->flags: 0);
  }

  void
  Container :: setFlags(int32_t newFlags)
  {
    if (mFormatContext)
      mFormatContext->flags = newFlags;
  }

  bool
  Container :: getFlag(IContainer::Flags flag)
  {
    bool result = false;
    if (mFormatContext)
      result = mFormatContext->flags& flag;
    return result;
  }

  void
  Container :: setFlag(IContainer::Flags flag, bool value)
  {
    if (mFormatContext)
    {
      if (value)
      {
        mFormatContext->flags |= flag;
      }
      else
      {
        mFormatContext->flags &= (~flag);
      }
    }

  }
  
  const char*
  Container :: getURL()
  {
    return mFormatContext && *mFormatContext->filename ? mFormatContext->filename : 0;
  }
  
  int32_t
  Container :: flushPackets()
  {
    int32_t retval = -1;
    try
    {
      if (this->getType() != WRITE)
        throw std::runtime_error("cannot write packet to read only container");

      if (!mFormatContext)
        throw std::runtime_error("no format context allocated");

      // Do the flush
      put_flush_packet(mFormatContext->pb);
      retval = 0;
    }
    catch (std::exception & e)
    {
      VS_LOG_ERROR("Error: %s", e.what());
      retval = -1;
    }

    return retval;
  }

  int32_t
  Container :: getReadRetryCount()
  {
    return mReadRetryCount;
  }
  
  void
  Container :: setReadRetryCount(int32_t aCount)
  {
    mReadRetryCount = aCount;
  }
  
  IContainerParameters*
  Container :: getParameters()
  {
    return mParameters.get();
  }
  
  void
  Container :: setParameters(IContainerParameters* params)
  {
    if (params)
      mParameters.reset(params, true);
  }
  
}}}
