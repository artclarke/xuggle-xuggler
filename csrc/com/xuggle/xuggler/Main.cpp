#include <com/xuggle/xuggler/Global.h>
#include <stdio.h>

using namespace com::xuggle::xuggler;

/**
 * This program exists just to ensure that the shared library we build
 * can be linked against and all symbols can resolve.
 */
int main(int argc, char** argv)
{
  Global::init();
  fprintf(stdout, "%s: Version: %s (avformat: %s; avcodec: %s)\n",
    argc > 0 ? argv[0] : "xuggle-xuggler-main",
    Global::getVersionStr(),
    Global::getAVCodecVersionStr(),
    Global::getAVFormatVersionStr());
  return 0;
}
