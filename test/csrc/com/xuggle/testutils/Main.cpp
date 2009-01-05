#include <cxxtest/ErrorPrinter.h>
#include <com/xuggle/ferry/JNIHelper.h>

int main() {
 int retval = CxxTest::ErrorPrinter().run();
 // This method is used to clean up static memory
 // to that Valgrind doesn't think I'm a sloppy leaker.
 com::xuggle::ferry::JNIHelper::shutdownHelper();
 return retval;
}

