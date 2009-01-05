/*
 * Copyright (c) 2008 by Vlideshow Inc. (a.k.a. The Yard).  All rights reserved.
 *
 * It is REQUESTED BUT NOT REQUIRED if you use this library, that you let 
 * us know by sending e-mail to info@theyard.net telling us briefly how you're
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
package com.xuggle.xuggler.demos;

import com.xuggle.xuggler.Global;
import com.xuggle.xuggler.ICodec;
import com.xuggle.xuggler.IContainer;
import com.xuggle.xuggler.IStream;
import com.xuggle.xuggler.IStreamCoder;

/**
 * Opens up a media container, and prints out a summary of the contents.
 * 
 * @author aclarke
 *
 */
public class GetContainerInfo
{
  /**
   * Takes a media container (file) as the first argument, opens it, and tells you what's inside the container.
   * @param args Must contain one string which represents a filename
   */
  public static void main(String[] args)
  {
    if (args.length <= 0)
      throw new IllegalArgumentException("must pass in a filename as the first argument");
    
    String filename = args[0];
    
    // Create a Xuggler container object
    IContainer container = IContainer.make();
    
    // Open up the container
    if (container.open(filename, IContainer.Type.READ, null) < 0)
      throw new IllegalArgumentException("could not open file: " + filename);
    
    // query how many streams the call to open found
    int numStreams = container.getNumStreams();
    System.out.printf("file \"%s\": %d stream%s\n",
        filename,
        numStreams,
        numStreams == 1 ? "" : "s");
    
    // and iterate through the streams to print their meta data
    for(int i = 0; i < numStreams; i++)
    {
      // Find the stream object
      IStream stream = container.getStream(i);
      // Get the pre-configured decoder that can decode this stream;
      IStreamCoder coder = stream.getStreamCoder();
      
      // and now print out the meta data.
      System.out.printf("stream %d: ",    i);
      System.out.printf("type: %s; ",     coder.getCodecType());
      System.out.printf("codec: %s; ",    coder.getCodecID());
      System.out.printf("duration: %s; ", stream.getDuration() == Global.NO_PTS ? "unknown" : "" + stream.getDuration());
      
      if (coder.getCodecType() == ICodec.Type.CODEC_TYPE_AUDIO)
      {
        System.out.printf("sample rate: %d; ", coder.getSampleRate());
        System.out.printf("channels: %d; ",    coder.getChannels());
        System.out.printf("format: %s",        coder.getSampleFormat());
      } else if (coder.getCodecType() == ICodec.Type.CODEC_TYPE_VIDEO)
      {
        System.out.printf("width: %d; ",  coder.getWidth());
        System.out.printf("height: %d; ", coder.getHeight());
        System.out.printf("format: %s; ", coder.getPixelType());
      }
      System.out.printf("\n");
    }
    
  }
}
