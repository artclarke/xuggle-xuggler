%typemap(javapackage) com::xuggle::ferry::RefCounted, com::xuggle::ferry::RefCounted*, com::xuggle::ferry::RefCounted& "com.xuggle.ferry"

%include <com/xuggle/ferry/RefCounted.h>
