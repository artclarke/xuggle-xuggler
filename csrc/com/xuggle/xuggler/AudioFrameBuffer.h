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
#ifndef AUDIOFRAMEBUFFER_H_
#define AUDIOFRAMEBUFFER_H_

#include <com/xuggle/xuggler/AudioSamples.h>
#include <com/xuggle/ferry/RefPointer.h>
#include <list>

namespace com { namespace xuggle { namespace xuggler {

  /**
   * Maintains a buffer of audio data to be encoded along with
   * associated time stamps, and let's
   * a StreamCoder retrieve data in frame-size chunks.
   * 
   * It has a very special optimization; if you pass in IAudioSamples
   * with EXACTLY frame-size worth of audio, then we will never copy
   * memory into the internal buffer in this frame buffer; instead we'll
   * use the data in the IAudioSamples directly.
   * 
   * Not for use outside this library.
   */
  class AudioFrameBuffer
  {
  public:
    /**
     * Create an AudioFrameBuffer
     * 
     * @param aRequestor The object the is creating this AudioFrameBuffer,
     *   or null if you don't want to tell us.
     * @param aFrameSize The frame size the caller will request,
     *   in samples.
     * @param aAudioChannels The number of channels of audio in each
     *   IAudioSamples that will be added and retrieved from this
     *   frame buffer.
     * @param aSampleRate The sample rate of audio in each IAudioSamples
     *   that will be added and retrieved from this frame buffer
     * @param aBitsPerSample The number of bits in each audio sample.
     *   
     */
    AudioFrameBuffer(
        com::xuggle::ferry::RefCounted* aRequestor,
        int32_t aFrameSize,
        int32_t aAudioChannels,
        int32_t aSampleRate,
        int32_t aBitsPerSample);
    
    /**
     * Destroy a frame buffer
     */
    virtual ~AudioFrameBuffer();
    
    /**
     * Add samples to this frame buffer.
     *
     * @param aInputSamples A set of samples you want to add to this
     *   audio frame buffer.  aInputSamples.getPts() must be > than
     *   the last sample of the last input samples or an error is
     *   returned.
     *
     * @return number of accepted samples, or <0 on error.
     */
    int32_t addSamples(IAudioSamples *aInputSamples);
    
    /**
     * Get the next frame of raw audio for encoding.
     *
     * @param aOutputBuf If non null, on output *aOutputBuf will point to the
     *   audio frame for encoding.  This frame is only good until the
     *   next call to #getNextFrame(void **, int32_t, int32_t, int64_t).
     *
     * @param aSamplesWritten If non null, the total number of samples
     *   in *aOutputBuf.
     *
     * @param aStartingTimestamp If non null, the time stamp in micro-seconds
     *   for the first sample of audio in *aOutputBuf.
     *
     * @return Number of samples written to output; if not equal to the
     *   frameSize then something went wrong.
     */
    int32_t getNextFrame(
        void ** aOutputBuf,
        int32_t *aSamplesWritten,
        int64_t *aStartingTimestamp);

    /**
     * Get the number of channels this frame buffer will output
     * 
     * @return number of channels
     */
    int32_t getChannels() { return mChannels; }
    
    /**
     * Get the sample bit depth of audio in this frame buffer
     * 
     * @return sample bit depth
     */
    int32_t getSampleBitDepth() { return mBitDepth; }
    
    /**
     * Get the sample rate of samples we expect
     * @return the sample rate
     */
    int32_t getSampleRate() { return mSampleRate; }
    
    /**
     * Get the number of bytes in each sample (defined as
     *  #getChannels() * #getSampleBitDepth() / 8
     *  
     * @return the bit depth
     */
    int32_t getBytesPerSample() { return getChannels()*getSampleBitDepth()/8; }

    /**
     * Return the number of samples in each frame we'll return.
     * 
     * @return The number of samples in a frame.
     */
    int32_t getFrameSize() { return mFrameSize; }
    
    /**
     * Is this there at least one frame of audio available
     * 
     * @return true if at least one frame available; false if not
     */
    bool isFrameAvailable();
    
  private:
    /**
     * The queue of samples that are candidates for the next frame
     * worth of audio
     */
    typedef std::list< com::xuggle::ferry::RefPointer<AudioSamples> > AudioSamplesQueue;
    AudioSamplesQueue mQueue;

    /**
     * The current set of samples being worked; this will be popped
     * out of the queue.
     */
    com::xuggle::ferry::RefPointer<AudioSamples> mCurrentSamples;
    
    /**
     * The offset in mCurrentSamples where we start getting next samples.
     */
    int32_t mOffsetInCurrentSamples;
    
    /**
     * Number of channels in audio added and taken from this frame buffer.
     */
    int32_t mChannels;
    
    /**
     * Sample rate of audio added and taken from this frame buffer.
     */
    int32_t mSampleRate;
    
    /**
     * Bit depth of audio added and taken from this frame buffer.
     */
    int32_t mBitDepth;
    
    /**
     * The frame size we'll return
     */
    int32_t mFrameSize;
    
    /**
     * A frame we keep around if we need to copy audio data
     */
    com::xuggle::ferry::RefPointer< com::xuggle::ferry::IBuffer >mFrame;
  };

}}}
#endif /* AUDIOFRAMEBUFFER_H_ */
