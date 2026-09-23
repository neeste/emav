// version.h

#define VERSION     "PUTT version 2.29, 19-Oct-10"
#define PGM_NAME    "Pure Tone Threshold Test"
#define COPYRIGHT   "Copyright 1992-2010"
#define BTNRH       "Boys Town National Research Hospital"

/**********************************************************************
* version 2.29 - 19-Oct-10
- Added top_message to Cavity, Hearing, Probe, & Tone windows
- Implemented ANSI variation of UPDN level adjustment
- Changed initial length estimation to use imp. resp. autocor.
- Implemented ANSI stopping rules
* version 2.28 - 27-Aug-10
- Allow spaces in tokstr
- Default DSP_code=ASIO
- Calculate soundcard sensitivity from ARSC VFS
* version 2.27 - 5-Feb-09
- cleaned up for Mac
* version 2.26 - 19-Jun-08
- put stimulus into .CAL file instead of name
- set first zl to zchr
- add pr=p+/pl to .THL file
- add sr=src_rfl to .THL file
- add TML=SPL_TM to DAT file
* version 2.25 - 24-Apr-08
- Fixed old data format in do_hear1_test
- Fixed timing error in dsp_arsc
* version 2.24 - 19-Mar-08
- Added G10  & RET to TTH file
- Fixed old data format in do_hear1_test
* version 2.23 - 17-Mar-08
- Open files with  "rt" and "wt"
- Add 9 & 10 kHz when hfthr=y
- Add dBmV to TTH data file
* version 2.22 - 8-Mar-08
- Fixed aputt
- Added ~bbn stimulus
* version 2.21 - 2-Mar-08
- Fixed bug in writing SIL & FPL to DAT file
* version 2.20 - 24-Feb-08
- Updated to use ARSC
- Include SIL & FPL in DAT file
* version 2.19 - 16-Nov-04
- Fixed larger memory space
- Added internal sweep stimulus
* version 2.18 - 18-Mar-03
- First Windows version
* version 2.17 - 4-Jan-00
- Fixed Y2K bug in genfn()
- Split BTNRH from COPYRIGHT macro
* version 2.16 - 17-Dec-99
- Modified for WIN32
* version 2.15 - 19-Jun-98
- added sample rates
* version 2.14 - 14-May-98
- moved mainmenu functions to menu.lib
* version 2.13 - 15-Sep-97
- changed RETSPL from Killion (1978) to ANSI (1997)
* version 2.12 - 28-Jul-96
- added new protocol (F2,F1,Fd)
* version 2.11 - 20-Dec-96
- moved adjust_rate90 to rdwrdram()
* version 2.10 - 21-Nov-96
- cleaned up for lint
- put dsp functions into library
- tested PROBE CAL and HEARING
* version 2.9 - 31-Oct-96
- put program version into data files
* version 2.8 - 28-Oct-96
- fixed bug in pkmn/pkmx
- reset conductance display during chkfit
* version 2.7 - 27-Oct-96
- fixed bugs in reading files
* version 2.6 - 23-Aug-96
- fix counter initialization
- show conductance when reading .TTH file
* version 2.5 - 17-Aug-96
- increment hear_counter for each hear_cal file.
* version 2.4 - 14-Aug-96
- fixed ADC/DAC synch problem (hopefully)
* version 2.3 - 14-Aug-96
- compute SPL during tone test
* version 2.2 - 11-Aug-96
- Added random frequency order & connect plot options
- Added ML fit for final threshold estimate
- Put slope and assymptote estimates into data file
* version 2.1 - 9-Aug-96
- Added temperature to option/probe/thevenin menu
* version 2.0 - 5-Aug-96
- added probe calibration with source impedance calculation
- added frequency set selection
* version 1.1 - 1-Oct-95
- added soundcard functions from EMAV 2.11
- cleaned up for lint
* version 1.0 - 29-Oct-93
- based on EMAV version 1.5
**********************************************************************/
