/*
 * This file is part of Xuggler.
 * 
 * Xuggler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * 
 * Xuggler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public
 * License along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ISTREAMCODER_H_
#define ISTREAMCODER_H_

#include <com/xuggle/ferry/RefCounted.h>
#include <com/xuggle/xuggler/Xuggler.h>
#include <com/xuggle/xuggler/ICodec.h>
#include <com/xuggle/xuggler/IRational.h>
#include <com/xuggle/xuggler/IPixelFormat.h>
#include <com/xuggle/xuggler/IAudioSamples.h>
#include <com/xuggle/xuggler/IVideoPicture.h>
#include <com/xuggle/xuggler/IPacket.h>
#include <com/xuggle/xuggler/IProperty.h>

namespace com { namespace xuggle { namespace xuggler
{

  class IStream;
  
  /**
   * The work horse of the Xuggler: Takes {@link IPacket} data from an {@link IContainer}
   * (representing an {@link IStream}) and an {@link ICodec} and allows you to decode or encode
   * that data.
   */
  class VS_API_XUGGLER IStreamCoder : public com::xuggle::ferry::RefCounted
  {
  public:

    /**
     * The Direction in which this StreamCoder will work.
     */
    typedef enum Direction
    {
      ENCODING,
      DECODING
    } Direction;

    /**
     * XUGGLER Flags that can be passed to the setFlag(Flags, bool) method
     */
    typedef enum {
      FLAG_QSCALE =0x0002,  // Use fixed qscale.
      FLAG_4MV    =0x0004,  // 4 MV per MB allowed / advanced prediction for H.263.
      FLAG_QPEL   =0x0010,  // Use qpel MC.
      FLAG_GMC    =0x0020,  // Use GMC.
      FLAG_MV0    =0x0040,  // Always try a MB with MV=<0,0>.
      FLAG_PART   =0x0080,  // Use data partitioning.
      /*
       * The parent program must guarantee that the input for B-frames containing
       * streams is not written to for at least s->max_b_frames+1 frames, if
       * this is not set the input will be copied.
       */
      FLAG_INPUT_PRESERVED =0x0100,
      FLAG_PASS1           =0x0200,   // Use internal 2pass ratecontrol in first pass mode.
      FLAG_PASS2           =0x0400,   // Use internal 2pass ratecontrol in second pass mode.
      FLAG_EXTERN_HUFF     =0x1000,   // Use external Huffman table (for MJPEG).
      FLAG_GRAY            =0x2000,   // Only decode/encode grayscale.
      FLAG_EMU_EDGE        =0x4000,   // Don't draw edges.
      FLAG_PSNR            =0x8000,   // error[?] variables will be set during encoding.
      FLAG_TRUNCATED       =0x00010000, /* Input bitstream might be truncated at a random
                                                  location instead of only at frame boundaries. */
      FLAG_NORMALIZE_AQP  =0x00020000, // Normalize adaptive quantization.
      FLAG_INTERLACED_DCT =0x00040000, // Use interlaced DCT.
      FLAG_LOW_DELAY      =0x00080000, // Force low delay.
      FLAG_ALT_SCAN       =0x00100000, // Use alternate scan.
      FLAG_TRELLIS_QUANT  =0x00200000, // Use trellis quantization.
      FLAG_GLOBAL_HEADER  =0x00400000, // Place global headers in extradata instead of every keyframe.
      FLAG_BITEXACT       =0x00800000, // Use only bitexact stuff (except (I)DCT).
      /* Fx : Flag for h263+ extra options */
      FLAG_AC_PRED        =0x01000000, // H.263 advanced intra coding / MPEG-4 AC prediction
      FLAG_H263P_UMV      =0x02000000, // unlimited motion vector
      FLAG_CBP_RD         =0x04000000, // Use rate distortion optimization for cbp.
      FLAG_QP_RD          =0x08000000, // Use rate distortion optimization for qp selectioon.
      FLAG_H263P_AIV      =0x00000008, // H.263 alternative inter VLC
      FLAG_OBMC           =0x00000001, // OBMC
      FLAG_LOOP_FILTER    =0x00000800, // loop filter
      FLAG_H263P_SLICE_STRUCT =0x10000000,
      FLAG_INTERLACED_ME  =0x20000000, // interlaced motion estimation
      FLAG_SVCD_SCAN_OFFSET =0x40000000, // Will reserve space for SVCD scan offset user data.
      FLAG_CLOSED_GOP     =0x80000000,
      FLAG2_FAST          =0x00000001, // Allow non spec compliant speedup tricks.
      FLAG2_STRICT_GOP    =0x00000002, // Strictly enforce GOP size.
      FLAG2_NO_OUTPUT     =0x00000004, // Skip bitstream encoding.
      FLAG2_LOCAL_HEADER  =0x00000008, // Place global headers at every keyframe instead of in extradata.
      FLAG2_BPYRAMID      =0x00000010, // H.264 allow B-frames to be used as references.
      FLAG2_WPRED         =0x00000020, // H.264 weighted biprediction for B-frames
      FLAG2_MIXED_REFS    =0x00000040, // H.264 one reference per partition, as opposed to one reference per macroblock
      FLAG2_8X8DCT        =0x00000080, // H.264 high profile 8x8 transform
      FLAG2_FASTPSKIP     =0x00000100, // H.264 fast pskip
      FLAG2_AUD           =0x00000200, // H.264 access unit delimiters
      FLAG2_BRDO          =0x00000400, // B-frame rate-distortion optimization
      FLAG2_INTRA_VLC     =0x00000800, // Use MPEG-2 intra VLC table.
      FLAG2_MEMC_ONLY     =0x00001000, // Only do ME/MC (I frames -> ref, P frame -> ME+MC).
      FLAG2_DROP_FRAME_TIMECODE =0x00002000, // timecode is in drop frame format.
      FLAG2_SKIP_RD       =0x00004000, // RD optimal MB level residual skipping
      FLAG2_CHUNKS        =0x00008000, // Input bitstream might be truncated at a packet boundaries instead of only at frame boundaries.
      FLAG2_NON_LINEAR_QUANT =0x00010000, // Use MPEG-2 nonlinear quantizer.
      FLAG2_BIT_RESERVOIR =0x00020000, // Use a bit reservoir when encoding if possible
    } Flags;

    /** Get the direction.
     * @return The direction this StreamCoder works in.
     */
    virtual Direction getDirection()=0;

    /**
     * The associated Stream we're working on.
     *
     * @return The stream associated with this object.
     */
    virtual IStream* getStream()=0;

    /**
     * The Codec this StreamCoder will use.
     *
     * @return The Codec used by this StreamCoder, or 0 (null) if none.
     */
    virtual ICodec* getCodec()=0;

    /**
     * A short hand for getCodec().getType().
     *
     * <p>
     * <b>
     * Note for Native (C++) users:
     * </b>
     * </p>
     * If you actually write code like the above
     * from Native code, you'd leak
     * a Codec() since you didn't call release() on it.
     * This method is a short hand way to avoid you having to
     * worry about releasing in between.
     *
     * @return The Type of the Codec we'll use.
     */
    virtual ICodec::Type getCodecType()=0;

    /**
     *
     * A short hand for getCodec().getID().
     *
     * <p>
     * <b>
     * Note for Native (C++) users:
     * </b>
     * </p>
     * If you actually write code like the above
     * from Native code, you'd leak
     * a Codec() since you didn't call release() on it.
     * This method is a short hand way to avoid you having to
     * worry about releasing in between.
     *
     * @return The ID of the Codec we'll use.
     */
    virtual ICodec::ID getCodecID()=0;

    /**
     * Set the Codec to the passed in Codec, discarding the old
     * Codec if set.
     *
     * @param codec Codec to set.
     */
    virtual void setCodec(ICodec *codec)=0;

    /**
     * Look up a Codec based on the passed in ID, and then set it.
     * To see if you actually set the correct ID, call getCodec() and
     * check for 0 (null).
     *
     * @param id ID of codec to set.
     */
    virtual void setCodec(ICodec::ID id)=0;

    /**
     * The bit rate.
     *
     * @return The bit-rate the stream is, or will be, encoded in.
     */
    virtual int32_t getBitRate()=0;

    /**
     * When ENCODING, sets the bit rate to use.  No-op when DECODING.
     * @see #getBitRate()
     *
     * @param rate The bit rate to use.
     */
    virtual void setBitRate(int32_t rate)=0;

    /**
     * The bit rate tolerance
     *
     * @return The bit-rate tolerance
     */
    virtual int32_t getBitRateTolerance()=0;

    /**
     * When ENCODING set the bit rate tolerance.  No-op when DECODING.
     *
     * @param tolerance The bit rate tolerance
     */
    virtual void setBitRateTolerance(int32_t tolerance)=0;

    /**
     * The height, in pixels.
     *
     * @return The height of the video frames in the attached stream
     *   or -1 if an audio stream, or we cannot determine the height.
     */
    virtual int32_t getHeight()=0;

    /**
     * Set the height, in pixels.
     *
     * @see #getHeight()
     *
     * @param height Sets the height of video frames we'll encode.  No-op when DECODING.
     */
    virtual void setHeight(int32_t height)=0;

    /**
     * The width, in pixels.
     *
     * @return The width of the video frames in the attached stream
     *   or -1 if an audio stream, or we cannot determine the width.
     */
    virtual int32_t getWidth()=0;

    /**
     * Set the width, in pixels
     *
     * @see #getWidth()
     *
     * @param width Sets the width of video frames we'll encode.  No-op when DECODING.
     */
    virtual void setWidth(int32_t width)=0;

    /**
     * Get the time base this stream will ENCODE in, or the time base we
     * detect while DECODING.
     *
     * Caller must call release() on the returned value.
     *
     * @return The time base this StreamCoder is using.
     */
    virtual IRational* getTimeBase()=0;

    /**
     * Set the time base we'll use to ENCODE with.  A no-op when DECODING.
     *
     * As a convenience, we forward this call to the Stream#setTimeBase()
     * method.
     *
     * @see #getTimeBase()
     *
     * @param newTimeBase The new time base to use.
     */
    virtual void setTimeBase(IRational* newTimeBase)=0;

    /**
     * Get the frame-rate the attached stream claims to be using when
     * DECODING, or the frame-rate we'll claim we're using when ENCODING.
     *
     * @return The frame rate.
     */
    virtual IRational* getFrameRate()=0;

    /**
     * Set the frame rate we'll set in the headers of this stream while
     * ENCODING.  Note that you can set whatever frame-rate you'd like,
     * but the TimeBase and the PTS you set on the encoded audio
     * and video frames can override this.
     *
     * As a convenience, we forward this call to the Stream::setFrameRate()
     * method.
     *
     * @see #getFrameRate()
     *
     * @param newFrameRate The new frame rate to use.
     */
    virtual void setFrameRate(IRational* newFrameRate)=0;

    /**
     * The the number of pictures in this Group of Pictures (GOP).  See the
     * MPEG specs for what a GOP is officially, but this is the minimum
     * number of frames between key-frames (or Intra-Frames in MPEG speak).
     *
     * @return the GOPS for this stream.
     */
    virtual int32_t getNumPicturesInGroupOfPictures()=0;

    /**
     * Set the GOPS on this stream.  Ignored if DECODING.
     *
     * @see #getNumPicturesInGroupOfPictures()
     *
     * @param gops The new GOPS for the stream we're encoding.
     */
    virtual void setNumPicturesInGroupOfPictures(int32_t gops)=0;

    /**
     * For Video streams, get the Pixel Format in use by the stream.
     *
     * @return the Pixel format, or IPixelFormat::NONE if audio.
     */
    virtual IPixelFormat::Type getPixelType()=0;

    /**
     * Set the pixel format to ENCODE with.  Ignored if audio or
     * DECODING.
     *
     * @param pixelFmt Pixel format to use.
     */
    virtual void setPixelType(IPixelFormat::Type pixelFmt)=0;

    /**
     * Get the sample rate we use for this stream.
     *
     * @return The sample rate (in Hz) we use for this stream, or -1 if unknown or video.
     */
    virtual int32_t getSampleRate()=0;

    /**
     * Set the sample rate to use when ENCODING.  Ignored if DECODING
     * or a non-audio stream.
     *
     * @param sampleRate New sample rate (in Hz) to use.
     */
    virtual void setSampleRate(int32_t sampleRate)=0;

    /**
     * Get the audio sample format.
     *
     * @return The sample format of samples for encoding/decoding.
     */
    virtual IAudioSamples::Format getSampleFormat()=0;

    /**
     * Set the sample format when ENCODING.  Ignored if DECODING
     * or if the coder is already open.
     *
     * @param aFormat The sample format.
     */
    virtual void setSampleFormat(IAudioSamples::Format aFormat)=0;

    /**
     * Get the number of channels in this audio stream
     *
     * @return The sample rate (in Hz) we use for this stream, or 0 if unknown.
     */
    virtual int32_t getChannels()=0;

    /**
     * Set the number of channels to use when ENCODING.  Ignored if a
     * non audio stream, or if DECODING.
     *
     * @param channels The number of channels we'll encode with.
     */
    virtual void setChannels(int32_t channels)=0;


    /**
     * For this stream, get the number of audio samples that are
     * represented in a packet of information.
     *
     * @return Number of samples per 'frame' of encoded audio
     */
    virtual int32_t getAudioFrameSize()=0;

    /**
     * Get the Global Quality setting this codec uses for video if
     * a VideoPicture doesn't have a quality set.
     *
     * @return The global quality.
     */
    virtual int32_t getGlobalQuality()=0;

    /**
     * Set the Global Quality to a new value.
     *
     * @param newQuality The new global quality.
     *
     */
    virtual void setGlobalQuality(int32_t newQuality)=0;

    /**
     * Get the flags associated with this codec.
     *
     * @return The (compacted) value of all flags set.
     */
    virtual int32_t getFlags()=0;

    /**
     * Set the FFMPEG flags to use with this codec.  All values
     * must be ORed (|) together.
     *
     * @see Flags
     *
     * @param newFlags The new set flags for this codec.
     */
    virtual void setFlags(int32_t newFlags) = 0;

    /**
     * Get the setting for the specified flag
     *
     * @param flag The flag you want to find the setting for
     *
     * @return 0 for false; non-zero for true
     */
    virtual bool getFlag(Flags flag) = 0;

    /**
     * Set the flag.
     *
     * @param flag The flag to set
     * @param value The value to set it to (true or false)
     *
     */
    virtual void setFlag(Flags flag, bool value) = 0;


    /**
     * For this stream, get the next Pts that we expect to decode.
     *
     * Note that this may not actually be the next Pts (for example
     * due to transmission packet drops in the input source).  Still
     * it can be a useful tool.
     *
     * @return The next presentation time stamp we expect to decode
     *   on this stream.  This is always in units of 1/1,000,000 seconds
     */
    virtual int64_t getNextPredictedPts()=0;

    /**
     * Open the Codec associated with this StreamCoder.
     *
     * You can get the codec through getCodec(...) and
     * set it with setCodec(...).  You cannot call any
     * set* methods after you've called open() on this StreamCoder
     * until you close() it.
     *
     * You must call close() when you're done, but if you don't,
     * the container will clean up after you (but yell at you)
     * when it is closed.
     *
     * @return >= 0 on success; < 0 on error.
     */
    virtual int32_t open()=0;
    /**
     * Close a Codec that was opened on this StreamCoder.
     *
     * @return >= 0 on success; < 0 on error.
     */
    virtual int32_t close()=0;

    /**
     * Decode this packet into pOutSamples.  It will
     * try to fill up the audio samples object, starting
     * from the byteOffset inside this packet.
     *
     * The caller is responsible for allocating the
     * IAudioSamples object.  This function will overwrite
     * any data in the samples object.
     *
     * @param pOutSamples The AudioSamples we decode.
     * @param packet    The packet we're attempting to decode from.
     * @param byteOffset Where in the packet payload to start decoding
     *
     * @return number of bytes actually processed from the packet, or negative for error
     */
    virtual int32_t decodeAudio(IAudioSamples * pOutSamples,
        IPacket *packet, int32_t byteOffset)=0;

    /**
     * Decode this packet into pOutFrame.
     *
     * The caller is responsible for allocating the
     * IVideoPicture object.  This function will potentially
     * overwrite any data in the frame object, but
     * you should pass the same IVideoPicture into this function
     * repeatedly until IVideoPicture::isComplete() is true.
     *
     * @param pOutFrame The AudioSamples we decode.
     * @param packet    The packet we're attempting to decode from.
     * @param byteOffset Where in the packet payload to start decoding
     *
     * @return number of bytes actually processed from the packet, or negative for error
     */
    virtual int32_t decodeVideo(IVideoPicture * pOutFrame,
        IPacket *packet, int32_t byteOffset)=0;

    /**
     * Encode the given frame using this StreamCoder.
     *
     * The VideoPicture will allocate a buffer to use internally for this, and
     * will free it when the frame destroys itself.
     *
     * Also, when done in order to flush the encoder, caller should call
     * this method passing in 0 (null) for pFrame to tell the encoder
     * to flush any data it was keeping a hold of.
     *
     * @param pOutPacket [out] The packet to encode into.  It will point
     *     to a buffer allocated in the frame.  Caller should check IPacket::isComplete()
     *     after call to find out if we had enough information to encode a full packet.
     * @param pFrame [in/out] The frame to encode
     * @param suggestedBufferSize The suggested buffer size to allocate or -1 for choose ourselves.
     *        If -1 we'll allocate a buffer exactly the same size (+1) as the decoded frame
     *        with the guess that you're encoding a frame because you want to use LESS space
     *        than that.
     *
     * @ return >= 0 on success; <0 on error.
     */
    virtual int32_t encodeVideo(IPacket * pOutPacket,
        IVideoPicture * pFrame, int32_t suggestedBufferSize)=0;

    /**
     * Encode the given samples using this StreamCoder.
     *
     * The VideoPicture will allocate a buffer to use internally for this, and
     * will free it when the frame destroys itself.
     *
     * Callers should call this repeatedly on a set of samples until
     * we consume all the samples.
     *
     * Also, when done in order to flush the encoder, caller should call
     * this method passing in 0 (null) for pSamples to tell the encoder
     * to flush any data it was keeping a hold of.
     *
     * @param pOutPacket [out] The packet to encode into.  It will point
     *          to a buffer allocated in the frame.  Caller should check IPacket::isComplete()
     *     after call to find out if we had enough information to encode a full packet.
     * @param pSamples [in] The samples to consume
     * @param sampleToStartFrom [in] Which sample you want to start with
     *          This is usually zero, but if you're using a codec that
     *          packetizes output with small number of samples, you may
     *          need to call encodeAudio repeatedly with different starting
     *          samples to consume all of your samples.
     *
     * @return number of samples we consumed when encoding, or negative for errors.
     */
    virtual int32_t encodeAudio(IPacket * pOutPacket,
        IAudioSamples* pSamples, uint32_t sampleToStartFrom)=0;

    /**
     * Create a standalone StreamCoder that can decode data without regard to
     * which IStream or IContainer it came from.
     * <p>
     * If you're reading or writing to a XUGGLER file or URL you almost definitely
     * don't want to use this method.  Use the {@link IContainer#getStream(long)}
     * and {@link IStream#getStreamCoder()} methods instead as it will set up the
     * resulting IStreamCoder with sensible defaults.  Use of a un-attached
     * StreamCoder returned from this method is for advanced users only.
     * </p>
     * @param direction The direction this StreamCoder will work in.
     * @return a new stream coder, or null if error.
     */
    static IStreamCoder* make(Direction direction);
  protected:
    IStreamCoder();
    virtual ~IStreamCoder();
  public:
    /*
     * Added for 1.17
     */

    /**
     * Returns the 4-byte FOURCC tag (Least Significant Byte first).
     *
     * This is really a packed 4-byte array so it's only useful if you use
     * bit-wise operations on it.  Some language wrappings may provide more
     * obvious ways of manipulating, but this is the safest way to do this that
     * will work with all wrappers.
     *
     * @return the FOURCC tag.
     */
    virtual int32_t getCodecTag()=0;
    /**
     * Set the 4-byte FOURCC tag for this coder.
     * @param fourcc The FOURCC to set, with Least Significant Byte first.
     */
    virtual void setCodecTag(int32_t fourcc)=0;
    
    /*
     * Added for 1.19
     */

    /**
     * Returns the total number of settable properties on this object
     * 
     * @return total number of options (not including constant definitions)
     */
    virtual int32_t getNumProperties()=0;
    
    /**
     * Returns the name of the numbered property.
     * 
     * @param propertyNo The property number in the options list.
     *   
     * @return an IProperty value for this properties meta-data
     */
    virtual IProperty *getPropertyMetaData(int32_t propertyNo)=0;

    /**
     * Returns the name of the numbered property.
     * 
     * @param name  The property name.
     *   
     * @return an IProperty value for this properties meta-data
     */
    virtual IProperty *getPropertyMetaData(const char *name)=0;
    
    /**
     * Sets a property on this Object.
     * 
     * All AVOptions supported by the underlying AVClass are supported.
     * 
     * @param name The property name.  For example "b" for bit-rate.
     * @param value The value of the property. 
     * 
     * @return >= 0 if the property was successfully set; <0 on error
     */
    virtual int32_t setProperty(const char *name, const char* value)=0;


    /**
     * Looks up the property 'name' and sets the
     * value of the property to 'value'.
     * 
     * @param name name of option
     * @param value Value of option
     * 
     * @return >= 0 on success; <0 on error.
     */
    virtual int32_t setProperty(const char* name, double value)=0;
    
    /**
     * Looks up the property 'name' and sets the
     * value of the property to 'value'.
     * 
     * @param name name of option
     * @param value Value of option
     * 
     * @return >= 0 on success; <0 on error.
     */
    virtual int32_t setProperty(const char* name, int64_t value)=0;
    
    /**
     * Looks up the property 'name' and sets the
     * value of the property to 'value'.
     * 
     * @param name name of option
     * @param value Value of option
     * 
     * @return >= 0 on success; <0 on error.
     */
    virtual int32_t setProperty(const char* name, bool value)=0;
    
    /**
     * Looks up the property 'name' and sets the
     * value of the property to 'value'.
     * 
     * @param name name of option
     * @param value Value of option
     * 
     * @return >= 0 on success; <0 on error.
     */
    virtual int32_t setProperty(const char* name, IRational *value)=0;

#ifdef SWIG
    %newobject getPropertyAsString(const char*);
    %typemap(newfree) char * "delete [] $1;";
#endif
    /**
     * Gets a property on this Object.
     * 
     * Note for C++ callers; you must free the returned array with
     * delete[] in order to avoid a memory leak.  Other language
     * folks need not worry.
     * 
     * @param name property name
     * 
     * @return an string copy of the option value, or null if the option doesn't exist.
     */
    virtual char * getPropertyAsString(const char* name)=0;

    /**
     * Gets the value of this property, and returns as a double;
     * 
     * @param name name of option
     * 
     * @return double value of property, or 0 on error.
     */
    virtual double getPropertyAsDouble(const char* name)=0;

    /**
     * Gets the value of this property, and returns as an long;
     * 
     * @param name name of option
     * 
     * @return long value of property, or 0 on error.
     */
    virtual int64_t getPropertyAsLong(const char* name)=0;

    /**
     * Gets the value of this property, and returns as an IRational;
     * 
     * @param name name of option
     * 
     * @return long value of property, or 0 on error.
     */
    virtual  IRational *getPropertyAsRational(const char* name)=0;

    /**
     * Gets the value of this property, and returns as a boolean
     * 
     * @param name name of option
     * 
     * @return boolean value of property, or false on error.
     */
    virtual bool getPropertyAsBoolean(const char* name)=0;

    /**
     * Returns true if this IStreamCoder is currently open.
     * 
     * @return true if open; false if not
     */
    virtual bool isOpen()=0;
    
    
    // Added for 1.21
    
    /**
     * Get the default audio frame size (in samples).
     * 
     * Some codecs, especially raw codecs, like PCM, don't have
     * a standard frame size.  In those cases, we use the value
     * of this setting to determine how many samples to encode into
     * a single packet.
     * 
     * @return the number of samples in an audio frame size if the codec
     *   doesn't specify the size.
     */
    virtual int32_t getDefaultAudioFrameSize()=0;
    
    /**
     * Set the default audio frame size.
     * 
     * @param aNewSize The new number of samples to use to encode
     *   samples into a packet.  This setting is ignored if <= 0
     *   or if the codec requires it's own frame size (e.g. Nellymoser).
     *   
     * @see #getDefaultAudioFrameSize()
     */
    virtual void setDefaultAudioFrameSize(int32_t aNewSize)=0;
    
    /*
     * Added for 1.22
     */
    
    /**
     * Creates a new IStreamCoder object by copying all the settings in copyCoder.
     * <p>
     * The new IStreamCoder is created by copying all the current properties on the
     * passed in StreamCoder.  If the passed in stream coder is in a different direction
     * than the one you want, this method still set the same codec ID, and the
     * IStreamCoder.open() method will check then to see if it can work in the
     * specified direction.
     * </p>
     * <p>
     * For example, imagine that direction is ENCODING and the copyCoder is a DECODING StreamCoder that is
     * of the CODEC_ID_VP6 type.  The resulting new IStreamCoder has it's code set to CODEC_ID_VP6.  However
     * (as of the writing of this comment) we don't support encoding to CODEC_ID_VP6, so when you
     * try to open the codec we will fail.
     * </p>  
     * @param direction The direction you want the new IStreamCoder to work in.
     * @param copyCoder The coder to copy settings from.
     * 
     * @return A new IStreamCoder, or null on error.
     */
    static IStreamCoder* make(Direction direction, IStreamCoder* copyCoder);
  };

}}}

#endif /*ISTREAMCODER_H_*/
