##############################################################################
#
# Makefile for x264 VFW driver
#
# Author: XviD project:
#            - Milan Cutka <cutka at szm.sk>,
#            - Edouard Gomez <ed.gomez at free.fr>
#            - Peter Ross <pross@xvid.org>
# Ported to x264 by Laurent Aimar <fenrir@via.ecp.fr>
#
# $Id: Makefile,v 1.1 2004/06/03 19:29:33 fenrir Exp $
##############################################################################

include config.mak

# Dll to build
DLL=x264vfw.dll

# Installer executable
INST_EXE=x264vfw.exe

# Current dir
DIR_CUR=$(shell pwd)

# Path to include filen library and src
DIR_INC=$(DIR_CUR)/../../..
DIR_LIB=$(DIR_CUR)/../../..
DIR_SRC=$(DIR_CUR)/../..

# Sources
SRC_C= codec.c config.c driverproc.c
SRC_RES= resource.rc

# Alias
RM= rm -rf
WINDRES=windres

##############################################################################
# CFLAGS
##############################################################################

# Constants which should not be modified
# The `mingw-runtime` package is required when building with -mno-cygwin
CFLAGS += -I$(DIR_SRC)/w32api -I$(DIR_INC)
CFLAGS += -D_WIN32_IE=0x0500
CFLAGS += -mno-cygwin

# Optional Compiler options
CFLAGS += -Wall
CFLAGS += -O3

##############################################################################
# Compiler flags for linking stage
##############################################################################

LDFLAGS += -L$(DIR_LIB) -lx264

##############################################################################
# Rules
##############################################################################

OBJECTS = $(SRC_C:.c=.obj)
OBJECTS+= $(SRC_RES:.rc=.obj)

.SUFFIXES: .obj .rc .c

DIR_BUILD= $(DIR_CUR)/bin
VPATH = $(DIR_SRC):$(DIR_BUILD)

all: $(DLL)

$(DIR_BUILD):
	@echo " D: $(DIR_BUILD)"
	@mkdir -p $(DIR_BUILD)

.rc.obj:
	@echo " W: $(@D)/$(<F)"
	@mkdir -p $(DIR_BUILD)/$(@D)
	@$(WINDRES) \
	--include-dir=$(DIR_SRC) \
	--input-format=rc \
	--output-format=coff \
	-o $(DIR_BUILD)/$@ $<

.c.obj:
	@echo " C: $(@D)/$(<F)"
	@mkdir -p $(DIR_BUILD)/$(@D)
	@$(CC) $(CFLAGS) -c -o $(DIR_BUILD)/$@ $<

$(DLL): $(DIR_BUILD) $(OBJECTS)
	@echo " L: $(@F)"
	@cp $(DIR_SRC)/driverproc.def $(DIR_BUILD)/driverproc.def
	@cd $(DIR_BUILD) && \
	$(CC) \
	-mno-cygwin -shared -Wl,-dll,--out-implib,$@.a,--enable-stdcall-fixup \
	-o $@ \
	$(OBJECTS) driverproc.def \
	-lgdi32 -lwinmm -lcomdlg32 -lcomctl32 $(LDFLAGS)

clean:
	@echo " Cl: Object files and target lib"
	@$(RM) $(DIR_BUILD)

install:
	@echo " I: x264vfw.dll"
	@cp $(DIR_BUILD)/$(DLL) $(DLL)
	@cp $(DIR_SRC)/build/win32/bin/x264vfw.inf .
	@rundll32.exe setupapi,InstallHinfSection DefaultInstall 132 $(DIR_CUR)/x264vfw.inf
	@rm $(DLL)
	@rm x264vfw.inf

uninstall:
	@echo " U: x264vfw.dll"
	@cp $(DIR_SRC)/build/win32/bin/x264vfw.inf .
	@rundll32.exe setupapi,InstallHinfSection Remove_x264 132 $(DIR_CUR)/x264vfw.inf
	@rm x264vfw.inf

##############################################################################
# Builds the NSIS installer script for Windows.
# NSIS 2.x is required and makensis.exe should be in the path
##############################################################################

build-installer: $(DLL)
	@cp $(DIR_BUILD)/$(DLL) $(DIR_SRC)/installer
	@makensis $(DIR_SRC)/installer/x264-conf.nsi
	@mv $(DIR_SRC)/installer/$(INST_EXE) $(DIR_BUILD)
	@rm $(DIR_SRC)/installer/$(DLL)
