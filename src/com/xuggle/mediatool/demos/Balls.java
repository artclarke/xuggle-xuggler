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
