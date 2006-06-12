
include config.mak

# Architecture dependant stuff

ifeq ($(SYS),MINGW)
datadir=.
TEST_OBJECTS+=x264gtk.o
LDFLAGS+=-mwindows
XGETTEXT=xgettext.exe
X264GTK_O = x264gtk.o
ICONV_LIB = -liconv
TEST_BIN = test.exe
ENCODE_BIN = x264_gtk_encode.exe
else
datadir=${prefix}/share
XGETTEXT=xgettext
X264GTK_O =
ICONV_LIB =
TEST_BIN = test
ENCODE_BIN = x264_gtk_encode
endif


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

EXTERNAL_DEPS= ../muxers.o ../matroska.o ../libx264.a


all: $(ENCODE_BIN) $(TEST_BIN) $(MO_FILES)

# Already provides iconv/intl
CPPFLAGS = -g `pkg-config --cflags gtk+-2.0 gthread-2.0` -I.. -DX264_DATA_DIR=\"${datadir}\"
LDFLAGS += `pkg-config --libs gtk+-2.0 gthread-2.0 x264`


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

# Program : test
$(TEST_BIN): $(OBJECTS_LIB) $(OBJECTS_TEST)
	@echo "  B: $(@F)"
#Needs iconv/intl
	@$(CC) -o $(TEST_BIN) $(OBJECTS_LIB) $(OBJECTS_TEST) $(LDFLAGS) $(ICONV_LIB) -lintl

# Program : x264_gtk_encode
$(ENCODE_BIN): $(OBJECTS_LIB) x264_icon.h libx264gtk.a $(OBJECTS_ENCODE) $(EXTERNAL_DEPS) $(X264GTK_O)
	@echo "  B: $(@F)"
	@$(CC) -o $(ENCODE_BIN) $(OBJECTS_LIB) $(OBJECTS_ENCODE) $(EXTERNAL_DEPS) $(LDFLAGS) $(X264GTK_O)

# Clean rule
clean:
	@rm -f *.o *.mo x264_gtk.pot $(TEST_BIN) $(ENCODE_BIN) libx264gtk.a x264_icon.h x264gtk.o

# Install rule
install: x264_gtk_encode
	@echo "  I: $(DESTDIR)$(includedir)/x264_gtk.h"
	@install -m 644 x264_gtk.h "$(DESTDIR)$(includedir)"
	@echo "  I: $(DESTDIR)$(includedir)/x264_gtk_enum.h"
	@install -m 644 x264_gtk_enum.h "$(DESTDIR)$(includedir)"
	@echo "  I: $(DESTDIR)$(libdir)/libx264gtk.a"
	@install -m 644 libx264gtk.a "$(DESTDIR)$(libdir)"
	@echo "  I: $(DESTDIR)$(bindir)/x264_gtk_encode"
	@install x264_gtk_encode "$(DESTDIR)$(bindir)"
	@echo "  D: ${prefix}/share/x264"
	@install -d "$(DESTDIR)${prefix}/share/x264"
	@echo "  I: ${prefix}/share/x264.png"
	@install -m 644 x264.png "$(DESTDIR)${prefix}/share/x264"
	@for L in $(ALL_LINGUAS); do \
	  echo "  D: $(DESTDIR)${prefix}/share/locale/$$L/LC_MESSAGES"; \
	  install -d "$(DESTDIR)${prefix}/share/locale/$$L/LC_MESSAGES"; \
	  echo "  I: $(DESTDIR)${prefix}/share/locale/$$L/LC_MESSAGES/x264_gtk.mo"; \
	  install -m 644 $$L.mo "$(DESTDIR)${prefix}/share/locale/$$L/LC_MESSAGES/x264_gtk.mo"; \
	done

# Uninstall rule
uninstall:
	@echo "  U: $(DESTDIR)$(includedir)/x264_gtk.h"
	@rm -f "$(DESTDIR)$(includedir)/x264_gtk.h"
	@echo "  U: $(DESTDIR)$(includedir)/x264_gtk_enum.h"
	@rm -f "$(DESTDIR)$(includedir)/x264_gtk_enum.h"
	@echo "  U: $(DESTDIR)$(libdir)/libx264gtk.a"
	@rm -f "$(DESTDIR)$(libdir)/libx264gtk.a"
	@echo "  U: $(DESTDIR)$(bindir)/x264_gtk_encode"
	@rm -f "$(DESTDIR)$(bindir)/x264_gtk_encode"
	@echo "  U: $(DESTDIR)${prefix}/share/x264"
	@rm -rf "$(DESTDIR)${prefix}/share/x264"
	@for L in $(ALL_LINGUAS); do \
	  echo "  U: $(DESTDIR)${prefix}/share/locale/$$L/LC_MESSAGES/x264_gtk.mo"; \
	  rm -f "$(DESTDIR)${prefix}/share/locale/$$L/LC_MESSAGES/x264_gtk.mo"; \
	done


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
