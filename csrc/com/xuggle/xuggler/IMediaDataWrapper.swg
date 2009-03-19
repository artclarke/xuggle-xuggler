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
