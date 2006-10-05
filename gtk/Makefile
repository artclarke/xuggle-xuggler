
include config.mak

# Architecture dependant stuff

ifeq ($(SYS),MINGW)
datadir="$(HOMEDRIVE)/Documents\ and\ Settings/$(USERNAME)/Application\ Data"
X264GTK_O = x264gtk.o
TEST_OBJECTS+=$(X264GTK_O)
LDFLAGS+=-mwindows
ICONV_LIB = -liconv
else
datadir=${prefix}/share
X264GTK_O =
ICONV_LIB =
endif

XGETTEXT=xgettext$(EXE)
TEST_BIN = test$(EXE)
ENCODE_BIN = x264_gtk_encode$(EXE)

# PO / MO files
PO_FILES = $(shell ls *.po)
MO_FILES = $(PO_FILES:%.po=%.mo)
ALL_LINGUAS = $(PO_FILES:%.po=%)

# Object files
OBJECTS_LIB =      \
x264_gtk_bitrate.o \
x264_gtk_cqm.o     \
x264_gtk_mb.o      \
x264_gtk_more.o    \
x264_gtk_rc.o      \
x264_gtk.o

OBJECTS_TEST = test.o

OBJECTS_ENCODE =                \
x264_gtk_encode_encode.o        \
x264_gtk_encode_main_window.o   \
x264_gtk_encode_status_window.o \
x264_gtk_encode.o

OBJECTS_ALL = $(OBJECTS_LIB) $(OBJECTS_TEST) $(OBJECTS_ENCODE)
SOURCES_ALL = $(OBJECTS_ALL:%.o=%.c)

X264GTK_PC = x264gtk.pc

EXTERNAL_DEPS= ../muxers.o ../matroska.o ../libx264.a


all: $(ENCODE_BIN) $(TEST_BIN) $(MO_FILES) $(X264GTK_PC)

# Already provides iconv/intl
CPPFLAGS = -g `pkg-config --cflags gtk+-2.0 gthread-2.0` -I.. -DX264_DATA_DIR=\"${datadir}\"
LDFLAGS += `pkg-config --libs gtk+-2.0 gthread-2.0` ../libx264.a


# gettext rules
x264_gtk.pot: $(SOURCES_ALL)
	@echo "  T: x264_gtk.pot"
	@$(XGETTEXT) -o x264_gtk.pot -k'_' -s --from-code iso-8859-1 \
        --default-domain=x264_gtk $(SOURCES_ALL)

%.mo: %.po x264_gtk.pot
	@echo "  T: $@"
	@msgmerge --update $< x264_gtk.pot
	@msgfmt -o $@ $<


# Compilation rule
%.o : %.c
	@echo "  C: $(@D)/$(<F)"
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

ifeq ($(SYS),MINGW)
%.o : %.rc
	@echo "  C: $(@D)/$(<F)"
	@windres -o $@ -i $<
endif


# Creation of the icon pixmap
x264_icon.h: x264.ico
	@gdk-pixbuf-csource --name=x264_icon --stream x264.ico > x264_icon.h


# Linking rule
libx264gtk.a: $(OBJECTS_LIB)
	@echo "  L: $(@F)"
	@ar rc libx264gtk.a $(OBJECTS_LIB)
	@ranlib libx264gtk.a

$(SONAMEGTK): $(OBJECTS_LIB)
	@echo "  L: $(@F)"
	@$(CC) -shared -o $@ $(OBJECTS_LIB) -Wl,-soname,$(SONAMEGTK) $(LDFLAGS)

# Program : test
$(TEST_BIN): $(OBJECTS_LIB) $(OBJECTS_TEST)
	@echo "  B: $(@F)"
#Needs iconv/intl
	@$(CC) -o $(TEST_BIN) $(OBJECTS_LIB) $(OBJECTS_TEST) $(LDFLAGS) $(ICONV_LIB)

# Program : x264_gtk_encode
$(ENCODE_BIN): x264_icon.h libx264gtk.a $(SONAMEGTK) $(OBJECTS_ENCODE) $(EXTERNAL_DEPS) $(X264GTK_O)
	@echo "  B: $(@F)"
	@$(CC) -o $(ENCODE_BIN) $(OBJECTS_LIB) $(OBJECTS_ENCODE) $(EXTERNAL_DEPS) $(LDFLAGS) $(X264GTK_O)

# x264gtk.pc file
$(X264GTK_PC):
	@echo "  S: $(X264GTK_PC)"
	@rm -f $(X264GTK_PC)
	@echo "prefix=$(DESTDIR)${prefix}" > $(X264GTK_PC)
	@echo "exec_prefix=$(DESTDIR)${exec_prefix}" >> $(X264GTK_PC)
	@echo "libdir=$(DESTDIR)${libdir}" >> $(X264GTK_PC)
	@echo "includedir=$(DESTDIR)${includedir}" >> $(X264GTK_PC)
	@echo "" >> $(X264GTK_PC)
	@echo "Name: Gtk+ interface for X264" >> $(X264GTK_PC)
	@echo "Description: Gtk+ interface to configure X264" >> $(X264GTK_PC)
	@echo "Requires: x264" >> $(X264GTK_PC)
	@echo "Version: 0.1" >> $(X264GTK_PC)
	@echo "Libs: -L$(DESTDIR)$(libdir) -lx264gtk" >> $(X264GTK_PC)
	@echo "Cflags: -I$(DESTDIR)$(includedir)" >> $(X264GTK_PC)

# Clean rule
clean:
	@rm -f *.o *.mo x264_gtk.pot $(TEST_BIN) $(ENCODE_BIN) libx264gtk.a $(SONAMEGTK) x264_icon.h

# Distclean rule
distclean:
	@rm -f config.mak $(X264GTK_PC)

# Install rule
install: $(ENCODE_BIN)
	@echo "  D: $(DESTDIR)$(includedir)"
	@install -d "$(DESTDIR)$(includedir)"
	@echo "  I: $(DESTDIR)$(includedir)/x264_gtk.h"
	@install -m 644 x264_gtk.h "$(DESTDIR)$(includedir)"
	@echo "  I: $(DESTDIR)$(includedir)/x264_gtk_enum.h"
	@install -m 644 x264_gtk_enum.h "$(DESTDIR)$(includedir)"
	@echo "  D: $(DESTDIR)$(libdir)"
	@install -d "$(DESTDIR)$(libdir)"
	@echo "  I: $(DESTDIR)$(libdir)/libx264gtk.a"
	@install -m 644 libx264gtk.a "$(DESTDIR)$(libdir)"
	@echo "  I: $(DESTDIR)$(libdir)/libx264gtk.so"
	@$(if $(SONAMEGTK), ln -sf $(SONAMEGTK) $(DESTDIR)$(libdir)/libx264gtk.so)
	@$(if $(SONAMEGTK), install -m 755 $(SONAMEGTK) $(DESTDIR)$(libdir))
	@echo "  D: $(DESTDIR)$(bindir)"
	@install -d "$(DESTDIR)$(bindir)"
	@echo "  I: $(DESTDIR)$(bindir)/$(ENCODE_BIN)"
	@install $(ENCODE_BIN) "$(DESTDIR)$(bindir)"
	@echo "  D: $(DESTDIR)${datadir}/x264"
	@install -d "$(DESTDIR)${datadir}/x264"
	@echo "  I: $(DESTDIR)${datadir}/x264/x264.png"
	@install -m 644 x264.png "$(DESTDIR)${datadir}/x264"
	@for L in $(ALL_LINGUAS); do \
	  echo "  D: $(DESTDIR)${prefix}/share/locale/$$L/LC_MESSAGES"; \
	  install -d "$(DESTDIR)${prefix}/share/locale/$$L/LC_MESSAGES"; \
	  echo "  I: $(DESTDIR)${prefix}/share/locale/$$L/LC_MESSAGES/x264_gtk.mo"; \
	  install -m 644 $$L.mo "$(DESTDIR)${prefix}/share/locale/$$L/LC_MESSAGES/x264_gtk.mo"; \
	done
	@echo "  D: $(DESTDIR)${libdir}/pkgconfig"
	@install -d "$(DESTDIR)${libdir}/pkgconfig"
	@echo "  I: $(DESTDIR)${libdir}/pkgconfig/$(X264GTK_PC)"
	@install -m 644 $(X264GTK_PC) "$(DESTDIR)${libdir}/pkgconfig"

# Uninstall rule
uninstall:
	@echo "  U: $(DESTDIR)$(includedir)/x264_gtk.h"
	@rm -f "$(DESTDIR)$(includedir)/x264_gtk.h"
	@echo "  U: $(DESTDIR)$(includedir)/x264_gtk_enum.h"
	@rm -f "$(DESTDIR)$(includedir)/x264_gtk_enum.h"
	@echo "  U: $(DESTDIR)$(libdir)/libx264gtk.a"
	@rm -f "$(DESTDIR)$(libdir)/libx264gtk.a"
	@echo "  U: $(DESTDIR)$(libdir)/libx264gtk.so"
	@$(if $(SONAMEGTK), rm -f "$(DESTDIR)$(libdir)/$(SONAMEGTK)")
	@rm -f "$(DESTDIR)$(libdir)/libx264gtk.so"
	@echo "  U: $(DESTDIR)$(bindir)/$(ENCODE_BIN)"
	@rm -f "$(DESTDIR)$(bindir)/$(ENCODE_BIN)"
	@echo "  U: $(DESTDIR)${datadir}/x264"
	@rm -rf "$(DESTDIR)${datadir}/x264"
	@for L in $(ALL_LINGUAS); do \
	  echo "  U: $(DESTDIR)${prefix}/share/locale/$$L/LC_MESSAGES/x264_gtk.mo"; \
	  rm -f "$(DESTDIR)${prefix}/share/locale/$$L/LC_MESSAGES/x264_gtk.mo"; \
	done
	@echo "  U: $(DESTDIR)${libdir}/pkgconfig/$(X264GTK_PC)"
	@rm -f "$(DESTDIR)${libdir}/pkgconfig/$(X264GTK_PC)"


x264_gtk.o: x264_gtk.h x264gtk.rc x264.ico x264_gtk_bitrate.h x264_gtk_bitrate.o x264_gtk_rc.h x264_gtk_rc.o x264_gtk_mb.h x264_gtk_mb.o x264_gtk_more.h x264_gtk_more.o  x264_gtk_cqm.h x264_gtk_cqm.o x264_gtk_i18n.h
x264_gtk_bitrate.o: x264_gtk_private.h x264_gtk_enum.h x264_gtk_bitrate.c x264_gtk_i18n.h
x264_gtk_rc.o: x264_gtk_private.h x264_gtk_rc.c x264_gtk_i18n.h
x264_gtk_mb.o: x264_gtk_private.h x264_gtk_mb.c x264_gtk_i18n.h
x264_gtk_more.o: x264_gtk_private.h x264_gtk_more.c x264_gtk_i18n.h
x264_gtk_cqm.o: x264_gtk_private.h x264_gtk_cqm.c x264_gtk_i18n.h
x264_gtk_encode_encode.o: x264_gtk_encode_private.h x264_gtk_encode_encode.c x264_gtk_i18n.h
x264_gtk_encode_status_window.o: x264_gtk_encode_private.h x264_gtk_encode_status_window.c x264_gtk_demuxers.h x264_gtk_i18n.h
x264_gtk_encode_main_window.o: x264_gtk_encode_private.h x264_gtk_encode_encode.o x264_gtk_encode_status_window.o x264_gtk.o x264_gtk_encode_main_window.c x264_gtk_demuxers.h x264_gtk_i18n.h
x264_gtk_encode.o: x264_gtk_encode_main_window.o x264_gtk_encode.c x264_gtk_i18n.h
test.o: x264_gtk.o test.c x264_gtk_i18n.h
