# Makefile

# Uncomment the line for your platform
ARCH=X86
#ARCH=PPC

# Uncomment the line for you operating system
SYS=LINUX
#SYS=MACOSX
#SYS=BEOS
#SYS=FREEBSD

SRCS_COMMON= common/mc.c common/predict.c common/pixel.c common/macroblock.c \
             common/frame.c common/dct.c common/cpu.c common/cabac.c \
             common/common.c common/mdate.c common/csp.c \
             encoder/analyse.c encoder/me.c encoder/ratecontrol.c \
             encoder/set.c encoder/macroblock.c encoder/cabac.c \
             encoder/cavlc.c encoder/encoder.c encoder/eval.c

# Compiler, global flags
CC=gcc
CFLAGS=-Wall -I. -O4 -funroll-loops -D__X264__ -DARCH_$(ARCH) -DSYS_$(SYS)
ifdef NDEBUG
CFLAGS+=-s -DNDEBUG
else
CFLAGS+=-g -DDEBUG
endif
SRCS= $(SRCS_COMMON)

# MMX/SSE optims
ifeq ($(ARCH),X86)
CFLAGS+=-DHAVE_MMXEXT -DHAVE_SSE2
SRCS+= common/i386/mc-c.c common/i386/dct-c.c common/i386/predict.c
ASMSRC= common/i386/dct-a.asm common/i386/cpu-a.asm \
        common/i386/pixel-a.asm common/i386/mc-a.asm
OBJASM= $(ASMSRC:%.asm=%.o)
endif

# AltiVec optims
ifeq ($(ARCH),PPC)
ifeq ($(SYS),MACOSX)
CFLAGS+=-faltivec
else
CFLAGS+=-maltivec -mabi=altivec
endif
SRCS+= common/ppc/mc.c common/ppc/pixel.c
endif

# stdint.h: everyone but BeOS
ifneq ($(SYS),BEOS)
CFLAGS+=-DHAVE_STDINT_H
endif

# malloc.h: everyone but OS X and FreeBSD
ifneq ($(SYS),MACOSX)
ifneq ($(SYS),FREEBSD)
CFLAGS+=-DHAVE_MALLOC_H
endif
endif

# Math libraries we have to link to
ifneq ($(SYS),BEOS)
MATHLIBS=-lm
endif
ifeq ($(SYS),MACOSX)
MATHLIBS+=-lmx
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
	$(CC) $(CFLAGS) -o x264 x264.o libx264.a $(MATHLIBS)

checkasm: testing/checkasm.c libx264.a
	$(CC) $(CFLAGS) -o checkasm $< libx264.a $(MATHLIBS)

%.o: %.asm
	$(AS) $(ASFLAGS) -o $@ $<

.depend: $(SRCS) x264.c
	rm -f .depend
# Hacky - because gcc 2.9x doesn't have -MT
	$(foreach SRC, $(SRCS) x264.c, ( echo -n "`dirname $(SRC)`/" && $(CC) $(CFLAGS) $(SRC) -MM -g0 ) 1>> .depend;)

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
