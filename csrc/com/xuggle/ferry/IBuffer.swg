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
     * Returns up to length bytes, starting at offset in the underlying
     * buffer we're managing.
     *
     * <p> 
     * 
     * The buffer position, mark and limit are initialized to zero.  If
     * the limit is going to be used in the calling context, the caller
     * must set the buffer limit to the number of elements in the
     * buffer.
     * 
     * </p>
     * <p>
     *
     * WARNING: If you use this method you are access the direct native
     * memory associated with this buffer.  That means changes you make
     * to this buffer are immediately reflected in the underlying
     * memory.
     *    
     * </p>
     * <p>
     *
     * <b>NOTE FOR THE TRULY PARANOID</b>: Once you call this method,
     * the underlying native memory allocated will not be released until
     * all references to the returned value are no longer reachable and
     * at least one call to {@link JNIMemoryManager#gc()} has been
     * performed.  The {@link JNIMemoryManager#gc()} is called whenever
     * xuggler tries to allocate new memory for any Xuggler interface,
     * so normally you don't need to care about this.  For most Xuggler
     * objects, you never need to care about this, because if for some
     * reason no other Xuggler object is ever allocated (forcing an
     * internal {@link JNIMemoryManager#gc()}), every Xuggler object has
     * a finalizer as well that will do the right thing.
     *
     * </p>
     * <p>
     *
     * But in the case of Java ByteBuffers, we can't set a finalizer, so
     * you may find situations where {@link JNIMemoryManager#gc()} is
     * not automatically called for you.  If you're truly paranoid or
     * haven't called a Xuggler interface in a a while, a call to {@link
     * JNIMemoryManager#gc} never hurts.
     *  
     * </p>
     * 
     * @param offset The offset (in bytes) into the buffer managed by
     *   this IBuffer
     * @param length The requested length (in bytes) you want to access.
     *   The buffer returned may actually be longer than length.
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
      
      // and tell Java this byte buffer is in native order
      retval.order(java.nio.ByteOrder.nativeOrder());

      // zero the position, mark and limit
      retval.position(0);
      retval.mark();
      retval.limit(0);
    }
    return retval;
  }
  
  // <<<<<<<<<<<<<<<<<<<<<<<<<<<
  // IBuffer.swg
%}

%{
static void IBuffer_javaDirectFreeFunc(void *, void * closure);
typedef jobject jNioByteArray;

%}

typedef jobject jNioByteArray;

%include <com/xuggle/ferry/IBuffer.h>
%{
  /**
   * This method is passed as a freefunc to the Buffer object.  Once
   * the IBuffer has no more references to it, this method will be called,
   * and will release the backing java.nio.ByteBuffer object that we got
   * data from.
   */
  static void
  IBuffer_javaDirectFreeFunc(void *, void * closure)
  {
    jobject globalRef = static_cast<jobject>(closure);
    JNIEnv* env = JNIHelper::sGetEnv();
    if (env && globalRef)
    {
      env->DeleteGlobalRef(globalRef);
    }
  }
  
%}

%extend com::xuggle::ferry::IBuffer {
// Used so that extend methods can return a jNioByteArray and it gets mapped
// to java.nio.ByteBuffer

  public:
  %javamethodmodifiers java_getByteBuffer() "private"
  jNioByteArray java_getByteBuffer(int32_t offset, int32_t length)
  {
    void * buffer = 0;
    jobject retval = 0;
    
    buffer = $self->getBytes(offset, length);
    if (buffer)
    {
      JNIEnv *env = JNIHelper::sGetEnv();
      if (env)
      {
        retval = env->NewDirectByteBuffer(buffer, length);
      }
    }
    return retval;
  }

    /**
     * Returns up to length bytes, starting at offset in the
     * underlying buffer we're managing.
     * <p> 
     * This method COPIES the data into the byte array being
     * returned..
     * </p><p>
     * If you don't NEED the direct access that getByteBuffer
     * offers (and most programs can in fact take the performance
     * hit of the copy), we recommend you use this method.
     * It's much harder to accidentally leave native memory lying
     * around waiting for cleanup then.
     * </p>
     * 
     * @param offset The offset (in bytes) into the buffer managed by this IBuffer
     * @param length The requested length (in bytes) you want to access.  The buffer returned may
     *   actually be longer than length.
     * 
     * @return A copy of the data that is in this IBuffer, or null
     *   if error.
     */
  jbyteArray getByteArray(int32_t offset, int32_t length)
  {
    const jbyte * buffer = 0;
    jbyteArray retval = 0;
    
    buffer = static_cast<const jbyte*>($self->getBytes(offset, length));
    if (buffer)
    {
      JNIEnv *env = JNIHelper::sGetEnv();
      if (env)
      {
        retval = env->NewByteArray(length);
        if (env->ExceptionCheck())
        {
          if (retval) env->DeleteLocalRef(retval);
          retval = 0;
        }
        if (retval)
        {
          // copy the data into the byte array
          env->SetByteArrayRegion(retval, 0, length, buffer);
          if (env->ExceptionCheck())
          {
            // an error occurred; release our byte array
            // reference and return.
            env->DeleteLocalRef(retval);
            retval = 0;
          }
        }
      }
    }
    return retval;
  }

  static IBuffer* make(com::xuggle::ferry::RefCounted* requestor,
     jbyteArray buffer, int32_t offset, int32_t length)
  {
    IBuffer* retval = 0;
    try
    {
      JNIEnv* env = JNIHelper::sGetEnv();
      if (!env)
        throw std::runtime_error("could not get java environment");
      
      if (env->ExceptionCheck())
        throw std::runtime_error("pending Java exception");

      if (!buffer)
        throw std::invalid_argument("no byte buffer passed in");
      
      jsize bufSize = env->GetArrayLength(buffer);
      if (env->ExceptionCheck())
        throw std::runtime_error("could not get java byteArray size");

      if (bufSize < offset + length)
        throw std::out_of_range("invalid offset and length");
      
      retval = IBuffer::make(requestor, length);
      if (!retval)
        throw std::runtime_error("could not allocate IBuffer");

      jbyte* bytes = static_cast<jbyte*>(retval->getBytes(0, length));
      if (!bytes)
        throw std::bad_alloc();
      
      // now try the copy
      env->GetByteArrayRegion(buffer, offset, length, bytes);
      if (!env)
        throw std::runtime_error("could not copy data into native IBuffer memory");
    }
    catch(std::exception & c)
    {
      VS_REF_RELEASE(retval);
      throw c;
    }
    return retval;

  }

  static IBuffer* make(com::xuggle::ferry::RefCounted* requestor, jNioByteArray directByteBuffer, int32_t offset, int32_t length)
  {
    IBuffer * retval = 0;
    jobject globalRef = 0;
    JNIEnv* env = JNIHelper::sGetEnv();
    try
    {
      if (!env)
        throw std::runtime_error("could not get java environment");
      
      if (env->ExceptionCheck())
        throw std::runtime_error("pending Java exception");

      if (!directByteBuffer)
        throw std::invalid_argument("no byte buffer passed in");
      
      jclass byteBufferClass = env->FindClass("java/nio/ByteBuffer");
      if (env->ExceptionCheck() || !byteBufferClass)
        throw std::runtime_error("could not get find java/nio/ByteBuffer class");
      jboolean rightClass = env->IsInstanceOf(directByteBuffer,byteBufferClass);
      env->DeleteLocalRef(byteBufferClass);
      if (env->ExceptionCheck())
        throw std::runtime_error("could not get instanceof passed in object");
      if (!rightClass)
      {
        jclass cls=env->FindClass("java/lang/IllegalArgumentException");
        if (cls)
          env->ThrowNew(cls, "object passed in is not instance of java.nio.ByteBuffer");
        throw std::runtime_error("object not instanceof java.nio.ByteBuffer");
      }
      // let's figure out if this is a direct buffer
      int32_t availableLength = env->GetDirectBufferCapacity(directByteBuffer);
      if (env->ExceptionCheck())
        throw std::runtime_error("could not get java byteArray size");
      int8_t* javaBuffer = static_cast<int8_t*>(env->GetDirectBufferAddress(directByteBuffer));
      if (env->ExceptionCheck())
        throw std::runtime_error("could not get java direct byte buffer");
      
      if (availableLength == -1 || !javaBuffer)
      {
        jclass cls=env->FindClass("java/lang/IllegalArgumentException");
        if (cls)
          env->ThrowNew(cls, "object passed in is not instance of java.nio.ByteBuffer or this JVM doesn't allow native code to access direct buffers");
        throw std::runtime_error("object not instanceof java.nio.ByteBuffer");
      }


      if (availableLength < length + offset)
        throw std::runtime_error("not enough data in byte buffer");
      
      // Let's try creating a wrapper around this object.
      // Now, let's get a global reference to remember 
      globalRef = env->NewGlobalRef(directByteBuffer);
      if (env->ExceptionCheck())
        throw std::runtime_error("could not get global reference to passed in byte array");
            
      retval = IBuffer::make(requestor, javaBuffer+offset, length,
          IBuffer_javaDirectFreeFunc, globalRef);
      if (!retval)
        throw std::runtime_error("could not wrap java byte array");
      globalRef = 0;

    }
    catch (std::exception & c)
    {
      if (env && globalRef)
        env->DeleteGlobalRef(globalRef);
      globalRef = 0;
      VS_REF_RELEASE(retval);
      throw c;
    }
    
    return retval;
  }


}
