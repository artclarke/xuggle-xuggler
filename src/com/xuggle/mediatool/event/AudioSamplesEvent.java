package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaGenerator;
import com.xuggle.mediatool.IMediaListener;
import com.xuggle.xuggler.IAudioSamples;

/**
 * {@link AEventMixin} for {@link IMediaListener#onAudioSamples(IAudioSamplesEvent)}
 * 
 * @author aclarke
 *
 */
public class AudioSamplesEvent extends ARawMediaMixin implements IAudioSamplesEvent
{
  public AudioSamplesEvent (IMediaGenerator source,
      IAudioSamples samples,
      int streamIndex)
  {
    super(source, samples, null, 0, null, streamIndex);
  }
  
  /* (non-Javadoc)
   * @see com.xuggle.mediatool.event.IAudioSamplesEvent#getMediaData()
   */
  @Override
  public IAudioSamples getMediaData()
  {
    return (IAudioSamples) super.getMediaData();
  }
  
  /* (non-Javadoc)
   * @see com.xuggle.mediatool.event.IAudioSamplesEvent#getAudioSamples()
   */
  public IAudioSamples getAudioSamples()
  {
    return getMediaData();
  }
  
}