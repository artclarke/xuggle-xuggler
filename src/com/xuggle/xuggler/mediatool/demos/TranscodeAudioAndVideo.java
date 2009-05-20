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

package com.xuggle.xuggler.mediatool.demos;

import java.io.File;


import com.xuggle.xuggler.mediatool.MediaReader;
import com.xuggle.xuggler.mediatool.MediaViewer;
import com.xuggle.xuggler.mediatool.MediaWriter;

import static java.lang.System.out;
import static java.lang.System.exit;

/** 
 * A very simple media transcoder which uses {@link MediaReader}, {@link
 * MediaWriter} and {@link MediaViewer}.
 */

public class TranscodeAudioAndVideo
{
  public static void main(String[] args)
  {
    if (args.length < 2)
    {
      out.println("To perform a simple media transcode.  The destination " +
        "format will be guessed from the file extention.");
      out.println("");
      out.println("   TranscodeAudioAndVideo <source-file> <destination-file>");
      out.println("");
      out.println(
        "The destination type will be guess from the supplied file extsion.");
      exit(0);
    }

    File source = new File(args[0]);
    if (!source.exists())
    {
      out.println("Source file does not exist: " + source);
      exit(0);
    }

    transcode(args[0], args[1]);
  }

  /**
   * Transcode a source url to a destination url.
   */

  public static void transcode(String sourceUrl, String destinationUlr)
  {
    out.printf("transcode %s -> %s\n", sourceUrl, destinationUlr);

    // create the media reader, not that no BufferedImages need to be
    // created because the video is not going to be manipulated

    MediaReader reader = new MediaReader(sourceUrl);

    // add a viewr to the reader, to see progress as the media is
    // transcodded

    reader.addListener(new MediaViewer(true));

    // create the media writer, notice that NO refence to the writer
    // needs to be created - the writer has been automatically added as
    // a listener to the reader and will be called when the reader
    // dispatches media

    new MediaWriter(destinationUlr, reader);

    // read packets from the source file, which dispatch events to the
    // writer, this will continue until 

    while (reader.readPacket() == null)
      ;
  }
}
