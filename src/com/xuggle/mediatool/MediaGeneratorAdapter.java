package com.xuggle.mediatool;

import java.util.Collection; 

public class MediaGeneratorAdapter extends AMediaToolMixin
implements IMediaGenerator
{
 /*
  * Implementation note: We declare and forward
  * to our parent every method in IMediaGenerator so that
  * it shows up obviously in JavaDoc that these are the
  * main methods people might override.
  */
  
  /**
   * {@inheritDoc}
   */
  public boolean addListener(IMediaListener listener)
  {
    return super.addListener(listener);
  }

  /**
   * {@inheritDoc}
   */
  public Collection<IMediaListener> getListeners()
  {
    return super.getListeners();
  }

  /**
   * {@inheritDoc}
   */
  public boolean removeListener(IMediaListener listener)
  {
    return super.removeListener(listener);
  }

}
