##############################################################################
#
# Makefile for lib264.a and x264
#
# Author: x264 by Laurent Aimar <fenrir@via.ecp.fr>
#
# $Id: Makefile,v 1.4 2004/06/18 02:00:40 chenm001 Exp $
##############################################################################

# Current dir
DIR_CUR=$(shell pwd)

# Path to src
DIR_SRC=$(DIR_CUR)/../..

# Sources
SRC_C= core/mc.c core/predict.c core/pixel.c core/macroblock.c \
       core/frame.c core/dct.c core/cpu.c core/cabac.c \
       core/common.c core/mdate.c core/csp.c \
       encoder/analyse.c encoder/me.c encoder/ratecontrol.c \
       encoder/set.c encoder/macroblock.c encoder/cabac.c encoder/cavlc.c \
       encoder/encoder.c \
       core/i386/mc-c.c core/i386/dct-c.c core/i386/predict.c

SRC_ASM= core/i386/dct.asm core/i386/cpu.asm core/i386/pixel.asm core/i386/mc.asm

# Alias
RM= rm -rf

##############################################################################
# PFLAGS
##############################################################################
PFLAGS=-DARCH_X86 -DHAVE_MMXEXT -UHAVE_SSE2

##############################################################################
# CFLAGS
##############################################################################

# Constants which should not be modified
# The `mingw-runtime` package is required when building with -mno-cygwin
CFLAGS += -I$(DIR_SRC)
CFLAGS += -mno-cygwin
CFLAGS += -D__X264__ -D_CYGWIN
CFLAGS += $(PFLAGS)

# Optional Compiler options
CFLAGS += -g -Wall -DDEBUG
CFLAGS += -O3
CFLAGS += -finline-functions
CFLAGS += -funroll-loops
CFLAGS += -ffast-math


##############################################################################
# Compiler flags for linking stage
##############################################################################

LDFLAGS += -L$(DIR_LIB) -lx264

##############################################################################
# ASM
##############################################################################
AS= nasm
ASFLAGS= -f win32 -DPREFIX $(PFLAGS)
##############################################################################
# Rules
##############################################################################

OBJECTS = $(SRC_C:.c=.obj)
OBJECTS+= $(SRC_ASM:.asm=.obj)

.SUFFIXES: .obj .asm .c

DIR_BUILD= $(DIR_CUR)/bin
VPATH = $(DIR_SRC):$(DIR_BUILD)

all: libx264.a x264.exe

$(DIR_BUILD):
	@echo " D: $(DIR_BUILD)"
	@mkdir -p $(DIR_BUILD)

.asm.obj:
	@echo " A: $(@D)/$(<F)"
	@mkdir -p $(DIR_BUILD)/$(@D)
	@$(AS) $(ASFLAGS) -o $(DIR_BUILD)/$@ $<

.c.obj:
	@echo " C: $(@D)/$(<F)"
	@mkdir -p $(DIR_BUILD)/$(@D)
	@$(CC) $(CFLAGS) -c -o $(DIR_BUILD)/$@ $<

libx264.a: $(DIR_BUILD) $(OBJECTS)
	@echo " L: $(@F)"
	@cd $(DIR_BUILD) && \
	ar rc libx264.a $(OBJECTS) && \
	cp -f libx264.a $(DIR_SRC)

x264.exe: $(DIR_BUILD) $(OBJECTS) x264.obj
	@echo " L: $(@F)"
	@cd $(DIR_BUILD) && \
	$(CC) $(CFLAGS) -o x264.exe x264.obj libx264.a -lm && \
	cp -f x264.exe $(DIR_SRC)

checkasm.exe: $(DIR_BUILD) libx264.a $(OBJECTS) testing/checkasm.obj
	@echo " L: $(@F)"
	@cd $(DIR_BUILD) && \
	$(CC) $(CFLAGS) -o checkasm.exe testing/checkasm.obj libx264.a -lm && \
	cp -f checkasm.exe $(DIR_SRC)

clean:
	@echo " Cl: Object files and target lib"
	@$(RM) $(DIR_BUILD)

