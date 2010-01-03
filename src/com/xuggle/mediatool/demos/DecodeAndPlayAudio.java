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

package com.xuggle.mediatool.demos;


import com.xuggle.mediatool.IMediaReader;
import com.xuggle.mediatool.IMediaViewer;
import com.xuggle.mediatool.ToolFactory;

/**
 * Using {@link IMediaReader}, takes a media container, finds the first audio
 * stream, decodes that stream, and plays the audio on your speakers.
 * 
 * @author aclarke
 * @author trebor
 */

public class DecodeAndPlayAudio
{

  /**
   * Takes a media container (file) as the first argument, opens it, opens up a
   * Swing window and displays video frames with the right
   * timing.
   * 
   * @param args
   *          Must contain one string which represents a filename
   */

  public static void main(String[] args)
  {
    if (args.length <= 0)
      throw new IllegalArgumentException(
          "must pass in a filename as the first argument");

    // create a media reader for processing video

    IMediaReader reader = ToolFactory.makeReader(args[0]);

    // Create a MediaViewer object and tell it to play audio only

    reader.addListener(ToolFactory.makeViewer(IMediaViewer.Mode.AUDIO_ONLY));

    // read out the contents of the media file, and sit back and watch

    while (reader.readPacket() == null)
      do {} while(false);
  }
}
