%typemap(javapackage) com::xuggle::ferry::IBuffer, com::xuggle::ferry::IBuffer*, com::xuggle::ferry::IBuffer& "com.xuggle.ferry"

%typemap(javabody_derived) com::xuggle::ferry::IBuffer %{
  // IBuffer.swg: Start generated code
  // >>>>>>>>>>>>>>>>>>>>>>>>>>>

  private volatile long swigCPtr;

  /**
   * DO NOT USE THIS CONSTRUCTOR - USE {@link #make(RefCounted,int)} instead.
   * <p>
   * Do not allocate this object using new.  Not part of public API.
   * </p>
   * <p>
   * Unfortunately this constructor is public because the internal
   * implementation needs it to be, but do not pass in values to this method
   * as you may end up crashing the virtual machine.
   * </p>
   *
   * @param pissOff Did we mention don't call this.
   * @param weMeanIt I'm not even going to tell you.  Stop it.  Go away.
   *
   */ 
  public $javaclassname(long pissOff, boolean weMeanIt) {
    super($imclassname.SWIG$javaclassnameUpcast(pissOff), weMeanIt);
    swigCPtr = pissOff;
  }
  
  /**
   * Not part of public API.
   *
   * Get the raw value of the native object that obj is proxying for.
   *   
   * @param obj The java proxy object for a native object.
   * @return The raw pointer obj is proxying for.
   */
  public static long getCPtr($javaclassname obj) {
    if (obj == null) return 0;
    return obj.getMyCPtr();
  }

  /**
   * Not part of public API.
   *
   * Get the raw value of the native object that we're proxying for.
   *   
   * @return The raw pointer we're proxying for.
   */  
  public long getMyCPtr() {
    if (swigCPtr == 0) throw new IllegalStateException("underlying native object already deleted");
    return swigCPtr;
  }
  
  /**
   * Create a new $javaclassname object that is actually referring to the
   * exact same underlying Native object.
   *
   * This method increases the ref count of the underlying Native object.
   *
   * @return the new Java object.
   */
  public $javaclassname copyReference() {
    if (swigCPtr == 0)
      return null;
    else
    {
      $javaclassname retval = new $javaclassname(swigCPtr, false);
      retval.acquire();
      return retval;
    }
  }

  /**
   * Compares two values, returning true if the underlying objects in native code are the same object.
   *
   * That means you can have two different Java objects, but when you do a comparison, you'll find out
   * they are the EXACT same object.
   *
   * @return True if the underlying native object is the same.  False otherwise.
   */
  public boolean equals(Object obj) {
    boolean equal = false;
    if (obj instanceof $javaclassname)
      equal = ((($javaclassname)obj).swigCPtr == this.swigCPtr);
    return equal;
  }
  
  /**
   * Get a hashable value for this object.
   *
   * @return the hashable value.
   */
  public int hashCode() {
     return (int)swigCPtr;
  }
  
    /**
     * Returns up to length bytes, starting at offset in the
     * underlying buffer we're managing.
     * <p> 
     * WARNING: If you use this method you are access the direct native
     * memory associated with this buffer.  That means changes
     * you make to this buffer are immediately reflected in the underlying
     * memory.
     * </p>
     * <p>
     * <b>NOTE FOR THE TRULY PARANOID</b>: Once you call this method,
     * the underlying native memory allocated will not be released
     * until all references to the returned value are no longer reachable
     * and at least one call to {@link JNIMemoryManager#gc()} has been
     * performed.  The {@link JNIMemoryManager#gc()} is called whenever
     * xuggler tries to allocate new memory for any Xuggler interface,
     * so normally you don't need
     * to care about this.  For most Xuggler objects, you never need to care about
     * this, because if for some reason no other Xuggler object is ever allocated
     * (forcing an internal {@link JNIMemoryManager#gc()}), every Xuggler object has
     * a finalizer as well that will do the right thing.
     * </p>
     * <p>But in the case of Java ByteBuffers, we can't set a finalizer, so you
     * may find situations where {@link JNIMemoryManager#gc()} is not automatically
     * called for you.  If you're truly paranoid or haven't called a Xuggler interface in a
     * a while, a call to {@link JNIMemoryManager#gc}
     * never hurts.
     *  
     * </p>
     * </p>
     * 
     * @param offset The offset (in bytes) into the buffer managed by this IBuffer
     * @param length The requested length (in bytes) you want to access.  The buffer returned may
     *   actually be longer than length.
     * 
     * @return A java.nio.ByteBuffer that directly accesses
     *   the native memory this IBuffer manages, or null if
     *   error.
     */
  public java.nio.ByteBuffer getByteBuffer(int offset, int length)
  {
    java.nio.ByteBuffer retval = this.java_getByteBuffer(offset, length);
    if (retval != null)
    {
      // increment the ref count of this class to reflect the
      // byte buffer
      FerryJNI.RefCounted_acquire(swigCPtr, null);
      
      // and use the byte buffer as the reference to track
      JNIWeakReference.createReference(retval, swigCPtr);
    }
    return retval;
  }
  
  // <<<<<<<<<<<<<<<<<<<<<<<<<<<
  // IBuffer.swg
%}

%include <com/xuggle/ferry/IBuffer.h>
