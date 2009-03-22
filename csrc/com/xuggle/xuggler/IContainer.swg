%typemap(javainterfaces) com::xuggle::xuggler::IContainer, com::xuggle::xuggler::IContainer*, com::xuggle::xuggler::IContainer& "com.xuggle.xuggler.IConfigurable";
%include <com/xuggle/xuggler/IContainer.h>
