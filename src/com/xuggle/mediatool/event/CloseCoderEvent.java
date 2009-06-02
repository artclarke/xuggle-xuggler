/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaGenerator;

public class CloseCoderEvent extends StreamEvent
{
  public CloseCoderEvent(IMediaGenerator source, Integer streamIndex)
  {
    super(source, streamIndex);
  }
}