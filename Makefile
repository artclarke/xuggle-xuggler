# Makefile: tuned for i386/MMX system only

# For FreeBSD, remove -DHAVE_MALLOC_H and add -DSYS_FREEBSD

# Uncomment this for Mac OS X
#SYS_MACOSX=1

SRCS_COMMON= common/mc.c common/predict.c common/pixel.c common/macroblock.c \
             common/frame.c common/dct.c common/cpu.c common/cabac.c \
             common/common.c common/mdate.c common/csp.c \
             encoder/analyse.c encoder/me.c encoder/ratecontrol.c \
             encoder/set.c encoder/macroblock.c encoder/cabac.c \
             encoder/cavlc.c encoder/encoder.c encoder/eval.c

ifdef SYS_MACOSX
PFLAGS=-DARCH_PPC -DSYS_MACOSX -faltivec
SRCS= $(SRCS_COMMON) common/ppc/mc.c common/ppc/pixel.c
else
PFLAGS=-DARCH_X86 -DHAVE_MMXEXT -DHAVE_SSE2 -DHAVE_MALLOC_H
SRCS= $(SRCS_COMMON) common/i386/mc-c.c common/i386/dct-c.c common/i386/predict.c
ASMSRC= common/i386/dct-a.asm common/i386/cpu-a.asm common/i386/pixel-a.asm common/i386/mc-a.asm
OBJASM= $(ASMSRC:%.asm=%.o)
endif

CC=gcc
CFLAGS=-Wall -I. -O4 -funroll-loops -D__X264__ $(PFLAGS)
ifdef NDEBUG
CFLAGS+=-s -DNDEBUG
else
CFLAGS+=-g -DDEBUG
endif

AS= nasm
# for linux
ASFLAGS=-f elf $(PFLAGS)
# for cygwin
#ASFLAGS=-f gnuwin32 -DPREFIX

OBJS = $(SRCS:%.c=%.o)
DEP  = depend

default: $(DEP) x264

libx264.a: $(OBJS) $(OBJASM)
	ar rc libx264.a $(OBJS) $(OBJASM)
	ranlib libx264.a

x264: libx264.a x264.o
	$(CC) $(CFLAGS) -o x264 x264.o libx264.a -lm

checkasm: testing/checkasm.c libx264.a
	$(CC) $(CFLAGS) -o checkasm $< libx264.a -lm

%.o: %.asm
	$(AS) $(ASFLAGS) -o $@ $<

.depend: $(SRCS) x264.c
	rm -f .depend
	$(foreach SRC, $(SRCS) x264.c, $(CC) $(CFLAGS) $(SRC) -MM -MT $(SRC:%.c=%.o) 1>> .depend;)

depend: .depend
ifneq ($(wildcard .depend),)
include .depend
endif

clean:
	rm -f $(OBJS) $(OBJASM) *.a x264.o .depend x264 TAGS

distclean: clean

etags: TAGS

TAGS:
	etags $(SRCS)
