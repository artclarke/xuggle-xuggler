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

import static org.junit.Assert.assertTrue;

import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IContainerFormat;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.IPacket;
import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IStreamCoder;

/*
 * A collection of methods that are helpful for
 * testing renderer classes.
 */
public class Helper
{
  public final String sampleFile = "fixtures/testfile.flv";

  public IContainer mContainer=null;
  public IContainerFormat mContainerFormat=null;
  public IPacket mPacket=null;
  public IStream[] mStreams=null;
  public ICodec[] mCodecs=null;
  public IStreamCoder[] mCoders=null;
  public IAudioSamples[] mSamples=null;
  public IVideoPicture[] mFrames=null;
  
  public void setupReadingObject(String url)
  {
    int retval = -1;
  
    mPacket = IPacket.make();
    assertTrue("Could not get a new packet",
        mPacket != null);
    
    mContainer = IContainer.make();
    assertTrue("Could not get a new container",
        mContainer != null);
    
    retval = mContainer.open(url, IContainer.Type.READ, null);
    assertTrue("Could not open url: " + url, retval >= 0);
    
    int numStreams = mContainer.getNumStreams();
    assertTrue("Could not find any streams in url: " + url,
        numStreams > 0);
    
    mStreams = new IStream[numStreams];
    mCodecs = new ICodec[numStreams];
    mCoders = new IStreamCoder[numStreams];
    mSamples = new IAudioSamples[numStreams];
    mFrames = new IVideoPicture[numStreams];
    for (int i = 0; i< numStreams; i++)
    {
      mStreams[i] = mContainer.getStream(i);
      assertTrue("Could not find stream #" + i +
          " in url: " + url,
          mStreams[i] != null);
      
      mCoders[i] = mStreams[i].getStreamCoder();
      assertTrue("Could not find coder for stream #" + i +
          " in url: " + url,
          mCoders[i] != null);
      
      mCodecs[i] = mCoders[i].getCodec();
      assertTrue("Could not find codec for stream #" + i +
          " in url: " + url,
          mCodecs[i] != null);
      
      if (mCoders[i].getCodecType() == ICodec.Type.CODEC_TYPE_VIDEO)
      {
        mFrames[i] = IVideoPicture.make(
            mCoders[i].getPixelType(),
            mCoders[i].getWidth(),
            mCoders[i].getHeight());
        assertTrue("could not allocate memory for frame", mFrames[i]!=null);
        mSamples[i] = null;
      } else if (mCoders[i].getCodecType() == ICodec.Type.CODEC_TYPE_AUDIO)
      {
        mSamples[i] = IAudioSamples.make(1024, 1);
        assertTrue("Could not get audio samples buffer",
            mSamples[i] != null);
        assertTrue("Could not allocate memory for samples",
            mSamples[i].getMaxBufferSize()>=1024);
        mFrames[i] = null;
      } else {
        mFrames[i] = null;
        mSamples[i] = null;
      }
    }
  }
  public void cleanupHelper()
  {
    int i = 0;
    if (mContainer != null)
    {
      mContainer.delete();
      mContainer = null;
    }
    if (mContainerFormat != null)
    {
      mContainerFormat.delete();
      mContainerFormat = null;
    }
    if (mPacket != null)
    {
      mPacket.delete();
      mPacket = null;
    }
    if (mStreams != null)
    {
      for (i = 0; i < mStreams.length; i++)
      {
        if (mStreams[i] != null)
        {
          mStreams[i].delete();
          mStreams[i] = null;
        }
      }
      mStreams = null;
    }
    if (mCodecs != null)
    {
      for (i = 0; i < mCodecs.length; i++)
      {
        if (mCodecs[i] != null)
        {
          mCodecs[i].delete();
          mCodecs[i] = null;
        }
      }
      mCodecs = null;
    }
    if (mCoders != null)
    {
      for (i = 0; i < mCoders.length; i++)
      {
        if (mCoders[i] != null)
        {
          mCoders[i].delete();
          mCoders[i] = null;
        }
      }
      mCoders = null;
    }
    if (mSamples != null)
    {
      for (i = 0; i < mSamples.length; i++)
      {
        if (mSamples[i] != null)
        {
          mSamples[i].delete();
          mSamples[i] = null;
        }
      }
      mSamples = null;
    }
    if (mFrames != null)
    {
      for (i = 0; i < mFrames.length; i++)
      {
        if (mFrames[i] != null)
        {
          mFrames[i].delete();
          mFrames[i] = null;
        }
      }
      mFrames = null;
    }
    // and suggest a garbage collection round for good measure
    System.gc();
  }
}
