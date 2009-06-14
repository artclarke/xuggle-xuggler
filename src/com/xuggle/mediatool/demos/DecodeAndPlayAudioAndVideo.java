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

import com.xuggle.mediatool.IMediaReader;
import com.xuggle.mediatool.MediaListenerAdapter;
import com.xuggle.mediatool.ToolFactory;

/**
 * Using {@link IMediaReader}, takes a media container, finds the first video stream,
 * decodes that stream, and then plays the audio and video.
 *
 * @author aclarke
 * @author trebor
 */

public class DecodeAndPlayAudioAndVideo extends MediaListenerAdapter
{
  /**
   * Takes a media container (file) as the first argument, opens it,
   * plays audio as quickly as it can, and opens up a Swing window and
   * displays video frames with <i>roughly</i> the right timing.
   *  
   * @param args Must contain one string which represents a filename
   */

  public static void main(String[] args)
  {
    if (args.length <= 0)
      throw new IllegalArgumentException(
        "must pass in a filename as the first argument");
    
    // create a new mr. decode an play audio and video
    IMediaReader reader = ToolFactory.makeReader(args[0]);
    reader.addListener(ToolFactory.makeViewer());
    while(reader.readPacket() == null)
      do {} while(false);
    
  }

}
