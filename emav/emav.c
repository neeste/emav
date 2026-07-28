/* ================================ emav.c ===============================
   The main program
   ======================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <dsp.h>
#include <menu.h>
#include <token.h>
#include <util.h>
#include "savage.h"
#include "color.h"
#include "screen.h"
#include "version.h"
#include "emav.h"		/* function prototypes */

#ifdef WIN32
#include <io.h>
#else /* WIN32 */
#include <unistd.h>
#define _access	    access
#define _stricmp    strcasecmp
#define _strnicmp   strncasecmp
#endif /* WIN32 */
#define MAXARG  16

int check_dpoae_file(char *, int);
int check_probe_file(char *, int);
int check_teoae_file(char *, int);
void rd_target(char *);

extern char auto_flag, *dpsigtyp[], *dpfrqtyp[];
extern float refresh_time, tolerance, F5_cutoff;
extern int nnsb;
extern TOGGLETYPE dpftype, dpnr_mode, f5type, stim_unit;
extern TOGGLETYPE xtype, datafmt, Sbin, dpsptype, Stim_mode, dp_octave;
extern TOGGLETYPE thv_itr, thv_ext, thv_xtk, dec_len;
extern TOKENFILE *t;

char    line[MAXLINE];
char    tokstr[MAXTOK];
char    exedir[MAXPATH] = {0};
char    file_name[MAXPATH] = "";
char    tmpbatchfile[MAXNAME] = "";
char    batchfile[MAXNAME] = "";
char    swp_file[MAXNAME] = "";
char    ths_file[MAXNAME] = "";
char    dsp_code_file[MAXNAME] = "";
char   *not_saved = "Not Saved";
char   *absci[NAB+1] = {"F2", "F1", "Fd", "L2", "L1", "L3", "Trial", ""};
char   **argv;
double  spl_ref = 20e-6;     /* SPL reference (rms Pa) */
float   F1_cutoff = (float) 2000.0, F2_cutoff = (float) 5656.9;
float   limit_mv = 5;
float   mstime1 = 6, mstime2 = 16, msramp = 1;
float   range_mv = 10;
float   teoae_refresh = 0;
float   red_thr = 0;
float   tmpcav = 25;
float   diacav = 0.8F;
float   rflcav = 1;
int     argc = 0;
int     dpoae_counter = 0;
int     teoae_counter = 0;
int     probe_counter = 0;
int     tmp_counter = 0;
int     binary_cnt = 0;
int     prnflg = 0;
int     sig_type = 0;
int     scope = 8192;
int     limits = 250;
int     itime1 = 100, itime2 = 450, iramp = 25;
int     base_addr = 0;
int     calphase = 1;
int     dspprm = 0;
int     dspnic = 1;
int     maxnpts = 0;
int     numadc = 1;
int     numcav = 1;
int     numsrc = 2;
int     contav = 0;
int     chofad = 0;
int     chofda = 0;
int     fft_xrange = 8;
int     fft_yrange = 80;
int     fft_levref = 0;
int     hpf_type = 1;   // Bessel high-pass
int     hpf_ord  = 3;   // 3rd-order filter
int     write_txt = 1;
int     rand_seed = 0;
int32_t   *accbuf, *accbuf_a, *accbuf_b, *swpbuf, *savbuf;
short  *outbuf, *stmbuf;
short   numsmo = 0;
sensitivity Sen = {
    (float) 5, (float) 0, (float) 1,
    {(float) 0, (float) 0}, 
    {(float) 5, (float) 5}, 
};
sensitivity Sys = {
    (float) 5, (float) 0, (float) 1,
    {(float) 0, (float) 0}, 
    {(float) 5, (float) 5}, 
};
struct _dpoae dpoae;
struct _teoae teoae;
struct _probe probe;
struct _tone tone;
STIM_PRES_MODE spm[MAXSPM] = {
    {1, 1, {{1}, {0}}, {1}, {1}, 1},
    {2, 1, {{1, -1}, {0, 0}}, {1, 0}, {1, 1}, 1},
    {3, 2, {{1, 1, -2}, {0, 0, 0}}, {1, 1, 0}, {1, 1, 1}, 1},
    {4, 3, {{1, 1, 1, -3}, {0, 0, 0, 0}}, {1, 1, 1, 0}, {1, 1, 1, 1}, 1},
    {3, 1, {{1, 0, 1}, {0, 1, 1}}, {1, 1, 0}, {1, 1, -1}, 1},
    {5, 1, {{1, 0, 1, 0, 2}, {0, 1, 0, 1, 2}}, {1, 1, 1, 1, 0}, 
    	{1, 1, 1, 1, -1}, 1},
};
WIND   *w_dis;

static int Test_menu();
static int View_mode();
static int set_dpoae_wind();
static int set_teoae_wind();
static int set_probe_wind();
static int set_cav_wind();
static int set_tone_wind();

static char cnt_file[MAXNAME] = "emav.cnt";
static char ini_file[MAXPATH] = "emav.ini";
static MAIN_MENU mainmenu[] = {
    {"&File", File_wind},
    {"&Info", Info_wind},
    {"&Options", Option_wind},
    {"&View", View_mode},
    {"&Test", Test_menu},
    {NULL, NULL}
};
static MENUITEM testmenu[] = {
    {"&DPOAE", NULL, NONE, 0, 0, 1, set_dpoae_wind},
    {"&TEOAE", NULL, NONE, 0, 0, 1, set_teoae_wind},
    {"&Probe", NULL, NONE, 0, 0, 1, set_probe_wind},
    {"&Cavity", NULL, NONE, 0, 0, 1, set_cav_wind},
    {"Ton&e", NULL, NONE, 0, 0, 1, set_tone_wind},
    {NULL, NULL, NONE, 0, 0, 1, NULL},
};

/* print the verison number and copyright info */
void
print_version()
{
    fprintf(stderr, "%s\n%s\n%s %s\n", VERSION, PGM_NAME, COPYRIGHT, BTNRH);
}

/* allocate space for data buffers, */
void
alloc_buffers()
{
    int accnpts, wi, wo;

    wi = (dsppar.code == 2) ? 2 : 1;
    wo = (dsppar.nic == 2) ? 2 : 1;
    maxnpts = (dspprm - base_addr) / (2 * (wi + wo));
    accnpts = maxnpts * wi;
    accbuf_a = (int32_t *) calloc(accnpts, 2 * sizeof(int32_t));
    outbuf = (short *) calloc(accnpts, 4 * sizeof(short));
    stmbuf = (short *) calloc(maxnpts, 4 * sizeof(short));
    if (accbuf_a == NULL || outbuf == NULL || stmbuf == NULL) {
        print_version();
	fprintf(stderr, "Not enough memory!  Program aborted!\n");
	exit(0);
    }
    accbuf = accbuf_a;
    accbuf_b = accbuf_a + accnpts;
    swpbuf = (int32_t *) outbuf;
    savbuf = (int32_t *) (stmbuf + 2 * maxnpts);
}

/* initialize memory model and allocate space for data buffers */
void
init_alloc_space()
{
    dsppar.code = 2;		// OAE mode, long stimulus
    dsppar.nic = 1;		// single-channel input
    dsppar.greg = dspgreg(3);	// 1=16kw, 2=65kw, 3=97kw, 4=129kw
    switch (dsppar.greg) {
    case 1 :			    /* 16 KW SMA */
        base_addr = 392 * 128;	    /* = 0x0C400 */
        dspprm = 510 * 128;	    /* = 0x0FF00 */
	break;
    case 2 :			    /* 65 KW SMA */
        base_addr = 0;		    /* = 0x00000 */
        dspprm = 512 * 128;	    /* = 0x10000 */
	break;
    case 3 :			    /* 97 KW SMA */
        base_addr = 0;		    /* = 0x00000 */
        dspprm = 768 * 128;	    /* = 0x18000 */
	break;
    case 4 :			    /* 129KW SMA */
        base_addr = 0;		    /* = 0x00000 */
        dspprm = 1023 * 128;	    /* = 0x20000 */
	break;
    }
    alloc_buffers();
}

/* set default parameter values */
void
defpar()
{
    numadc = 1;
    numcav = 1;
    numsrc = 2;
    fft_xrange = 8;
    fft_yrange = 80;
    fft_levref = 0;
    hpf_type = 1;       // Bessel high-pass
    hpf_ord  = 3;       // 3rd-order filter

    // soundcard sensitivity
    Sys.AD = 0;
    Sys.DA[0] = 0;
    Sys.DA[1] = 0;
    Sys.LS[0] = 5;
    Sys.LS[1] = 5;

    // DPOAE
    dpoae.rate = 32000;
    dpoae.size = 2048;
    dpoae.chk_swps = 4;
    dpoae.chk_atten = 10;
    dpoae.cal_swps = 16;
    dpoae.cal_atten = 20;
    dpoae.skips = 1;
    dpoae.sets = 2;
    dpoae.swp1set = 32;
    dpoae.levref = 0;
    dpoae.ramp = 10;
    dpoae.fft_xrange = 8;
    dpoae.fft_yrange = 80;
    dpoae.ord_min = -40;
    dpoae.ord_max = 80;
    dpoae.abs_min = 10;
    dpoae.abs_max = 100;
    dpoae.oct_min = -2;
    dpoae.oct_max = 4;
    dpoae.nnsb = 0;
    dpoae.nfsb = 0;
    dpoae.nic = 1;
    dpoae.modcyc = 0;
    dpoae.modper = 0;
    dpoae.mvmax = 2;
    dpoae.ramp_ms = 10;
    dpoae.nfdb = 9;
    dpoae.hpff[0] = 0;
    dpoae.hpff[1] = 0;
    dpoae.level_unit = 0;
    dpoae.contav = 0;
    dpoae.seed = 68131;
    strcpy(dpoae.checkfit, "");	    // not used
    strcpy(dpoae.calibrate, "~lsw");
    strcpy(dpoae.file, "param.lst");

    // PROBE
    probe.rate = 0;		// not used
    probe.size = 0;		// not used
    probe.chk_swps = 0;		// not used
    probe.chk_atten = 0;	// not used
    probe.chk_skps = 0;		// not used
    probe.cal_swps = 0;		// not used
    probe.cal_atten = 0;	// not used
    probe.cal_skps = 0;		// not used
    probe.cal_sets = 0;		// not used
    probe.levref = 1;
    probe.temp = 25;
    probe.diacav = 0.8F;
    probe.maxlen = 12;
    probe.minres = -1000;
    probe.f1_erf = 500;
    probe.f2_erf = 8000;
    probe.fft_xrange = 10;
    probe.fft_yrange = 40;
    probe.niter = 100;
    probe.ncav = 5;
    probe.nsrc = 2;
    probe.surge = 8;
    probe.seed = 0;		    // not used
    strcpy(probe.checkfit, "");	    // not used
    strcpy(probe.calibrate, "");    // not used

    // TEOAE
    teoae.rate = 32000;
    teoae.swp1set = 8;
    teoae.sets = 128;
    teoae.size = 512;
    teoae.fft_xrange = 8;
    teoae.fft_yrange = 40;
    teoae.levref = 0;
    teoae.chk_swps = 25;
    teoae.chk_atten = 10;
    teoae.mvmax = 1;
    teoae.hpff[0] = 0;
    teoae.hpff[1] = 0;
    strcpy(teoae.checkfit, "~ssw");
    strcpy(teoae.stim_file, "~ssw");

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
    tone.hpff[0] = 0;
    tone.hpff[1] = 0;
}

/* change the number of input channels */
void
change_nic(int nnic)
{
    dspnic = (nnic < 2) ? 1 : 2;
    if (dsppar.nic != dspnic) {
	dsppar.nic = dspnic;
	free(accbuf_a);
	free(outbuf);
	free(stmbuf);
	alloc_buffers();
    }
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

/* read int the command line args, and set up the flags and parameters
   input : ac	- the number of args
   	   av	- the pointer to array of args
   output: None
*/
void
read_args()
{
    char **av = argv;
    int ac = argc;

    FILE   *fpt = NULL;

    while (ac > 1) {
	if (av[1][0] == '-') {
	    switch (av[1][1]) {
	    case 'v':
		print_version();
		exit(0);
		break;
	    case 'i':
		ac--;
		av++;
		if (ac > 1)
		    strncpy(ini_file, av[1], MAXPATH - 1);
		break;
	    case 'p':
		prnflg = 1;
		break;
	    case 'f':
		ac--;
		av++;
		if (ac > 1)
		    strcpy(batchfile, av[1]);
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

/* test if the char is a delimiter.
   input : c	- the testing character
   output: 1	- is a delimiter
	   0	- not a delimiter
*/
int
is_delim(char c)
{
    if (c == '\t' || c == '=' || c == '\n' || c == '\r' || c == ':')	    // This may break Ear [STN Dec-2007]
    	return(1);
    return (0);
}

static int cnt = 0;
static int linelen;

/* when processing a line of statement, find the start point of the next
   token if any.
   input : None	("line" buffer, and "cnt" the index, "linelen" the line length)
   output: the pointer to the start of the next token
*/
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
    while (is_delim(line[cnt]) && cnt < linelen)    // skip leading delimiters
	cnt++;
    while ((line[cnt] == ' ') && cnt < linelen)    // skip leading spaces
	cnt++;
    while (is_delim(line[linelen - 1]) && cnt < linelen) {  // strip trailing delimiters
	linelen--;
	line[linelen] = '\0';
    }
    c = 0;
    while (c < (MAXTOK - 1) && cnt < linelen && !is_delim(line[cnt])) {
	tokstr[c++] = line[cnt++];
    }
    while (c > 0 && tokstr[c - 1] == ' ')   /* trim space from end of token */
	c--;
    tokstr[c] = 0;
}

/* get a file name from the line buffer,  move the "cnt" index,
   and put file name into the "tokstr" buffer
   max token length is MAXTOK chars
*/
void
get_file_name()
{
    int     c;

    cnt = 0;
    linelen = strlen(line);
    if (linelen > MAXLINE)
        linelen = MAXLINE;
    while (line[cnt] != '=' && cnt < linelen)
	cnt++;
    if (strchr(line + cnt, ':')) {
        while (line[cnt] != ':' && cnt < linelen)
            cnt++;
    }
    if (strchr(line + cnt, '/')) {
        while (line[cnt] != '/' && cnt < linelen)
            cnt++;
    }
    if (strchr(line + cnt, '\\')) {
        while (line[cnt] != '\\' && cnt < linelen)
            cnt++;
    }
    while (is_delim(line[cnt]) && cnt < linelen)
	cnt++;
    c = 0;
    while (!is_delim(line[cnt]) && cnt < linelen) {
	tokstr[c++] = line[cnt++];
	if (c > 39)
	    c = 39;
    }
    tokstr[c] = 0;
}

/* check is a string if in a list of keywords.
   input : s	- the testing string
	   lp	- the pointer to an array of keywords
   output: 1	- yes

	   0	- no
*/
int
check_list(char *s, char **lp)
{
    int     i;

    while (*s == ' ')
	s++;
    for (i = 0; *lp[i]; i++)
	if (!_strnicmp(s, lp[i], strlen(lp[i])))
	    return (++i);
    return (0);
}

/* check if tokstr contains the key word signaling the beginning of a new
   section.  (this is used in the initialization file)
*/
int
check_starter()
{
    char   *keys[] = {
	"[SYSTEM]", "[PRINTER]", "[TEOAE]", "[DPOAE]", "[PROBE]", "[TONE]", ""
    };

    return (check_list(tokstr, keys));
}

/* check for the keywords inside each section
   input : flag	- the index to a section of keywords
   output: 1	- yes
	   0	- no
*/
int
check_token(int flag)
{
    static char *sy_kws[] = {
	"MP_sensitivity", "AD_sensitivity", 
	"DA1_sensitivity", "DA2_sensitivity",
	"LS1_sensitivity", "LS2_sensitivity",
	"Refresh", "GREG", "AD_type", "DSP_code", "DSP", "MP_transfer",
	"AD_chnoff", "DA_chnoff", ""
    };
    static char *pr_kws[] = {
	"Port", "Type", "Orient", "Label", ""
    };
    static char *te_kws[] = {
	"rate", "sweeps", "sets", "size", "mode", "stimulus",
	"Limit", "Time1", "Time2", "Target", "FFTkHz", "FFTdB",
	"FFTref", "Adjust", "Tolerance", "Checkfit", "chk_swps", "chk_atten",
	"Trefresh", "F1", "F2", "HPF1", "HPF2", "Count", ""
    };
    static char *dp_kws[] = {
	"rate", "sweeps", "sets", "size", "checkfit",
	"CalibratePhase", "scope", "list", "Limit",
	"abscissa", "atten", "DP_freq", "FFTmin", "FFTmax",
	"FFTkHz", "FFTdB", "FFTref", "chk_swps", "chk_atten", "cal_swps",
	"signal", "nnsb", "skips", "DataFmt", "SaveBin", 
	"SpecFreqRange", "Octave", "ramp",
	"MinLevOrd", "MaxLevOrd", "MinLevAbs", "MaxLevAbs",
	"MinFrqOct", "MaxFrqOct", "RedThr", "NIC", "calibrate",
	"HPF_type", "HPF_freq", "Count", "HPF1", "HPF2", "thsf", 
	"Level_Unit", "ContAv", "seed", ""
    };
    static char *pb_kws[] = {
         "rate", "sweeps", "sets", "size", "mode", "Calibrate", "limit",
        "Target", "FFTkHz", "FFTdB", "FFTref", "chk_swps",
        "Atten", "diacav", "temp", "Niter", "Ext_Iter",
        "Max_Len", "Ncav", 
        "chk_skps", "cal_skps", "cal_swps", "cal_sets", 
        "minres", "Iterate", "Crosstalk", 
        "f1_erf", "f2_erf", "scope", "smooth", 
	"write_txt", "dec_len", "Nsrc", "Count", "seed",  "surge", ""
    };
    static char *tone_kws[] = {
	"rate", "size", "frequency", "level", "duration", 
	"ramp", "HPF1", "HPF2", ""
    };
    static char **kws[] = {sy_kws, pr_kws, te_kws, dp_kws, pb_kws, tone_kws};
    static int ns = sizeof(kws) / sizeof(kws[0]);

    if (flag > 0 && flag <= ns) {
	return (check_list(tokstr, kws[flag - 1]));
    }
    return (0);
}

/* set up the paramters in the SYSTEM section of the initialization file
   input : c	- indicates which keyword
	   num	- tokstr converted into double
*/
void
set_system(int c, double num)
{
    switch (c) {
    case 1:			/* MP_sensitivity */
	if (num > 0)
	    Sys.MP = (float) num;
	break;
    case 2:			/* AD_ sensitivity*/
	if (num > 0)
	    Sys.AD = (float) num;
	break;
    case 3:			/* DA1_sensitivity */
	if (num > 0)
	    Sys.DA[0] = (float) num;
	break;
    case 4:			/* DA2_sensitivity */
	if (num > 0)
	    Sys.DA[1] = (float) num;
	break;
    case 5:			/* LS1_sensitivity */
	if (num > 0)
	    Sys.LS[0] = (float) num;
	break;
    case 6:			/* LS2_sensitivity */
	if (num > 0)
	    Sys.LS[1] = (float) num;
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
	strncpy(dsp_code_file, tokstr, MAXNAME);
	break;
    case 11:			/* DSP select */
	break;
    case 12:			/* MP_transfer */
        strncpy(Sys.MPfn, tokstr, 80);
	break;
    case 13:			/* AD_chnoff */
        chofad = (int) num;
	break;
    case 14:			/* DA_chnoff */
        chofda = (int) num;
	break;
    }
}

/* set up the paramters in the PRINTER section of the initialization file
   input : c	- indicates which keyword
	   num	- tokstr converted into double
*/
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
	set_prn_label(tokstr);
	break;
    }
}

/* set up the paramters in the TEOAE section of the initialization file
   input : c	- indicates which keyword
	   num	- tokstr converted into double
*/
void
set_teoae(int c, double num)
{
    int     i = (int) num;

    switch (c) {
    case 1:			/* rate */
	if (num >= 100)
	    teoae.rate = (int32_t) num;
	break;
    case 2:			/* sweeps */
	if (i > 0)
	    teoae.swp1set = i;
	break;
    case 3:			/* sets */
	if (i > 0)
	    teoae.sets = i;
	break;
    case 4:			/* size */
	if (i > 0)
	    teoae.size = i;
	break;
    case 5:			/* mode */
	if (i >= 0 && i < MAXSPM)
	    Stim_mode.at = i;
	break;
    case 6:			/* stimulus */
	strcpy(teoae.stim_file, tokstr);
	break;
    case 7:			/* limit */
	teoae.mvmax = (float) num;
	break;
    case 8:			/* Time1 */
	mstime1 = (float) num;
	break;
    case 9:			/* Time2 */
	mstime2 = (float) num;
	break;
    case 10:			/* Target */
	rd_target(tokstr);
	break;
    case 11:			/* FFTkHz */
	if (i > 0)
	    teoae.fft_xrange = i;
	break;
    case 12:			/* FFTdB */
	if (i > 0)
	    teoae.fft_yrange = i;
	break;
    case 13:			/* FFTref */
	if (*tokstr == 'S')
	    teoae.levref = 0;
	else if (*tokstr == 'L')
	    teoae.levref = 1;
	break;
    case 14:			/* Adjust */
	i = toupper(*tokstr);
	if (i == 'A')
	    auto_flag = 'A';
	else if (i == 'M')
	    auto_flag = 'M';
	break;
    case 15:			/* tolerance */
	if (num > 0)
	    tolerance = (float) num;
	break;
    case 16:			/* checkfit stimulus */
	strcpy(teoae.checkfit, tokstr);
	break;
    case 17:			/* chk_swps */
	if (i > 0)
	    teoae.chk_swps = i;
	break;
    case 18:			/* chk_atten */
	if (num >= 0)
	    teoae.chk_atten = (float) num;
	break;
    case 19:			/* Trefresh */
	if (num > 0 && num < 600)
	    teoae_refresh = (float) num;
	break;
    case 20:			/* F1 */
	if (num > 0)
	    F1_cutoff = (float) num;
	break;
    case 21:			/* F2 */
	if (num > 0)
	    F2_cutoff = (float) num;
	break;
    case 22:			/* HPF1 */
	teoae.hpff[0] = (float) num;
	break;
    case 23:			/* HPF2 */
	teoae.hpff[1] = (float) num;
	break;
    case 24:                    /* Count */
	if (teoae_counter < i)
	    teoae_counter = i;
	break;
    }
    check_t1_t2();
}

/* set up the paramters in the DPOAE section of the initialization file
   input : c	- indicates which keyword
	   num	- tokstr converted into double
*/
void
set_dpoae(int c, double num)
{
    int     i = (int) num;

    switch (c) {
    case 1:			/* rate */
	if (num >= 100)
	    dpoae.rate = (int32_t) num;
	break;
    case 2:			/* sweeps */
	if (i > 0)
	    dpoae.swp1set = i;
	break;
    case 3:			/* sets */
	if (i > 0)
	    dpoae.sets = i;
	break;
    case 4:			/* size */
	if (i > 0)
	    dpoae.size = i;
	break;
    case 5:			/* checkfit */
	// not used
	break;
    case 6:                    /* CalibratePhase */
	i = toupper(tokstr[0]);
	if (i == 'N')
	    calphase = 0;
	else if (i == 'Y')
	    calphase = 1;
	break;
    case 7:			/* scope */
	if (i > 0)
	    scope = i;
	break;
    case 8:			/* list */
	strcpy(dpoae.file, tokstr);
	break;
    case 9:			/* limit */
	dpoae.mvmax = (float) num;
	break;
    case 10:			/* Abscissa */
	c = check_list(tokstr, absci);
	if (c > 0)
	    xtype.at = c - 1;
	break;
    case 11:			/* Atten */
	dpoae.cal_atten = (float) num;
	break;
    case 12:			/* DP_freq */
	i = check_list(tokstr, dpfrqtyp);
	if (i > 0)
	    sig_type = i - 1;
	break;
    case 13:			/* FFTmin (obselete) */
        dpoae.ord_min = i;
	break;
    case 14:			/* FFTmax (obselete) */
        dpoae.ord_max = i;
	break;
    case 15:			/* FFTkHz */
	if (i > 0)
	    dpoae.fft_xrange = i;
	break;
    case 16:			/* FFTdB */
	if (i > 0)
	    dpoae.fft_yrange = i;
	break;
    case 17:			/* FFTref */
	if (*tokstr == 'S')
	    dpoae.levref = 0;
	else if (*tokstr == 'L')
	    dpoae.levref = 1;
	break;
    case 18:                    /* chk_swps */
	if (i > 0)
	    dpoae.chk_swps = i;
	break;
    case 19:                    /* chk_attn */
	if (num >= 0)
	    dpoae.chk_atten = (float) num;
	break;
    case 20:                    /* cal_swps */
	if (num > 0)
	    dpoae.cal_swps = i;
	break;
    case 21:                    /* signal */
	c = check_list(tokstr, dpsigtyp);
	if (c > 0)
	    dpnr_mode.at = c - 1;
	break;
    case 22:                    /* nnsb */
	if (i >= -MAXNNSB && i <= MAXNNSB)
	    dpoae.nnsb = i;
	break;
    case 23:                    /* skips */
	if (i >= 0)
	    dpoae.skips = i;
	break;
    case 24:                    /* DataFmt */
	i = toupper(tokstr[0]);
    	if(i == 'N')       
	    datafmt.at = 0;     /* Normal */
	else if(i == 'H')  
	    datafmt.at = 1;     /* Higher-order */
	else if(i == 'E') 
	    datafmt.at = 2;     /* Extended */
	else if(i == 'M') 
	    datafmt.at = 3;     /* Multi */
	else if(i == 'S') 
	    datafmt.at = 4;     /* SFOAE */
	break;
    case 25:                    /* SaveBin */
	i = toupper(tokstr[0]);
	if (i == 'N')
	    Sbin.at = 0;
	else if (i == 'Y')
	    Sbin.at = 1;
	break;
    case 26:                    /* SpecFreqRange */
	i = toupper(tokstr[0]);
	if (i == 'F')
	    dpsptype.at = 0;
	else if (i == 'A')
	    dpsptype.at = 1;
	break;
    case 27:                    /* Octave */
        if (i >= 0 && i <= 2)
            dp_octave.at = i;
	break;
    case 28:			/* ramp */
	if (num > 0)
	    dpoae.ramp_ms = (float) num;
    case 29:			/* MinLevOrd */
        dpoae.ord_min = i;
	break;
    case 30:			/* MaxLevOrd */
        dpoae.ord_max = i;
	break;
    case 31:			/* MinLevAbs */
        dpoae.abs_min = i;
	break;
    case 32:			/* MaxLevAbs */
        dpoae.abs_max = i;
	break;
    case 33:			/* MinFrqOct */
        dpoae.oct_min = i;
	break;
    case 34:			/* MaxFrqOct */
        dpoae.oct_max = i;
	break;
    case 35:			/* RedThr */
        red_thr = (float) num;
	break;
    case 36:                     /* NIC */
        dpoae.nic = i;
	break;
    case 37:			/* calibrate */
	strcpy(dpoae.calibrate, tokstr);
	break;
    case 38:                    /* HPF_type */
	f5type.at = limit(0,i,2);
	break;
    case 39:                    /* HPF_freq */
	F5_cutoff = (float) num;
	break;
    case 40:                    /* Count */
	if (dpoae_counter < i)
	    dpoae_counter = i;
	break;
    case 41:			/* HPF1 */
	dpoae.hpff[0] = (float) num;
	break;
    case 42:			/* HPF2 */
	dpoae.hpff[1] = (float) num;
	break;
    case 43:			/* thsf */
	strcpy(dpoae.ths_file, tokstr);
	break;
    case 44:                    /* Level_Unit */
	dpoae.level_unit = set_level_unit(tokstr);
	break;
    case 45:                    /* ContAv */
	i = toupper(tokstr[0]);
	if (i == 'N')
	    dpoae.contav = 0;
	else if (i == 'Y')
	    dpoae.contav = 1;
	break;
    case 46:			/* seed */
	dpoae.seed = i;
	break;
    }
}

/* set up the paramters in the PROBE section of the initialization file
   input : c	- indicates which keyword
	   num	- tokstr converted into double
*/
void
set_probe(int c, double num)
{
    int     i = (int) num;

    switch (c) {
    case 1:			/* rate */
    case 2:			/* sweeps */
    case 3:			/* sets */
    case 4:			/* size */
    case 5:			/* mode */
    case 6:			/* calibrate */
    case 7:			/* limit (obselete) */
    case 8:			/* target (obselete) */
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
	break;
    case 13:			/* Atten */
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
        if (toupper(tokstr[0]) == 'T')
            thv_ext.at = 1;
        else if (toupper(tokstr[0]) == 'R')
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
    case 20:			/* chk_skps (unused ) */
	break;
    case 21:			/* cal_skps (unused) */
	break;
    case 22:			/* cal_swps */
	break;
    case 23:			/* cal_sets */
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
    case 27:			/* f1_erf */
        probe.f1_erf = (float) num;
	break;
    case 28:			/* f2_erf */
        probe.f2_erf = (float) num;
	break;
    case 29:			/* scope */
	break;
    case 30:			/* smooth */
        numsmo = i;
	break;
    case 31:                    /* write_txt */
	write_txt = i;
	break;
    case 32:			/* dec_len */
        if (toupper(tokstr[0]) == 'Y')
            dec_len.at = 1;
	else
            dec_len.at = 0;
	break;
    case 33:			/* Nsrc */
	if (i > 0)
	    probe.nsrc = i;
	break;
    case 34:                    /* Count */
	if (probe_counter < i)
	    probe_counter = i;
	break;
    case 35:			/* seed */
	break;
    case 36:			/* surge */
        probe.surge = i;
	break;
    }
}

/* set up the paramters in the TONE section of the initialization file
   input : c	- indicates which keyword
	   num	- tokstr converted into double
*/
void
set_tone(int c, double num)
{
    int     i = (int) num;

    switch (c) {
    case 1:			/* rate */
	if (num > 0)
	    tone.rate = (int32_t) num;
	break;
    case 2:			/* size */
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
    case 6:			/* ramp */
	if (num > 0)
	    tone.ramp_ms = (float) num;
	break;
    case 7:			/* HPF1 */
	dpoae.hpff[0] = (float) num;
	break;
    case 8:			/* HPF2 */
	dpoae.hpff[1] = (float) num;
	break;
    }
}

/* read line by line from the initilization file and set up the parameters
   accordingly.
*/
void
read_init_file()
{
    char   *env;
    double  num;
    int     c, flag;
    FILE   *fptr;
    struct PATH path;

    // Look for alternate configuration file
    env = getenv("EMAV.INI");
    if (env) {
	strncpy(ini_file, env, MAXPATH - 1);
    }

    // Locate directory where EXE file resides
    split_path(argv[0], path.drive, path.dir, path.name, path.ext);
    strcpy(exedir, path.drive);
    strcat(exedir, path.dir);

    if (!testexist(ini_file)) {
	return;
    }
    fptr = fopen(ini_file, "rt");
    if (fptr == NULL)
	return;

    do {
	if (fgets(line, MAXLINE, fptr) == NULL)
	    return;
	get_token(1);
	flag = check_starter();
    } while (!flag);

    while (fgets(line, MAXLINE, fptr) != NULL) {
	get_token(1);
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
	} else if (flag == 3) {	/* [TEOAE] */
	    set_teoae(c, num);
	} else if (flag == 4) {	/* [DPOAE] */
	    set_dpoae(c, num);
	} else if (flag == 5) {	/* [PROBE] */
	    set_probe(c, num);
	} else if (flag == 6) {	/* [TONE] */
	    set_tone(c, num);
	} else {
	    break;
	}
    }
    fclose(fptr);
    chk_ramp_dpoae();
    chk_ramp_tone();
}

/* read the initial values for the counters from the counter file
   The counters are used to generate file names
*/
void
read_counter_file()
{
    FILE   *fpt;

    if (today_file(cnt_file)) {
        fpt = fopen(cnt_file, "rt");
	fscanf(fpt, "%d %d %d %d", &teoae_counter, &dpoae_counter, 
	    &binary_cnt, &probe_counter);
        fclose(fpt);
    }
    if (teoae_counter < 0)
	teoae_counter = 0;
    if (dpoae_counter < 0)
	dpoae_counter = 0;
    if (binary_cnt < 0)
	binary_cnt = 0;
    if (probe_counter < 0)
	probe_counter = 0;
}

/* store the counter info into an ASCII file */
void
write_counter_file()
{
    FILE   *fpt;

    fpt = fopen(cnt_file, "wt");
    if (fpt == NULL)
	return;
    fprintf(fpt, "%5d %5d %5d %5d", teoae_counter, dpoae_counter,
	binary_cnt, probe_counter);
    fclose(fpt);
}

/* display a short message at the top-right corner */
void
top_message(char *s)
{
    gr_settc(scrn_c[C_INFB], txtpar.cbg);
    gr_text(530, txtpar.font_height, s);
}

/* the batch mode processing, that is batch display or batch printing */
void
proc_batch(char *fn)
{
    FILE   *fpt, *efp;
    int     i;

    fpt = fopen(fn, "rt");
    if (fpt == NULL)
	return;
    efp = NULL;
    top_message("Batching");
    while (fgets(o_file_name, MAXNAME, fpt) != NULL) {
	i = strlen(o_file_name) - 1;
	if (o_file_name[i] == '\n')
	    o_file_name[i] = 0;
	if (check_teoae_file(o_file_name, 0)) {
	    rd_teoae_file();
	} else if (check_dpoae_file(o_file_name, 0)) {
	    rd_dpoae_file();
	} else if (check_probe_file(o_file_name, 0)) {
            rd_dpcal_file();
	    rd_probe_file();
	} else {
	    if (prnflg) {
		if (efp == NULL)
		    efp = fopen("emav.log", "wt");
		if (efp != NULL)
		    fprintf(efp,"Open error:  %s is not a valid EMAV file\n",
		        o_file_name);
	    } else {
		decide(0, 3, "Open error:", o_file_name, 
		    "is not a valid EMAV file.");
	    }
	    continue;
	}
	i = 0;
	if (prnflg) {
	    prn_screen();
	    if (check_event())
		i = getevent();
	} else {
	    for (;;) {
		i = getevent();
		if (i == ' ' || i == 3 || i == 27)
		    break;
	    }
        }
	if (i == 3 || i == 27)
	    break;
    }
    fclose(fpt);
    if (efp != NULL)
	fclose(efp);
}

/* handles batch processing
*/
void
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
	remove(tmpbatchfile);
	c = 1;
    }
    if (c) {
	top_message("Finished");
	while (!check_event() && !pgm_done());
	top_message("        ");
    }
}

/* set to view file */
static int
View_mode()
{
    top_message("VIEW ");
    view_file();
    top_message("     ");
    return(0);
}

/* Tests menu */
static int
Test_menu()
{
    return(simple_submenu(menu_x(), menu_y(), testmenu));
}

/* set to run dpoae test */
static int
set_dpoae_wind()
{
    init_wind();
    top_message("DPOAE ");
    set_trailer(DPOAE_wind);
    top_message("     ");
    return(27);
}

/* set to run teoae test */
static int
set_teoae_wind()
{
    init_wind();
    top_message("TEOAE ");
    set_trailer(TEOAE_wind);
    top_message("     ");
    return(27);
}

/* set to run dpoae test */
static int
set_probe_wind()
{
    init_wind();
    top_message("Probe ");
    set_trailer(Probe_wind);
    top_message("     ");
    return(27);
}

/* set to run dpoae test */
static int
set_cav_wind()
{
    init_wind();
    top_message("Cavity");
    set_trailer(Cavity_wind);
    top_message("     ");
    return(27);
}

/* set to run tone test */
static int
set_tone_wind()
{
    init_wind();
    top_message("Tone  ");
    set_trailer(Tone_wind);
    top_message("     ");
    return(27);
}

/* Quit the program */

void
quit_pgm(char *no_use)
{
    end_gr();
    dsprst(TRUE);	/* Assert the TMS 320 reset	 */
    if (t != NULL)
	tclose(t);
    if (_access(tmpbatchfile, 0) == 0)
	remove(tmpbatchfile);
}

/* The main entry point */
int
MAIN_PGM(int ac, char **av)
{
    argc = ac;
    argv = av;
    defpar();
    set_title("EMAV");
    set_prn_title("EMAV");
    init_alloc_space();
    genfn("B", "TMP", tmpbatchfile, &tmp_counter);
    teoae_counter = 0;
    read_counter_file();
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
    quit_pgm(0);

    return(0);
}
