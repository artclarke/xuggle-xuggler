/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaGenerator;

public class OpenCoderEvent extends StreamEvent
{
  public OpenCoderEvent(IMediaGenerator source, Integer streamIndex)
  {
    super(source, streamIndex);
  }
}