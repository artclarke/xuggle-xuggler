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

#ifndef IAUDIOSAMPLES_H_
#define IAUDIOSAMPLES_H_

#include <com/xuggle/xuggler/Xuggler.h>
#include <com/xuggle/xuggler/IMediaData.h>
#include <com/xuggle/xuggler/IRational.h>
namespace com { namespace xuggle { namespace xuggler
{

  /**
   * A set of raw (decoded) samples, plus a timestamp for when to play those
   * samples relative to other items in a given {@link IContainer}.
   * 
   * The timestamp value in decoded data is always in Microseonds.
   * 
   */
  class VS_API_XUGGLER IAudioSamples : public IMediaData
  {
  public:
    // IAudioSamples

    /**
     * The format we use to represent audio.  Today
     * only FMT_S16 (signed integer 16-bit audio) is supported.
     */
    typedef enum {
      FMT_NONE = -1,
      FMT_U8,
      FMT_S16,
      FMT_S24,
      FMT_S32,
      FMT_FLT,
    } Format;

    /**
     * Returns whether or not we think this buffer has been filled
     * with data.
     * 
     * 
     * @return Has setComplete() been called and the buffer populated.
     */
    virtual bool isComplete()=0;

    /**
     * Find the sample rate of the samples in this audio buffer.
     * 
     * 
     * @return The Sampling Rate of the samples in this buffer (e.g. 22050).
     */
    virtual int32_t getSampleRate()=0;

    /**
     * Return the number of channels of the samples in this buffer.  For example,
     * 1 is mono, 2 is stereo.
     * 
     * 
     * @return The number of channels.
     */
    virtual int32_t getChannels()=0;

    /**
     * Find out the bit-depth of the samples in this buffer.
     * 
     * 
     * @return Number of bits in a raw sample (per channel)
     */
    virtual uint32_t getSampleBitDepth()=0;

    /**
     * Find the Format of the samples in this buffer.  Right now
     * only FMT_S16 is supported.
     * 
     * 
     * @return The format of the samples.
     */
    virtual Format getFormat()=0;

    /**
     * Get the number of samples in this video.
     * 
     * 
     * For example, if you have 100 bytes of stereo (2-channel) 16-bit
     * audio in this buffer, there are 25 samples.  If you have
     * 100 bytes of mono (1-channel) 16-bit audio in this buffer, you
     * have 50 samples.
     * 
     * 
     * @return The number of samples.
     */
    virtual uint32_t getNumSamples()=0;

    /**
     * @return Maximum number of bytes that can be put in
     * this buffer.  To get the number of samples you can
     * put in this IAudioSamples instance, do the following
     *   num_samples = getMaxBufferSize() / (getSampleSize())
     */
    virtual uint32_t getMaxBufferSize()=0;

    /**
     * @return Maximum number of samples this buffer can hold.
     */
    virtual uint32_t getMaxSamples()=0;

    /**
     * @return Number of bytes in a single sample of audio (including channels).
     *    You can also get this by getSampleBitDepth()*getChannels()/8.
     */
    virtual uint32_t getSampleSize()=0;

    /**
     * What is the Presentation Time Stamp of this set of audio samples.
     *
     * @return the presentation time stamp (pts)
     */
    virtual int64_t getPts()=0;
    
    /**
     * Set the Presentation Time Stamp for this set of samples.
     * 
     * @param aValue the new value
     */
    virtual void setPts(int64_t aValue)=0;

    /**
     * What would be the next Presentation Time Stamp after all the
     * samples in this buffer were played?
     *
     * @return the next presentation time stamp (pts)
     */
    virtual int64_t getNextPts()=0;

    /**
     * Call this if you modify the samples and are now done.  This
     * updates the pertinent information in the structure.
     *
     * @param complete Is this set of samples complete?
     * @param numSamples Number of samples in this update (note that
     *   4 shorts of 16-bit audio in stereo is actually 1 sample).
     * @param sampleRate The sample rate (in Hz) of this set of samples.
     * @param channels The number of channels in this set of samples.
     * @param format The sample-format of this set of samples.
     * @param pts The presentation time stamp of the starting sample in this buffer.
     *   Caller must ensure pts is in units of 1/1,000,000 of a second
     */
    virtual void setComplete(bool complete, uint32_t numSamples,
        int32_t sampleRate, int32_t channels, Format format,
        int64_t pts)=0;

    /**
     * Sets the sample at the given index and channel to the sample.  In
     * theory we assume input is the given Format, and will convert
     * if needed, but right now we only support FMT_S16 anyway.
     *
     *
     * @param sampleIndex The zero-based index into the set of samples
     * @param channel The zero-based channel number.  If this set of samples doesn't
     *   have that given channel, an error is returned.
     * @param format The format of the given sample
     * @param sample The actual sample
     *
     * @return >= 0 on success; -1 on error.
     */
    virtual int32_t setSample(uint32_t sampleIndex, int32_t channel, Format format, int32_t sample)=0;

    /**
     * Get the sample at the given sampleIndex and channel, and return it in
     * the asked for format.
     *
     * @param sampleIndex The zero-based index into this set of samples.
     * @param channel The zero-based channel to get the sample from
     * @param format The format to return in
     *
     * @return The sample if available.  If that sample is not available
     *   (e.g. because the channel doesn't exist, or the samples have not
     *   been #setComplete(bool, uint32_t, int32_t, int32_t, Format, int64_t)),
     *   then this method returns 0.  It is up to the caller to ensure
     *   the inputs are valid given that 0 is also a valid sample value.
     */
    virtual int32_t getSample(uint32_t sampleIndex, int32_t channel, Format format)=0;

    /**
     * A convenience method that returns the # of bits in a given
     * format.  Be aware that right now this library only supports
     * 16-bit audio.
     *
     * @param format The format you want to find the number of bits in.
     *
     *
     * @return The number of bits (not bytes) in the passed in format.
     */
    static uint32_t findSampleBitDepth(Format format);
    
    /**
     * Get a new audio samples buffer.
     * <p>
     * Note that any buffers this objects needs will be
     * lazily allocated (i.e. we won't actually grab all
     * the memory until we need it).
     * </p>
     * @param numSamples The minimum number of samples you're
     *   going to want to put in this buffer.  We may (and probably
     *   will) return a larger buffer, but you cannot assume that.
     * @param numChannels The number of channels in the audio you'll
     *   want to put in this buffer.
     * @return A new object, or null if we can't allocate one.
     */
    static IAudioSamples* make(uint32_t numSamples,
        uint32_t numChannels);
    

    /**
     * Converts a number of samples at a given sampleRate into 
     * Microseconds.
     * @param samples Number of samples.
     * @param sampleRate sample rate that those samples are recorded at.
     * @return number of microseconds it would take to play that audio.
     */
    static int64_t samplesToDefaultPts(int64_t samples, int sampleRate);

    /**
     * Converts a duration in microseconds into
     * a number of samples, assuming a given sampleRate.
     * @param duration The duration in microseconds.
     * @param sampleRate sample rate that you want to use.
     * @return The number of samples it would take (at the given sampleRate) to take duration microseconds to play.
     */
    static int64_t defaultPtsToSamples(int64_t duration, int sampleRate);

  protected:
    IAudioSamples();
    virtual ~IAudioSamples();
  };

}}}

#endif /*IAUDIOSAMPLES_H_*/
