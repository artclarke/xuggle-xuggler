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

#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <com/xuggle/ferry/Mutex.h>
#include <com/xuggle/ferry/RefCounted.h>
#include <com/xuggle/xuggler/Xuggler.h>
#include <com/xuggle/xuggler/IContainer.h>
#include <com/xuggle/xuggler/IContainerFormat.h>
#include <com/xuggle/xuggler/ICodec.h>
#include <com/xuggle/xuggler/IRational.h>
#include <com/xuggle/xuggler/IAudioSamples.h>
#include <com/xuggle/xuggler/IVideoPicture.h>
#include <com/xuggle/xuggler/IAudioResampler.h>
#include <com/xuggle/xuggler/IVideoResampler.h>
#include <com/xuggle/xuggler/IMediaDataWrapper.h>
#include <com/xuggle/xuggler/config.h>

namespace com { namespace xuggle { namespace xuggler
{
/**
 * A collection of static functions that refer to the entire package (like version getters).
 *
 */
  class VS_API_XUGGLER Global : public com::xuggle::ferry::RefCounted
  {
  public:
    /**
     * A value that means no time stamp is set for a given object.
     * if the {@link IMediaData#getTimeStamp()} method of an
     * object returns this value it means the time stamp wasn't set.
     */
    static const int64_t NO_PTS=0x8000000000000000LL;
    /**
     * The default time units per second that we use for decoded
     * {@link IAudioSamples} and {@link IVideoPicture} objects. 
     *
     * This means that 1 tick of a time stamp is 1 Microsecond.
     */
    static const int64_t DEFAULT_PTS_PER_SECOND=1000000;

    /**
     * Returns a 64 bit version number for this library.
     * 
     * @return a 64-bit integer version number for this library.  The top 16 bits is
     * the {@link #getVersionMajor()} value.  The next 16-bits are the {@link #getVersionMinor()}
     * value, and the last 32-bits are the {@link #getVersionRevision()} value.
     */
    static int64_t getVersion();
    
    /**
     * Get the major version number of this library.
     * @return the major version number of this library or 0 if unknown.
     */
    static int32_t getVersionMajor();
    /**
     * Get the minor version number of this library.
     * @return the minor version number of this library or 0 if unknown.
     */
    static int32_t getVersionMinor();
    /**
     * Get the revision number of this library.
     * @return the revision number of this library, or 0 if unknown.
     */
    static int32_t getVersionRevision();
    /**
     * Get a string representation of the version of this library.
     * @return the version of this library in string form.
     */
    static const char* getVersionStr();

    /**
     * Get the version of the FFMPEG libavformat library we are compiled against.
     * @return the version.
     */
    static int getAVFormatVersion();
    /**
     * Get the version of the FFMPEG libavformat library we are compiled against.
     * @return the version.
     */
    static const char* getAVFormatVersionStr();
    /**
     * Get the version of the FFMPEG libavcodec library we are compiled against.
     * @return the version.
     */
    static int getAVCodecVersion();
    /**
     * Get the version of the FFMPEG libavcodec library we are compiled against.
     * @return the version.
     */
    static const char* getAVCodecVersionStr();

#ifndef SWIG
    /**
     * Performs a global-level lock of the XUGGLER library.
     * While this lock() is held, every other method that calls
     * lock() in the same DLL/SO will block.
     * <p>
     * Use this sparingly.  FFMPEG's libraries have few
     * real global objects, but when they do exist, you must lock.
     * </p>
     * <p>
     * Lastly, if you lock, make damn sure you call #unlock()
     * </p>
     */
    static void lock();

    /**
     * Unlock the global lock.
     * @see #lock()
     */
    static void unlock();

    /**
     * Methods using the C++ interface that will not necessarily
     * create other Global object should call this.  In general,
     * unless you're extending xuggler directly yourself, ordinary
     * users of this library don't need to call this.
     * <p>
     * It's main purpose is to ensure any FFMPEG required environment
     * initialization functions are called, and any XUGGLER required
     * environmental contexts are set up.
     * </p>
     * It is safe to call multiple times.
     */
    static void init();

#endif // ! SWIG

  private:
    Global();
    ~Global();

    static void destroyStaticGlobal(JavaVM*, void*closure);
    static Global* sGlobal;
    com::xuggle::ferry::Mutex* mLock;
  };
}}}

#endif /*GLOBAL_H_*/
