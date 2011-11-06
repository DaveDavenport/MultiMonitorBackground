VALAC:=$(shell which valac)

PACKAGES:=gtk+-2.0\
		gdk-x11-2.0

SOURCES:=$(wildcard *.vala)

VALA_ARG:=$(foreach parm, $(PACKAGES), --pkg=$(parm)) 

PROGRAM:=MultiDesktopBackground

all: $(PROGRAM)


$(PROGRAM): $(SOURCES) Makefile
	$(VALAC) -o $(PROGRAM) $(VALA_ARG) $(SOURCES)
