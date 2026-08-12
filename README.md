# EMAV (Emission Averager)

**EMAV** is a program for measuring and displaying otoacoustic emissions, developed at the **Boys Town National Research Hospital**. 

It features advanced capabilities for distortion-product otoacoustic emission (DPOAE) tests and transient-evoked otoacoustic emission (TEOAE) tests. EMAV uses high-quality soundcards for stimulus generation and synchronous averaging for noise reduction.

## Documentation
For comprehensive instructions on how to configure and use EMAV, please see the full **[EMAV User Guide](http://audres.org/downloads/emavtm.pdf)**.

## Building from Source

### Windows (x86 & ARM64)
EMAV can be built natively for both Windows x86 and ARM64 architectures using Microsoft Visual Studio.
To build both architectures and automatically generate the installer (if Inno Setup is installed), simply run:
```bat
build_all.bat
```
This unified script will compile the `av.sln` solution and then invoke the Inno Setup Compiler (`ISCC`) to produce `EMAV_Setup.exe` in the `VS18/Output` directory.

### macOS
To compile the macOS executable and create the deployment archive, run:
```bash
make mac_deploy
```
This will build the binaries using GCC and compress them into `EMAV_Mac.zip`, before uploading it to the deployment server.

## License
Creative Commons Attribution 4.0 International (CC-BY)
