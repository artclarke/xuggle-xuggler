# Makefile: tuned for i386/MMX system only
# For ppc append
#  SRCS: core/ppc/mc.c core/ppc/pixel.c 
#  Defines: HAVE_PPC
#  CFLAGS: -faltivec
#
PFLAGS=-DARCH_X86 -DHAVE_MMXEXT -DHAVE_SSE2
CC=gcc
CFLAGS=-g -Wall -I. -DDEBUG -O4 -funroll-loops -D__X264__ -DHAVE_MALLOC_H $(PFLAGS)

SRCS=  core/mc.c core/predict.c core/pixel.c core/macroblock.c \
       core/frame.c core/dct.c core/cpu.c core/cabac.c \
       core/common.c core/mdate.c core/csp.c \
       encoder/analyse.c encoder/me.c encoder/ratecontrol.c \
       encoder/set.c encoder/macroblock.c encoder/cabac.c encoder/cavlc.c \
       encoder/encoder.c \
       core/i386/mc-c.c core/i386/dct-c.c core/i386/predict.c \
       x264.c

AS= nasm
# for linux
ASFLAGS=-f elf $(PFLAGS)
# for cygwin
#ASFLAGS=-f gnuwin32 -DPREFIX

ASMSRC= core/i386/dct.asm core/i386/cpu.asm core/i386/pixel.asm  core/i386/mc.asm
OBJASM= $(ASMSRC:%.asm=%.o)

OBJS = $(SRCS:%.c=%.o)
DEP  = depend

default: $(DEP) x264

libx264.a: $(OBJS) $(OBJASM)
	ar rc libx264.a $(OBJS) $(OBJASM)

x264: libx264.a x264.o
	$(CC) $(CFLAGS) -o x264 x264.o libx264.a -lm

checkasm: testing/checkasm.c libx264.a
	$(CC) $(CFLAGS) -o checkasm $< libx264.a -lm

%.o: %.asm
	$(AS) $(ASFLAGS) -o $@ $<

.depend: $(SRCS) x264.c
	$(CC) -MM $(CFLAGS) $(SRCS) x264.c 1> .depend

depend: .depend
ifneq ($(wildcard .depend),)
include .depend
endif

clean:
	rm -f $(OBJS) $(OBJASM) *.a x264.o .depend x264

distclean:
	rm -f $(OBJS) $(OBJASM) *.a x264.o .depend x264

