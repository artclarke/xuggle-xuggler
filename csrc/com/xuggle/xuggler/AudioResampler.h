/*******************************************************************************
 * Copyright (c) 2008, 2010 Xuggle Inc.  All rights reserved.
 *  
 * This file is part of Xuggle-Xuggler-Main.
 *
 * Xuggle-Xuggler-Main is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xuggle-Xuggler-Main is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Xuggle-Xuggler-Main.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/

#ifndef AUDIORESAMPLER_H_
#define AUDIORESAMPLER_H_

#include <com/xuggle/xuggler/IAudioResampler.h>

struct ReSampleContext;

namespace com { namespace xuggle { namespace xuggler
  {

  class AudioResampler : public IAudioResampler
  {
  private:
    VS_JNIUTILS_REFCOUNTED_OBJECT_PRIVATE_MAKE(AudioResampler);
  public:

    // IAudioResampler
    virtual int getOutputChannels();
    virtual int getOutputRate();
    virtual int getInputChannels();
    virtual int getInputRate();
    virtual int resample(IAudioSamples *pOutputSamples, IAudioSamples *pInputSamples,
        unsigned int numSamples);

    virtual IAudioSamples::Format getOutputFormat();
    virtual IAudioSamples::Format getInputFormat();
    virtual int32_t getFilterLen();
    virtual int32_t getLog2PhaseCount();
    virtual bool isLinear();
    virtual double getCutoffFrequency();
    
    // Not for calling from Java
    static AudioResampler* make(int32_t outputChannels, int32_t inputChannels,
        int32_t outputRate, int32_t inputRate);

    static AudioResampler* make(int32_t outputChannels, int32_t inputChannels,
        int32_t outputRate, int32_t inputRate,
        IAudioSamples::Format outputFmt, IAudioSamples::Format inputFmt);

    static AudioResampler* make(int32_t outputChannels, int32_t inputChannels,
        int32_t outputRate, int32_t inputRate,
        IAudioSamples::Format outputFmt, IAudioSamples::Format inputFmt,
        int32_t filterLen, int32_t log2PhaseCount,
        bool isLinear, double cutoff);
        
    virtual int32_t getMinimumNumSamplesRequiredInOutputSamples(
        IAudioSamples* inSamples);
    virtual int32_t getMinimumNumSamplesRequiredInOutputSamples(
        int32_t numSamples);

  protected:
    AudioResampler();
    virtual ~AudioResampler();
  private:
    ReSampleContext *mContext;
    int32_t mOChannels;
    int32_t mOSampleRate;
    int32_t mIChannels;
    int32_t mISampleRate;
    IAudioSamples::Format mOFmt;
    IAudioSamples::Format mIFmt;
    int32_t mFilterLen;
    int32_t mLog2PhaseCount;
    bool mIsLinear;
    double mCutoff ;
    int64_t mNextPts;
    int64_t mPtsOffset;
  };

  }}}

#endif /*AUDIORESAMPLER_H_*/
