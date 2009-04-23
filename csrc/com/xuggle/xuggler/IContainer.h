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
#ifndef ICONTAINER_H_
#define ICONTAINER_H_

#include <com/xuggle/ferry/RefCounted.h>
#include <com/xuggle/xuggler/Xuggler.h>
#include <com/xuggle/xuggler/IContainerParameters.h>
#include <com/xuggle/xuggler/IContainerFormat.h>
#include <com/xuggle/xuggler/IStream.h>
#include <com/xuggle/xuggler/IPacket.h>
#include <com/xuggle/xuggler/IProperty.h>

namespace com { namespace xuggle { namespace xuggler
{

  /**
   * A file (or network data source) that contains one or more {@link IStream}s of
   * audio and video data.
   */
  class VS_API_XUGGLER IContainer : public com::xuggle::ferry::RefCounted
  {
  public:
    /**
     * The different types of Containers Xuggler supports.  A container
     * may only be opened in a uni-directional mode.
     */
    typedef enum {
      READ,
      WRITE,
    } Type;

    /**
     * Set the buffer length Xuggler will suggest to FFMPEG for reading inputs.
     *
     * If called when a IContainer is open, the call is ignored and -1 is returned.
     *
     * @param size The suggested buffer size.
     * @return size on success; <0 on error.
     */
    virtual int32_t setInputBufferLength(uint32_t size)=0;

    /**
     * Return the input buffer length.
     *
     * @return The input buffer length Xuggler's told FFMPEG to assume.
     *   0 means FFMPEG should choose it's own
     *   size (and it'll probably be 32768).
     */
    virtual uint32_t getInputBufferLength()=0;

    /**
     * Is this container opened?
     * @return true if opened; false if not.
     */
    virtual bool isOpened()=0;

    /**
     * Has a header been successfully written?
     * @return true if yes, false if no.
     */
    virtual bool isHeaderWritten()=0;

    /**
     * Open this container and make it ready for reading or writing.
     *
     * The caller must call close() when done, but if not, the
     * {@link IContainer} will eventually close
     * them later but warn to the logging system.
     *
     * This just forwards to {@link #open(String, Type, IContainerFormat, boolean, boolean)}
     * passing false for aStreamsCanBeAddedDynamically, and true for aLookForAllStreams.
     *
     * @param url The resource to open; The format of this string is any
     *   url that FFMPEG supports (including additional protocols if added
     *   through the xuggler.io library).
     * @param type The type of this container.
     * @param pContainerFormat A pointer to a ContainerFormat object specifying
     *   the format of this container, or 0 (NULL) if you want us to guess.
     *
     * @return >= 0 on success; < 0 on error.
     */
    virtual int32_t open(const char *url, Type type,
        IContainerFormat* pContainerFormat)=0;

    /**
     * Open this container and make it ready for reading or writing, optionally
     * reading as far into the container as necessary to find all streams.
     *
     * The caller must call close() when done, but if not, the
     * {@link IContainer} will eventually close
     * them later but warn to the logging system.
     *
     * @param url The resource to open; The format of this string is any
     *   url that FFMPEG supports (including additional protocols if added
     *   through the xuggler.io library).
     * @param type The type of this container.
     * @param pContainerFormat A pointer to a ContainerFormat object specifying
     *   the format of this container, or 0 (NULL) if you want us to guess.
     * @param aStreamsCanBeAddedDynamically If true, open() will expect that new
     *   streams can be added at any time, even after the format header has been read.
     * @param aQueryStreamMetaData If true, open() will call {@link #queryStreamMetaData()}
     *   on this container, which will potentially block until it has ready
     *   enough data to find all streams in a container.  If false, it will only
     *   block to read a minimal header for this container format.
     *
     * @return >= 0 on success; < 0 on error.
     */
    virtual int32_t open(const char *url, Type type,
        IContainerFormat* pContainerFormat,
        bool aStreamsCanBeAddedDynamically,
        bool aQueryStreamMetaData)=0;

        /**
     * Returns the IContainerFormat object being used for this IContainer,
     * or null if the {@link IContainer} doesn't yet know.
     *
     * @return the IContainerFormat object, or null.
     */
    virtual IContainerFormat *getContainerFormat()=0;

    /**
     * Close the container.  open() must have been called first, or
     * else an error is returned.
     *
     * @return >= 0 on success; < 0 on error.
     */
    virtual int32_t close()=0;

    /**
     * Find out the type of this container.
     *
     * @return The Type of this container.  READ if not yet opened.
     */
    virtual Type getType()=0;

    /**
     * The number of streams in this container.
     *
     * If opened in READ mode, this will query the stream and find out
     * how many streams are in it.
     *
     * If opened in WRITE mode, this will return the number of streams
     * the caller has added to date.
     *
     * @return The number of streams in this container.
     */
    virtual int32_t getNumStreams()=0;

    /**
     * Get the stream at the given position.
     * 
     * @param streamIndex the index of this stream in the container
     * @return The stream at that position in the container, or null if none there.
     */
    virtual IStream* getStream(uint32_t streamIndex)=0;

    /**
     * Creates a new stream in this container and returns it.
     *
     * @param id A format-dependent id for this stream.
     *
     * @return A new stream.
     */
    virtual IStream* addNewStream(int32_t id)=0;

    /**
     * Adds a header, if needed, for this container.
     *
     * Call this AFTER you've added all streams you want to add,
     * opened all IStreamCoders for those streams (with proper
     * configuration) and
     * before you write the first frame.  If you attempt to write
     * a header but haven't opened all codecs, this method will log
     * a warning.
     *
     * @return 0 if successful.  < 0 if not.  Always -1 if this is
     *           a READ container.
     */
    virtual int32_t writeHeader()=0;

    /**
     * Adds a trailer, if needed, for this container.
     *
     * Call this AFTER you've written all data you're going to write
     * to this container but BEFORE you close your IStreamCoders.
     * <p>
     * You must call {@link #writeHeader()} before you call
     * this (and if you don't, the IContainer will warn loudly and not
     * actually write the trailer).
     * </p>
     * <p>
     * If you have closed any of the IStreamCoder objects that were open when you called
     * {@link #writeHeader()}, then this method will fail.
     * </p>
     * @return 0 if successful.  < 0 if not.  Always -1 if this is
     *           a READ container.
     */
    virtual int32_t writeTrailer()=0;

    /**
     * Reads the next packet into the IPacket.  This method will
     * release any buffers currently help by this packet and allocate
     * new ones (sorry; such is the way FFMPEG works).
     *
     * @param  packet [In/Out] The packet the IContainer will read into.
     *
     * @return 0 if successful, or <0 if not.
     */
    virtual int32_t readNextPacket(IPacket *packet)=0;

    /**
     * Writes the contents of the packet to the container.
     *
     * @param packet [In] The packet to write out.
     * @param forceInterleave [In] If true, then this {@link IContainer} will
     *   make sure all packets
     *   are interleaved by DTS (even across streams in a container). 
     *   If false, the {@link IContainer} won't,
     *   and it's up to the caller to interleave if necessary.
     *
     * @return # of bytes written if successful, or -1 if not.
     */
    
    virtual int32_t writePacket(IPacket *packet, bool forceInterleave)=0;

    /**
     * Writes the contents of the packet to the container, but make sure the
     * packets are interleaved.
     *
     * This means the {@link IContainer} may have to queue up packets from one
     * stream while waiting for packets from another.
     *
     * @param packet [In] The packet to write out.
     *
     * @return # of bytes written if successful, or -1 if not.
     */
    
    virtual int32_t writePacket(IPacket *packet)=0;
    
    /**
     * Create a new container object.
     *
     * @return a new container, or null on error.
     */

    static IContainer* make();
    protected:
      virtual ~IContainer()=0;

    /*
     * Added as of 1.17
     */
    public:

    /**
     * Attempts to read all the meta data in this stream, potentially by reading ahead
     * and decoding packets.
     *
     * Any packets this method reads ahead will be cached and correctly returned when you
     * read packets, but this method can be non-blocking potentially until end of container
     * to get all meta data.  Take care when you call it.
     *
     * After this method is called, other meta data methods like {@link #getDuration()} should
     * work.
     *
     * @return >= 0 on success; <0 on failure.
     */
    virtual int32_t queryStreamMetaData()=0;

    /**
     * Seeks to the key frame at (or the first one after) the given timestamp.  This method will
     * always fail for any IContainer that is not seekable (e.g. is streamed).  When successful
     * the next call to {@link #readNextPacket(IPacket)} will get the next keyframe from the
     * sought for stream.
     *
     * @param streamIndex The stream to search for the keyframe in; must be a
     *   stream the IContainer has either queried
     *   meta-data about or already ready a packet for.
     * @param timestamp The timestamp, in the timebase of the stream you're looking in (not necessarily Microseconds).
     * @param flags Flags to pass to com.xuggle.xuggler.io.IURLProtocolHandler's seek method.
     *
     * @return >= 0 on success; <0 on failure.
     */
    virtual int32_t seekKeyFrame(int streamIndex, int64_t timestamp, int32_t flags)=0;

    /**
     * Gets the duration, if known, of this container.
     *
     * This will only work for non-streamable containers where IContainer 
     * can calculate the container size.
     *
     * @return The duration, or {@link Global#NO_PTS} if not known.
     */
    virtual int64_t getDuration()=0;

    /**
     * Get the starting timestamp in microseconds of the first packet of the earliest stream in this container.
     *
     * This will only return value values either either (a) for non-streamable
     * containers where IContainer can calculate the container size or
     * (b) after IContainer has actually read the
     * first packet from a streamable source.
     *
     * @return The starting timestamp in microseconds, or {@link Global#NO_PTS} if not known.
     */
    virtual int64_t getStartTime()=0;

    /**
     * Get the file size in bytes of this container.
     *
     * This will only return a valid value if the container is non-streamed and supports seek.
     *
     * @return The file size in bytes, or <0 on error.
     */
    virtual int64_t getFileSize()=0;

    /**
     * Get the calculated overall bit rate of this file.
     *
     * This will only return a valid value if the container is non-streamed and supports seek.
     *
     * @return The overall bit rate in bytes per second, or <0 on error.
     */
    virtual int32_t getBitRate()=0;
    
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

    typedef enum {
      FLAG_GENPTS=0x0001,
      FLAG_IGNIDX=0x0002,
      FLAG_NONBLOCK=0x0004,
    } Flags;
    
    /**
     * Get the flags associated with this object.
     *
     * @return The (compacted) value of all flags set.
     */
    virtual int32_t getFlags()=0;

    /**
     * Set the flags to use with this object.  All values
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
     * Get the URL the IContainer was opened with, or null if unknown.
     * 
     * @return the URL opened, or null.
     */
    virtual const char* getURL()=0;
    
    /**
     * Flush all packets to output.
     *
     * Will only work on output containers.
     *  
     * @return >= 0 on success; <0 on error
     */
    virtual int32_t flushPackets()=0;
    
    /*
     * Added for 1.23
     */
    
    /**
     * Get the number of times {@link IContainer#readNextPacket(IPacket)}
     * will retry a read if it gets a {@link IError.Type#ERROR_AGAIN}
     * value back.
     * 
     * Defaults to 1 times.  <0 means it will keep retrying indefinitely.
     * 
     * @return the read retry count
     */
    virtual int32_t getReadRetryCount()=0;
    
    /**
     * Sets the read retry count.
     * 
     * @see #getReadRetryCount()
     * 
     * @param count The read retry count.  <0 means keep trying.
     */
    virtual void setReadRetryCount(int32_t count)=0;

    
    /**
     * Get the parameters that will be used when opening.
     * 
     * @see #setParameters(IContainerParameters)
     * 
     * @return The parameters
     */
    virtual IContainerParameters *getParameters()=0;

    
    /**
     * Set the parameters for this container.
     * 
     * Normally this is not required, but if you're opening
     * something like a webcam, you need to specify to the
     * Container parameters such as a time base, width, height,
     * etc.
     * 
     * @param parameters The parameters to set.  Ignored if null.
     */
    virtual void setParameters(IContainerParameters* parameters)=0;
  };
}}}
#endif /*ICONTAINER_H_*/
