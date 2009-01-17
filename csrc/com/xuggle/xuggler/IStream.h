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
#ifndef ISTREAM_H_
#define ISTREAM_H_
#include <com/xuggle/ferry/RefCounted.h>
#include <com/xuggle/xuggler/Xuggler.h>
namespace com { namespace xuggle { namespace xuggler
{
  class IStreamCoder;
  class IRational;

  /**
   * Represents a stream of similar data (eg video) in a {@link IContainer}.
   * <p>
   * Streams are really virtual concepts; {@link IContainer}s really just contain
   * a bunch of {@link IPacket}s.  But each {@link IPacket} usually has a stream
   * id associated with it, and all {@link IPacket}s with that stream id represent
   * the same type of (usually time-based) data.  For example in many FLV
   * video files, there is a stream with id "0" that contains all video data, and
   * a stream with id "1" that contains all audio data.
   * </p><p>
   * You use an {@link IStream} object to get properly configured {@link IStreamCoder}
   * for decoding, and to tell {@link IStreamCoder}s how to encode {@link IPacket}s when
   * decoding.
   * </p>
   */
  class VS_API_XUGGLER IStream : public com::xuggle::ferry::RefCounted
  {
  public:
    /**
     * The direction this stream is going (based on the container).
     *
     * If the container Container is opened in Container::READ mode
     * then this will be INBOUND.  If it's opened in Container::WRITE
     * mode, then this will be OUTBOUND.
     */
    typedef enum Direction {
      INBOUND,
      OUTBOUND,
    } Direction;
    /**
     * @return The direction of this stream.
     */
    virtual Direction getDirection()=0;
    /**
     * @return The Index within the Container of this stream.
     */
    virtual int getIndex()=0;

    /**
     * @return The (container format specific) id of this stream.
     */
    virtual int getId()=0;

    /**
     * This method gets the StreamCoder than can manipulate this stream.
     * If the stream is an INBOUND stream, then the StreamCoder can
     * do a IStreamCoder::DECODE.  IF this stream is an OUTBOUND stream,
     * then the StreamCoder can do all IStreamCoder::ENCODE methods.
     *
     * @return The StreamCoder assigned to this object.
     */
    virtual IStreamCoder * getStreamCoder()=0;

    /**
     * Get the (sometimes estimated) frame rate of this container.
     * For variable frame-rate containers (they do exist) this is just
     * an approimation.  Better to use getTimeBase().
     *
     * For contant frame-rate containers, this will be 1 / ( getTimeBase() )
     *
     * @return The frame-rate of this container.
     */
    virtual IRational * getFrameRate()=0;

    /**
     * The time base in which all timestamps (e.g. Presentation Time Stamp (PTS)
     * and Decompression Time Stamp (DTS)) are represented.  For example
     * if the time base is 1/1000, then the difference between a PTS of 1 and
     * a PTS of 2 is 1 millisecond.  If the timebase is 1/1, then the difference
     * between a PTS of 1 and a PTS of 2 is 1 second.
     *
     * @return The time base of this stream.
     */
    virtual IRational * getTimeBase()=0;

    /**
     * @return The start time, in getTimeBase units, when this stream started.
     */
    virtual int64_t getStartTime()=0;

    /**
     * @return The duration (in getTimeBase units) of this stream, if known.
     */
    virtual int64_t getDuration()=0;

    /**
     * @return The current Decompression Time Stamp that will be used on this stream.
     */
    virtual int64_t getCurrentDts()=0;

    /**
     * @return The number of index entries in this stream.
     */
    virtual int getNumIndexEntries()=0;

    /**
     * Returns the number of encoded frames if known.  Note that frames here means
     * encoded frames, which can consist of many encoded audio samples, or
     * an encoded video frame.
     *
     * @return The number of frames (encoded) in this stream.
     */
    virtual int64_t getNumFrames()=0;

  protected:
    virtual ~IStream()=0;
    IStream();
  /** Added in 1.17 */
  public:

    /**
     * Gets the sample aspect ratio.
     *
     * @return The sample aspect ratio.
     */
    virtual IRational* getSampleAspectRatio()=0;
    /**
     * Sets the sample aspect ratio.
     *
     * @param newRatio The new ratio.
     */
    virtual void setSampleAspectRatio(IRational* newRatio)=0;

    /**
     * Get the 4-character language setting for this stream.
     *
     * This will return null if no setting.  When calling
     * from C++, callers must ensure that the IStream outlives the
     * value returned.
     */
    virtual const char* getLanguage()=0;

    /**
     * Set the 4-character language setting for this stream.
     *
     * If a string longer than 4 characters is passed in, only the
     * first 4 characters is copied.
     *
     * @param language The new language setting.  null is equivalent to the
     *   empty string.  strings longer than 4 characters will be truncated
     *   to first 4 characters.
     */
    virtual void setLanguage(const char* language)=0;
  };
}}}

#endif /*ISTREAM_H_*/
