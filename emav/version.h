#include <stdlib.h>
#include <math.h>
/* version.h */

#define VERSION         "EMAV  version 3.37, 18-Jun-2021"
#define PGM_NAME        "Otoacoustic Emission Averager"
#define COPYRIGHT       "Copyright 1992-2021"
#define BTNRH           "Boys Town National Research Hospital"

/**********************************************************************
* version 3.37 17-Jun-2021
- Recompiled for latest version of ARSC
* version 3.36 20-Aug-2020
- Moved from VS9 to VS16
* version 3.35 7-Feb-2019
- Modiefied for 64-bit Linux compatibility
* version 3.34 3-Dec-2018
- Changed to default probe.minres=-1000
- Link to ARSC version 0.52
* version 3.33 25-Feb-16
- Launch ER10X Stepper when present
- Check for THS & CAL mismatch
* version 3.32 6-Jan-15
- Corrected factor of 4 error in divide_by_stim
- Correct variable args in decide function
- Runs on Mac
* version 3.31 23-Nov-14
- Revert to all emav code to version 3.24
- Remove unused variables from emav.c: ad_type, dsp_select, greg
- Rename access to _access in dpoae_w.c
- Link to ARSC version 0.47
- Fixed cavity_length prototype in emav.h
- Deleted len_est prototype in emav.h
- Change "dsphpf" to "input_filter" in {dpoae,probe,teoae,tone}_w.c
- Add "input_filter" function to savage.c
- Eliminate syntax warnings in {calfile,dofft,{dpoae,probe,teoae}_w,showdp}.c
- Put device list on F2
- Update COPYRIGHT range
- Add Ext_iter=R to emav.ini
- Restore iteration of ideal-cavity reflection
- Update z_surge call in thev_cmp_prz0
- Add R= to dis_cal
- Added functions ldimp & surge_gain
- Fixed crash due to get_rate after device open failure.
* version 3.30 22-Dec-11
- Revert to v3.24 buffer allocation in emav.c & rdwrdram.c
* version 3.29 6-Nov-11
- Fixed scale in CAL & PRB (bug in version 3.28)
* version 3.28 9-Aug-11
- Removed dependence on "sets" in fft_A_B to fix level "jumping"
* version 3.27 9-Jul-11
- Multiply swp1set by 2 in show_tone for agreement with RMS level
* version 3.26 17-Jun-11
- Added option to iterate ideal-cavity reflection
- Moved anafilt functions from dsp to util
- Calculate f_notch from load_len
- Added fmul option to LST file
- Fixed variable arg in menu/decide function
- Added change_nic to grab_probe
* version 3.25 7-Apr-11
- Partial restructure of DSP code to increase independence
- Fix dual-channel recording in DPOAE test
* version 3.24 26-Feb-11
- Enable artifact reject for cavity test
- Add Stepper launch option
- Postpone message erase when accepting calibration
* version 3.23 07-Feb-11
- Iterate surge impedance
- Add [Probe] parameter surge to specify interation count
* version 3.22 23-Nov-10
- Adjust output scaling for agreement with SLM
* version 3.21 18-Nov-10
- Adjust min. high-pass freq. for stable 3rd-order Bessel
- Add ramp_down to Tone & DPOAE tests to eliminate clicks
- Adjust vertical spacing of info text during cavity measurements
- Make z_chr constant & real in probe_w.c
- Correct P/V in THS & THL files
- Replace dsphpff & dsphpft with dsphpf in dsp_arsc.c
- Revert take_the_aver() to demean not detrend
- Check min. wn for high-pass filter
* version 3.20 2-Nov-10
- Add "sets" to end of CAL/PRB file
- Modify minimum index in cavity_length function
- Simplify chk_ramp functions
* version 3.19 29-Oct-10
- Add "sets" to end of CAL/PRB file
- Add "sets" to end of THS/THL file
- Changes variable name "nsweeps" to "cal_sets"
* version 3.18 17-Oct-10
- Adjust for cal_atten difference in thev_load
- Separated cavity_length function from  len_est
* version 3.17 12-Oct-10
- Avoid recalculating ~bbn in getstim
- Capitilize Test menu items Probe & Tone
- Add "Cavity" to Test menu
- Fix call to thev_src_sav in probe_w
- Display cavity length in checkfit
* version 3.16 4-Aug-10
- Added option to set DSP_code to device number > 0
* version 3.15 13-Jul-10
- Fix calculation of soundcard sensitivity from ARSC vfs
* version 3.14 29-May-10
- Fix ASIO default device selection in ARSC
* version 3.13 15-May-10
- Fix crash in mat_read when BIN file is missing
- Fix artifact indicator when Sen.AD=0
- Fix SFOAE extraction to not skip first response
* version 3.12 15-Apr-10
- Default to ARSC vfs for adsen & dasen
- Added continuous-averaging (ContAv) feature to DPOAE test
- Added [System] parameters ad_chnoff & da_chnoff
* version 3.11 22-Dec-09
- Fixed typo in load calibration
* version 3.10 4-Aug-09
- Fixed DAT file column headings
* version 3.09 16-Jul-09
- Disambiguate Calibrate in rd_dpoae_file()
- Create FILE_SAVE macro to avoid disabling menu option
* version 3.08 2-Jun-09
- Change cal_swps in emav.ini
- Set swp1set after thev_adj in proc_acu().
* version 3.07 17-May-09
- Change sf_in0 in dsp_arsc from 1/512 to 1/256 to correct 6-dB error in AD_sensitivity
* version 3.06 28-Apr-09
- Set ADC/HPF during probe test
- Implement ADC/HPF on both channels
- Enhance IIR filter code in dsp_arsc.c & dsp_wa.c
- Change high-pass input filter to third-order Bessel
* version 3.05 2-Apr-09
- Cleaned up for Mac
- Fixed bug in Fdp abscissa
- Fix F1 dsp_init loop
- Add reflectance (RFL) to DAT file
- Fixed bug in reading "CalibratePhase" from .ini file
* version 3.04 12-Oct-08
- Added "Count" parameter to both TEOAE & PROBE sections of config file.
- Fixed bug in File/New due to invalid ini_file name.
* version 3.03 4-Jul-08
- Allow any fd for AM-SFOAE
- Improve read_init_file function
* version 3.02 12-Jun-08
- Fixed bug in File/Save feature
- Re-wrote "util/trim" function
- Increased ncfn from 16 to 24 in util/sfile.c
- Added Threshold field to Patient Info
* version 3.01 24-May-08
- Fixed bug do_dpoae_task that failed to reset nadj when stim_unit=Volts
* version 3.00 25-Apr-08
- Cleaned up menus and config file details for updated user's manual
- Removed (void) casts from function calls
- Corrected typos (from Judy)
- Put "About" dialog box back on F1 funciton key.
- Took out redundant dsp_start in dsp_check to correct timing error
- Tweaked ARSC and dsp_arsc for Linux
* version 2.99 18-Feb-08
- Fixed swpacc count in dsp_arsc that caused a 2-dB error
- Fixed a 6-dB calibration scaling error
- Removed ASIO default in dsp_arsc for Linux
* version 2.98 8-Feb-08
- Added support for RECD/FPL experiment
- Corrected 19-dB stimulus level error by replacing dofft.c
- Set skips to dpoae.skips for DPOAE test
- Added internal broad-band-noise stimulus ~bbn
- Fixed stimulus length in File/New function
* version 2.97 4-Feb-08
- Fixed buzzing & down-ramp in ARSC dsp_arsc
- Fixed artifact adjustment hang when arsc_asio stalls
- Added time-out feature to "decide" function
- First implementation of Nsrc in Probe test
* version 2.96 23-Jan-08
- Strip trailing delims from token lines for Linux
- Fix TOK file open for Linux
- Improved ARSC/ASIO handling of io_close and xrun
* version 2.95 18-Jan-08
- Fixed tone.level error caused by cal_gain scaling fix
- Fixed cal_file for PRB save with cavity restart
- Changed PrintScreen to ^P
- Fixed bug in take_the_aver
- Added DC remove option to Tone test
- Fixed symbol key for AM-SFOAE in rd_dpoae
- Fixed persistence of modcyc & modper in dpoae_w
* version 2.94 15-Dec-07
- Fixed cal_gain scaling for frequency-dependent MP_sensitivity
* version 2.93 15-Dec-07
- Fixed frequency-compilation bug in modulesq
* version 2.92 15-Dec-07
- Don't assume I/O rate matches MP_transfer rate in MAT file
- Delay MP_transfer computation until after rate is set
* version 2.91 14-Dec-07
- Call mp_transfer when reading config file
- Fix tokstr parsing so both print_label and Ear work
- Fixed freq. scale bug in call to mp_sens
* version 2.90 15-Nov-07
- Added da1sn & da2sn to CAL (& PRB) files
- Set default da1sn & da2sn to adsn
* version 2.89 9-Jul-07
- Link with ARSC
* version 2.88 10-Jun-07
- Allow MP tranfer phase to be specified
- Read MP transfer from SYSRES data file
* version 2.87 6-Jun-07
- Avoid changing level_unit when reading DPOAE file
- Allow setting MP_Sens filename in Option menu
* version 2.86 13-May-07
- Open PRB files on command line
- Set Level_Unit when reading DAT files
- Show thev_adj when viewing DAT files
- Enhanced View mode for PRB files
* version 2.85 9-May-07
- Added "decreasing length" feature to Probe calibration
- Update tmpcav & diacav display while iterating
- Added SIL calibration
* version 2.84 4-May-07
- Tweaked Probe test & Thevenin calculations
* version 2.83 1-May-07
- Implemented Thevinen source and load calculations
- Implemented FPL calibration in DPOAE test
- Environement variable EMAV.INI specifies alternate configuration file.
* version 2.82 24-Apr-07
- fixed bugs in TEOAE test
- added Probe test for Thevenin calibration
* version 2.81 16-Mar-07
- use 32-bit buffers for tonal stimuli
- allow MP_sensitivity in file
* version 2.80 13-Mar-07
- allow primary stimulus amplitude to be zero
* version 2.79 7-Feb-07
- Fixed bug in reading Ear from DAT file
* version 2.78 28-Jul-06
- Fixed bug in checking short values in menus
* version 2.77 26-Jul-06
- Fixed bug in initializing fdp in do_dpoae_task
* version 2.76 13-Jul-06
- Fixed a problem with long DAT file names
* version 2.75 8-Jun-06
- Set MAT mode for CAL files (again?)
* version 2.74 18-Dec-05
- Added F1 modulation for SFOAE test
* version 2.73 26-Oct-05
- Fix Target value for TEOAE test
- Fix longptr error in TEOAE test
- Consolidate dac buffer download
- Made HPFF per test
* version 2.72 14-Jul-05
- Fix DPOAE stimulus artifact
* version 2.71 22-May-05
- Added stimulus waveform to CAL file
- Added scale to CAL file
* version 2.70 25-Apr-05
- Added Count to DPOAE section of config file
* version 2.69 20-Apr-05
- Added digital high-pass filter in oae_get_sweep
- Added noise-floor-separation rejection
- Added HPF1 to [System] section of emav.ini
* version 2.68 5-Jan-05
- Removed set_input_atten() function
- changed format of CAL files to be MAT compatible
* version 2.67 27-Oct-04
- Expanded tic intervals
- Added chkfit_time to allow timeout
- Fixed -r command line option
- Put "Protocol File" in Option/DPOAE/Stimulus menu
* version 2.66 25-Jun-04
- Added higher sampling rates to 200000
* version 2.65 22-Jun-04
- Added config parameters HPF_type and HPF_freq
* version 2.64 6-Jun-04
- Fixed SFOAE extraction for JHS compatibility
* version 2.63 17-May-04
- Implemented phase calibration
- Added CalibratePhase parameter to DPOAE section of config file
* version 2.62 13-May-04
- Use BIN file for SFOAE extraction
* version 2.61 1-May-04
- Improved SFOAE mode of DAT file display
- Added "Extract SFOAE" function
- Increased window size on hi-res screens
- Added Color PostScript printing
* version 2.60 14-Mar-04
- Added second channel A/D to accbuf and bin file
- Added NIC parameter to DPOAE section of config file
- Added NIC parameter list file header
- Revised to allow DJC compilation
- Allow escape from LST file selection
* version 2.59 8-Jan-04
- Changed Trial axis
* version 2.58 16-Dec-03
- Added outdbv to DAT file extended format
* version 2.58 17-Nov-03
- Changed default config file to "c:\program files\emav\emav.ini".
* version 2.57 3-Nov-03
- Added Indigo support, 24-bits, poor I/O synch
- Added artifact reduction threshold
- Fixrd bug in art. red. for negative NNSB
- Fixed 16kW buffer size
- Added red_thr parameter
* version 2.56 12-Oct-03
- Added record/playback feature
* version 2.55 12-Mar-03
- Added noise reduction algorithm
* version 2.54 18-Sep-02
- Add desired primary levels (L1,L2,L3,L4) to extended data format.
* version 2.53 1-Mar-02
- Allow selection of windows soundcard through dsp_code parameter.
* version 2.52 20-May-01
- Added F1 and F1 parameters to TEOAE section of configuration file
* version 2.51 23-May-01
- Fixed artifact limit control
* version 2.50 12-Apr-01
- change "dp" to "sf" in symbol key when f1=f2
- put abscissa label on RHS
- Added ramps for tones for both wave_audio and Pinnacle/Fiji
- Fixed 3-byte shift bug in wa.c
* version 2.49 3-Apr-01
- Works with windows "wave audio"
- Fixed chkfit bug by zeroing channel B
- Changed color of print label text
- Change default of dsppar.greg to 1.
- Restore screen when exiting DPOAE VIEW mode
- Cleaned up abscissa labels
* version 2.48 26-Sep-00
- Added parameter to select "periodic" file processing
* version 2.47 28-Jul-00
- Added option to select calibration file
* version 2.46 28-Jun-00
- Added L3 abscissa mode
* version 2.45 17-May-00
- Fixed F9 key for pause in WEMAV
* version 2.44 26-Apr-00
- Made XBPN a fourth suppresor choice
- Added data file format "Multi" for multi-F2 experiment
* version 2.43 14-Jan-00
- Fixed View mode for num_oct > 0
- Added XBPN for band-pass noise excluding DPs
* version 2.42 7-Jan-00
- Cleaned up annotations in show_dp()
- Erase w_spec window when BIN file is missing
* version 2.41 5-Jan-00
- Added control of DPOAE abscissa level range
* version 2.40 4-Jan-00
- Fixed Y2K bug in genfn()
- Split BTNRH from COPYRIGHT macro
* version 2.39 21-Dec-99
- Added mouse control to view_teoae_file()
* version 2.38 17-Dec-99
- Modified for Win32
- Added display of binary DPOAE files
- Added BPN option to Suppr stimulus
* version 2.37 23-Nov-99
- Fixed buffer overflow bug in do_dpoae_task().
* version 2.36 25-Oct-99
- Changed list file creation to use constant F1 step for F1 sweep
* version 2.35 4-Oct-99
- Added input attenuation option to tone test
* version 2.34 13-May-99
- Tone test (only) works under Win32
* version 2.33 12-May-99
- Fixed problem (in menu.lib) using ^C to exit.
* version 2.32 5-Mar-99
- Fixed set_input_atten()
- Re-enabled dsp2satt
* version 2.31a 1-Feb-99
- Temporarly disabled dsp2satt() function
* version 2.31 28-Jan-99
- Fixed computation of 5*f1-4*f2 in do_dpoae_task()
* version 2.30 30-Oct-97
- Added fourth tone to DP test
- Added adjustment of F1 level and phase
* version 2.29 19-Jun-97
- added more sample rates
* version 2.28 8-Jun-97
- added cabability to do 2 or 3 primary pairs at octaves
* version 2.27 13-May-97
- made menu.lib backward compatible
* version 2.26 6-May-97
- removed global mouse vaiables: mouse_on, mxpos, mypos
* version 2.25 8-Aug-97
- Fixed F3 amplitude bug
- Fixed Ndp computation for data file when NNSB > 0
* version 2.24 30-Jul-97
- changed adjustf() to double precision
* version 2.23 28-Jul-97
- added SL units
* version 2.22 25-Jul-97
- added call to dis_units in tone_wind
- fixed sensitivity items in options/system menu
* version 2.21 23-Jun-97
- supports Pinnacle
* version 2.20 14-Mar-97
- increased MAXLINE to 256 (for Siegel)
* version 2.19 4-Mar-97
- added N1, N2, F3, L3, N3, & <3 to ext. data fmt (for Siegel)
* version 2.18 24-Feb-97
- added phase of f1 & f2 components to ext. data fmt (for Siegel)
* version 2.17 15-Feb-97
- added decimal point to levels in ext. data fmt (for Siegel)
* version 2.16 21-Jan-97
- fixed f3 freq adj problem (for Siegel)
- use 18 bits of DAC
* version 2.15 20-Dec-96
- added optional suppressor tone to DP test (for Siegel)
- moved adjust_rate() to rdwrdram.c
* version 2.14 21-Nov-96
- compute itime1/itime2/iramp from mstime1/mstime2/msramp
- cleaned up for lint
- put adsen and mpsen into data files
- created mtb5a & sweep2ka
- tested TEOAE & DPOAE
* version 2.13 13-Nov-96
- added F2-F1 compenent for Siegel
* version 2.12 18-Apr-96
- don't remove items from stimulus list if noise or snr differ
* version 2.11 1-Oct-95
 - eliminated "ifdef MSND" by adding dspdev() and dspclk()
 - rewrote dsp_init() to fetch "hexfile" from dspcode()
 - changed noise calculation to A+B for NNSB > 0
* version 2.10 29-Sep-95
 - fixed bug that prevented cal display when reading data file
 - fixed bug in do_dpoae_task for fdp4 < 0
 - fixed scale factor when reading cal files
 - reduced number of calloc calls in alloc_space
* version 2.09 7-Aug-95
 - fixed bug in teoae_aver() reported by John Skeens
* version 2.08 25-Jul-95
 - added support for TB Multisound Monterey
* version 2.07 15-Aug-94
 - moved location of close(dup()) to avoid (?) hang during DP test
* version 2.06 12-Jul-94
 - fixed format for NNSB= display
 - minor change to heading alignment in DP data file
 - minor changes to linked list in attempt to locate crash bug
* version 2.05 1-Jul-94
 - fixed command line option -m
 - added .ini & menu options for auto DP spectrum frequency range
* version 2.04 16-Jun-94
 - added display_label() to pcl_pr()
 - fixed escape from check fit (bug due to "continue" fix)
 - fixed num_conds bug in create list
 - tried (again) to patch memory leak in del_data_lnk()
 - fixed comment line format error in higher-order & extended data files
 - added parsing of "abscissa" when reading data files
 - added NNSB to create list menu and .DAT file header
 - split create list menu int two parts
 - changed dpfreq.at to DP_freq in .DAT file header
* version 2.03 7-Jun-94
 - fixed "continue" bug during check fit
 - tried to fix memory leaks
* version 2.02 23-May-94
 - restored DSP calls to Ariel library for 386 (custom dspfunc wasn't working)
* version 2.01 5-May-94
 - fixed units=volts option in list file
* version 2.00 9-Apr-94
 - restored earlier (3-20-93) verion of oae.asm
* version 1.08 28-Feb-94
 - added extended data format with 2*F2-F1
 - added option of specifying stimulus in volts
* version 1.07 22-Jan-94
 - fixed math errors when input is constant
 - fixed display of stimulus name when full path name is read
* version 1.06 27-Oct-93
 - display label at top when printing
 - modify calls to menu library
* version 1.05 25-SEP-93
 - added the frame and ticks to the teoae display
 - added the xrange, xbegin into teoae display
* version 1.04 9-Jul-93
 - fixed the range error when 4*f1-3*f2 < 0, No fft spectrum power
   is available.
 - fixed the bug in atan2 when both arguments are zero -> error.
 - fixed the bug of DPx computation when the signal power is less than
   the noise power, log10 has range error (argument negative)
 - added new data format to save higher order distortion for
   3*f1-2*f2, 4*f1-3*f2, 4*f1-3*f2
* version 1.03 6-JUL-93
 - added features to skip calibration after the first run.
 - added features to pause and checkfit and re-calibrate during dpoae test.
* version 1.02 29-APR-93
 - added the create list file function in the File menu
 - subtracted the DC offset when displaying tone waveform
 - used fixed address for channel A, B and accumulator to solve the
   host depend adressing problem (1 word off)
* version 1.01 26-MAR-93
 - added Fdp sweep into DPOAE test
 - added skips in [DPOAE] section of the initialization file to specify
   the skips.  This parameter is stored in dpoae.skips
 - added F2 - F1 distortion component as an option
 - modified the buffer length to allow 4k points stimulus for single
   channel
 - modified the buffer length to allow 2K stimulus for two channels
   and 4K stimulus for one channel
 - fixed the bug when there is only one file to search for.
* version 1.00  15-MAR-93
 - revise the oae.asm assembly code to ensure better syncronization
   thus solved the problem of the overall A+B floor level above A-B noise
   level for 6dB or more.  The floor levels for A+B and A-B are same now.
 - change the default sweeps per set to 100 which is about 2 seconds
   to reduce the system distortion actifact (SDA).   Thus reduced the
   SDA to below -20dBSPL for frequncy range from 1000 to 8000
* version 0.9
 - GREG=16 works now.  15K RAM can be use to stored data.  The rest are
   program and parameters.
 - make NNSB (number of noise-side-bands setable from list files.
 - cal_swps was added to the emav.ini file to specify
   the sweeps per set during the calibration phase.
 - added option GREG=8, 16 for choosing different size
   of global static RAM on DSP board, apparently 8K
   option works fine, but 16K does not work.
   contacted ARIEL for help.  No reply yet.
 - added DPx display option into DPOAE, setable from
   initialization file emav.ini
   signal = DPx or DP+	(default DP+)
 - noise-nearby-side-band accounting added
   setable from emav.ini file:  NNSB=0, 1, 2
* version 0.8 - 18-FEB-93
* version 0.7 - 06-FEB-93
**********************************************************************/
