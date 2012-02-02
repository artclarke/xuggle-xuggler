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

package com.xuggle.xuggler;

import static junit.framework.Assert.assertTrue;

import com.xuggle.test_utils.NameAwareTestClassRunner;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IPacket;
import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IStreamCoder;
import com.xuggle.xuggler.TestAudioSamplesGenerator;

import org.junit.*;
import org.junit.runner.RunWith;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;



@RunWith(NameAwareTestClassRunner.class)
public class CreateAudioFileExhaustiveTest
{
  private final Logger log = LoggerFactory.getLogger(this.getClass());

  private String mTestName = null;
  private IContainer mContainer = null;
  private IStream mStream = null;
  private IStreamCoder mCoder = null;
  private IPacket mPacket = null;
  private IAudioSamples mSamples = null;
  private TestAudioSamplesGenerator mAudioGenerator = null;
  private int mSampleRate = 0;
  private int mNumChannels = 0;

  @Before
  public void setUp()
  {
    mTestName = NameAwareTestClassRunner.getTestMethodName();
    log.debug("-----START----- {}", mTestName);
    mAudioGenerator = new TestAudioSamplesGenerator();
  }
  @After
  public void tearDown()
  {
    log.debug("----- END ----- {}", mTestName);
  }

  @Test
  public void testCreateMono44100Audio()
  {
    // open the file and prepare all objects
    prepareFile("file:" + this.getClass().getName() + "_" +mTestName + ".flv",
        1, 44100);

    // Create 15 seconds worth of audio
    writeFile(15);
    // close the file
    closeFile();

  }
  
  @Test
  public void testCreateStereo44100Audio()
  {
    // open the file and prepare all objects
    prepareFile("file:" +this.getClass().getName() + "_" + mTestName + ".flv",
        2, 44100);

    // Create 15 seconds worth of audio
    writeFile(15);
    // close the file
    closeFile();
  }
  
  @Test
  public void testCreateMono22050Audio()
  {
    // open the file and prepare all objects
    prepareFile("file:" + this.getClass().getName() + "_" + mTestName + ".flv",
        1, 22050);

    // Create 15 seconds worth of audio
    writeFile(15);
    // close the file
    closeFile();
  }

  @Test
  public void testCreateStereo22050Audio()
  {
    // open the file and prepare all objects
    prepareFile("file:" + this.getClass().getName() + "_" + mTestName + ".flv",
        2, 22050);

    // Create 15 seconds worth of audio
    writeFile(15);
    // close the file
    closeFile();
  }

  private void closeFile()
  {
    int retval = -1;
    // write the trailer
    retval = mContainer.writeTrailer();
    assertTrue("couldn't write the trailer", retval >= 0);

    // close the coder
    retval = mCoder.close();
    assertTrue("couldn't close coder", retval >= 0);

    // close the container
    retval = mContainer.close();
    assertTrue("couldn't close the container", retval >= 0);

  }

  private void writeFile(int secondsToGenerate)
  {
    int retval = -1;

    double remainingSecs = secondsToGenerate;
    while (remainingSecs > 0)
    {
      mAudioGenerator.fillNextSamples(mSamples, mSamples.getMaxSamples());

      log.debug("Generated {} samples with {} secs remaining",
          mSamples.getNumSamples(),
          remainingSecs);
      
      int numSamplesConsumed = 0;
      while(numSamplesConsumed < mSamples.getNumSamples())
      {
        retval = mCoder.encodeAudio(mPacket, mSamples, numSamplesConsumed);
        if (retval < 0)
          throw new RuntimeException("Could not encode audio");

        numSamplesConsumed += retval;

        log.debug("consumed {} of {} samples", numSamplesConsumed, mSamples.getNumSamples());

        if (mPacket.isComplete())
        {
          log.debug("Wrote packet");
          retval = mContainer.writePacket(mPacket);
          if (retval < 0)
            throw new RuntimeException("could not write packet");
        }
      }
      remainingSecs -= (double) mSamples.getNumSamples() / (double) mSampleRate;
    }
  }

  private void prepareFile(String url,
      int numChannels,
      int sampleRate)
  {
    int retval = -1;
    mSampleRate = sampleRate;
    mNumChannels = numChannels;

    mAudioGenerator.prepare(mNumChannels, mSampleRate);

    mSamples = IAudioSamples.make(1024, mNumChannels);
    assertTrue("couldn't get samples", mSamples != null);

    mPacket = IPacket.make();
    assertTrue("couldn't get a packet", mPacket != null);

    mContainer = IContainer.make();

    retval = mContainer.open(url, IContainer.Type.WRITE, null);
    assertTrue("couldn't open file", retval >= 0);

   ICodec codec = ICodec.guessEncodingCodec(null, null, url, null,
        ICodec.Type.CODEC_TYPE_AUDIO);
    assertTrue("could not guess a codec", codec != null);

    mStream = mContainer.addNewStream(codec);
    assertTrue("couldn't add a stream", mStream != null);

    mCoder = mStream.getStreamCoder();
    assertTrue("couldn't get a stream coder", mCoder != null);

    mCoder.setChannels(mNumChannels);
    log.debug("Setting channels: {}", mCoder.getChannels());
    mCoder.setSampleRate(mSampleRate);
    log.debug("Setting sample rate: {}", mCoder.getSampleRate());
    mCoder.setBitRate(64000);
    log.debug("Setting bit rate: {}", mCoder.getBitRate());
    mCoder.setGlobalQuality(0);
    log.debug("Setting global quality: {}", mCoder.getGlobalQuality());
    // and open the coder
    retval = mCoder.open(null, null);
    assertTrue("couldn't open coder", retval >= 0);

    // and write the header
    retval = mContainer.writeHeader();
    assertTrue("couldn't write header", retval >= 0);
  }
}
