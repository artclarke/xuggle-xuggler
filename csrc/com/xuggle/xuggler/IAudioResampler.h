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
#ifndef IAUDIORESAMPLER_H_
#define IAUDIORESAMPLER_H_

#include <com/xuggle/ferry/RefCounted.h>
#include <com/xuggle/xuggler/Xuggler.h>
#include <com/xuggle/xuggler/IAudioSamples.h>

namespace com { namespace xuggle { namespace xuggler
  {
  /**
   * Used to resample {@link IAudioSamples} to different sample rates or number of channels.
   */
  class VS_API_XUGGLER IAudioResampler : public com::xuggle::ferry::RefCounted
  {
  public:
    /**
     * number of channels in output audio.
     * @return Number of channels we'll resample the output to.
     */
    virtual int getOutputChannels()=0;
    /**
     * sample rate of output audio.
     * @return Sample Rate we'll resample the output to.
     */
    virtual int getOutputRate()=0;
    
    /**
     * number of channels expected in input audio.
     * @return Number of channels we'll expect in the input samples
     */
    virtual int getInputChannels()=0;
    /**
     * sample rate expected in input audio.
     * @return Sample rate we'll expect in the input samples
     */
    virtual int getInputRate()=0;
    /**
     * Re-sample up to numSamples from inputSamples to outputSamples.
     * 
     * This function re-samples the audio in inputSamples to have the same
     * number of channels, and the same sample rate, as this {@link IAudioResampler} was
     * initialized with.
     * 
     * 
     * @param outputSamples  [out] The sample buffer we output to.
     * @param inputSamples [in] The samples we're going to re-sample.
     * @param numSamples [in] The number of samples from inputSamples to use.  if 0,
     *    this defaults to inputSamples.getNumSamples().
     * 
     * @return Number of samples written to outputSamples, or <0 on error.
     */
    virtual int resample(IAudioSamples *outputSamples, IAudioSamples *inputSamples,
        unsigned int numSamples)=0;
    
    /**
     * Create a new {@link IAudioResampler} object.
     * <p>
     * Creation of {@link IAudioResampler} objects is relatively expensive compared
     * to the {@link #resample(IAudioSamples, IAudioSamples, long)} method,
     * so users are encouraged to create once and use often.
     * </p>
     * <p>
     * This method assumes all samples are in IAudioSamples.Format.FMT_S16 format.
     * </p>
     * @param outputChannels The number of channels you will want
     *   in resampled audio we output.
     * @param inputChannels The number of channels you will pass
     *   in the source audio for resampling.
     * @param outputRate The sample rate you will want
     *   in resampled audio we output.
     * @param inputRate The sample rate you will pass
     *   in the source audio for resampling.
     * @return A new object, or null if we can't allocate one.
     */
    static IAudioResampler* make(int32_t outputChannels, int32_t inputChannels,
            int32_t outputRate, int32_t inputRate);
    
    /*
     * Added for 1.21
     */
    
    /**
     * Get the sample format we expect to resample to.
     * @return the sample format for output.
     */
    virtual IAudioSamples::Format getOutputFormat()=0;
    
    /**
     * Get the sample format we expect to resample from.
     * @return the sample format for input.
     */
    virtual IAudioSamples::Format getInputFormat()=0;
    
    /**
     * Get the length of each filter in the resampler filter bank.
     * @return the filter length
     */
    virtual int32_t getFilterLen()=0;
    
    /**
     * Get log2(number of entries in filter bank).
     * @return log2(number of entries in filter bank).
     */
    virtual int32_t getLog2PhaseCount()=0;
    
    /**
     * Are we linearly interpolating between filters?
     * @return true if interpolating, false if just choosing closest.
     */
    virtual bool isLinear()=0;
    
    /**
     * What is the cuttoff frequency used?
     * @return the cuttoff frequency
     */
    virtual double getCutoffFrequency()=0;

    /**
     * Create a new {@link IAudioResampler} object.
     * <p>
     * Creation of {@link IAudioResampler} objects is relatively expensive compared
     * to the {@link #resample(IAudioSamples, IAudioSamples, long)} method,
     * so users are encouraged to create once and use often.
     * </p>
     * <p>
     * &quot;Sensible&quot; defaults are passed in for filter length and other
     * parameters.
     * </p>
     * @param outputChannels The number of channels you will want
     *   in resampled audio we output.
     * @param inputChannels The number of channels you will pass
     *   in the source audio for resampling.
     * @param outputRate The sample rate you will want
     *   in resampled audio we output.
     * @param inputRate The sample rate you will pass
     *   in the source audio for resampling.
     * @param outputFmt The format of the output samples.
     * @param inputFmt The format of the input samples.
     * 
     * @return A new object, or null if we can't allocate one.
     */
    static IAudioResampler* make(int32_t outputChannels, int32_t inputChannels,
        int32_t outputRate, int32_t inputRate,
        IAudioSamples::Format outputFmt, IAudioSamples::Format inputFmt);

    /**
     * Create a new {@link IAudioResampler} object.
     * <p>
     * Creation of {@link IAudioResampler} objects is relatively expensive compared
     * to the {@link #resample(IAudioSamples, IAudioSamples, long)} method,
     * so users are encouraged to create once and use often.
     * </p>

     * @param outputChannels The number of channels you will want
     *   in resampled audio we output.
     * @param inputChannels The number of channels you will pass
     *   in the source audio for resampling.
     * @param outputRate The sample rate you will want
     *   in resampled audio we output.
     * @param inputRate The sample rate you will pass
     *   in the source audio for resampling.
     * @param outputFmt The format of the output samples.
     * @param inputFmt The format of the input samples.
     * @param filterLen The length of each filter in the filterbank, relative to the cutoff frequency.
     * @param log2PhaseCount log2 of the number of entries in the polyphase filterbank
     * @param linear If true, the used filter will be linearly interpolated between the 2 closest filters. 
     *   if false, the closest will be used.
     * @param cutoffFrequency Cutoff frequency.  1.0 is 1/2 the output sampling rate.
     * 
     * @return A new object, or null if we can't allocate one.
     */
    static IAudioResampler* make(int32_t outputChannels, int32_t inputChannels,
        int32_t outputRate, int32_t inputRate,
        IAudioSamples::Format outputFmt, IAudioSamples::Format inputFmt,
        int32_t filterLen, int32_t log2PhaseCount,
        bool isLinear, double cutoffFrequency);

  protected:
    IAudioResampler();
    virtual ~IAudioResampler();
  };

  }}}

#endif /*IAUDIORESAMPLER_H_*/
