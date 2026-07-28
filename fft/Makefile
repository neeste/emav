# makefile - for FFT library on Mac [8-Jun-2023]

CC=gcc
CFLAGS = -Wall -DALSA -DANSI_C -I$(INCDIR) -O2
OFILES = fastc.o fsstc.o faip.o fsip.o fasubs.o fssubs.o unic.o
LIBDIR=../lib/mac
INCDIR=../include

test: fmc
	./fmc > fmc.txt

fmc: fmc.o libfft.a
	$(CC) -o fmc fmc.o libfft.a -lm
	
libfft.a: $(OFILES)
	ar rus libfft.a $(OFILES)

$(LIBDIR)/libfft.a : libfft.a
	mkdir -p $(LIBDIR)
	cp libfft.a $(LIBDIR)

$(INCDIR)/fftlib.h : fftlib.h
	mkdir -p $(INCDIR)
	cp fftlib.h $(INCDIR)

install: $(LIBDIR)/libfft.a $(INCDIR)/fftlib.h

clean:
	rm -f *.o *.bak *.a fmc fmc.txt
