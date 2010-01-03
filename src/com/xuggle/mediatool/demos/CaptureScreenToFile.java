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
import java.awt.Rectangle;
import java.awt.Robot;
import java.awt.Toolkit;
import java.awt.image.BufferedImage;
import java.util.concurrent.TimeUnit;

import com.xuggle.mediatool.ToolFactory;
import com.xuggle.mediatool.IMediaWriter;
import com.xuggle.xuggler.IRational;

/**
 * Using {@link IMediaWriter}, takes snapshots of your desktop and
 * encodes them to video.
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
      // This is the robot for taking a snapshot of the
      // screen.  It's part of Java AWT
      final Robot robot = new Robot();
      final Toolkit toolkit = Toolkit.getDefaultToolkit();
      final Rectangle screenBounds = new Rectangle(toolkit.getScreenSize());
      
      // First, let's make a IMediaWriter to write the file.
      final IMediaWriter writer = ToolFactory.makeWriter(outFile);
      
      // We tell it we're going to add one video stream, with id 0,
      // at position 0, and that it will have a fixed frame rate of
      // FRAME_RATE.
      writer.addVideoStream(0, 0,
          FRAME_RATE,
          screenBounds.width, screenBounds.height);
      
      // Now, we're going to loop
      long startTime = System.nanoTime();
      for (int index = 0; index < SECONDS_TO_RUN_FOR*FRAME_RATE.getDouble(); index++)
      {
        // take the screen shot
        BufferedImage screen = robot.createScreenCapture(screenBounds);
        
        // convert to the right image type
        BufferedImage bgrScreen = convertToType(screen,
            BufferedImage.TYPE_3BYTE_BGR);
        
        // encode the image
        writer.encodeVideo(0,bgrScreen,
            System.nanoTime()-startTime, TimeUnit.NANOSECONDS);

        System.out.println("encoded image: " +index);
        
        // sleep for framerate milliseconds
        Thread.sleep((long) (1000 / FRAME_RATE.getDouble()));

      }
      // Finally we tell the writer to close and write the trailer if
      // needed
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
