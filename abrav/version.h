/* version.h */

#define VERSION_NUM	132
#define VERSION		"ABRAV  version 1.32, 14-Mar-10"
#define PGM_NAME        "Auditory Brainstem Response Averager"
#define COPYRIGHT	"Copyright 1993-2010"
#define BTNRH           "Boys Town National Research Hospital"

/**********************************************************************
* version 1.32 - 10-Mar-09
- Fixed Tone test
- Changed _inline to __inline in dsp_arsc.c
* version 1.31 - 4-Feb-09
- Cleaned up for Mac
* version 1.30 - 19-Mar-03
- Adapted for Windows and OptiAmp
- Changed "daily calibration" into Test/Probe
* version 1.29 - 4-Jan-00
- Fixed Y2K bug in genfn()
- Split BTNRH from COPYRIGHT macro
* version 1.28 - 17-Dec-99
- Modified for WIN32
* version 1.27 - 10-Oct-98
- Track Nbb instead of Nav when available
* version 1.26 - 25-Sep-98
- added menu option to change electrode polarity
* version 1.25 - 19-Jun-98
- added more sample rates
* version 1.24 - 6-Jun-98
- added "EP_start" as a config parameter
- new sweep file type (SW6) includes AD sens. (possibly negative)
- display routines allow for negative AD sens.
- reject mode set to "None" for old sweep files
* version 1.23 - 2-Jun-98
- added "polarity" as a config parameter 
* version 1.22 - 21-May-98
- fixed check_event in menu.lib to remove slight pause
* version 1.21 - 13-May-98
- fixed bug caused by SPSQ define in savage.h
* version 1.20 - 6-May-98
- removed global mouse variables: mouse_on, mxpos, mypos
* version 1.19 - 1-May-98
- remove unused ABR keywords
- added trkbuf variable to aux info
- if make_template, then trkbuf=3  & NO intermed. waveforms stored
* version 1.18 - 28-Apr-98
- additional changes for RTC proj I
* version 1.17 - 25-Apr-98
- major changes for RTC proj I
* version 1.16 - 20-Dec-96
- minor changes for compatibility with EMAV & PUTT
- moved adjust_rate() to rdwrdram.c
* version 1.15 - 2-Oct-95
- added soundcard support from EMAV 2.11
- cleaned up for lint
* version 1.14 - 4-Nov-94
- set vmp_min to zero
- implemented wide-mean variance, but set wmvflg to zero
* version 1.13 - 28-Apr-94
- deglitch first point in wavform in zpf()
- put lower limit (vmp_min) on between-block variance
- template & parameters compatible with scor (rel=1.19,N=50)
* version 1.12 - 4-Apr-94
- implemented zero-phase filter and Scor statistic
- removed level dependent paramters support
- changed level reference from SPL to nHL
* version 1.11 - 28-Feb-94
- switch colors of +/- spectra to match waveforms
- moved FFTdB and FFTkHz to edit mode shift-F4
* version 1.10 - 22-Jan-94
- fixed month in test date (off by 1)
- changed name of temp. file to confusion with .ABR files
- changed date format to DD-MMM-YY
* version 1.9 - 20-Jan-94
- added SW4 sweep file format with individual single points stored
- fixed bug which caused uploading before dsp completed sweeps
- fixed bug in shift-Z function (wrong header size)
* version 1.8 - 02-Dec-93
- fixed bug in open_file() which set test_level incorrectly
- fixed xrange in rd_abr_file()
- clear screen when reading .cal file
- changed file_save() for .cal file
- changed max_level default from 110 to 105
* version 1.7 - 29-Nov-93
- changed edit_abr() to allow editing after sweep playback
- added -R option to record using optional name on command line
- changed -r option to record using data file name
- removed automatic writing of .log files
- replaced the 'A' in the generated filename with the machine ID.
* version 1.6 - 19-Nov-93
- moved sweep writing before filtering
- fixed bug in formula for multi-point variance
- changed assignment of colors to EEG display
- added global variables time1_ms and time2_ms
- moved tok_init() from do_abr_task() to abr_aver()
- overwrite last value in monitor track if trklen is to short
- fixed twt_A/B when displaying intermediate waveforms
* version 1.5 - 17-Nov-93
- fixed another bug in formula for weighted single-point variance
- added command line option -P for playback mode
- copy filename from command line after -r or -P
- replaced DOB with Level in File/Open
* version 1.4 - 15-Nov-93
- fixed a new bug in formula for weighted single-point variance
* version 1.3 - 13-Nov-93
- fixed bug in formula for weighted variance
- changed definition of Nsp from var(SP) to var(SP)/Nswp
- replaced Aroc with SNR and Fmp as alternate quality measures
- fixed bug when opening .SWP files too quickly
* version 1.2 - 04-Nov-93
- added the blackman window to smooth the curves when editing
* version 1.1 - 27-Oct-93
- added record mode with command line option -r
* version 1.0 - 11-Oct-93
- added calibration mode with command line option -c
- added open calibration file (*.cal)
* version 0.9 - 19-Sep-93
- sum and difference display modes
- F9 works again under shell control
* version 0.8 - 15-Sep-93
- four different display points 128, 256, 512, ALL
- F4 toggle message and FFT spectrum
- F5 for different message display
- F6 toggles between  intermediate wave forms and the final ones
- Tone_Level and Tone_Freq in initialize file
- intermediate wave stored on oper OK or Fsp2
- Repro. criterion removed from stopping rules
* version 0.7 - 4-Sep-93
- Added Nsp and Nav noise tracks
- Fixed minor scaling bug in show_A_B
- Disabled options and Esc under shell controll
- Prompt for oper. O.K. at end of sweeps if under shell control
* version 0.6 - 2-Sep-93
- Added the artifact rejection adjust using '<' and '>' keys
- open sweep files and set the correct run mode and the file name
* version 0.5 - 1-Sep-93
- Fixed initialization of marks
- Changed F10 to toggle PAUSE collect
* version 0.4 - 1-Sep-93
- Fsp implemented ready for beta test
* version 0.3 - 27-Aug-93
- waveforms look good with lpi8
- user interface O.K.
- termination codes implemented
- weighted averaging implemeted
- record and playback implemented
* version 0.2 - 27-Aug-93
* version 0.1 - 24-Aug-93
- based on EMAV version 1.4
**********************************************************************/
