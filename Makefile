# makefile for AV on Mac

MAKE= make 
PGMS= abrav/abrav emav/emav putt/putt
LIBD=lib/linux
LIBS= $(LIBD)/libfft.a $(LIBD)/libmenu.a $(LIBD)/libtok.a \
	$(LIBD)/libutil.a $(LIBD)/libdsp.a  

build : $(PGMS)

install: $(PGMS)
	cd abrav ; $(MAKE) install
	cd emav ; $(MAKE) install
	cd putt ; $(MAKE) install

# build apps

abrav/abrav : $(LIBS)
	cd abrav ; $(MAKE)
emav/emav : $(LIBS)
	cd emav ; $(MAKE)
putt/putt : $(LIBS)
	cd putt ; $(MAKE)

# install libs

$(LIBD)/libdsp.a : dsp/libdsp.a $(LIBDIR)
	cd dsp ; $(MAKE) install
$(LIBD)/libfft.a : fft/libfft.a $(LIBDIR)
	cd fft ; $(MAKE) install
$(LIBD)/libmenu.a : menu/libmenu.a $(LIBDIR)
	cd menu ; $(MAKE) install
$(LIBD)/libtok.a : tok/libtok.a lib $(LIBDIR)
	cd tok ; $(MAKE) install
$(LIBD)/libutil.a : util/libutil.a lib $(LIBDIR)
	cd util ; $(MAKE) install

# build libs

$(LIBDIR) :
	mkdir lib 
	mkdir $(LIBDIR)
dsp/libdsp.a :
	cd dsp ; $(MAKE) libdsp.a
fft/libfft.a :
	cd fft ; $(MAKE) libfft.a
menu/libmenu.a :
	cd menu ; $(MAKE) libmenu.a
tok/libtok.a :
	cd tok ; $(MAKE) libtok.a
util/libutil.a :
	cd util ; $(MAKE) libutil.a

avsc.zip :
	rm -f avsc.zip
	zip avsc *.lnx *.mac
	zip avsc abrav/*.asm  abrav/wabrav/*.txt
	zip avsc abrav/*.c abrav/*.h abrav/*.bat abrav/*.ini abrav/*.lst
	zip avsc abrav/*.iss
	zip avsc abrav/*.rc abrav/*.ico 
	zip avsc abrav/abrav/*.lnx abrav/*.mac abrav/*.pl
	zip avsc dsp/*.c dsp/*.h dsp/*.asm 
	zip avsc dsp/dsp/*.lnx dsp/*.mac
	zip avsc emav/*.asm emav/*.txt emav/wemav/*.txt
	zip avsc emav/*.c emav/*.h emav/*.bat emav/*.ini 
	zip avsc emav/*.iss
	zip avsc emav/*.rc emav/*.ico 
	zip avsc emav/emav/*.lnx emav/*.mac emav/*.pl
	zip avsc emav/std.lst emav/fdpswp.lst emav/4kio.lst emav/suppr.lst 
	zip avsc fft/*.c fft/*.h 
	zip avsc fft/fft/*.lnx fft/*.mac
	zip avsc include/*.h
	zip avsc lib/dj/*.txt lib/linux/*.txt
	zip avsc menu/*.c menu/*.h 
	zip avsc menu/menu/*.lnx menu/*.mac 
	zip avsc putt/*.c putt/*.h putt/*.bat putt/*.ini
	zip avsc putt/*.iss putt/*.m
	zip avsc putt/*.rc putt/*.ico 
	zip avsc putt/putt/*.lnx putt/*.mac putt/*.pl
	zip avsc tok/*.c tok/*.h 
	zip avsc tok/tok/*.lnx tok/*.mac 
	zip avsc util/*.c util/*.h 
	zip avsc util/util/*.lnx util/*.mac 
	zip avsc VS18/*.sln VS18/*.vcproj

zipsrc : avsc.zip

# build distributions

dist : $(PGMS) avsc.zip
	mkdir -p ../dist
	mv avsc.zip ../dist
	cd emav ; $(MAKE) dist

# clean

clean:
	rm -f $(LIBDIR)/*.a
	cd abrav ; $(MAKE) clean
	cd emav ; $(MAKE) clean
	cd putt ; $(MAKE) clean
	cd dsp ; $(MAKE) clean
	cd fft ; $(MAKE) clean
	cd menu ; $(MAKE) clean
	cd tok ; $(MAKE) clean
	cd util ; $(MAKE) clean


deploy:
	curl --ftp-create-dirs -v -T VS18/Output/EMAV_Setup.exe "ftp://audres_deploy%40bonkachen.com:BTNRH1982%21@bonkachen.com/downloads/EMAV_Setup.exe"

deploy-web:
	curl --ftp-create-dirs -v -T web/index.html "ftp://audres_deploy%40bonkachen.com:BTNRH1982%21@bonkachen.com/rc/emav/index.html"

tag:
	@VER=$$(awk '/#define VERSION /{print $$5}' emav/version.h | tr -d '",') ; \
	if git rev-parse "v$$VER" >/dev/null 2>&1; then \
		echo "Tag v$$VER already exists." ; \
	else \
		echo "Tagging git with v$$VER" ; \
		git tag "v$$VER" ; \
	fi

mac_deploy: emav/emav
	rm -f EMAV_Mac.zip
	zip -j EMAV_Mac.zip emav/emav
	curl --ftp-create-dirs -v -T EMAV_Mac.zip "ftp://audres_deploy%40bonkachen.com:BTNRH1982%21@bonkachen.com/downloads/EMAV_Mac.zip"
