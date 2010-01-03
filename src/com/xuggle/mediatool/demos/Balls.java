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

import java.awt.image.BufferedImage;

/**
 * An API for getting images and sounds representing {@link Balls} that
 * are to be encoded into a video.
 * 
 * @author trebor
 *
 */
public interface Balls
{

  /**
   * Get a picture of a set of balls. Each new call should provide the right
   * picture assuming elapsedTime, in MICROSECONDS, has passed.
   * 
   * @param elapsedTime the time in MICROSECONDS which has elapsed since the
   *        last video frame
   */

  public abstract BufferedImage getVideoFrame(long elapsedTime);

  /**
   * Get the next set of audio for the balls.  Samples returned
   * should assume they are contiguous to the last samples returned.
   * 
   * @param sampleRate the number of samples in a second
   */

  public abstract short[] getAudioFrame(int sampleRate);

}
