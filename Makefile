# Makefile

include config.mak

SRCS = common/mc.c common/predict.c common/pixel.c common/macroblock.c \
       common/frame.c common/dct.c common/cpu.c common/cabac.c \
       common/common.c common/mdate.c common/csp.c common/set.c \
       common/quant.c \
       encoder/analyse.c encoder/me.c encoder/ratecontrol.c \
       encoder/set.c encoder/macroblock.c encoder/cabac.c \
       encoder/cavlc.c encoder/encoder.c encoder/eval.c

SRCCLI = x264.c matroska.c muxers.c

# Visualization sources
ifeq ($(VIS),yes)
SRCS   += common/visualize.c common/display-x11.c
endif

# MMX/SSE optims
ifeq ($(ARCH),X86)
SRCS   += common/i386/mc-c.c common/i386/dct-c.c common/i386/predict-c.c
ASMSRC  = common/i386/dct-a.asm common/i386/cpu-a.asm \
          common/i386/pixel-a.asm common/i386/mc-a.asm \
          common/i386/mc-a2.asm common/i386/predict-a.asm \
          common/i386/pixel-sse2.asm common/i386/quant-a.asm \
          common/i386/deblock-a.asm
OBJASM  = $(ASMSRC:%.asm=%.o)
ASFLAGS += -Icommon/i386/
endif

# MMX/SSE optims
ifeq ($(ARCH),X86_64)
SRCS   += common/i386/mc-c.c common/i386/dct-c.c common/i386/predict-c.c
ASMSRC  = common/amd64/dct-a.asm common/amd64/cpu-a.asm \
          common/amd64/pixel-a.asm common/amd64/mc-a.asm \
          common/amd64/mc-a2.asm common/amd64/predict-a.asm \
          common/amd64/pixel-sse2.asm common/amd64/quant-a.asm \
          common/amd64/deblock-a.asm
OBJASM  = $(ASMSRC:%.asm=%.o)
ASFLAGS += -Icommon/amd64
endif

# AltiVec optims
ifeq ($(ARCH),PPC)
SRCS += common/ppc/mc.c common/ppc/pixel.c common/ppc/dct.c
endif

# VIS optims
ifeq ($(ARCH),UltraSparc)
ASMSRC += common/sparc/pixel.asm
OBJASM  = $(ASMSRC:%.asm=%.o)
endif

ifneq ($(HAVE_GETOPT_LONG),1)
SRCS += extras/getopt.c
endif

OBJS = $(SRCS:%.c=%.o)
OBJCLI = $(SRCCLI:%.c=%.o)
DEP  = depend

.PHONY: default fprofiled clean distclean install uninstall
default: $(DEP) x264$(EXE)

libx264.a: .depend $(OBJS) $(OBJASM)
	ar rc libx264.a $(OBJS) $(OBJASM)
	ranlib libx264.a

$(SONAME): .depend $(OBJS) $(OBJASM)
	$(CC) -shared -o $@ $(OBJS) $(OBJASM) -Wl,-soname,$(SONAME) $(LDFLAGS)

x264$(EXE): $(OBJCLI) libx264.a 
	$(CC) -o $@ $+ $(LDFLAGS)

x264vfw.dll: libx264.a $(wildcard vfw/*.c vfw/*.h)
	$(MAKE) -C vfw/build/cygwin

checkasm: tools/checkasm.o libx264.a
	$(CC) -o $@ $+ $(LDFLAGS)

common/amd64/*.o: common/amd64/amd64inc.asm
common/i386/*.o: common/i386/i386inc.asm
%.o: %.asm
	$(AS) $(ASFLAGS) -o $@ $<

.depend: config.mak
	rm -f .depend
# Hacky - because gcc 2.9x doesn't have -MT
	$(foreach SRC, $(SRCS) $(SRCCLI), ( echo -n "`dirname $(SRC)`/" && $(CC) $(CFLAGS) $(SRC) -MM -g0 ) 1>> .depend;)

config.mak: $(wildcard .svn/entries */.svn/entries */*/.svn/entries)
	./configure $(CONFIGURE_ARGS)

depend: .depend
ifneq ($(wildcard .depend),)
include .depend
endif

SRC2 = $(SRCS) $(SRCCLI)
# These should cover most of the important codepaths
OPT0 = --crf 30 -b1 -m1 -r1 --me dia --no-cabac
OPT1 = --crf 18 -b2 -m3 -r3 --me hex -8 --cqm jvt --direct spatial
OPT2 = --crf 24 -b3 -m6 -r6 --me umh -8 -w -t1 -A all --b-pyramid --b-rdo --mixed-refs

ifeq (,$(VIDS))
fprofiled:
	@echo 'usage: make fprofiled VIDS="infile1 infile2 ..."'
	@echo 'where infiles are anything that x264 understands,'
	@echo 'i.e. YUV with resolution in the filename, or avisynth.'
else
fprofiled:
	$(MAKE) clean
	mv config.mak config.mak2
	sed -e 's/CFLAGS.*/& -fprofile-generate/; s/LDFLAGS.*/& -fprofile-generate/' config.mak2 > config.mak
	$(MAKE) x264$(EXE)
	$(foreach V, $(VIDS), $(foreach I, 0 1 2, ./x264$(EXE) $(OPT$I) $(V) --progress -o $(DEVNULL) ;))
	rm -f $(SRC2:%.c=%.o)
	sed -e 's/CFLAGS.*/& -fprofile-use/; s/LDFLAGS.*/& -fprofile-use/' config.mak2 > config.mak
	$(MAKE)
	rm -f $(SRC2:%.c=%.gcda) $(SRC2:%.c=%.gcno)
	mv config.mak2 config.mak
endif

clean:
	rm -f $(OBJS) $(OBJASM) $(OBJCLI) $(SONAME) *.a x264 x264.exe .depend TAGS
	rm -f checkasm checkasm.exe tools/checkasm.o
	rm -f tools/avc2avi tools/avc2avi.exe tools/avc2avi.o
	rm -rf vfw/build/cygwin/bin
	rm -f $(SRC2:%.c=%.gcda) $(SRC2:%.c=%.gcno)
	- sed -e 's/ *-fprofile-\(generate\|use\)//g' config.mak > config.mak2 && mv config.mak2 config.mak
	$(MAKE) -C gtk clean

distclean: clean
	rm -f config.mak config.h vfw/build/cygwin/config.mak gtk/config.mak x264.pc

install: x264 $(SONAME)
	install -d $(DESTDIR)$(bindir) $(DESTDIR)$(includedir)
	install -d $(DESTDIR)$(libdir) $(DESTDIR)$(libdir)/pkgconfig
	install -m 644 x264.h $(DESTDIR)$(includedir)
	install -m 644 libx264.a $(DESTDIR)$(libdir)
	install -m 644 x264.pc $(DESTDIR)$(libdir)/pkgconfig
	install x264 $(DESTDIR)$(bindir)
	ranlib $(DESTDIR)$(libdir)/libx264.a
	$(if $(SONAME), ln -sf $(DESTDIR)$(libdir)/$(SONAME) $(DESTDIR)$(libdir)/libx264.so)
	$(if $(SONAME), install -m 755 $(SONAME) $(DESTDIR)$(libdir))

uninstall:
	rm -f $(DESTDIR)$(includedir)/x264.h $(DESTDIR)$(libdir)/libx264.a
	rm -f $(DESTDIR)$(bindir)/x264 $(DESTDIR)$(libdir)/pkgconfig/x264.pc
	$(if $(SONAME), rm -f $(DESTDIR)$(libdir)/$(SONAME) $(DESTDIR)$(libdir)/libx264.so)

etags: TAGS

TAGS:
	etags $(SRCS)
