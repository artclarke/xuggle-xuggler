/**
 * 
 */
package com.xuggle.mediatool.event;

import com.xuggle.mediatool.IMediaGenerator;

public class AddStreamEvent extends StreamEvent
{
  public AddStreamEvent(IMediaGenerator source, Integer streamIndex)
  {
    super(source, streamIndex);
  }
}