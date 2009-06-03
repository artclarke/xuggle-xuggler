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
import java.awt.Rectangle;
import java.awt.Robot;
import java.awt.Toolkit;
import java.awt.image.BufferedImage;
import java.util.concurrent.TimeUnit;

import com.xuggle.mediatool.ToolFactory;
import com.xuggle.mediatool.IMediaWriter;
import com.xuggle.xuggler.IRational;

/**
 * A demo of using MediaTool to take screen shots.
 * 
 * @author aclarke
 * 
 */

public class CaptureScreenToFile
{
  private static IRational FRAME_RATE=IRational.make(3,1);
  private static final int SECONDS_TO_RUN_FOR = 15;
  
  /**
   * Takes a screen shot of your entire screen and writes it to
   * output.flv
   * 
   * @param args
   */
  public static void main(String[] args)
  {
    try
    {
      final String outFile;
      if (args.length > 0)
        outFile = args[0];
      else
        outFile = "output.mp4";
      int index = 0;
      final Robot robot = new Robot();
      final Toolkit toolkit = Toolkit.getDefaultToolkit();
      final Rectangle screenBounds = new Rectangle(toolkit.getScreenSize());
      
      // make a media writer
      final IMediaWriter writer = ToolFactory.makeWriter(outFile);
      writer.addVideoStream(0, 0,
          FRAME_RATE,
          screenBounds.width, screenBounds.height);
      
      long startTime = System.nanoTime();
      while (index < SECONDS_TO_RUN_FOR*FRAME_RATE.getDouble())
      {
        System.out.println("encoded image: " +index);
        
        // take the screen shot
        BufferedImage screen = robot.createScreenCapture(screenBounds);
        
        // convert to the right type
        BufferedImage bgrScreen = convertToType(screen,
            BufferedImage.TYPE_3BYTE_BGR);
        
        // encode the image
        writer.encodeVideo(0,bgrScreen,
            System.nanoTime()-startTime, TimeUnit.NANOSECONDS);
        
        // sleep for framerate milliseconds
        Thread.sleep((long) (1000 / FRAME_RATE.getDouble()));

        // and increment the frame count
        index++;
      }
      // Close the writer
      writer.close();
    }
    catch (Throwable e)
    {
      System.err.println("an error occurred: " + e.getMessage());
    }
  }
  /**
   * Convert a {@link BufferedImage} of any type, to {@link BufferedImage} of a
   * specified type. If the source image is the same type as the target type,
   * then original image is returned, otherwise new image of the correct type is
   * created and the content of the source image is copied into the new image.
   * 
   * @param sourceImage
   *          the image to be converted
   * @param targetType
   *          the desired BufferedImage type
   * 
   * @return a BufferedImage of the specifed target type.
   * 
   * @see BufferedImage
   */

  public static BufferedImage convertToType(BufferedImage sourceImage,
      int targetType)
  {
    BufferedImage image;

    // if the source image is already the target type, return the source image

    if (sourceImage.getType() == targetType)
      image = sourceImage;

    // otherwise create a new image of the target type and draw the new
    // image

    else
    {
      image = new BufferedImage(sourceImage.getWidth(),
          sourceImage.getHeight(), targetType);
      image.getGraphics().drawImage(sourceImage, 0, 0, null);
    }

    return image;
  }

}
