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
#ifndef IPIXELFORMAT_H_
#define IPIXELFORMAT_H_

#include <com/xuggle/xuggler/Xuggler.h>
#include <com/xuggle/ferry/RefCounted.h>

namespace com { namespace xuggle { namespace xuggler
{
  // From libavutil Version 49.6.0
  class IVideoPicture;
  
  /**
   * Information about how video data is formatted in an {@link IVideoPicture} object.
   * 
   * This specifies the color space and how many bits pixel data takes.  It also
   * includes some utility methods for dealing with {@link Type#YUV420P} data; the
   * most common type of encoding used in video files I've run across.
   */
  class VS_API_XUGGLER IPixelFormat : public com::xuggle::ferry::RefCounted {
  public:
    /**
     * Pixel format. Notes:
     *
     * RGB32 is handled in an endian-specific manner. A RGBA
     * color is put together as:
     *  (A << 24) | (R << 16) | (G << 8) | B
     * This is stored as BGRA on little endian CPU architectures and ARGB on
     * big endian CPUs.
     *
     * When the pixel format is palettized RGB (PAL8), the palettized
     * image data is stored in AVFrame.data[0]. The palette is transported in
     * AVFrame.data[1] and, is 1024 bytes long (256 4-byte entries) and is
     * formatted the same as in RGB32 described above (i.e., it is
     * also endian-specific). Note also that the individual RGB palette
     * components stored in AVFrame.data[1] should be in the range 0..255.
     * This is important as many custom PAL8 video codecs that were designed
     * to run on the IBM VGA graphics adapter use 6-bit palette components.
     */
    typedef enum Type {
      NONE= -1,
      YUV420P,
      YUYV422,
      RGB24,
      BGR24,
      YUV422P,
      YUV444P,
      RGB32,
      YUV410P,
      YUV411P,
      RGB565,
      RGB555,
      GRAY8,
      MONOWHITE,
      MONOBLACK,
      PAL8,
      YUVJ420P,
      YUVJ422P,
      YUVJ444P,
      XVMC_MPEG2_MC,
      XVMC_MPEG2_IDCT,
      UYVY422,
      UYYVYY411,
      BGR32,
      BGR565,
      BGR555,
      BGR8,
      BGR4,
      BGR4_BYTE,
      RGB8,
      RGB4,
      RGB4_BYTE,
      NV12,
      NV21,
      RGB32_1,
      BGR32_1,
      GRAY16BE,
      GRAY16LE,
      YUV440P,
      YUVJ440P,
      YUVA420P,
      NB,
    } Type;
    
    typedef enum {
      YUV_Y=0,
      YUV_U=1,
      YUV_V=2,
    } YUVColorComponent;
    
    /**
     * Returns the byte for the coordinates at x and y for the color component c.
     * 
     * @param frame The frame to get the byte from
     * @param x X coordinate in pixels, where 0 is the left hand edge of the image. 
     * @param y Y coordinate in pixels, where 0 is the top edge of the image. 
     * @param c YUVColor component
     * 
     * @throws std::exception frame is null, the coordinates are invalid, or if the pixel format is not YUV420P
     * 
     * @return the pixel byte for that x, y, c combination 
     */
    static unsigned char getYUV420PPixel(IVideoPicture *frame, int x, int y, YUVColorComponent c);

    /**
     * Sets the value of the color component c at the coordinates x and y in the given frame.
     * 
     * @param frame The frame to set the byte in
     * @param x X coordinate in pixels, where 0 is the left hand edge of the image. 
     * @param y Y coordinate in pixels, where 0 is the top edge of the image. 
     * @param c YUVColor component to set
     * @param value The new value of that pixel color component
     * 
     * @throws std::exception frame is null, the coordinates are invalid, or if the pixel format is not YUV420P 
     */
    static void setYUV420PPixel(IVideoPicture *frame, int x, int y, YUVColorComponent c, unsigned char value);

    /**
     * For a given x and y in a frame, and a given color components, this method
     * tells you how far into the actual data you'd have to go to find the byte that
     * represents that color/coordinate combination.
     * 
     * @param frame The frame to get the byte from
     * @param x X coordinate in pixels, where 0 is the left hand edge of the image. 
     * @param y Y coordinate in pixels, where 0 is the top edge of the image. 
     * @param c YUVColor component
     * 
     * @throws std::exception frame is null, the coordinates are invalid, or if the pixel format is not YUV420P
     * 
     * @return the offset in bytes, starting from the start of the frame data, where
     *   the data for this pixel resides.
     */
    static int getYUV420PPixelOffset(IVideoPicture *frame, int x, int y, YUVColorComponent c);
  protected:
    IPixelFormat();
    virtual ~IPixelFormat();

  };
}}}


#endif /*IPIXELFORMAT_H_*/
