/* putt.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <dsp.h>
#include <menu.h>
#include <util.h>
#include "savage.h"
#include "color.h"
#include "screen.h"
#include "version.h"
#include "putt.h"

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#define _access access
#define _stricmp strcasecmp
#endif /* WIN32 */

int     cav_test();
int     screen_print();
int     thrsd_opt(char *);
int     set_cav_wind();
int     set_hear_wind();
int     set_probe_wind();
int     set_tone_wind();
int     today_file(char *);
int     File_wind();
int     Info_wind(void);
int     Option_wind(void);
int     Quit_wind();
int     Test_wind();
void    init_graphics(void);
void    wait_for_key(double);
void    Tone_wind(void);
void    Hear_wind(void);

extern float refresh_time;
extern TOGGLETYPE thresh_algo, thresh_task, sim_obs;
extern TOGGLETYPE thresh_rand, thresh_cnct, levref, cond_thl;
extern TOGGLETYPE thv_itr, thv_ext, thv_xtk, thv_dbg;
extern TOGGLETYPE freq_set_1;

static MAIN_MENU mainmenu[] = {
    {"&File", File_wind},
    {"&Info", Info_wind},
    {"&Options", Option_wind},
    {"&Test", Test_wind},
    {NULL, NULL}
};

MENUITEM testmenu[] = {
    {"&Probe", NULL, NONE, 0, 0, 1, set_probe_wind},
    {"&Cavity", NULL, NONE, 0, 0, 1, set_cav_wind},
    {"&Hearing", NULL, NONE, 0, 0, 1, set_hear_wind},
    {"&Tone", NULL, NONE, 0, 0, 1, set_tone_wind},
    {NULL, NULL, NONE, 0, 0, 1, NULL},
};

char  **argv;
double  lentst = 6.04;      // length of test cavity (cm)
double  rho_c = 40;	    // for ref = 10^-16 W/cm^2
int     argc;
int     hear_cal_cnt = 0;
int     hear_tth_cnt = 0;
int     prob_cal_cnt = 0;
int     prnflg = 0;
int     rclflg = 0;
int     sig_type = 0;
int     base_addr = 0;
int     dspprm = 0;
int     maxnpts = 0;
int     rand_seed = 0;
float   spl_ref = 20e-6F;     /* SPL reference (rms Pa) */

WIND   *w_dis;

struct _probe probe;
struct _hear hear;
struct _tone tone;

ATTENUATION attenu = {80.0, 80.0, 10000.0, 1};

sensitivity Sen = {
    0.5, 0, {0, 0}, {43.0F/2, 55.5F/2}, 
};
sensitivity Sys = {
    0.5, 0, {0, 0}, {43.0F/2, 55.5F/2}, 
};

char    ilstim[MAXNAME] = "b4.ils";
char    exedir[MAXNAME] = "";
char    o_file_name[MAXNAME] = "";
char    file_name[MAXNAME] = "putt.ini";
char    puttcounter[MAXNAME] = "putt.cnt";
char    tmpbatchfile[MAXNAME] = "";
char    batchfile[MAXNAME] = "";
char    dsp_code_file[MAXNAME] = "ASIO";
char    thsfn[MAXNAME] = "";
char    ite1fn[MAXNAME] = "";
char    ite2fn[MAXNAME] = "";
char    tmp_cal_fn[MAXNAME] = "out.cal";
char    cali_file[MAXNAME] = "";

char *month_name[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

int     scope = 2048;
int     mono_screen = 0;
short   *outbuf, *dspbuf, *stimulus;
float  *accbuf, *accbuf_a, *accbuf_b, *fltbuf, *sav_stim;

char    line[MAXLINE];
char    tokstr[MAXTOK];

static int cnt = 0, linelen = 0;

int
Test_wind()
{
    simple_submenu(menu_x(), menu_y(), testmenu);
    return (0);
}

/* print the verison number and copyright info */
void
print_version()
{
    fprintf(stderr, "%s\n%s\n%s %s\n", VERSION, PGM_NAME, COPYRIGHT, BTNRH);
}

void
init_alloc_space()
{
    int bufsiz = 0;
    int accnpts, wi, wo;

    dsppar.nic = 1;		// single-channel input
    dsppar.code = 1;		// oae code, 16-bit stimulus
    dsppar.greg = dspgreg(2);	// 1=16kw, 2=65kw */

    switch (dsppar.greg) {
    case 0 :			    /*  8 KW SMA */
	maxnpts = 1024;		    /* = 0x00400 */
        base_addr = 448 * 128;	    /* = 0x0E000 */
        dspprm = 510 * 128;	    /* = 0x0FF00 */
        bufsiz = 1024;		    /* This value is obselete. */
	break;
    case 1 :			    /* 16 KW SMA */
	maxnpts = 3 * 1024;	    /* = 0x00C00 */
        base_addr = 392 * 128;	    /* = 0x0C400 */
        dspprm = 510 * 128;	    /* = 0x0FF00 */
        bufsiz = 4 * 1024;	    /* Why is bufsiz larger than maxnpts ??? */
	break;
    case 2 :			    /* 65 KW SMA */
	maxnpts = 8 * 1024;	    /* = 0x02000 */
        base_addr = 0;		    /* = 0x00000 */
        dspprm = 512 * 128;	    /* = 0x10000 */
        bufsiz = 8 * 1024;	    /* This value needs to be tested. */
	break;
    }

    wi = (dsppar.code == 2) ? 2 : 1;
    wo = (dsppar.nic == 2) ? 2 : 1;
    maxnpts = (dspprm - base_addr) / (2 * (wi + wo));
    accnpts = maxnpts * wi;
    accbuf = (float *) calloc(2 * accnpts, sizeof(float));
    outbuf = (short *) calloc(2 * accnpts, sizeof(float));
    stimulus = (short *) calloc(2 * accnpts, sizeof(int));
    if (accbuf == NULL || outbuf == NULL || stimulus == NULL) {
	print_version();
	fprintf(stderr, "Not enough memory!  Program aborted!\n");
	exit(0);
    }
    accbuf_a = accbuf;
    accbuf_b = accbuf + bufsiz;
    fltbuf = (float *) outbuf;
    dspbuf = outbuf + bufsiz;
    sav_stim = (float *) (stimulus + bufsiz);
}

/* set default parameter values */
void
defpar()
{

    // soundcard sensitivity
    Sys.AD = 0;
    Sys.DA[0] = 0;
    Sys.DA[1] = 0;
    Sys.LS[0] = 5;
    Sys.LS[1] = 5;

    // PROBE
    probe.rate = 32000;
    probe.size = 2048;
    probe.chk_swps = 10;
    probe.chk_atten = 10;
    probe.chk_skps = 0;
    probe.cal_swps = 50;
    probe.cal_atten = 10;
    probe.cal_skps = 2;
    probe.cal_sets = 10;
    probe.levref = 1;
    probe.temp = 25;
    probe.diacav = 0.8F;
    probe.maxlen = 12;
    probe.minres = 10;
    probe.f1_erf = 500;
    probe.f2_erf = 8000;
    probe.fft_xrange = 10;
    probe.fft_yrange = 40;
    probe.niter = 100;
    probe.ncav = 5;
    probe.nsrc = 1;
    probe.seed = 68131;
    strcpy(probe.checkfit, "~lsw");
    strcpy(probe.calibrate, "~lsw");

    // HEAR
    hear.start = 40;
    hear.step = 5;
    hear.fin = 5;
    hear.dur = 0.5F;
    hear.ramp_ms = 10;
    hear.f_start = 500;
    hear.f_end = 8000;
    hear.min_gap_s = 0.5F;
    hear.max_gap_s = 1;
    hear.rsp_tim_s = 2;
    hear.sd = 3;
    hear.rejlim = 10;
    hear.minfz = 3000;
    hear.maxfz = 13000;
    hear.minlev = -60;
    hear.maxlev = 90;
    hear.obn_max = 4000;
    hear.pts_oct = 1;
    hear.max_trial = 20;
    hear.protocol = 1;
    hear.hfthr = 0;
    strcpy(hear.file, "param.lst");

    // TONE
    tone.rate = 32000;
    tone.size = 2048;
    tone.ramp = 10;
    tone.skips = 1;
    tone.fft_xrange = 8;
    tone.fft_yrange = 80;
    tone.levref = 0;
    tone.freq = 1000; 
    tone.level = 65;
    tone.duration = 1;
    tone.ramp_ms = 10;
}

/* test the existance of a file by searching through the currrent directory
   and then the PATH directories.
   input : n	- the filename string pointer
   output: 1	- successful
	   0	- failed
*/
int
testexist(char *n)
{
    char   *env;
    char    fname[MAXNAME], p[MAXPATH];
    int     c;
    struct PATH tmp;

    if (_access(n, 0) == 0) {	// check current directory
	return (1);
    }
    strcpy(p, n);
    split_path(p, tmp.drive, tmp.dir, tmp.name, tmp.ext);
    if (strlen(tmp.drive) || strlen(tmp.dir))
        return (0);
    strcpy(fname, tmp.name);
    strcat(fname, tmp.ext);
    strcpy(p, exedir);
    strcat(p, fname);
    if (_access(p, 0) == 0) {	// check program directory
	strcpy(n, p);
	return (1);
    }
    env = getenv("PATH");
    if (env == NULL)
	return (0);

    c = 0;
    do {			// check PATH directories
	if (*env == ';' || *env == 0) {
	    if (c && p[c - 1] != '\\' && p[c - 1] != '/')
		p[c++] = '\\';
	    p[c] = 0;
	    strcat(p, fname);
	    if (_access(p, 0) == 0) {
		strcpy(n, p);
		return (1);
	    }
	    c = 0;
	} else
	    p[c++] = *env;
    } while (*env++ != 0);
    return (0);
}

void
read_args()
{
    char **av = argv;
    int    ac = argc;
    struct PATH path;
    FILE   *fpt = NULL;

    split_path(av[0], path.drive, path.dir, path.name, path.ext);
    strcpy(exedir, path.drive);
    strcat(exedir, path.dir);
    while (ac > 1) {
	if (av[1][0] == '-') {
	    switch (av[1][1]) {
	    case 'm':
		mono_screen = 1;
		break;
	    case 'v':
                print_version();
		exit(0);
		break;
	    case 'i':
		ac--;
		av++;
		if (ac > 1)
		    strcpy(file_name, av[1]);
		break;
	    case 'p':
		prnflg = 1;
		break;
	    case 'r':
		rclflg = 1;
		break;
	    case 'f':
		ac--;
		av++;
		if (ac > 1)
		    strcpy(batchfile, av[1]);
		break;
	    case 'H':
		if (strcmp("HL", av[1] + 1) == 0)
		    levref.at = 1;
		break;
	    case 'S':
		if (strcmp("SPL", av[1] + 1) == 0)
		    levref.at = 0;
		if (strcmp("SIL", av[1] + 1) == 0)
		    levref.at = 2;
		break;
	    }
	} else if (strchr(av[1], '*') == NULL && strchr(av[1], '?') == NULL) {
	    if (!fpt) {
		fpt = fopen(tmpbatchfile, "wt");
	    }
	    if (fpt)
		fprintf(fpt, "%s\n", av[1]);
	}
	ac--;
	av++;
    }
    if (fpt)
	fclose(fpt);
    else
	tmpbatchfile[0] = 0;
}

int
is_delim(char c)
{
    if (c == '\t' || c == '=' || c == '\n' || c == '\r'	|| c == ':')
	return (1);
    else
	return (0);
}

void
strip(char *s)
{
    char *b = s;

    while (*s && *s != ';' && *s != '\n' && *s != '\r')
	s++;
    while (s > b && s[-1] == ' ')
	s--;
    *s = '\0';
}

char   *
atline()
{
    int     i;

    i = cnt;
    while (is_delim(line[i]) && i < linelen)
	i++;
    if (line[linelen - 1] == '\n')
	line[--linelen] = 0;
    return (&line[i]);
}

/* get a token from the line buffer, and move the "cnt" index, the token is
   put into the "tokstr" buffer
   max token length is MAXTOK chars
*/
void
get_token(int reset)
{
    int     c;

    if (reset) {
	cnt = 0;
	linelen = strlen(line);
	if (linelen > MAXLINE)
	    linelen = MAXLINE;
    }
    while (is_delim(line[cnt]) && cnt < linelen)
	cnt++;
    c = 0;
    while (c < (MAXTOK - 1) && cnt < linelen && !is_delim(line[cnt])) {
	if (reset && line[cnt] == ' ') {
	    break;
	}
	tokstr[c++] = line[cnt++];
    }
    tokstr[c] = 0;
}

int
check_list(char *s, char **lp)
{
    int     i;

    while (*s == ' ')
	s++;
    for (i = 0; *lp[i]; i++)
	if (!_stricmp(s, lp[i]))
	    return (++i);
    return (0);
}

int
check_starter()
{
    char   *keys[] = {
	"[SYSTEM]", "[PRINTER]", "[HEARING]", "[PROBE]", "[TONE]", ""
    };

    return (check_list(tokstr, keys));
}

int
check_token(int flag)
{
    static char *sy_kws[] = {
	"MP_sensitivity", "AD_sensitivity", 
	"DA1_sensitivity", "DA2_sensitivity",
	"LS1_sensitivity", "LS2_sensitivity",
	"Refresh", "GREG", "AD_TYPE", "DSP_code", "DSP", ""
    };
    static char *pr_kws[] = {
	"Port", "Type", "Orient", "Label", ""
    };
    static char *pb_kws[] = {
        "rate", "sweeps", "sets", "size", "mode", "Calibrate", "limit",
        "Target", "FFTkHz", "FFTdB", "FFTref", "chk_swps",
        "Atten", "diacav", "temp", "Niter", "Ext_Iter",
        "Max_Len", "Ncav", 
        "chk_skps", "cal_skps", "cal_swps", "cal_sets", 
        "minres", "Iterate", "Crosstalk", "Debug", 
        "f1_erf", "f2_erf", "lentst", "scope", "seed", ""
    };
    static char *hr_kws[] = {
        "list", "limit", "Level_start", "Level_step", "Level_end",
        "Freq_start", "Freq_stop", "Per_octave", "Tone_duration",
        "Min_gap", "Max_gap", "Response_time", "Max_trials", "Min_sd", 
        "Simulate", "Algo", "Task", "Random", "Connect", 
        "MinFz", "MaxFz", "Obn_max", "Cond_THL", "Protocol", "scope", 
	"hfthr", "Freq_set", "MinLev", "MaxLev", "ramp", ""
    };
    static char *tone_kws[] = {
	"rate", "size", "frequency", "level", "duration", "ramp", "skips", 
	""
    };
    static char **kws[] = {sy_kws, pr_kws, hr_kws, pb_kws, tone_kws};
    static int ns = sizeof(kws) / sizeof(kws[0]);

    if (flag > 0 && flag <= ns) {
	return (check_list(tokstr, kws[flag - 1]));
    }
    return (0);
}

void
set_system(int c, double num)
{
    switch (c) {
	case 1:			/* MP_sensitivity */
	if (num > 0)
	    Sen.MP = (float) num;
	break;
    case 2:			/* AD_ sensitivity*/
	if (num > 0)
	    Sen.AD = (float) num;
	break;
    case 3:			/* DA1_sensitivity */
	if (num > 0)
	    Sen.DA[0] = (float) num;
	break;
    case 4:			/* DA2_sensitivity */
	if (num > 0)
	    Sen.DA[1] = (float) num;
	break;
    case 5:			/* LS1_sensitivity */
	if (num > 0)
	    Sen.LS[0] = (float) num;
	break;
    case 6:			/* LS2_sensitivity */
	if (num > 0)
	    Sen.LS[1] = (float) num;
	break;
    case 7:			/* Refreshing */
	if (num > 0 && num < 600)
	    refresh_time = (float) num;
	break;
    case 8:                     /* GREG */
	break;
    case 9:                     /* AD_type */
	break;
    case 10:			/* DSP code file */
	strcpy(dsp_code_file, tokstr);
	break;
    case 11:			/* DSP_select */
	if (*tokstr == 'A' || *tokstr == 'D')
		num = 0;
	else if (*tokstr == 'T' || *tokstr == 'M')
		num = 1;
	else if (*tokstr == 'P')
		num = 2;
	break;
    }
}

void
set_printer(int c, double num)
{
    switch (c) {
    case 1:			/* Port */
	if (strlen(tokstr))
	    set_prn_port(tokstr);
	break;
    case 2:			/* Type */
	if (!_stricmp(tokstr, "PostScript"))
	    set_prn_type(0);
	else if (!_stricmp(tokstr, "PCL"))
	    set_prn_type(1);
	break;
    case 3:			/* Orient */
	if (!_stricmp(tokstr, "Landscape"))
	    set_prn_orient(0);
	else if (!_stricmp(tokstr, "Portrait"))
	    set_prn_orient(1);
	break;
    case 4:			/* Label */
	get_token(1);
	set_prn_label(atline());
	break;
    }
}

void
set_probe(int c, double num)
{
    int     i = (int) num;

    switch (c) {
    case 1:			/* rate */
	if (num >= 100)
	    probe.rate = (int32_t) num;
	break;
    case 2:			/* sweeps */
	if (i > 0)
	    probe.cal_swps = i;
	break;
    case 3:			/* sets */
	if (i > 0)
	    probe.cal_sets = i;
	break;
    case 4:			/* size */
	if (i > 0)
	    probe.size = i;
	break;
    case 5:			/* mode */
        /* obselete */
	break;
    case 6:			/* stimulus */
	strcpy(probe.calibrate, tokstr);
	break;
    case 7:			/* limit */
        probe.rejlim = (float) num;
	break;
    case 8:			/* Target (obselete) */
	break;
    case 9:			/* FFTkHz */
	if (i > 0)
	    probe.fft_xrange = i;
	break;
    case 10:			/* FFTdB */
	if (i > 0)
	    probe.fft_yrange = i;
	break;
    case 11:			/* FFTref */
	if (toupper(tokstr[0]) == 'S')
	    probe.levref = 0;
	else if (toupper(tokstr[0]) == 'L')
	    probe.levref = 1;
	break;
    case 12:			/* checkfit sweeps */
	if (i > 0)
	    probe.chk_swps = i;
	break;
    case 13:			/* Atten */
	if (num > 0)
	    probe.cal_atten = (float) num;
	break;
    case 14:			/* diacav */
	if (num > 0)
	    probe.diacav = (float) num;
	break;
    case 15:			/* temp */
	if (num > 0)
	    probe.temp = (float) num;
	break;
    case 16:			/* Niter */
	if (i > 0)
	    probe.niter = i;
	break;
    case 17:			/* Ext_iter */
	i = toupper(tokstr[0]);
        if (i == 'Y' || i == 'T')
            thv_ext.at = 1;
        else if (i == 'S')
            thv_ext.at = 2;
        else
            thv_ext.at = 0;
	break;
    case 18:			/* Max_Len */
	if (num > 0)
	    probe.maxlen = (float) num;
	break;
    case 19:			/* Ncav */
	if (i > 0)
	    probe.ncav = i;
	break;
    case 20:			/* chk_skps */
	if (i > 0)
	    probe.chk_skps = i;
	break;
    case 21:			/* cal_skps */
	if (i > 0)
	    probe.cal_skps = i;
	break;
    case 22:			/* cal_swps */
	if (i > 0)
	    probe.cal_swps = i;
	break;
    case 23:			/* cal_sets */
	if (i > 0)
	    probe.cal_sets = i;
	break;
    case 24:			/* minres */
        probe.minres = (float) num;
	break;
    case 25:			/* Iterate */
        if (toupper(tokstr[0]) == 'Y')
            thv_itr.at = 1;
        else if (toupper(tokstr[0]) == 'M')
            thv_itr.at = 2;
        else
            thv_itr.at = 0;
	break;
    case 26:			/* Crosstalk */
        thv_xtk.at = (toupper(tokstr[0]) == 'Y');
	break;
    case 27:			/* Debug */
        thv_dbg.at = (toupper(tokstr[0]) == 'Y');
	break;
    case 28:			/* f1_erf */
        probe.f1_erf = (float) num;
	break;
    case 29:			/* f2_erf */
        probe.f2_erf = (float) num;
	break;
    case 30:			/* lentst */
        lentst = num;
	break;
    case 31:			/* scope */
	if (i > 0)
            scope = i;
	break;
    case 32:			/* seed */
	probe.seed = i;
	break;
    }
}

void
set_hearing(int c, double num)
{
    int     i = (int) num;

    switch (c) {
    case 1:			/* list */
	strcpy(hear.file, tokstr);
	break;
    case 2:			/* limit */
        hear.rejlim = (float) num;
	break;
    case 3:			/* Level_start */
        hear.start = (float) num;
	break;
    case 4:			/* Level_step */
	if (num > 0)
	    hear.step = (float) num;
	break;
    case 5:			/* Level_end */
	if (num > 0)
	    hear.fin = (float) num;
	break;
    case 6:			/* Freq_start */
	if (num > 0)
	    hear.f_start = (float) num;
	break;
    case 7:			/* Freq_stop */
	if (num > 0)
	    hear.f_end = (float) num;
	break;
    case 8:			/* Per_octave */
	if (i > 0)
	    hear.pts_oct = i;
	break;
    case 9:			/* Tone_duration */
	if (num > 0)
	    hear.dur = (float) num;
	break;
    case 10:			/* Min_gap */
	if (num > 0)
	    hear.min_gap_s = (float) num;
	break;
    case 11:			/* Max_gap */
	if (num > 0)
	    hear.max_gap_s = (float) num;
	break;
    case 12:			/* Response_Time */
	if (num > 0)
	    hear.rsp_tim_s = (float) num;
	break;
    case 13:			/* Max_trials */
	if (i > 0)
	    hear.max_trial = i;
	break;
    case 14:			/* Min_sd */
	if (num > 0)
	    hear.sd = (float) num;
	break;
    case 15:			/* Simulate */
        sim_obs.at = (toupper(tokstr[0]) == 'Y');
	break;
    case 16:			/* Algo */
        if (toupper(tokstr[0]) == 'M')
            thresh_algo.at = (toupper(tokstr[1]) == 'L') ? 0 : 1;
        else if (toupper(tokstr[0]) == 'U')
            thresh_algo.at = 2;
        else if (toupper(tokstr[0]) == 'A')
            thresh_algo.at = 3;
	break;
    case 17:			/* Task */
        if (toupper(tokstr[0]) == 'Y')
            thresh_task.at = 0;
        else
            thresh_task.at = 1;
	break;
    case 18:			/* Random */
        thresh_rand.at = (toupper(tokstr[0]) == 'Y');
	break;
    case 19:			/* Connect */
        thresh_cnct.at = (toupper(tokstr[0]) == 'Y');
	break;
    case 20:			/* MinFz */
	if (num > 0)
	    hear.minfz = (float) num;
	break;
    case 21:			/* MaxFz */
	if (num > 0)
	    hear.maxfz = (float) num;
	break;
    case 22:			/* Obn_max */
	if (num > 0)
	    hear.obn_max = (float) num;
	break;
    case 23:			/* Cond_THL */
        if (toupper(tokstr[2]) == 'H')
            cond_thl.at = 0;
        else
            cond_thl.at = 1;
	break;
    case 24:			/* protocol */
	if (i > 0)
	    hear.protocol = i;
	break;
    case 25:			/* scope */
	if (i > 0)
            scope = i;
	break;
    case 26:			/* hfthr */
        if (toupper(tokstr[0]) == 'Y')
            hear.hfthr = 1;
        else
            hear.hfthr = 0;
	break;
    case 27:			/* Freq_set */
        if (toupper(tokstr[0]) == 'C')
            freq_set_1.at = 0;
        else if (toupper(tokstr[0]) == 'S' && toupper(tokstr[1]) == 'T')
            freq_set_1.at = 1;
        else if (toupper(tokstr[0]) == 'N')
            freq_set_1.at = 2;
        else if (toupper(tokstr[0]) == 'S' && toupper(tokstr[1]) == 'I')
            freq_set_1.at = 3;
        else if (toupper(tokstr[0]) == 'W')
            freq_set_1.at = 4;
        else if (toupper(tokstr[0]) == 'O')
            freq_set_1.at = 5;
	break;
    case 28:			/* MinLev */
	hear.minlev = (float) num;
	break;
    case 29:			/* MaxLev */
	hear.maxlev = (float) num;
	break;
    case 30:                    /* ramp */
	if (num >= 0)
	    hear.ramp_ms = (float) num;
	break;
    }
}

void
set_tone(int c, double num)
{
    int     i = (int) num;

    switch (c) {
    case 1:			/* rate */
	if (num >= 100)
	    tone.rate = (int32_t) num;
	break;
    case 2:			/* buffer size */
	if (i > 0)
	    tone.size = i;
	break;
    case 3:			/* frequency */
	if (num > 0)
	    tone.freq = (float) num;
	break;
    case 4:			/* level */
	if (num > 0)
	    tone.level = (float) num;
	break;
    case 5:			/* duration */
	if (num > 0)
	    tone.duration = (float) num;
	break;
    case 6:                     /* ramp */
	if (num >= 0)
	    tone.ramp_ms = (float) num;
	break;
    case 7:                     /* skips */
	if (i >= 0)
	    tone.skips = i;
	break;
    }
}

void
read_init_file()
{
    FILE   *fptr;
    double  num;
    int     c, flag;

    if (!testexist(file_name))
	return;
    fptr = fopen(file_name, "rt");
    if (fptr == NULL)
	return;

    do {
	if (fgets(line, MAXLINE, fptr) == NULL)
	    return;
	strip(line);
	get_token(1);
	flag = check_starter();
    } while (!flag);

    while (fgets(line, MAXLINE, fptr) != NULL) {
	strip(line);
	get_token(1);
	if (*tokstr == '\0') {
	    continue;
	}
	c = check_starter();
	if (c) {
	    flag = c;
	    continue;
	}
	c = check_token(flag);
	get_token(0);
	num = atof(tokstr);
	if (flag == 1) {	/* [SYSTEM] */
	    set_system(c, num);
	} else if (flag == 2) {	/* [PRINTER] */
	    set_printer(c, num);
	} else if (flag == 3) {	/* [HEARING] */
	    set_hearing(c, num);
	} else if (flag == 4) {	/* [PROBE] */
	    set_probe(c, num);
	} else if (flag == 5) {	/* [TONE] */
	    set_tone(c, num);
	} else {
	    break;
	}
    }
    fclose(fptr);
}

void
read_counter_file()
{
    FILE   *fpt;
    int hcc, pcc, htc;

    prob_cal_cnt = hear_cal_cnt = hear_tth_cnt = 0;
    if (today_file(puttcounter)) {
        fpt = fopen(puttcounter, "rt");
	fscanf(fpt, "%d %d %d", &pcc, &hcc, &htc);
        fclose(fpt);
        prob_cal_cnt = limit(0, pcc, 255);
        hear_cal_cnt = limit(0, hcc, 255);
        hear_tth_cnt = limit(0, htc, 255);
    }
}

void
write_counter_file()
{
    FILE   *fpt;

    fpt = fopen(puttcounter, "wt");
    if (fpt == NULL)
	return;
    fprintf(fpt, "%5d %5d %5d", prob_cal_cnt, hear_cal_cnt, hear_tth_cnt);
    fclose(fpt);
}

/* display a short message at the top-right corner */
void
top_message(char *s)
{
    txtpar.text_wind_len = strlen(s);
    text_color(scrn_c[C_INFB], txtpar.cbg);
    gprintf(530, txtpar.font_height, s);
    check_event();
}

/* corner message */
void
corner(char *s)
{
    gr_settc(scrn_c[C_INFB], txtpar.cbg);
    gr_text(560, txtpar.font_height, s);
}

int
set_tone_wind()
{
    set_trailer(Tone_wind);
    return (27);
}

int
set_hear_wind()
{
    set_trailer(Hear_wind);
    return (27);
}

int
set_cav_wind()
{
    set_trailer((void (*)(void)) cav_test);
    return (27);
}

int
set_probe_wind()
{
    set_trailer((void (*)(void)) probe_test);
    return (27);
}

/* the batch mode processing, that is batch display or batch printing */
void
proc_batch(char *fn)
{
    FILE   *fpt;
    int     i;

    fpt = fopen(fn, "rt");
    if (fpt == NULL)
	return;
    top_message("Batching");
    while (fgets(o_file_name, 128, fpt) != NULL) {
	strip(o_file_name);
	i = strlen(o_file_name);
	if (i > 128) {
	    o_file_name[127] = 0;
	    i = 128;
	}
	i--;
	if (o_file_name[i] == '\n')
	    o_file_name[i] = 0;
        if (check_hearing_file(o_file_name, 0)) {
	    rd_hearing_file();
	} else if (check_cal_file(o_file_name)) {
	    rd_cal_file();
	} else if (!prnflg) {
	    decide(0, 2, o_file_name, "Not a valid HEARING file");
	}
	i = 0;
	if (prnflg) {
	    prn_screen();
	    wait_for_key(2.0);    /* 2 sec pause */
	    if (check_event())
		i = getevent();
	} else
	    for (;;) {
		i = getevent();
		if (i == ' ' || i == 27 || i == 3)
		    break;
	    }
	if (i == 27 || i == 3)
	    break;
    }
    fclose(fpt);
}

static void
batch()
{
    int     c;

    c = 0;
    if (batchfile[0]) {
	proc_batch(batchfile);
	c = 1;
    }
    if (tmpbatchfile[0]) {
	proc_batch(tmpbatchfile);
	c = 1;
    }
    if (c) {
	top_message("Finished");
	while (!check_event());
	top_message("        ");
    }
}

int
MAIN_PGM(int ac, char **av)
{
    argc = ac;
    argv = av;
    defpar();
    set_title("PUTT");
    set_prn_title("PUTT");
    init_alloc_space();
    read_counter_file();
    genfn("P", "CAL", tmpbatchfile, &prob_cal_cnt);
    genfn("H", "CAL", tmpbatchfile, &hear_cal_cnt);
    genfn("H", "TTH", tmpbatchfile, &hear_tth_cnt);
    strcpy(tmpbatchfile, "batch.fnf");
    read_init_file();
    read_args();
    Sen = Sys;
    dis_units(Sen.AD, Sen.MP);

    init_graphics();
    mouse_init();
    mainmenu_init(mainmenu);
    dsprst(TRUE);

    while (!pgm_done()) {
	clr_scr();
	text_color(menucolor.mfg, menucolor.mbg);
	init_wind();
        mainmenu_show(NULL);
        batch();
	mainmenu_driver();
    }
    Quit_wind(0);

    return (0);
}
