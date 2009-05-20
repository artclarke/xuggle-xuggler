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

package com.xuggle.xuggler;

import java.nio.ByteBuffer;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;


import com.xuggle.ferry.IBuffer;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.ITimeValue;
import com.xuggle.xuggler.Utils;

/**
 * This class generates fake audio data that is an A-note (a sine-wave at 440hz).
 * 
 * @author aclarke
 *
 */
public class TestAudioSamplesGenerator
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());

  private int mNumChannels = 0;
  private int mSampleRate = 0;
  private int mDesiredNoteFrequency = 440; // default to the note: 'A'

  private long mSamplesGenerated = 0;

  private long mStartingPts = 0;
  
  public TestAudioSamplesGenerator()
  {
    log.trace("firing up");
  }
  public void fillNextSamples(IAudioSamples samples, long samplesRequested)
  {
    long samplesToGen = Math.min(samples.getMaxSamples(), samplesRequested);
    
    if (samples.getChannels() != mNumChannels)
    {
      throw new IllegalArgumentException("unmatched channels ");
    }
    
    if (samplesToGen <= 0)
    {
      throw new IllegalArgumentException("incorrect number of samples to generate");
    }
    
    // we're going to access the raw data..
    IBuffer buf = samples.getData();
    if (buf == null)
      throw new RuntimeException("could not get IBuffer from samples");
    
    ByteBuffer data = buf.getByteBuffer(0, buf.getBufferSize());
    if (data == null)
      throw new RuntimeException("could not get raw data from samples");
    // clear out all the old data
    data.clear();
   
    long startingSample = mSamplesGenerated;
    long startingPts = Utils.samplesToTimeValue(mSamplesGenerated, mSampleRate).get(ITimeValue.Unit.MICROSECONDS)
      + mStartingPts;
    
    for(long i = 0; i < samplesToGen; i++)
    {
      // figure out where I am in time....
      double time = (double)mSamplesGenerated / (double) mSampleRate;

      double croppedTime = time % ((double)1/(double)mDesiredNoteFrequency);
      
      // where intra the desired sample frequency we are
      double percentOfRange = croppedTime / ((double)1 /(double) mDesiredNoteFrequency);
      
      double cycleValue = percentOfRange*Math.PI*2;
      
      double samp = Math.sin(cycleValue);
      
      // keep the amplitude low; at 50%
      short aNoteSample = (short) ((samp * 256*256*.5));
      
      /*
      log.trace("generated: {}, {}, {}, {}, {}, {}, {}",
          new Object[]{
            mSamplesGenerated,
            aNoteSample,
            samp,
            time,
            croppedTime,
            percentOfRange,
            cycleValue
      });
      */
      
      for (int j = 0; j < mNumChannels; j++)
      {
        /**
         * We don't ByteBuffer.putShort here because
         * we seem to need to flip the bytes into a
         * low byte first format.
         */
        data.put((byte)(aNoteSample&0xFF));
        data.put((byte)((aNoteSample>>8)&0xFF));
      }
      ++mSamplesGenerated;
    }
    data.flip();
    
    log.trace("completed: {}, {}, {}, {}, {}, {}",
        new Object[]{
        samplesToGen,
        mSampleRate,
        mNumChannels,
        startingPts,
        startingSample,
        IAudioSamples.Format.FMT_S16
    });
    
    samples.setComplete(true, samplesToGen, mSampleRate, mNumChannels,
        IAudioSamples.Format.FMT_S16,
        startingPts);
  }
  
  public void prepare(int numChannels, int sampleRate)
  {
    prepare(numChannels, sampleRate, 0);
  }
  
  public void prepare(int numChannels, int sampleRate, long startingPts)
  {
    mNumChannels = numChannels;
    mSampleRate = sampleRate;
    mStartingPts  = startingPts;
    log.debug("prepare - channels: {}, sampleRate: {}", numChannels, sampleRate);
  }
  
  public void setDesiredNoteFrequency(int note)
  {
    mDesiredNoteFrequency = note;
  }
  
  public int getDesiredNoteFrequency()
  {
    return mDesiredNoteFrequency;
  }
}
