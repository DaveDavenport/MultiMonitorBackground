PREFIX=/opt/mpd/
VALAC:=$(shell which valac)

PACKAGES:=gtk+-2.0\
		gdk-x11-2.0

SOURCES:=$(wildcard *.vala)
DESKTOP:=$(wildcard *.desktop)

VALA_ARG:=$(foreach parm, $(PACKAGES), --pkg=$(parm)) 

PROGRAM:=MultiMonitorBackground

all: $(PROGRAM)


$(PROGRAM): $(SOURCES) Makefile
	$(VALAC) -o $(PROGRAM) $(VALA_ARG) $(SOURCES)

install: $(PROGRAM) $(DESKTOP)
	cp $(PROGRAM) $(PREFIX)/bin/
	cp $(DESKTOP) $(PREFIX)/share/applications/
