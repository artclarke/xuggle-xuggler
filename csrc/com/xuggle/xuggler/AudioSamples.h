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
#ifndef AUDIOSAMPLES_H_
#define AUDIOSAMPLES_H_

#include <com/xuggle/ferry/RefPointer.h>
#include <com/xuggle/xuggler/Xuggler.h>
#include <com/xuggle/xuggler/IAudioSamples.h>
#include <com/xuggle/ferry/IBuffer.h>
#include <com/xuggle/xuggler/IRational.h>

namespace com { namespace xuggle { namespace xuggler
{

  class AudioSamples : public IAudioSamples
  {
    VS_JNIUTILS_REFCOUNTED_OBJECT_PRIVATE_MAKE(AudioSamples)
  public:
    // IMediaData
    virtual int64_t getTimeStamp() { return getPts(); }
    virtual void setTimeStamp(int64_t aTimeStamp) { setPts(aTimeStamp); }
    virtual IRational* getTimeBase() { return mTimeBase.get(); }
    virtual void setTimeBase(IRational *aBase) { mTimeBase.reset(aBase, true); }
    virtual int32_t getSize() { return getNumSamples()*getSampleSize(); }
    virtual bool isKey() { return true; }

    // IAudioSamples
    virtual bool isComplete();
    virtual int32_t getSampleRate();
    virtual int32_t getChannels();
    virtual Format getFormat();
    virtual uint32_t getSampleBitDepth();

    virtual uint32_t getNumSamples();
    virtual uint32_t getMaxBufferSize();
    virtual uint32_t getSampleSize();
    virtual uint32_t getMaxSamples();
    virtual com::xuggle::ferry::IBuffer* getData();
    virtual int64_t getPts();
    virtual void setPts(int64_t aValue);
    virtual int64_t getNextPts();
    virtual int32_t setSample(uint32_t sampleIndex, int32_t channel, Format format, int32_t sample);
    virtual int32_t getSample(uint32_t sampleIndex, int32_t channel, Format format);

    /*
     * Convenience method that from C++ returns the buffer
     * managed by getData() above.
     *
     * @param startingSample The sample to start the array at.
     *   That means that only getNumSamples()-startingSample
     *   samples are available in this AudioSamples collection.
     */
    short *getRawSamples(uint32_t startingSample);
    /*
     * Should be called by a C++ user of this class
     * if you change the number of samples in the
     * buffer.
     */
    virtual void setComplete(bool complete, uint32_t numSamples,
        int32_t sampleRate, int32_t channels, Format sampleFmt,
        int64_t pts);

    /*
     * This creates an audio sample.
     */
    static AudioSamples* make(uint32_t numSamples,
        uint32_t numChannels);
  protected:
    AudioSamples();
    virtual ~AudioSamples();
    void allocInternalSamples();
    com::xuggle::ferry::RefPointer<com::xuggle::ferry::IBuffer> mSamples;
    com::xuggle::ferry::RefPointer<IRational> mTimeBase;
    uint32_t mNumSamples;
    uint32_t mRequestedSamples;
    int32_t mSampleRate;
    int32_t mChannels;
    int32_t mIsComplete;
    Format mSampleFmt;
    int64_t mPts;
  };

}}}

#endif /*AUDIOSAMPLES_H_*/
