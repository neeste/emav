# makefile - for TOKEN library on Mac [8-Jun-2023]

CC=gcc
CFLAGS = -Wall -DALSA -DANSI_C -I$(INCDIR) -O2
OFILES= topen.o tcreat.o tinit.o tread.o twrite.o tclose.o
LIBDIR=../lib/mac
INCDIR=../include

libtok.a : $(OFILES)
	ar rus libtok.a $(OFILES)

# install

$(LIBDIR)/libtok.a : libtok.a
	mkdir -p $(LIBDIR)
	cp  libtok.a $(LIBDIR)/libtok.a

$(INCDIR)/token.h : token.h
	mkdir -p $(INCDIR)
	cp  token.h $(INCDIR)/token.h

install : $(LIBDIR)/libtok.a $(INCDIR)/token.h

clean:
	rm -f *~ *.bak *.o *.a *.obj *.lib

# dependencies

misc.o : misc.c
topen.o : topen.c tokio.h
tcreat.o : topen.c tokio.h
tinit.o : topen.c tokio.h
tread.o : tread.c tokio.h
twrite.o : twrite.c tokio.h
tclose.o : tclose.c tokio.h
