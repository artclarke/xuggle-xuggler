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

package com.xuggle.xuggler.mediatool;

import java.io.File;
import java.nio.ShortBuffer;

import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.geom.Rectangle2D;
import java.awt.image.BufferedImage;

import org.junit.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.swing.WindowConstants;

import com.xuggle.xuggler.IError;
import com.xuggle.xuggler.IAudioSamples;
import com.xuggle.xuggler.IVideoPicture;
import com.xuggle.xuggler.video.ConverterFactory;

import static com.xuggle.xuggler.mediatool.MediaViewer.Mode.*;

import static junit.framework.Assert.*;

public class MediaViewerTest
{
  // the log

  private final Logger log = LoggerFactory.getLogger(this.getClass());
  { log.trace("<init>"); }

  // the media view mode
  
  final MediaViewer.Mode mViewerMode = MediaViewer.Mode.valueOf(
    System.getProperty(this.getClass().getName() + ".ViewerMode", 
      //AUDIO_VIDEO.name()
      DISABLED.name()
      ));

  // standard test name prefix

  final String PREFIX = this.getClass().getName() + "-";

  // location of test files

  public static final String TEST_FILE_DIR = "fixtures";
 
  public static final String INPUT_FILENAME = TEST_FILE_DIR + "/testfile.flv";
  //  public static final String INPUT_FILENAME = TEST_FILE_DIR + "/goose.wmv";
//   public static final String INPUT_FILENAME = 
//     TEST_FILE_DIR + "/testfile_bw_pattern.flv";

  // test thet the proper number of events are signaled during reading
  // and writng of a media file

  @Test()
  public void testViewer()
  {
    //com.xuggle.ferry.JNIMemoryAllocator.setMirroringNativeMemoryInJVM(false);
    
    File inputFile = new File(INPUT_FILENAME);
    assert (inputFile.exists());

    MediaReader reader = new MediaReader(INPUT_FILENAME);

    reader.addListener(new MediaViewer(mViewerMode, true, 
        WindowConstants.EXIT_ON_CLOSE));
    // reader.addListener(new MediaDebugListener(MediaDebugListener.Mode.EVENT,
    // MediaDebugListener.Event.ALL));

    IError rv;
    while ((rv = reader.readPacket()) == null)
      ;

    assertEquals(IError.Type.ERROR_EOF, rv.getType());
  }

  //@Test()
  public void testModifyMedia()
  {
    //com.xuggle.ferry.JNIMemoryAllocator.setMirroringNativeMemoryInJVM(false);
    
    File inputFile = new File(INPUT_FILENAME);
    assert (inputFile.exists());

    // create a media reader

    MediaReader reader = new MediaReader(INPUT_FILENAME,
      ConverterFactory.XUGGLER_BGR_24);
    
    // add a writer to the reader which receives the decoded media,
    // encodes it and writes it out to the specified file
    
    MediaWriter writer = new MediaWriter("output.mov", reader)
      {
        public void onVideoPicture(IMediaTool tool, IVideoPicture picture,
          BufferedImage image, int streamIndex)
        {
          Graphics2D g = image.createGraphics();
          String timeStamp = picture.getFormattedTimeStamp();
          Rectangle2D bounds = g.getFont().getStringBounds(timeStamp,
            g.getFontRenderContext());
          
          double inset = bounds.getHeight() / 2;
          g.translate(inset, image.getHeight() - inset);
          
          g.setColor(Color.WHITE);
          g.fill(bounds);
          g.setColor(Color.BLACK);
          g.drawString(timeStamp, 0, 0);

          super.onVideoPicture(tool, picture, image, streamIndex);
        }
  
        public void onAudioSamples(IMediaTool tool, IAudioSamples samples, 
          int streamIndex)
        {
          // get the raw audio byes and reduce the value to 1 quarter

          ShortBuffer buffer = samples.getByteBuffer().asShortBuffer();
          for (int i = 0; i < buffer.limit(); ++i)
            buffer.put(i, (short)(buffer.get(i) / 4));

          // pas modifed audio to the writer to be written

          super.onAudioSamples(tool, samples, streamIndex);
        }
      };

    // add a listener to the writer to see media modified media
    
    writer.addListener(new MediaViewer(mViewerMode));

    // read and decode packets from the source file and
    // then encode and write out data to the output file
    
    while (reader.readPacket() == null)
      ;
  }
}
