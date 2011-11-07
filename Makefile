PREFIX=/opt/mpd/
VALAC:=$(shell which valac)

PACKAGES:=gtk+-2.0\
		gdk-x11-2.0


SOURCES:=$(wildcard *.vala)

# Desktop files.
DESKTOP_SOURCE=$(wildcard *.desktop.in)
DESKTOP_FILE=$(DESKTOP_SOURCE:.desktop.in=.desktop)

VALA_ARG:=$(foreach parm, $(PACKAGES) posix, --pkg=$(parm)) --vapidir=. --pkg=test

PROGRAM:=MultiMonitorBackground

all: $(PROGRAM) $(DESKTOP_FILE)

$(DESKTOP_FILE): $(DESKTOP_SOURCE)
	sed  's|{PREFIX}|/opt/mpd|g' $^ | sed 's|{PROGRAM}|MultiMonitorBackground|g' > $@

$(PROGRAM): $(SOURCES) Makefile test.c
	$(VALAC) -o $(PROGRAM) $(VALA_ARG) $(SOURCES) -C
	gcc *.c -o $(PROGRAM) `pkg-config --libs --cflags $(PACKAGES) gdk-pixbuf-xlib-2.0 ` -I/usr/include/ -Wall -O2

install: $(PROGRAM) $(DESKTOP) $(DESKTOP_FILE)
	cp $(PROGRAM) $(PREFIX)/bin/
	cp $(DESKTOP_FILE) $(PREFIX)/share/applications/


clean:
	@rm -rf $(PROGRAM) $(DESKTOP_FILE) *.c
