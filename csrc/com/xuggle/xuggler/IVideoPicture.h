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
     * Is this picture completely decoded?
     * 
     * @return is this picture completely decoded?
     */
    virtual bool isComplete()=0;
    
    /**
     * Total size in bytes of the decoded picture.
     * 
     * @return number of bytes of decoded picture
     */
    virtual int32_t getSize()=0;
        
    /**
     * What is the width of the picture.
     * 
     * @return the width of the picture
     */
    virtual int getWidth()=0;
    
    /**
     * What is the height of the picture
     * 
     * @return the height of the picture
     */
    virtual int getHeight()=0;

    /**
     * Returns the pixel format of the picture.
     * 
     * @return the pixel format of the picture.
     */
    virtual IPixelFormat::Type getPixelType()=0;
    
    /**
     * What is the Presentation Time Stamp (in Microseconds) of this picture.
     * 
     * The PTS is is scaled so that 1 PTS = 
     * 1/1,000,000 of a second.
     *
     * @return the presentation time stamp (pts)
     */
    virtual int64_t getPts()=0;
    
    /**
     * Set the Presentation Time Stamp (in Microseconds) for this picture.
     * 
     * @see #getPts()
     * 
     * @param value the new timestamp
     */
    virtual void setPts(int64_t value)=0;
    
    /**
     * This value is the quality setting this VideoPicture had when it was
     * decoded, or is the value to use when this picture is next
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
     * @param format The pixel format of the data in this picture.  Must match
     *   what the picture was originally constructed with.
     * @param width The width of the data in this picture.  Must match what
     *   the picture was originally constructed with.
     * @param height The height of the data in this picture.  Must match what
     *   the picture was originally constructed with.
     * @param pts The presentation timestamp of the picture that is now complete.
     *   The caller must ensure this PTS is in units of 1/1,000,000 seconds.
     */
    virtual void setComplete(bool aIsComplete, IPixelFormat::Type format,
        int width, int height, int64_t pts)=0;

    /**
     * Copy the contents of the given picture into this picture.  All
     * buffers are copied by value, not be reference.
     * 
     * @param srcPicture The picture you want to copy.
     * 
     * @return true if a successful copy; false if not.
     */
    virtual bool copy(IVideoPicture* srcPicture)=0;
    
    /**
     * Get a new picture object.
     * <p>
     * You can specify -1 for width and height, in which case all getData() methods
     * will return error until XUGGLER decodes something into this frame.  In general
     * you should always try to specify the width and height.
     * </p>
     * <p>
     * Note that any buffers this objects needs will be
     * lazily allocated (i.e. we won't actually grab all
     * the memory until we need it).
     * </p>
     * <p>This is useful because
     * it allows you to hold a IVideoPicture object that remembers
     * things like format, width, and height, but know
     * that it doesn't actually take up a lot of memory until
     * the first time someone tries to access that memory.
     * </p>
     * @param format The pixel format (for example, YUV420P).
     * @param width The width of the picture, in pixels, or -1 if you want XUGGLER to guess when decoding.
     * @param height The height of the picture, in pixels, or -1 if you want XUGGLER to guess when decoding.
     * @return A new object, or null if we can't allocate one.
     */
    static IVideoPicture* make(IPixelFormat::Type format, int width, int height);
    
    /**
     * Get a new picture by copying the data in an existing frame.
     * @param src The picture to copy.
     * @return The new picture, or null on error.
     */
    static IVideoPicture* make(IVideoPicture* src);

    /*
     * Added for 3.1
     */
    
    /**
     * The different types of images that we can set. 
     * 
     * @see #getPictureType()
     */
    typedef enum {
      DEFAULT_TYPE=0,
      I_TYPE = 1,
      P_TYPE = 2,
      B_TYPE = 3,
      S_TYPE = 4,
      SI_TYPE = 5,
      SP_TYPE = 6,
      BI_TYPE = 7,
    } PictType;
    
    /**
     * Get the picture type.
     * <p>
     * This will be set on decoding to tell you what type of
     * packet this was decoded from, and when encoding
     * is a request to the encoder for how to encode the picture.
     * </p>
     * <p>
     * The request may be ignored by your codec.
     * </p>
     * @return the picture type.
     */
    virtual PictType getPictureType()=0;
    /**
     * Set the picture type.
     * 
     * @param type The type.
     * 
     * @see #getPictureType()
     */
    virtual void setPictureType(PictType type)=0;

    /*
     * Added for 3.1
     */
    
    /**
     * Get a new picture object, by wrapping an existing
     * {@link com.xuggle.ferry.IBuffer}.
     * <p>
     * Use this method if you have existing video data that you want
     * to have us wrap and pass to FFmpeg.  Note that if decoding
     * into this video picture and the decoded data actually takes more
     * space than is in this buffer, this object will release the reference
     * to the passed in buffer and allocate a new buffer instead so the decode
     * can continue.
     * </p>
     * @param buffer The {@link com.xuggle.ferry.IBuffer} to wrap.
     * @param format The pixel format (for example, YUV420P).
     * @param width The width of the picture, in pixels.
     * @param height The height of the picture, in pixels.
     * @return A new object, or null if we can't allocate one.
     */
    static IVideoPicture* make(
        com::xuggle::ferry::IBuffer* buffer,
        IPixelFormat::Type format, int width, int height);

  protected:
    IVideoPicture();
    virtual ~IVideoPicture();
  };

}}}

#endif /*IVIDEOPICTURE_H_*/
