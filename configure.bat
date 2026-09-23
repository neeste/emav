@echo off
@echo Configure AV for MGW.
mkdir lib\mgw
cp -f makefile.mgw Makefile
cp -f abrav/makefile.mgw  abrav/Makefile
cp -f dsp/makefile.mgw    dsp/Makefile
cp -f emav/makefile.mgw   emav/Makefile
cp -f fft/makefile.mgw    fft/Makefile
cp -f putt/makefile.mgw   putt/Makefile
cp -f menu/makefile.mgw   menu/Makefile
cp -f tok/makefile.mgw    tok/Makefile
cp -f util/makefile.mgw   util/Makefile
@echo Type 'make'.
