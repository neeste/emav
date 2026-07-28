# makefile - for ABRAV on Mac [8-Jun-2023]

CC=gcc
CFLAGS = -Wall -DALSA -DANSI_C -I$(INCDIR) -O2 -I/usr/X11/include
LIBDIR=../lib/mac
INCDIR=../include
BINDIR=/usr/local/bin
LFLAGS=-framework CoreServices  -framework CoreAudio \
	-framework AudioUnit -framework AudioToolbox \
	-L$(LIBDIR) -L/usr/X11/lib -L/usr/local/lib
LIBS = -ldsp -lfft -lmenu -ltok -lutil -larsc -lX11 -lm #

CFILES = abrav.c savage.c naverage.c dcremo.c \
    disadbuf.c event.c dofft.c screen.c \
    infowind.c abr_w.c rdwrdram.c edit.c \
    tone_w.c file.c var.c epamp.c 
OFILES=abrav.o savage.o naverage.o dcremo.o \
    disadbuf.o event.o dofft.o screen.o \
    infowind.o abr_w.o rdwrdram.o edit.o \
    tone_w.o file.o var.o epamp.o 
EXEFILES=abrav

all: $(EXEFILES) 

abrav : $(OFILES)
	$(CC) $(LFLAGS) -o abrav $(OFILES) $(LIBS)

install: $(BINDIR)/abrav 

clean:
	rm -f *.bak *.o *.out *.pdb *.hex *.map *.log *.ncb *.opt
	rm -f *.plg *.cnt *.cv4 *.sts abrav abrav.exe *~

version: 
	perl version.pl

# C file dependencies
# (makedep will replace everything after this line) 

abr_w.o: savage.h color.h screen.h version.h
abrav.o: savage.h color.h screen.h version.h abrav.h
disadbuf.o: savage.h color.h
dofft.o: savage.h color.h
edit.o: savage.h color.h screen.h
event.o: color.h abrav.h
file.o: savage.h color.h screen.h version.h abrav.h
infowind.o: savage.h color.h
naverage.o: savage.h color.h screen.h version.h
rdwrdram.o: savage.h color.h
savage.o: savage.h color.h
screen.o: screen.h color.h
tone_w.o: savage.h color.h screen.h
