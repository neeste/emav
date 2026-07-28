# makefile - for MENU library on Mac [8-Jun-2023]

CC=gcc
CFLAGS = -Wall -DALSA -DANSI_C -I$(INCDIR) -O2 -I/usr/X11/include
LIBDIR=../lib/mac
INCDIR=../include
LFLAGS=libmenu.a -L/usr/X11/lib
CFILES = menu.c mainmenu.c decide.c print.c evnt_lnx.c g_xw.c
OFILES = menu.o mainmenu.o decide.o print.o evnt_lnx.o g_xw.o
LIBS = -lX11 -lm
PGMS= test keycode

all: $(PGMS)

test: test.c libmenu.a
	$(CC) -o test $(LFLAGS) test.c -lmenu $(LIBS)

libmenu.a : $(OFILES)
	ar rus libmenu.a $(OFILES)

keycode: keycode.c
	$(CC) -o keycode keycode.c kb.c

$(LIBDIR)/libmenu.a : libmenu.a
	mkdir -p $(LIBDIR)
	cp libmenu.a $(LIBDIR)

$(INCDIR)/menu.h : menu.h
	mkdir -p $(INCDIR)
	cp menu.h $(INCDIR)

install: $(INCDIR)/menu.h $(LIBDIR)/libmenu.a

clean:
	rm -f *.a *.o *.obj *.bak *.exe *.lib *.log *.ps
	rm -f *.ncb *.plg *.opt *~ $(PGMS)

# (makedep will replace everything after this line) 

evnt_lnx.o: menu.h
mainmenu.o: menu.h
menu.o: menu.h
test.o: menu.h
