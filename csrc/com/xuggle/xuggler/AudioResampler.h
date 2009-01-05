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

    // Not for calling from Java
    static AudioResampler* make(int outputChannels, int inputChannels,
        int outputRate, int inputRate);
  protected:
    AudioResampler();
    virtual ~AudioResampler();
  private:
    ReSampleContext *mContext;
    int mOChannels;
    int mOSampleRate;
    int mIChannels;
    int mISampleRate;
    int64_t mNextPts;
    int64_t mPtsOffset;
  };

  }}}

#endif /*AUDIORESAMPLER_H_*/
