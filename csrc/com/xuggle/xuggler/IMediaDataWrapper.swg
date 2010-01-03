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

 %typemap (javacode) com::xuggle::xuggler::IMediaDataWrapper,com::xuggle::xuggler::IMediaDataWrapper*,com::xuggle::xuggler::IMediaDataWrapper& %{

  /**
   * Return the object being wrapped
   * 
   * @return the wrapped object
   */
  public IMediaData get()
  {
    IMediaData retval = null;
    if ((retval = getVideoPicture()) == null)
      if ((retval = getAudioSamples()) == null)
        if ((retval = getPacket()) == null)
          if ((retval = getMediaDataWrapper()) == null)
            retval = null;
    return retval;
  }
  
  /**
   * Gets the non IMediaDataWrapper object ultimately wrapped in this
   * wrapper, or null if there isn't one.
   * 
   * @return The non IMediaDataWrapper object ultimately wrapped
   */
  public IMediaData unwrap()
  {
    IMediaData retval = null;
    if ((retval = unwrapVideoPicture()) == null)
      if ((retval = unwrapAudioSamples()) == null)
        if ((retval = unwrapPacket()) == null)
          if ((retval = unwrapMediaDataWrapper()) == null)
            retval = null;
    return retval;
  }
%}
%include <com/xuggle/xuggler/IMediaDataWrapper.h>

%extend com::xuggle::xuggler::IMediaDataWrapper {
  public:
    %javamethodmodifiers getPacket() "protected"
    IPacket* getPacket() {
      IMediaData* data = $self->get();
      IPacket* retval = dynamic_cast<IPacket*>(data);
      if (data && !retval) data->release();
      return retval;
    }
    %javamethodmodifiers getAudioSamples() "protected"
    IAudioSamples* getAudioSamples() {
      IMediaData* data = $self->get();
      IAudioSamples* retval = dynamic_cast<IAudioSamples*>(data);
      if (data && !retval) data->release();
      return retval;
    }
    %javamethodmodifiers getVideoPicture() "protected"
    IVideoPicture* getVideoPicture() {
      IMediaData* data = $self->get();
      IVideoPicture* retval = dynamic_cast<IVideoPicture*>(data);
      if (data && !retval) data->release();
      return retval;
    }
    %javamethodmodifiers getMediaDataWrapper() "protected"
    IMediaDataWrapper* getMediaDataWrapper() {    
      IMediaData* data = $self->get();
      IMediaDataWrapper* retval = dynamic_cast<IMediaDataWrapper*>(data);
      if (data && !retval) data->release();
      return retval;
    }    
    %javamethodmodifiers unwrapPacket() "protected"
    IPacket* unwrapPacket() {
      IMediaData* data = $self->unwrap();
      IPacket* retval = dynamic_cast<IPacket*>(data);
      if (data && !retval) data->release();
      return retval;
    }
    %javamethodmodifiers unwrapAudioSamples() "protected"
    IAudioSamples* unwrapAudioSamples() {
      IMediaData* data = $self->unwrap();
      IAudioSamples* retval = dynamic_cast<IAudioSamples*>(data);
      if (data && !retval) data->release();
      return retval;
    }
    %javamethodmodifiers unwrapVideoPicture() "protected"
    IVideoPicture* unwrapVideoPicture() {
      IMediaData* data = $self->unwrap();
      IVideoPicture* retval = dynamic_cast<IVideoPicture*>(data);
      if (data && !retval) data->release();
      return retval;
    }
    %javamethodmodifiers unwrapMediaDataWrapper() "protected"
    IMediaDataWrapper* unwrapMediaDataWrapper() {    
      IMediaData* data = $self->unwrap();
      IMediaDataWrapper* retval = dynamic_cast<IMediaDataWrapper*>(data);
      if (data && !retval) data->release();
      return retval;
    }
}
