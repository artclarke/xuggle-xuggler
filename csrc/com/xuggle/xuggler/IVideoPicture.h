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
#ifndef IVIDEOPICTURE_H_
#define IVIDEOPICTURE_H_

#include <com/xuggle/xuggler/Xuggler.h>
#include <com/xuggle/xuggler/IMediaData.h>
#include <com/xuggle/xuggler/IPixelFormat.h>
#include <com/xuggle/ferry/IBuffer.h>
#include <com/xuggle/xuggler/IRational.h>

namespace com { namespace xuggle { namespace xuggler
{

  /**
   * Represents one raw (undecoded) picture in a video stream, plus a timestamp
   * for when to display that video picture relative to other items in a {@link IContainer}.
   * <p>
   * All timestamps for this object are always in Microseconds.
   * </p>
   */
  class VS_API_XUGGLER IVideoPicture : public IMediaData
  {
  public:
    /**
     * Is this a key frame?
     * 
     * @return is this a key frame
     */
    virtual bool isKeyFrame()=0;
    
    /**
     * Reset if this is a key frame or not.  Note that regardless of how
     * this flag is set, an IVideoPicture always contains raw video data (hence the
     * key setting isn't really that important).
     * 
     * @param aIsKey True if a key frame; false if not.
     */
    virtual void setKeyFrame(bool aIsKey)=0;
    
    /**
     * Is this frame completely decoded?
     * 
     * @return is this key frame completely decoded?
     */
    virtual bool isComplete()=0;
    
    /**
     * Total size in bytes of the decoded frame.
     * 
     * @return number of bytes of decoded frame
     */
    virtual int32_t getSize()=0;
        
    /**
     * What is the width of the frame.
     * 
     * @return the width of the frame
     */
    virtual int getWidth()=0;
    
    /**
     * What is the height of the frame
     * 
     * @return the height of the frame
     */
    virtual int getHeight()=0;

    /**
     * Returns the pixel format of the frame.
     * 
     * @return the pixel format of the frame.
     */
    virtual IPixelFormat::Type getPixelType()=0;
    
    /**
     * What is the Presentation Time Stamp of this frame.
     * 
     * The PTS is is scaled so that 1 PTS = 
     * 1/1,000,000 of a second.
     *
     * @return the presentation time stamp (pts)
     */
    virtual int64_t getPts()=0;
    
    /**
     * Set the Presentation Time Stamp for this frame.
     * 
     * @see #getPts()
     * 
     * @param value the new timestamp
     */
    virtual void setPts(int64_t value)=0;
    
    /**
     * This value is the quality setting this VideoPicture had when it was
     * decoded, or is the value to use when this frame is next
     * encoded (if reset with setQuality()
     * 
     * @return The quality.
     */
    virtual int getQuality()=0;
    
    /**
     * Set the Quality to a new value.  This will be used the
     * next time this VideoPicture is encoded by a StreamCoder
     * 
     * @param newQuality The new quality.
     * 
     */
    virtual void setQuality(int newQuality)=0;    

    /**
     * Return the size of each line in the VideoPicture data.  Usually there
     * are no more than 4 lines, but the first line no that returns 0
     * is the end of the road.
     * 
     * @param lineNo The line you want to know the (byte) size of.
     * 
     * @return The size (in bytes) of that line in data.
     */
    virtual int getDataLineSize(int lineNo)=0;
    
    /**
     * After modifying the raw data in this buffer, call this function to
     * let the object know it is now complete.
     * 
     * @param aIsComplete Is this VideoPicture complete
     * @param format The pixel format of the data in this frame.  Must match
     *   what the frame was originally constructed with.
     * @param width The width of the data in this frame.  Must match what
     *   the frame was originally constructed with.
     * @param height The height of the data in this frame.  Must match what
     *   the frame was originally constructed with.
     * @param pts The presentation timestamp of the frame that is now complete.
     *   The caller must ensure this PTS is in units of 1/1,000,000 seconds.
     */
    virtual void setComplete(bool aIsComplete, IPixelFormat::Type format,
        int width, int height, int64_t pts)=0;

    /**
     * Copy the contents of the given srcFrame into this frame.  All
     * buffers are copied by value, not be reference.
     * 
     * @param srcFrame The frame you want to copy.
     * 
     * @return true if a successful copy; false if not.  If not, the caller
     *   should release the destination frame (i.e. this) as it's state
     *   is likely garbage.
     */
    virtual bool copy(IVideoPicture* srcFrame)=0;
    
    /**
     * Get a new frame object.
     * <p>
     * You can specify -1 for width and height, in which case all getData() methods
     * will return error until XUGGLER decodes something into this frame.  In general
     * you should always try to specify the width and height.
     * </p>
     * <p>
     * Note that any buffers this objects needs will be
     * lazily allocated (i.e. we won't actually grab all
     * the memory until we need it).<p>This is useful because
     * it allows you to hold a IVideoPicture object that remembers
     * things like format, width, and height, but know
     * that it doesn't actually take up a lot of memory until
     * the first time someone tries to access that memory.
     * </p>
     * @param format The pixel format (for example, YUV420P).
     * @param width The width of the frame, in pixels, or -1 if you want XUGGLER to guess when decoding.
     * @param height The height of the frame, in pixels, or -1 if you want XUGGLER to guess when decoding.
     * @return A new object, or null if we can't allocate one.
     */
    static IVideoPicture* make(IPixelFormat::Type format, int width, int height);
    
    /**
     * Get a new frame by copying the data in an existing frame.
     * @param srcFrame The frame to copy.
     * @return The new frame, or null on error.
     */
    static IVideoPicture* make(IVideoPicture* srcFrame);

  protected:
    IVideoPicture();
    virtual ~IVideoPicture();
  };

}}}

#endif /*IVIDEOPICTURE_H_*/
