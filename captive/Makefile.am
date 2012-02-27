ACLOCAL_AMFLAGS=-I m4
include $(abs_top_builddir)/Makefile.global
EXTRA_DIST=COPYING
# Order matters; dependency tree is:
#
# libz
#  |
# openssl                                                    ogg
#    |                                                        |
#  -----                                                -------------
#  |   |                                                |     |     |
#  | rtmp vo-aacenc opencore-amr speex mp3lame x264 vorbis theora   |
#  |   |      |          |         |      |      |      |     |     |
#  ------------------------------------------------------------------
#                                  |
#                               ffmpeg
SUBDIRS= \
  zlib openssl librtmp libvo-aacenc libopencore-amr libspeex \
  libogg libvorbis libtheora \
  libmp3lame libx264 ffmpeg

