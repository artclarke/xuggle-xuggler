#include <com/xuggle/xuggler/Global.h>
#include <stdio.h>

/**
 * This program exists just to ensure that the shared library we build
 * can be linked against and all symbols can resolve.
 */
int main(int argc, char** argv)
{
  com::xuggle::xuggler::Global::init();
  fprintf(stdout, "%s: Version: %s (avformat: %s; avcodec: %s)\n",
    argc > 0 ? argv[0] : "xuggle-xuggler-main",
  com::xuggle::xuggler::Global::getVersionStr(),
  com::xuggle::xuggler::Global::getAVCodecVersionStr(),
  com::xuggle::xuggler::Global::getAVFormatVersionStr());
  return 0;
}
