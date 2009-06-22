## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir)/include 

noinst_PROGRAMS = dump_video $(BUILDABLE_EXAMPLES)

# possible contents of BUILDABLE_EXAMPLES:
EXTRA_PROGRAMS = player_example encoder_example png2theora

AM_CFLAGS = $(OGG_CFLAGS)
LDADD = ../lib/libtheora.la $(OGG_LIBS)
LDADDENC = ../lib/libtheoraenc.la ../lib/libtheoradec.la $(OGG_LIBS)

dump_video_SOURCES = dump_video.c
EXTRA_dump_video_SOURCES = getopt.c getopt1.c getopt.h
dump_video_LDADD = $(GETOPT_OBJS) $(LDADD)
dump_video_DEPENDENCIES = $(GETOPT_OBJS)

player_example_SOURCES = player_example.c
player_example_CFLAGS = $(SDL_CFLAGS) $(OGG_CFLAGS) $(VORBIS_CFLAGS)
player_example_LDADD = $(LDADD) $(SDL_LIBS) $(VORBIS_LIBS)

encoder_example_SOURCES = encoder_example.c
EXTRA_encoder_example_SOURCES = getopt.c getopt1.c getopt.h
encoder_example_CFLAGS = $(OGG_CFLAGS) $(VORBIS_CFLAGS)
encoder_example_LDADD = $(GETOPT_OBJS) $(LDADDENC) $(VORBIS_LIBS) $(VORBISENC_LIBS)
encoder_example_DEPENDENCIES = $(GETOPT_OBJS)

png2theora_SOURCES = png2theora.c
png2theora_CFLAGS = $(OGG_CFLAGS) $(PNG_CFLAGS)
png2theora_LDADD = $(GETOPT_OBJS) $(LDADD) $(PNG_LIBS) -lm
png2theora_DEPENDENCIES = $(GETOPT_OBJS)

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

