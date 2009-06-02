package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaGenerator;
import com.xuggle.mediatool.IMediaListener;
import com.xuggle.xuggler.IAudioSamples;

/**
 * {@link Event} for {@link IMediaListener#onAudioSamples(AudioSamplesEvent)}
 * 
 * @author aclarke
 *
 */
public class AudioSamplesEvent extends RawMediaEvent
{
  public AudioSamplesEvent (IMediaGenerator source,
      IAudioSamples samples,
      int streamIndex)
  {
    super(source, samples, null, 0, null, streamIndex);
  }
  
  /**
   * {@inheritDoc}
   */
  @Override
  public IAudioSamples getMediaData()
  {
    return (IAudioSamples) super.getMediaData();
  }
  
  public IAudioSamples getAudioSamples()
  {
    return getMediaData();
  }
  
}