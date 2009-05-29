/*
 * Copyright (c) 2008, 2009 by Xuggle Incorporated.  All rights reserved.
 * 
 * This file is part of Xuggler.
 * 
 * You can redistribute Xuggler and/or modify it under the terms of the GNU
 * Affero General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any
 * later version.
 * 
 * Xuggler is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with Xuggler.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.xuggle.mediatool.demos;


import com.xuggle.mediatool.MediaReader;
import com.xuggle.mediatool.MediaViewer;

/**
 * Using {@link MediaReader}, takes a media container, finds the first audio
 * stream, decodes that stream, and plays the audio on your spakers.
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

    MediaReader mediaReader = new MediaReader(args[0]);

    // Create a MediaViewer object and tell it to play audio only

    mediaReader.addListener(new MediaViewer(MediaViewer.Mode.AUDIO_ONLY));

    // read out the contents of the media file, and sit back and watch

    while (mediaReader.readPacket() == null)
      ;
  }
}
