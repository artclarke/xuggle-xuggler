
include config.mak

OBJECTS = x264_gtk_bitrate.o x264_gtk_rc.o x264_gtk_mb.o x264_gtk_more.o x264_gtk.o
EXTERNAL_DEPS= ../muxers.o ../matroska.o ../libx264.a

TEST_OBJECT = test.o
ENCODE_OBJECT = x264_gtk_encode_encode.o x264_gtk_encode_status_window.o x264_gtk_encode_main_window.o x264_gtk_encode.o

ifeq ($(SYS),MINGW)
datadir=.
OBJECTS+=x264gtk.o
LDFLAGS+=-mwindows
else
datadir=${prefix}/share/x264
endif

all: x264_gtk_encode test

CPPFLAGS = `pkg-config --cflags gtk+-2.0 gthread-2.0 x264` -DX264_DATA_DIR=\"${datadir}\"
LDFLAGS += `pkg-config --libs gtk+-2.0 gthread-2.0 x264`

# Compilation rule
%.o : %.c
	@echo "  C: $(@D)/$(<F)"
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@
%.o : %.rc
	@echo "  C: $(@D)/$(<F)"
	@windres -o $@ -i $<

x264_icon.h: x264.ico
	@gdk-pixbuf-csource --name=x264_icon --stream x264.ico > x264_icon.h

# Linking rule
libx264gtk.a: $(OBJECTS)
	@echo "  L: $(@F)"
	@ar rc libx264gtk.a $(OBJECTS)
	@ranlib libx264gtk.a

test: $(OBJECTS) $(TEST_OBJECT)
	@echo "  B: $(@F)"
	@$(CC) -o test $(OBJECTS) $(TEST_OBJECT) $(LDFLAGS)

x264_gtk_encode: $(OBJECTS) x264_icon.h libx264gtk.a $(ENCODE_OBJECT) $(EXTERNAL_DEPS)
	@echo "  B: $(@F)"
	@$(CC) -o x264_gtk_encode $(OBJECTS) $(ENCODE_OBJECT) $(EXTERNAL_DEPS) $(LDFLAGS)

# Clean rule
clean:
	@rm -f *.o test x264_gtk_encode libx264gtk.a x264_icon.h

# Install rule
install: x264_gtk_encode
	@echo "  I: $(DESTDIR)$(includedir)/x264_gtk.h"
	@install -m 644 x264_gtk.h $(DESTDIR)$(includedir)
	@echo "  I: $(DESTDIR)$(includedir)/x264_gtk_enum.h"
	@install -m 644 x264_gtk_enum.h $(DESTDIR)$(includedir)
	@echo "  I: $(DESTDIR)$(libdir)/libx264gtk.a"
	@install -m 644 libx264gtk.a $(DESTDIR)$(libdir)
	@echo "  I: $(DESTDIR)$(bindir)/x264_gtk_encode"
	@install x264_gtk_encode $(DESTDIR)$(bindir)
	@echo "  D: $(DESTDIR)$(datadir)"
	@install -d $(DESTDIR)$(datadir)
	@echo "  I: $(DESTDIR)$(datadir)/x264.png"
	@install -m 644 x264.png $(DESTDIR)$(datadir)

# Uninstall rule
uninstall:
	@echo "  U: $(DESTDIR)$(includedir)/x264_gtk.h"
	@rm -f $(DESTDIR)$(includedir)/x264_gtk.h
	@echo "  U: $(DESTDIR)$(includedir)/x264_gtk_enum.h"
	@rm -f $(DESTDIR)$(includedir)/x264_gtk_enum.h
	@echo "  U: $(DESTDIR)$(libdir)/libx264gtk.a"
	@rm -f $(DESTDIR)$(libdir)/libx264gtk.a
	@echo "  U: $(DESTDIR)$(bindir)/x264_gtk_encode"
	@rm -f $(DESTDIR)$(bindir)/x264_gtk_encode
	@echo "  U: $(DESTDIR)$(datadir)"
	@rm -rf $(DESTDIR)$(datadir)

x264gtk.o: x264_gtk.h x264gtk.rc x264.ico x264_gtk_bitrate.h x264_gtk_bitrate.o x264_gtk_rc.h x264_gtk_rc.o x264_gtk_mb.h x264_gtk_mb.o x264_gtk_more.h x264_gtk_more.o 
x264_gtk_bitrate.o: x264_gtk_private.h x264_gtk_enum.h x264_gtk_bitrate.c
x264_gtk_rc.o: x264_gtk_private.h x264_gtk_rc.c
x264_gtk_mb.o: x264_gtk_private.h x264_gtk_mb.c
x264_gtk_more.o: x264_gtk_private.h x264_gtk_more.c
x264_gtk_encode_encode.o: x264_gtk_encode_private.h x264_gtk_encode_encode.c
x264_gtk_encode_status_window.o: x264_gtk_encode_private.h x264_gtk_encode_status_window.c
x264_gtk_encode_main_window.o: x264_gtk_encode_private.h x264_gtk_encode_encode.o x264_gtk_encode_status_window.o x264_gtk.o x264_gtk_encode_main_window.c
x264_gtk_encode.o: x264_gtk_encode_main_window.o x264_gtk_encode.c
test.o: x264_gtk.o test.c
