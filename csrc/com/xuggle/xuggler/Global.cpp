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
#include <cmath>
#include <cstring>

#include <com/xuggle/ferry/JNIHelper.h>
#include <com/xuggle/ferry/Logger.h>
#include <com/xuggle/ferry/Mutex.h>


#include <com/xuggle/xuggler/Global.h>
#include <com/xuggle/xuggler/FfmpegIncludes.h>
#include <com/xuggle/xuggler/Container.h>
#include <com/xuggle/xuggler/ContainerFormat.h>
#include <com/xuggle/xuggler/Codec.h>
#include <com/xuggle/xuggler/Rational.h>
#include <com/xuggle/xuggler/AudioSamples.h>
#include <com/xuggle/xuggler/VideoPicture.h>
#include <com/xuggle/xuggler/AudioResampler.h>
#include <com/xuggle/xuggler/VideoResampler.h>
#include <com/xuggle/xuggler/MediaDataWrapper.h>

VS_LOG_SETUP(VS_CPP_PACKAGE);

namespace com { namespace xuggle { namespace xuggler
{
  using namespace com::xuggle::ferry;

  /*
   * This function will be called back by Ffmpeg anytime
   * it wants to log.  We then use it to dump
   * stuff into our own logs.
   */
  static void
  xuggler_log_callback(void* ptr, int level, const char* fmt, va_list va)
  {
    static Logger* ffmpegLogger = 0;
    AVClass* avc = ptr ? *(AVClass**)ptr : 0;

    if (!ffmpegLogger)
    {
      ffmpegLogger = Logger::getStaticLogger( "org.ffmpeg" );
    }

    Logger::Level logLevel = Logger::LEVEL_ERROR;
    if (level == AV_LOG_ERROR ||
        level == AV_LOG_FATAL)
      logLevel = Logger::LEVEL_ERROR;
    else if (level == AV_LOG_WARNING)
      logLevel = Logger::LEVEL_WARN;
    else if (level == AV_LOG_INFO)
      logLevel = Logger::LEVEL_INFO;
    else if (level == AV_LOG_VERBOSE ||
        level == AV_LOG_DEBUG)
      logLevel = Logger::LEVEL_DEBUG;
    else
      logLevel = Logger::LEVEL_ERROR;

    // Revise the format string to add additional useful info
    char revisedFmt[1024];
    revisedFmt[sizeof(revisedFmt)-1] = 0;
    if (avc)
    {
      snprintf(revisedFmt, sizeof(revisedFmt), "[%s @ %p] %s", avc->item_name(ptr), ptr, fmt);
    }
    else
    {
      snprintf(revisedFmt, sizeof(revisedFmt), "%s", fmt);
    }
    int len = strlen(revisedFmt);
    if (len > 0 && revisedFmt[len-1] == '\n')
    {
      revisedFmt[len-1]=0;
      --len;
    }
    if (len > 0)
      // it's not useful to pass in filenames and line numbers here.
      ffmpegLogger->logVA(0, 0, logLevel, revisedFmt, va);
  }

  Global* Global :: sGlobal = 0;

  void
  Global :: init()
  {
    if (!sGlobal)
    {
      av_log_set_callback(xuggler_log_callback);
      av_register_all();
      // turn down logging
      sGlobal = new Global();
      VS_LOG_TRACE("initialized");
    }
  }

  void
  Global :: destroyStaticGlobal(JavaVM*vm,void*closure)
  {
    Global *val = (Global*)closure;
    if (!vm && val) {
      delete val;
    }
  }

  Global :: Global()
  {
    com::xuggle::ferry::JNIHelper::sRegisterTerminationCallback(
        Global::destroyStaticGlobal,
        this);
    mLock = com::xuggle::ferry::Mutex::make();
  }

  Global :: ~Global()
  {
    if (mLock)
      mLock->release();
  }
  
  void
  Global :: lock()
  {
    Global::init();
    if (sGlobal && sGlobal->mLock)
      sGlobal->mLock->lock();
  }

  void
  Global :: unlock()
  {
    Global::init();
    if (sGlobal && sGlobal->mLock)
      sGlobal->mLock->unlock();
  }

  int64_t
  Global :: getVersion()
  {
    return ((int64_t)getVersionMajor())<<48 | ((int64_t)getVersionMinor())<<32 | (int64_t)getVersionRevision();
  }
  int32_t
  Global :: getVersionMajor()
  {
    Global::init();
    return VS_LIB_MAJOR_VERSION;
  }
  int32_t
  Global:: getVersionMinor()
  {
    Global::init();
    return VS_LIB_MINOR_VERSION;
  }
  int32_t
  Global :: getVersionRevision()
  {
    Global::init();
    return VS_REVISION;
  }
  
  const char *
  Global :: getVersionStr()
  {
    Global::init();
    return PACKAGE_VERSION "."  VS_STRINGIFY(VS_REVISION);
  }
  int32_t
  Global :: getAVFormatVersion()
  {
    Global::init();
    return LIBAVFORMAT_VERSION_INT;
  }
  const char *
  Global :: getAVFormatVersionStr()
  {
    Global::init();
    return AV_STRINGIFY(LIBAVFORMAT_VERSION);
  }
  int
  Global :: getAVCodecVersion()
  {
    Global::init();
    return LIBAVCODEC_VERSION_INT;
  }
  const char *
  Global :: getAVCodecVersionStr()
  {
    Global::init();
    return AV_STRINGIFY(LIBAVCODEC_VERSION);
  }

}}}
