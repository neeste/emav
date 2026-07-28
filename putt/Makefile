# makefile - for PUTT on Mac [8-Jun-2023]

CC=gcc
CFLAGS = -Wall -DALSA -DANSI_C -I$(INCDIR) -O2 -I/usr/X11/include
LIBDIR=../lib/mac
INCDIR=../include
BINDIR=/usr/local/bin
LFLAGS=-framework CoreServices  -framework CoreAudio \
	-framework AudioUnit -framework AudioToolbox \
	-L$(LIBDIR) -L/usr/X11/lib -L/usr/local/lib
LIBS = -ldsp -lfft -lmenu -ltok -lutil -larsc -lX11 -lm #

CFILES=putt.c savage.c naverage.c \
    setatt.c disadbuf.c event.c dofft.c screen.c calfile.c \
    infowind.c chkfit.c calibr.c rdwrdram.c button.c \
    tone_w.c file.c hear_w.c mlmeer.c probe_w.c simp.c
OFILES=putt.o savage.o naverage.o \
    setatt.o disadbuf.o event.o dofft.o screen.o calfile.o \
    infowind.o chkfit.o calibr.o rdwrdram.o button.o \
    tone_w.o file.o hear_w.o mlmeer.o probe_w.o simp.o
EXEFILES=putt

all: $(EXEFILES)

putt : $(OFILES)
	$(CC) $(LFLAGS) -o putt $(OFILES) $(LIBS)

$(BINDIR)/putt : putt
	mkdir -p $(BINDIR)
	install -m 755 $< $(BINDIR)

version: 
	perl version.pl

clean:
	rm -f *.bak *.o *.out *.pdb *.hex *.map *.log *.ncb *.opt
	rm -f *.plg *.cnt *.cv4 *.sts *~ putt putt.exe

install: $(BINDIR)/putt

# C file dependencies
# (makedep will replace everything after this line) 

calfile.o: savage.h putt.h
calibr.o: savage.h color.h screen.h putt.h
chkfit.o: savage.h color.h screen.h putt.h
disadbuf.o: savage.h color.h
dofft.o: savage.h color.h putt.h
event.o: screen.h color.h version.h
file.o: savage.h color.h screen.h putt.h
hear_w.o: savage.h color.h screen.h version.h putt.h
hf.o: putt.h
infowind.o: savage.h color.h
naverage.o: savage.h color.h screen.h putt.h
probe_w.o: savage.h color.h screen.h putt.h
putt.o: savage.h color.h screen.h version.h putt.h
rdwrdram.o: savage.h color.h putt.h
savage.o: savage.h color.h screen.h putt.h
screen.o: screen.h color.h
setatt.o: savage.h
tone_w.o: savage.h color.h screen.h putt.h
