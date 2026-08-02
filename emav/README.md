# EMAV (Otoacoustic Emission Averager)

EmAv measures and displays otoacoustic emissions. Either transient-evoked otoacoustic emissions (TEOAE) or distortion product otoacoustic emissions (DPOAE) can be measured.

**Copyright:** 1988-2026 Boys Town National Research Hospital  
**Web page:** [http://audres.org/rc/emav/](http://audres.org/rc/emav/)  

---

## Windows Installation

**Precompiled Installation:**
1. Download the setup file from [http://audres.org/downloads/emav-setup.zip](http://audres.org/downloads/emav-setup.zip)
2. Run the included `Setup` executable.
3. EMAV requires Microsoft C-runtime libraries. If Windows complains that *"This application has failed to start because the application configuration is incorrect,"* download and install the C-runtime installation program from [http://audres.org/downloads/vcredist_x86.exe](http://audres.org/downloads/vcredist_x86.exe).

**Configuration:**
The `emav.ini` configuration file sets up default parameter values and audio device mapping (via `DSP_CODE`).
- You can place `emav.ini` in the same directory as the executable, or set the environment variable `EMAV.INI` to specify an alternate location.

---

## macOS Compilation and Installation

EMAV runs as a command-line program under macOS. It uses the X window system (XQuartz) to provide the graphical user interface, and the `ARSC` library with Apple's CoreAudio framework to communicate with the soundcard.

**Prerequisites:**
1. Install **Xcode Command Line Tools**: `xcode-select --install`
2. Install **XQuartz** for X11 support (headers are required for building).

**Building EMAV:**
1. First, navigate to the `arsc` library directory and build it for macOS:
   ```bash
   cd ../arsc
   make -f makefile.mac
   sudo make -f makefile.mac install
   ```
2. Navigate to the `av/emav` directory and compile EMAV:
   ```bash
   cd ../av/emav
   make -f makefile.mac
   ```
3. Install the executable to your system path:
   ```bash
   sudo cp emav /usr/local/bin/
   ```

**Configuring Audio for macOS (Important!):**
Because EMAV simultaneously generates a stimulus and records a response, it expects the configured audio device (specified in `emav.ini` via `DSP_CODE`) to have **both input and output channels**. Modern Macs treat the built-in speakers and built-in microphone as completely separate devices.

1. Open the macOS **Audio MIDI Setup** app (found in `/Applications/Utilities`).
2. Click the `+` button in the bottom left corner and select **Create Aggregate Device**.
3. Check the "Use" box for your desired Output (e.g., MacBook Pro Speakers).
4. Check the "Use" box for your desired Input (e.g., MacBook Pro Microphone).
5. Name the aggregate device something memorable (e.g., `Input/Output`).
6. Update your `emav.ini` file (which can be placed in `/usr/local/etc` or your working directory) to match this new device:
   ```ini
   DSP_CODE=Input/Output
   ```

*(Note: Depending on your CoreAudio driver versions, device names might be internally truncated. If your device name is very long and EMAV fails to match it, try using the first 10-15 characters in `DSP_CODE`).*
