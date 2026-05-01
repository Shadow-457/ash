CC      = gcc
CFLAGS  = -O2 -Wall -Wextra $(shell pkg-config --cflags gtk+-3.0 vte-2.91)
LIBS    = $(shell pkg-config --libs   gtk+-3.0 vte-2.91)
PREFIX  = /usr/local

all: ash

ash: ash.c config.h
	$(CC) $(CFLAGS) -o $@ ash.c $(LIBS)

install: ash
	install -Dm755 ash         $(DESTDIR)$(PREFIX)/bin/ash
	install -Dm644 ash.svg     $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/ash.svg
	install -Dm644 ash.desktop $(DESTDIR)$(PREFIX)/share/applications/ash.desktop
	gtk-update-icon-cache /usr/share/icons/hicolor 2>/dev/null || true
	update-desktop-database 2>/dev/null || true

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/ash
	rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/ash.svg
	rm -f $(DESTDIR)$(PREFIX)/share/applications/ash.desktop

clean:
	rm -f ash

.PHONY: all install uninstall clean
