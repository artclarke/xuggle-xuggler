# Makefile

include config.mak

SRCS = common/mc.c common/predict.c common/pixel.c common/macroblock.c \
       common/frame.c common/dct.c common/cpu.c common/cabac.c \
       common/common.c common/mdate.c common/csp.c \
       encoder/analyse.c encoder/me.c encoder/ratecontrol.c \
       encoder/set.c encoder/macroblock.c encoder/cabac.c \
       encoder/cavlc.c encoder/encoder.c encoder/eval.c

# MMX/SSE optims
ifeq ($(ARCH),X86)
SRCS   += common/i386/mc-c.c common/i386/dct-c.c common/i386/predict.c
ASMSRC  = common/i386/dct-a.asm common/i386/cpu-a.asm \
          common/i386/pixel-a.asm common/i386/mc-a.asm \
          common/i386/mc-a2.asm common/i386/predict-a.asm
OBJASM  = $(ASMSRC:%.asm=%.o)
endif

# AltiVec optims
ifeq ($(ARCH),PPC)
SRCS += common/ppc/mc.c common/ppc/pixel.c
endif

OBJS = $(SRCS:%.c=%.o)
DEP  = depend

default: $(DEP) x264

libx264.a: $(DEP) $(OBJS) $(OBJASM)
	ar rc libx264.a $(OBJS) $(OBJASM)
	ranlib libx264.a

x264: libx264.a x264.o
	$(CC) -o $@ x264.o libx264.a $(LDFLAGS)

x264vfw.dll: libx264.a $(wildcard vfw/*.c vfw/*.h)
	make -C vfw/build/cygwin

checkasm: testing/checkasm.o libx264.a
	$(CC) -o $@ $< libx264.a $(LDFLAGS)

%.o: %.asm
	$(AS) $(ASFLAGS) -o $@ $<

.depend: config.mak config.h
	rm -f .depend
# Hacky - because gcc 2.9x doesn't have -MT
	$(foreach SRC, $(SRCS) x264.c, ( echo -n "`dirname $(SRC)`/" && $(CC) $(CFLAGS) $(SRC) -MM -g0 ) 1>> .depend;)

config.h: $(wildcard .svn/entries */.svn/entries */*/.svn/entries)
	./version.sh

depend: .depend
ifneq ($(wildcard .depend),)
include .depend
endif

clean:
	rm -f $(OBJS) $(OBJASM) config.h *.a x264.o .depend x264 TAGS

distclean: clean
	rm -f config.mak vfw/build/cygwin/config.mak

DIR_INSTALL="/usr/local"
install: x264
	install x264.h $(DIR_INSTALL)/include/
	install libx264.a $(DIR_INSTALL)/lib/
	install x264 $(DIR_INSTALL)/bin/

etags: TAGS

TAGS:
	etags $(SRCS)
