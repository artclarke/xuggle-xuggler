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
#ifndef IAUDIORESAMPLER_H_
#define IAUDIORESAMPLER_H_

#include <com/xuggle/ferry/RefCounted.h>
#include <com/xuggle/xuggler/Xuggler.h>
namespace com { namespace xuggle { namespace xuggler
  {
  class IAudioSamples;
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
    static IAudioResampler* make(int outputChannels, int inputChannels,
            int outputRate, int inputRate);
  protected:
    IAudioResampler();
    virtual ~IAudioResampler();
  };

  }}}

#endif /*IAUDIORESAMPLER_H_*/
