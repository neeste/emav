/* abrav.c */

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
#include "abrav.h"

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#define _access access
#define _stricmp strcasecmp
#endif /* WIN32 */

double  epamp_agn(double);
double  epamp_lpf(double);
int     today_file(char *);
int     Info_wind();
int     Option_wind();
int     Edit_wind();
int     File_wind();
int     ABR_wind();
int     Quit_wind(char *);
int     open_file();
int     screen_print();
int	ini_lst(char *);
int	nxt_lst(void);
void    init_wind(void);
void    init_graphics();
void    clr_scr();
void    wind_color(WIND *, int, int);
void    text_color(int, int);
void    rd_abr_file();
void	grab_abr();
void	points_msg(void);
void	batch_format();
void    Tone_wind(void);
TOKENFILE *check_abr_file(char *, int);
void    epamp_init(int, int, int, int);
void    split_path(char *, char *, char *, char *, char *);


extern int NNSB, differ_flag;
extern float refresh_time, tolerance;
extern float F1_cutoff, F2_cutoff;
extern TOGGLETYPE aqm, pqm, filter_type, weight, rejmod;
extern TOGGLETYPE epa_hpf, epa_lpf, epa_gain, epa_acc, epa_pgn, epa_notch, polar; 
extern TOGGLETYPE r_mode, pqm_mode, ok_mode;
extern TOGGLETYPE xtype, datafmt, Sbin;

char    exedir[MAXPATH] = {0};
char    file_name[MAXNAME] = "";
char    cali_file[MAXNAME] = "";
char    data_file[MAXNAME] = "";
char    init_file[MAXNAME] = "abrav.ini";
char    record_file[MAXNAME] = "record.swp";
char    log_file[16] = "abrav.log";
char    abravcounter[16] = "abrav.cnt";
char    tmpbatchfile[16] = "";
char    batchfile[40] = "";
char   *abr_tmp_name = "out.abr";
char    dsp_code_file[MAXNAME] = "";
char   *month_name[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
char    line[MAXLINE], tokstr[MAXTOK];
double  dsp_scale = 1;
float   def_xrange = 16, def_yrange = (float) 2e-6;
float   ep_start = 0;
float   ad_sens = 0;
float   time1_ms = 4, time2_ms = 14;
float   max_level = 70.0, nhl_level = 35, test_level = 0;
float   repr_crit1 = (float) 0.6, repr_crit2 = (float) 0.9;
float   VR_crit1 = 2, VR_crit2 = 3;
float  *accbuf, *accbuf_1, *accbuf_2, *longptr;
float   spl_ref = (float) 20e-6;    /* SPL reference (rms Pa) */
float   rej_mv = 1500;		    /* artifact reject threshold (mv) */
int     nab = 6;
int     abr_refresh = 0;
int     dc_offset_mic = 0;
int     dc_offset_abr = 0;
int     lstflg = 0;
int     mono = 0;
int     nmp = 5;
int     abr_count = 0;
int     bin_count = 0;
int     min_sweeps = 20, min_swps = 0;
int     reject_mode = 0;
int     prnflg = 0;
int     batch_fmt = 0;
int     recflg = 0;
int	calibr_flag = 0;
int	cal_type = 0;
int	check_signal = 0;
int     ad_pol = 1;
int     ep_chan = 1 | 2;
int     ep_chan_cfg = 1 | 2;
int     mic_chan = 0;
int     weight_type = 0;
int     aqm_type = 0;
int     pqm_type = 0;
int     pre_gain = 1000;
int     ver_num = VERSION_NUM;
int     ctrl_set = 0, ctrl_level = 0,  term_code = 0;
int     make_template = 1, trkbuf = 3;
int     epa_flag;
int     maxnpts = MAXNPTS;
short	*fsprepro;
short   *outbuf, *stimulus;
short	abr_pk[2][7];
unsigned int base_addr = (unsigned) 0;
unsigned int dspprm = (unsigned) 510 * 128;
unsigned int dspspts = (unsigned) 506 * 128;
unsigned int dspmnmx = (unsigned) 510 * 128 + 64;
WIND   *w_dis;
FILE *binfp;
sensitivity Sen = {
    5, 9400, {1700, 1700}, {2, 2}, 
};
struct abr abr_ = {
    20, 100, 1024, 100, 0, 32000L, 2, 40, 1,
    1024, 20, 20, 31.5, 11.5, "", "", 
    0, 0, 0, 0,
    (float) 0.1, 2, 100,
    1000, 1000, 0, 2
};
struct tone tone = {
    32000L, 1024, 1024, 1, 8, 80, 0, 1000.0, 65.0, 1.0,
};
struct LEV_DEP L30 = {
    3, 13, 2.5, 4.0, 60.0, 70.0
};
struct LEV_DEP L50 = {
    2, 11, 2.5, 6.0, 60.0, 80.0
};
struct LEV_DEP L70 = {
    1, 9, 2.5, 8.0, 60.0, 90.0
};
struct stimParam *head_param = NULL;
struct stimParam *curr_param;
struct stim stim_;

static int Test_wind();
static int set_abr_wind();
static int set_probe_wind();
static int set_tone_wind();
static MAIN_MENU mainmenu[] = {
    {"&File", File_wind},
    {"&Info", Info_wind},
    {"&Options", Option_wind},
    {"&Edit", Edit_wind},
    {"&Test", Test_wind},
    {NULL, NULL}
};
static MENUITEM testmenu[] = {
    {"&ABR", NULL, NONE, 0, 0, 1, set_abr_wind},
    {"&Cavity", NULL, NONE, 0, 0, 1, set_probe_wind},
    {"&TONE", NULL, NONE, 0, 0, 1, set_tone_wind},
    {NULL, NULL}
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
    maxnpts = MAXNPTS;
    accbuf = (float *) malloc((unsigned) (8 * MAXNPTS) * sizeof(float));
    outbuf = (short *) malloc((unsigned) (8 * MAXNPTS) * sizeof(float));
    stimulus = (short *) malloc((6 * MAXNPTS) * sizeof(short));
    fsprepro = (short *) malloc((6 * MAXNPTS) * sizeof(short));
    if (fsprepro == NULL) {
	print_version();
	fprintf(stderr, "Not enough memory!  Program aborted!\n");
	exit(0);
    }
    accbuf_1 = accbuf;
    accbuf_2 = accbuf + (4 * MAXNPTS);
    longptr = (float *) outbuf;
    sav_stim = (float *) (stimulus + (2 * MAXNPTS));
}

/* initialize memory model and allocate space for data buffers */
void
init_alloc_space()
{
    dsppar.code = 1;		// ABR mode
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

int
testexist(char *n)
{
    char   *env;
    char    fname[14], p[128];
    int     c;
    struct PATH tmp;

    if (*n == '\0')
	return (0);
    if (_access(n, 0) == 0)
	return (1);
    strcpy(p, n);
    split_path(p, tmp.drive, tmp.dir, tmp.name, tmp.ext);
    if (strlen(tmp.drive) || strlen(tmp.dir)) 
        return (0);
    strcpy(fname, tmp.name);
    strcat(fname, tmp.ext);
    strcpy(p, exedir);
    strcat(p, fname);
    if (_access(p, 0) == 0) {
	strcpy(n, p);
	return (1);
    }
    env = getenv("PATH");
    if (env == NULL)
	return (0);

    c = 0;
    do {
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
usage()
{
    printf("usage:  abrav [-options] [filename ...]\n");
    printf("options:\n");
    printf("    -c      run calibration mode\n");
    printf("    -f F    process multiple ABR files from file F\n");
    printf("    -i F    use file F to initialize defaults\n");
    printf("    -l F    process multiple SWP files from file F\n");
    printf("    -m      monochrome graphics mode\n");
    printf("    -p      print files in batch mode\n");
    printf("    -P F    playback sweep file F\n");
    printf("    -r      record sweep using data file name\n");
    printf("    -R F    record sweep file F\n");
    printf("    -v      version\n");
    printf("    -x F    use file F for control mode\n");
    printf("If filenames are specified program begins in batch mode.\n");
    exit(0);
}

void
read_args(int ac, char *av[])
{
    struct PATH path;
    int     cnt;
    FILE   *fpt = NULL;

    (void) genfn("B", "TMP", tmpbatchfile, &cnt);
    split_path(av[0], path.drive, path.dir, path.name, path.ext);
    strcpy(exedir, path.drive);
    strcat(exedir, path.dir);
    while (ac > 1) {
	if (av[1][0] == '-') {
	    switch (av[1][1]) {
	    case 'b':
		batch_fmt = 1;
		break;
	    case 'c':
		calibr_flag = 1;
		break;
	    case 'f':
		if (ac > 2) {
		    ac--;
		    av++;
		    strcpy(batchfile, av[1]);
		}
                break;
	    case 'i':
		if (ac > 2) {
		    ac--;
		    av++;
		    strcpy(init_file, av[1]);
		}
		break;
	    case 'l':
		if (ac > 2) {
		    lstflg = ini_lst(av[2]);
		    ac = 0;
		}
		break;
	    case 'm':
		mono = 1;
		break;
	    case 'p':
		prnflg = 1;
		break;
	    case 'P':
		r_mode.at = 2;
		if (ac > 2) {
		    strcpy(record_file, av[2]);
		    ac = 0;
		}
		break;
	    case 'r':
		recflg = 1;
		r_mode.at = 1;
		break;
	    case 'R':
		r_mode.at = 1;
		if (ac > 2) {
		    strcpy(record_file, av[2]);
		    ac = 0;
		}
		break;
	    case 'v':
		print_version();
		exit(0);
		break;
	    default:
		usage();
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
    if (c == ' ' || c == '\t' || c == '=' || c == '\n' || c == '\r'
	|| c == ':')
	return (1);
    else
	return (0);
}

static int cnt = 0;
static int linelen;

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
    char   *keys[] =
    {
	"[SYSTEM]", "[PRINTER]", "[ABR]", "[TONE]", ""
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
	"Refresh", "GREG", "AD_type", "DSP_code", "DSP",
	"MP_channel", "EP_channel", "EP_start", ""
    };
    static char *pr_kws[] = {
    	"Port", "Type", "Orient", "Label", ""
    };
    static char *abr_kws[] = {
	"rate", "sweeps", "sets", "size", "stimulus", "limit", "Time1",
	"Time2", "Target", "FFTkHz", "FFTdB", "Tolerance", "chk_swps",
	"chk_atten", "F1", "F2", "MAX_level", "Xrange", "Yrange",
	"Wt_Av", "Reject_Mode", "Min_Sets", "VR1", "VR2", "repr1", "repr2",
	"Tone_Freq", "Tone_Level", "Run_mode", "Record_File", "AQM", "PQM",
	"Filter", "Nmp", "NHL_level", "EPA_HPF", "EPA_LPF", "EPA_AGN",
	"EPA_PGN", "EPA_ACC", "EPA_NTF", "PQM_Stop", "OK_Stop", 
	"Make_Template", "List", "cal_atten", "polarity", 
	"cal_type", "cal_swps", "chk_size", "check_signal", "EPA_FLAG", ""
    };
    static char *tone_kws[] = {
	"rate", "size", "frequency", "level", "duration", "ramp", ""
    };
    static char **kws[] = {sy_kws, pr_kws, abr_kws, tone_kws};
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
    case 2:			/* AD_sensitivity*/
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
    case 10:			/* DSP_code */
	strcpy(dsp_code_file, tokstr);
	break;
    case 11:			/* DSP_select */
	if (*tokstr == 'A' || *tokstr == 'D' || *tokstr == 'W')
	    num = 0;
	else if (*tokstr == 'T' || *tokstr == 'M')
            num = 1;
	else if (*tokstr == 'P')
            num = 2;
        ad_pol = (num == 2) ? -1 : 1;
	break;
    case 12:			/* microphone channel */
	c = toupper(tokstr[0]);
	if (c == 'A')
	    mic_chan = 0;
	else if (c == 'B')
	    mic_chan = 1;
	break;
    case 13:			/* EP channels */
        ep_chan_cfg = 0;
	if (strchr(tokstr, 'A') != NULL)
	    ep_chan_cfg |= 1;
	if (strchr(tokstr, 'B') != NULL)
	    ep_chan_cfg |= 2;
	if (ep_chan_cfg == 0)
	    ep_chan_cfg = 1 | 2;
	break;
    case 14:			/* EP start */
        ep_start = (float) num;
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
	get_token(1);
	set_prn_label(atline());
	break;
    }
}

void
set_abr(int c, double num)
{
    int     i = (int) num;

    switch (c) {
    case 1:			/* rate */
	if (num >= 100) {
	    abr_.rate = (int32_t) num;
	    abr_.rate = dspadjrt(abr_.rate);
	}
	break;
    case 2:			/* sweeps */
	if (i > 0)
	    abr_.swp1set = i;
	break;
    case 3:			/* sets */
	if (i > 0)
	    abr_.sets = i;
	break;
    case 4:			/* size */
	if (i > 0)
	    abr_.size = i;
	break;
    case 5:			/* stimulus */
	strcpy(abr_.stim_file, tokstr);
	break;
    case 6:			/* limit */
	if(num > 0)
	    rej_mv = (float) num;
	break;
    case 7:			/* Time1 */
        if (num > 0)
	    time1_ms = (float) num;
	break;
    case 8:			/* Time2 */
        if (num > 0)
	    time2_ms = (float) num;
	break;
    case 9:			/* Target */
	target_db = (float) num;
	break;
    case 10:			/* FFTkHz */
	if (i > 0)
	    abr_.fft_xrange = i;
	break;
    case 11:			/* FFTdB */
	if (i > 0)
	    abr_.fft_yrange = i;
	break;
    case 12:			/* tolerance */
	if (num > 0)
	    tolerance = (float) num;
	break;
    case 13:			/* checkfit sweeps */
	if (i > 0)
	    abr_.chk_swps = i;
	break;
    case 14:			/* chk_atten */
	if (num >= 0)
	    abr_.chk_atten = (float) num;
	break;
    case 15:			/* F1 cutoff */
	F1_cutoff = (float) num;
	break;
    case 16:			/* F2 cutoff */
	F2_cutoff = (float) num;
	break;
    case 17:			/* MAX_level */
	if (num > 0) 
	    max_level = (float) num;
	break;
    case 18:                    /* Xrange */
	if (num > 0)
	    def_xrange = (float) num;
	break;
    case 19:                    /* Yrange */
	if (num > 0)
	    def_yrange = (float) num;
	break;
    case 20:                    /* Wt_Av */
	if (*tokstr == '0' || *tokstr == 'N')
	    weight_type = 0;
	else if (*tokstr == '1' || *tokstr == 'R')
	    weight_type = 1;
	else if (*tokstr == '2' || *tokstr == 'V')
	    weight_type = 2;
	else if (*tokstr == '3' || tokstr[2] == 'V')
	    weight_type = 3;
	else if (*tokstr == '4' || tokstr[2] == 'R')
	    weight_type = 4;
        weight.at = weight_type;
	break;
    case 21:                    /* Reject_Mode */
	if (*tokstr == 'N')
	    rejmod.at = 0;
	else if (*tokstr == 'M')
	    rejmod.at = 1;
	else if (*tokstr == 'R')
	    rejmod.at = 2;
	break;
    case 22:                    /* Min_Sets */
	if (i > 0)
	    min_sweeps = i;
	break;
    case 23:                    /* VR1 */
	if (num > 0)
	    VR_crit1 = (float) num;
	break;
    case 24:                    /* VR2 */
	if (num > 0)
	    VR_crit2 = (float) num;
	break;
    case 25:                    /* repr1 */
	if (num > 0)
	    repr_crit1 = (float) (num / 100);
	break;
    case 26:                    /* repr2 */
	if (num > 0)
	    repr_crit2 = (float) (num / 100);
	break;
    case 27:
	if(num > 0) 
	    tone.freq = (float) num;	/* tone_freq in Hz */
	break;
    case 28:
	tone.level = (float) num;	/* tone_level in dBSPL */
	break;
    case 29:				/* run_mode */
	if(i >= 0 && i <= 2) {
	    r_mode.at = i;
	}
	break;
    case 30:				/* record_file */
	strcpy(record_file, tokstr);
	break;
    case 31:				/* AQM */
        for (i = 0; i < 5; i++) {
            if (_stricmp(tokstr, aqm.strs[i]) == 0)
                aqm.at = i;
        }
	break;
    case 32:				/* PQM */
        for (i = 0; i < 5; i++) {
            if (_stricmp(tokstr, pqm.strs[i]) == 0)
                pqm.at = i;
        }
	break;
    case 33:				/* Filter type */
	if (*tokstr == 'N') {
	    filter_type.at = 0;
	} else if (*tokstr == 'Z') {
	    filter_type.at = 1;
	}
	break;
    case 34:                            /* Nmp */
	if(i < MAXNMP && i > 0)
	    nmp = i;
	break;
    case 35:			/* NHL_level */
	if (num > 0) 
	    nhl_level = (float) num;
	break;
    case 36:			/* EPA_HPF */
        for (i = 0; i < 8; i++) 
        {
            if (_stricmp(tokstr, epa_hpf.strs[i]) == 0)
                epa_hpf.at = i;
        }
	break;
    case 37:			/* EPA_LPF */
        for (i = 0; i < 8; i++) 
        {
            if (_stricmp(tokstr, epa_lpf.strs[i]) == 0)
                epa_lpf.at = i;
        }
	break;
    case 38:			/* EPA_AGN */
        for (i = 0; i < 8; i++) 
        {
            if (_stricmp(tokstr, epa_gain.strs[i]) == 0)
                epa_gain.at = i;
        }
	break;
    case 39:			/* EPA_PGN */
        for (i = 0; i < 2; i++) {
            if (_stricmp(tokstr, epa_pgn.strs[i]) == 0)
                epa_pgn.at = i;
        }
	break;
    case 40:			/* EPA_ACC */
        for (i = 0; i < 2; i++) {
            if (_stricmp(tokstr, epa_acc.strs[i]) == 0)
                epa_acc.at = i;
        }
	break;
    case 41:			/* EPA_NTF */
        for (i = 0; i < 2; i++) {
            if (_stricmp(tokstr, epa_notch.strs[i]) == 0)
                epa_notch.at = i;
        }
	break;
    case 42:				/* PQM_stop */
	if(i >= 0 && i <= 2) {
	    pqm_mode.at = i;
	}
	break;
    case 43:				/* OK_stop */
	if(i >= 0 && i <= 2) {
	    ok_mode.at = i;
	}
	break;
    case 44:				/* Make_Template */
        make_template = (num > 0);
	break;
    case 45:                            /* List */
	strcpy(abr_.list_file, tokstr);
	break;
    case 46:                            /* cal_atten */
	if (num >= 0)
	    abr_.cal_atten = (float) num;
	break;
    case 47:                            /* polarity */
	if (*tokstr == 'N')
	    polar.at = 0;
	else if (*tokstr == 'R')
	    polar.at = 1;
	break;
    case 48:                            /* cal_type */
	if (num >= 0)
	    cal_type = i;
	break;
    case 49:				/* cal_swps */
	if (i > 0)
	    abr_.cal_swps = i;
	break;
    case 50:				/* chk_size */
	if (i > 0)
	    abr_.chk_size = i;
	break;
    case 51:				/* check_signal */
        check_signal = i;
	break;
    case 52:
        epa_flag = i;
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
    switch (c) {
    case 1:			/* rate */
	if (num > 0)
	    tone.rate = (int32_t) num;
	break;
    case 2:			/* buffer size */
	if (num > 0)
	    tone.size = (int) num;
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
//	if (num > 0)
//	    tone.ramp_ms = (float) num;
	break;
    }
}

void
read_init_file()
{
    FILE   *fptr;
    double  num;
    int     c, flag;

    if (!testexist(init_file))
	return;
    fptr = fopen(init_file, "rt");
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
	} else if (flag == 3) {	/* [ABR] */
	    set_abr(c, num);
	} else if (flag == 4) {	/* [TONE] */
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

    if (today_file(abravcounter)) {
        fpt = fopen(abravcounter, "rt");
	fscanf(fpt, "%d %d", &abr_count, &bin_count);
        fclose(fpt);
    }
    if (abr_count < 0)
	abr_count = 0;
    if (bin_count < 0)
	bin_count = 0;
}

void
write_counter_file()
{
    FILE   *fpt;

    fpt = fopen(abravcounter, "wt");
    if (fpt == NULL)
	return;
    fprintf(fpt, "%5d %5d", abr_count, bin_count);
    fclose(fpt);
}

void
strip_comment(char *s)
{
    char   *c;

    c = s;
    while (*c > 0 && *c != ';')
	c++;
    while (c > s && isspace(c[-1]))
	c--;
    *c = '\0';
}

/* display a short message at the top-right corner */
void
top_message(char *s)
{
    gr_settc(scrn_c[C_INFB], txtpar.cbg);
    gr_text(530, txtpar.font_height, s);
}

/* corner message */
//void
//corner(char *s)
//{
//    gr_settc(scrn_c[C_INFB], txtpar.cbg);
//    gr_text(560, txtpar.font_height, s);
//}

/* the batch mode processing, that is batch display or batch printing */
void
proc_batch(char *fn)
{
    FILE   *fpt, *efp;
    TOKENFILE *tfp;
    int     i;

    fpt = fopen(fn, "rt");
    if (fpt == NULL)
	return;
    efp = NULL;
    top_message("Batching");
    while (fgets(o_file_name, 128, fpt) != NULL) {
	i = strlen(o_file_name);
	if (i > 128) {
	    o_file_name[127] = 0;
	    i = 128;
	}
	i--;
	if (o_file_name[i] == '\n')
	    o_file_name[i] = 0;
	tfp = check_abr_file(o_file_name, 0);
	if (tfp == NULL) {
	    if (prnflg) {
		if (efp == NULL)
		    efp = fopen(log_file, "wt");
		if (efp != NULL)
		    fprintf(efp,
			"Open %s error!  Not a valid ABR file, or does not exist\n",
			o_file_name);
	    } else {
		(void) decide(0, 3, o_file_name, "Not a valid ABR file",
		    "or even not exist");
	    }
	    continue;
	} else {
	    tclose(tfp);
	    rd_abr_file();
	}
	if (batch_fmt)
	    batch_format();
	i = 0;
	if (prnflg) {
	    prn_screen();
	    if (check_event())
		i = getevent();
	} else
	    for (;;) {
		i = getevent();
		if (i == ' ' || i == 27 || i == 3)
		    break;
	    }
	if (i == 27)
	    break;
    }
    fclose(fpt);
    if (efp != NULL)
	fclose(efp);
}

/* handles daily calibration
*/
void
daily_calibration()
{
    int     mo, da, yr;
    char    *env;

    strcpy(patient.Lastname, "Calibration");
    strcpy(patient.Firstname, "Daily");
    get_date(&mo, &da, &yr);
    sprintf(patient.Dob, "%02d-%s-%02d", da, month_name[mo], yr);
    env = getenv("ID");
    if(env != NULL) {
        strncpy(patient.ID, env, 11);
    }
    info[5].valtype = NONE;
    /*
    info[5].exe = show_calibr_msg;
    putbackevent('C');
    (void) Info_wind();
    info[5].valtype = STRING;
    info[6].exe = NULL;
    patient.Comment[0] = 0; */
    (void) ABR_wind();
    /*
    if (patient.Comment[0]) 
    {
	if (differ_flag) 
	{
	    decide(0, 5, bar, "WARNING:",
		"Probe calibration is NOT within acceptable limits.",
		"Please check probe and repeat calibration.", bar);
	} else 
	{
	    decide(0, 3, bar, "Probe calibration is within acceptable limits", bar);
	}
    } else {
        decide(0, 4, bar, "WARNING:", "Calibration has been aborted.", bar);
    }
    */
}

/* handles batch processing
*/
void
batch()
{
    int     c;

    if(calibr_flag) {
	daily_calibration();
    } else {
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
}

/* Tests menu */
static int
Test_wind()
{
    (void) simple_submenu(menu_x(), menu_y(), testmenu);
    return(0);
}

/* set to run ABR test */
static int
set_abr_wind()
{
    calibr_flag = 0;
    top_message("ABR   ");
    set_trailer((void (*)(void)) ABR_wind);
    return(27);
}

/* set to run tone test */
static int
set_tone_wind()
{
    top_message("TONE  ");
    set_trailer(Tone_wind);
    return(27);
}

/* set to run tone test */
static int
set_probe_wind()
{
    calibr_flag = 1;
    top_message("CAVITY");
    set_trailer((void (*)(void)) daily_calibration);
    return(27);
}

static FILE *lfp = NULL;

int
ini_lst(char *fn)
{
    lfp = fopen(fn, "r");
    if (lfp == NULL)
        return (0);
    r_mode.at = 2;
    return (nxt_lst());
}

int
nxt_lst()
{
    int i;
 
    do {
        if (fgets(record_file, 128, lfp) == NULL) {
            fclose(lfp);
            return (0);
        }
        for (i = 0; i < 128 && record_file[i] > ' '; i++)
            continue;
        if (i < 128 && record_file[i] > '\0')
            record_file[i] = '\0';
    } while (i <= 0);
    return (1);
}

/* The main entry point */
int
MAIN_PGM(int argc, char **argv)
{
    set_title("ABRAV");
    set_prn_title("ABRAV");
    init_alloc_space();
    read_counter_file();
    read_args(argc, argv);
    dsppar.greg = 0;
    read_init_file();
    init_graphics();
    mouse_init();
    if(epa_flag && (_access("OPTIAMP.EXE", 0) != -1 ) )
        epamp_init(epa_gain.at, epa_hpf.at, epa_lpf.at, epa_notch.at);
    mainmenu_init(mainmenu);
    (void) dsprst(TRUE);

    while(!pgm_done()) 
    {
	clr_scr();
	text_color(menucolor.mfg, menucolor.mbg);
	init_wind();
	grab_abr();
	points_msg();
        (void) mainmenu_show(NULL);
        batch();
	mainmenu_driver();
    }
    (void) Quit_wind(0);

    return (0);
}
