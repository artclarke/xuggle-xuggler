%typemap (javacode) com::xuggle::xuggler::IStreamCoder,com::xuggle::xuggler::IStreamCoder*,com::xuggle::xuggler::IStreamCoder& %{

  /**
   * Returns the fourcc tag, in order of least significant byte
   * to most significant byte.
   *
   * @return a 4 char array of the fourcc
   */
  public char[] getCodecTagArray()
  {
    char[] retval = new char[4];
    int fourcc = this.getCodecTag();
    
    retval[0] = (char)(fourcc & 0xFF);
    retval[1] = (char)((fourcc >> 8) & 0xFF);
    retval[2] = (char)((fourcc >> 16) & 0xFF);
    retval[3] = (char)((fourcc >> 24) & 0xFF);
    return retval;
  }
  /**
   * Set the fourcc tag.
   *
   * @param fourcc A four char array, in order of least significant byte
   *  to most significant byte.
   *
   * @throws IllegalArgumentException if the array passed in is not exactly 4 bytes.
   */
   public void setCodecTag(char[] fourcc)
   {
     if (fourcc == null || fourcc.length != 4)
      throw new IllegalArgumentException();
     int tag = 0;
     tag = (fourcc[3]<<24)+(fourcc[2]<<16)+(fourcc[1]<<8)+fourcc[0];
     this.setCodecTag(tag);
   }
%}

%typemap(javainterfaces) com::xuggle::xuggler::IStreamCoder, com::xuggle::xuggler::IStreamCoder*, com::xuggle::xuggler::IStreamCoder& "com.xuggle.xuggler.IConfigurable";
%include <com/xuggle/xuggler/IStreamCoder.h>

