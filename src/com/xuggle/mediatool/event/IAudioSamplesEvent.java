package com.xuggle.mediatool.event;

import com.xuggle.xuggler.IAudioSamples;

public interface IAudioSamplesEvent extends IRawMediaEvent
{

  /**
   * {@inheritDoc}
   */
  public abstract IAudioSamples getMediaData();

  public abstract IAudioSamples getAudioSamples();

}
