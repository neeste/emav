/* savage.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <dsp.h>
#include <menu.h>
#include <token.h>
#include <util.h>
#include <fftlib.h>
#include "savage.h"
#include "color.h"
#include "screen.h"
#include "emav.h"		/* function prototyes */

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif /* WIN32 */

extern char dsp_code_file[MAXNAME];
extern float limit_mv, red_thr, chkfit_time;
extern float F1_cutoff, F2_cutoff, F5_cutoff;
extern int maxnpts, rand_seed;
extern short numsmo;
extern struct _dpoae dpoae;
extern struct _teoae teoae;
extern struct _probe probe;
extern struct _tone tone;
extern WIND w_text;

char    stmfn[FNAMESIZE] = {""};
char   *dpsigtyp[] = {"DP+", "DPnr1", "DPnr2", ""};
char   *dpfrqtyp[] = {
    "2*F1-F2", "3*F1-2*F2", "4*F1-3*F2", "2*F2-F1", "F2-F1", ""
};
double  target_voltage = 0.3162;    /* 70dB SPL (80dB SPL = 1v) */
int     buflen = 1024;
int     ntokens = 3, tcount = 0;
int     acc_sets = 128;
int     swp1set = 8;
int     invert = 0;
int32_t    rate = 50000;
struct _upload dsppar = {0};
ATTENUATION attenu = {80.0, 80.0, 10000.0};
TOKENFILE *t = (TOKENFILE *) NULL;

TOGGLETYPE dpnr_mode = {
    0,
    {"DP+", "DPnr1", "DPnr2", NULL},
};
TOGGLETYPE dpftype = {
    0,
    { "2*F1-F2", "3*F1-2*F2", "4*F1-3*F2", "2*F2-F1", "F2-F1", NULL },
};
TOGGLETYPE f5type = {
    1,
    { "FIXED", "AUTO", "OFF", NULL },
};
TOGGLETYPE dpsptype = {
    0,
    { "FIXED", "AUTO", NULL },
};
TOGGLETYPE distype = {
    1,
    { "V", "Pa", NULL },
};
TOGGLETYPE xtype = {
    0,
    {	"F2 frequency", "F1 frequency","Fd frequency", 
	"L2 level", "L1 level", "L3 level", 
	"Trial sweep", NULL
    }
};
TOGGLETYPE datafmt = {
    0,
    { "Normal", "High Order", "Extended", "Multi", "SFOAE", NULL },
};
TOGGLETYPE dp_octave = {
    0,
    { "0", "1", "2", NULL },
};
TOGGLETYPE Sbin = {
    0,
    { "No", "Yes", NULL },
};
TOGGLETYPE Suppr = {
    0,
    { "None", "Tone", "BPN", "XBPN", NULL },
};
TOGGLETYPE F1_adj = {
    0,
    { "No", "Yes", NULL },
};
TOGGLETYPE Stim_mode = {
    3,
    { "+1", "+1 -1", "+1 +1 -2", "+1 +1 +1 -3", "L R B", "L R L R B*2", NULL },
};
TOGGLETYPE stim_unit = {
    0,
    { "SPL", "SL", "V", "FPL", "SIL", NULL },
};
TOGGLETYPE r_mode = {
    0,
    { "REAL TIME", "RECORD", "PLAY BACK", NULL },
};
TOGGLETYPE thv_xtk = {
    1,
    { "No", "Yes", NULL },
};
TOGGLETYPE thv_itr = {
    1,
    { "No", "Yes", "Maybe", NULL },
};
TOGGLETYPE thv_ext = {
    0,
    { "Length only", "Length+T+D", "Length+R", NULL },
};
TOGGLETYPE dec_len = {
    0,
    { "No", "Yes", NULL },
};
TOGGLETYPE z_cav_type = {
    0,
    { "0", "1", NULL },
};

static int check_th_off(void);
static int chk_rate_tone(void);
static int sys_menu(char *);
static int sen_menu(char *);
static int dpoae_opt(char *);
static int teoae_opt(char *);
static int tone_opt(char *);
static int teoae_opt(char *);
static int dpoae_opt(char *);
static int display_opt(char *);
static int spectrum_opt(char *);
static int data_opt(char *);
static int dp_stim_opt(char *);
static int hpf_opt(char *);
static int noise_opt(char *);
static int pb_opt(char*);
static int p_c_opt(char*);
static int p_t_opt(char*);

static  int text_wind_flag = 0;
static  int upperv = 0;
static float mvmax = 0;

static MENUITEM option[] = {
    {"&System...", NULL, NONE, 0, 0, 1, sys_menu},
    {"&DPOAE...", NULL, NONE, 0, 0, 1, dpoae_opt},
    {"&TEOAE...", NULL, NONE, 0, 0, 1, teoae_opt},
    {"&PROBE...", NULL, NONE, 0, 0, 1, pb_opt},
    {"TON&E...", NULL, NONE, 0, 0, 1, tone_opt},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

static MENUITEM sys_opt[] = {
    {"Display &Units (V or Pa) =", (char *) &distype, TOGGLE, 4, 0, 1, NULL},
    {"I/O &Sensitivity ...", NULL, NONE, 0, 0, 1, sen_menu},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

static MENUITEM sen_opt[] = {
    {"&AD Sensitivity (cnt/V) :", (char *) &Sys.AD, FLOAT, 8, 0, 1, NULL},
    {"DA&1 Sensitivity (cnt/V) :", (char *) &Sys.DA[0], FLOAT, 6, 0, 1, NULL},
    {"DA&2 Sensitivity (cnt/V) :", (char *) &Sys.DA[1], FLOAT, 6, 0, 1, NULL},
    {"MP &Transfer :", Sys.MPfn, STRING, 32, 0, 1, NULL},
    {"&MP Sensitivity (V/Pa)  :", (char *) &Sys.MP, FLOAT, 8, 2, 1, NULL},
    {"&LS1 Sensitivity (V/Pa) :", (char *) &Sys.LS[0], FLOAT, 6, 3, 1, NULL},
    {"L&S2 Sensitivity (V/Pa) :", (char *) &Sys.LS[1], FLOAT, 6, 3, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

static MENUITEM d_option[] = {
    {"&Buffer Size :", (char *) &dpoae.size, SHORT, 6, 0, 1, chk_size},
    {"&Rate of Clock (Hz) :", (char *) &dpoae.rate, LONG, 6, 0, 1, chk_long},
    {"S&weeps per Set :", (char *) &dpoae.swp1set, SHORT, 6, 0, 1, chk_short},
    {"DP &Frequency =", (char *) &dpftype, TOGGLE, 0, 0, 1, NULL},
    {"&Input Mode =", (char *) &r_mode, TOGGLE, 0, 0, 1, NULL},
    {"&Stimuli...", NULL, NONE, 0, 0, 1, dp_stim_opt},
    {"&Display...", NULL, NONE, 0, 0, 1, display_opt},
    {"S&pectrum...", NULL, NONE, 0, 0, 1, spectrum_opt},
    {"D&ata File...", NULL, NONE, 0, 0, 1, data_opt},
    {"&High-Pass Filter...", NULL, NONE, 0, 0, 1, hpf_opt},
    {"&Noise...", NULL, NONE, 0, 0, 1, noise_opt},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

static MENUITEM d1_option[] = {
    {"M&aximum Ordinate Level (dB) :", (char *) &dpoae.ord_max, SHORT, 6, 0, 1, NULL},
    {"M&inimum Ordinate Level (dB) :", (char *) &dpoae.ord_min, SHORT, 6, 0, 1, NULL},
    {"Ma&ximum Abscissa Level (dB) :", (char *) &dpoae.abs_max, SHORT, 6, 0, 1, NULL},
    {"Mi&nimum Abscissa Level (dB) :", (char *) &dpoae.abs_min, SHORT, 6, 0, 1, NULL},
    {"&Maximum Frequency (oct) :", (char *) &dpoae.oct_max, SHORT, 6, 0, 1, NULL},
    {"Minimum &Frequency (oct) :", (char *) &dpoae.oct_min, SHORT, 6, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

static MENUITEM d2_option[] = {
    {"Spec. Freq. Range &Type =", (char *) &dpsptype, TOGGLE, 0, 0, 1, NULL},
    {"Spec. &Frequency Range (kHz) :", (char *) &dpoae.fft_xrange, SHORT, 6, 0, 1, NULL},
    {"Spec. &Level Range (dB) :", (char *) &dpoae.fft_yrange, SHORT, 6, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

static MENUITEM d3_option[] = {
    {"Data &Format Type =", (char *) &datafmt, TOGGLE, 6, 0, 1, NULL},
    {"Save Response &Binary =", (char *) &Sbin, TOGGLE, 6, 0, 1, NULL},
    {"&DP Octave =", (char *) &dp_octave, TOGGLE, 6, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

static MENUITEM d4_option[] = {
    {"HPF &Frequency (Hz) :", (char *) &F5_cutoff, FLOAT, 10, 1, 1, NULL},
    {"HPF &Type =", (char *) &f5type, TOGGLE, 0, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

static MENUITEM d5_option[] = {
    {"&Calibration Stimulus :", dpoae.calibrate, STRING, 42, 0, 1, NULL},
    {"Calibration &Attenuation :", (char *) &dpoae.cal_atten, FLOAT, 5, 0, 1, NULL},
    {"&Suppressor =", (char *) &Suppr, TOGGLE, 6, 0, 1, NULL},
    {"A&djust F1 =", (char *) &F1_adj, TOGGLE, 6, 0, 1, NULL},
    {"&Ramp Time (ms) :", (char *) &dpoae.ramp_ms, FLOAT, 6, 1, 1, chk_ramp_dpoae},
    {"CheckFit &Time (sec) :", (char *) &chkfit_time, FLOAT, 4, 0, 1, NULL},
    {"&Protocol File :", dpoae.file, STRING, 42, 0, 1, NULL},
    {"Level &Unit =", (char *) &stim_unit, TOGGLE, 6, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

static MENUITEM d6_option[] = {
    {"N&NSB :", (char *) &dpoae.nnsb, SHORT, 6, 0, 1, NULL},
    {"N&FSB :", (char *) &dpoae.nfsb, SHORT, 6, 0, 1, NULL},
    {"NF &separation (dB) :", (char *) &dpoae.nfdb, FLOAT, 6, 0, 1, NULL},
    {"Reduction &Mode =", (char *) &dpnr_mode, TOGGLE, 6, 0, 1, NULL},
    {"Reduction &Threshold :", (char *) &red_thr, FLOAT, 4, 2, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

MENUITEM p_option[] = 
{
    {"&Buffer Size :", (char *) &dpoae.size, SHORT, 6, 0, 1, chk_size},
    {"&Rate of Clock (Hz) :", (char *) &dpoae.rate, LONG, 6, 0, 1, chk_long},
    {"Spectrum Frequency Range (&kHz) :", (char *) &probe.fft_xrange, SHORT, 4, 0, 1, NULL},
    {"Spectrum Level Range (&dB) :", (char *) &probe.fft_yrange, SHORT, 6, 0, 1, NULL},
    {"&Calibration...", NULL, NONE, 0, 0, 1, p_c_opt},
    {"&Thevenin Computation...", NULL, NONE, 0, 0, 1, p_t_opt},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM p_c_option[] = 
{
    {"&Sweeps per Set :", (char *) &dpoae.cal_swps, SHORT, 10, 0, 1, chk_int},
    {"&Number of Sweep Sets :", (char *) &dpoae.sets, SHORT, 10, 0, 1, NULL},
    {"Stimulus &File :", dpoae.calibrate, STRING, 42, 0, 1, NULL},
    {"&Attenuation :", (char *) &dpoae.cal_atten, FLOAT, 5, 0, 1, NULL},
    {"Number of &Cavities to Test :", (char*)&probe.ncav, SHORT, 4, 0, 1, chk_int},	
    {"Number of Sou&rces to Test :", (char*)&probe.nsrc, SHORT, 4, 0, 1, chk_int},	
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM p_t_option[] = 
{
    {"&Compute Crosstalk =", (char *) &thv_xtk, TOGGLE, 5, 0, 1, NULL},
    {"&Iterate Length =", (char *) &thv_itr, TOGGLE, 0, 0, 1, NULL},
    {"&Also Iterate :", (char *) &thv_ext, TOGGLE, 0, 0, 1, NULL},
    {"&Smooth :", (char *) &numsmo, SHORT, 5, 0, 1, NULL},
    {"f&1_erf (Hz) :", (char *) &probe.f1_erf, FLOAT, 6, 0, 1, NULL},
    {"f&2_erf (Hz) :", (char *) &probe.f2_erf, FLOAT, 6, 0, 1, NULL},
    {"Te&mperature (deg C) :", (char *) &probe.temp, FLOAT, 5, 1, 1, NULL},
    {"&Diameter (cm) :", (char *) &probe.diacav, FLOAT, 5, 1, 1, NULL},
    {"Min.&Res. (ohm) :", (char *) &probe.minres, FLOAT, 5, 1, 1, NULL},
    {"&Z_cav =", (char *) &z_cav_type, TOGGLE, 0, 0, 1, NULL},
    {"&Length decrease =", (char *) &dec_len, TOGGLE, 0, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

static MENUITEM t_option[] = {
    {"&Buffer Size :", (char *) &teoae.size, SHORT, 10, 0, 1, chk_size},
    {"&Rate of Clock (Hz) :", (char *) &teoae.rate, LONG, 10, 0, 1, check_t1_t2},
    {"S&weeps per Set :", (char *) &teoae.swp1set, SHORT, 10, 0, 1, chk_short},
    {"&Number of Sweep Sets :", (char *) &teoae.sets, SHORT, 10, 0, 1, NULL},
    {"Stimulus &Presentation Mode =", (char *) &Stim_mode, TOGGLE, 9, 0, 1, NULL},
    {"Stimulus &File :", teoae.stim_file, STRING, 42, 0, 1, NULL},
    {"Check&Fit Stimulus :", teoae.checkfit, STRING, 42, 0, 1, NULL},
    {"&Artifact Rejection and Response Filter...", NULL, NONE, 0, 0, 1, set_rej},
    {"Spectrum Frequency Range (kH&z) :", (char *) &teoae.fft_xrange, SHORT, 6, 0, 1, NULL},
    {"Spectrum Level Range (d&B) :", (char *) &teoae.fft_yrange, SHORT, 6, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

static MENUITEM tone_option[] = {
    {"&Buffer Size :", (char *) &tone.size, SHORT, 10, 0, 1, chk_size},
    {"&Rate of Clock (Hz) :", (char *) &tone.rate, LONG, 10, 0, 1, chk_rate_tone},
    {"Ramp &Time (ms) :", (char *) &tone.ramp_ms, FLOAT, 6, 1, 1, chk_ramp_tone},
    {"&Skipped Sweeps :", (char *) &tone.skips, SHORT, 3, 0, 1, NULL},
    {"Spectrum &Frequency Range (kHz) :", (char *) &tone.fft_xrange, SHORT, 4, 0, 1, NULL},
    {"Spectrum &Level Range (dB) :", (char *) &tone.fft_yrange, SHORT, 6, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM rej_menu[] = {
    {"&Hi Pass Filter Frequency (Hz) :", (char *) &F1_cutoff, FLOAT, 10, 3, 1, NULL},
    {"&Lo Pass Filter Frequency (Hz) :", (char *) &F2_cutoff, FLOAT, 10, 3, 1, NULL},
    {"Rejection &Threshold (mPa) :", (char *) &mvmax, FLOAT, 10, 3, 1, check_th_off},
    {"&Start time (ms) :", (char *) &mstime1, FLOAT, 10, 3, 1, check_t1_t2},
    {"&End time (ms) :", (char *) &mstime2, FLOAT, 10, 3, 1, check_t1_t2},
    {"&Ramp time (ms) :", (char *) &msramp, FLOAT, 10, 3, 1, check_t1_t2},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

int
open_text_w(int *i)
{
    text_wind_flag = save_w(&w_text);
    *i = draw_w(&w_text);
    return (w_text.ytop + txtpar.font_height);
}

void
close_text_w(void)
{
    if (text_wind_flag)
	close_w(&w_text);
    text_wind_flag = 0;
}

int
Option_wind(void)
{
    return (simple_submenu(menu_x(), txtpar.menu_height, option));
}

static int
sys_menu(char *no_use)
{
    int i;

    i = simple_submenu(menu_x(), menu_y(), sys_opt);
    return (i);
}

static int
sen_menu(char *no_use)
{
    int i;

    i = simple_submenu(menu_x(), menu_y(), sen_opt);
    return (i);
}

static int
dpoae_opt(char *no_use)
{
    int     ret;

    grab_dpoae();
    ret = simple_submenu(menu_x(), menu_y(), d_option);
    dpoae.level_unit = (short) stim_unit.at;
    return (ret);
}

static int
display_opt(char *no_use)
{
    int     ret;

    ret = simple_submenu(menu_x(), menu_y(), d1_option);
    return (ret);
}

static int
spectrum_opt(char *no_use)
{
    int     ret;

    ret = simple_submenu(menu_x(), menu_y(), d2_option);
    return (ret);
}

static int
data_opt(char *no_use)
{
    int     ret;

    ret = simple_submenu(menu_x(), menu_y(), d3_option);
    return (ret);
}

static int
hpf_opt(char *no_use)
{
    int     ret;

    ret = simple_submenu(menu_x(), menu_y(), d4_option);
    return (ret);
}

static int
dp_stim_opt(char *no_use)
{
    int     ret;

    ret = simple_submenu(menu_x(), menu_y(), d5_option);
    return (ret);
}

static int
noise_opt(char *no_use)
{
    int     ret;

    ret = simple_submenu(menu_x(), menu_y(), d6_option);
    return (ret);
}

static int
teoae_opt(char *no_use)
{
    int     ret;

    grab_teoae();
    ret = simple_submenu(menu_x(), menu_y(), t_option);
    grab_teoae();
    return (ret);
}

int 
p_c_opt(char* no_use) {
    int ret;

    ret = simple_submenu(menu_x(), menu_y(), p_c_option);
    grab_probe();
    return (ret);
}

int 
p_t_opt(char* no_use) {
    int ret;

    ret = simple_submenu(menu_x(), menu_y(), p_t_option);
    grab_probe();
    return (ret);
}

int 
pb_opt(char* no_use) {
    int ret;

    ret = simple_submenu(menu_x(), menu_y(), p_option);
    grab_probe();
    return (ret);
}

static int
tone_opt(char *no_use)
{
    int     ret;

    grab_tone();
    ret = simple_submenu(menu_x(), menu_y(), tone_option);
    grab_tone();
    return (ret);
}

int
chk_long(int32_t **r)
{
    if (**r < 1)
	**r = 1;
    return(0);
}

int
chk_int(int **i)
{
    if (**i < 1)
	**i = 1;
    return (0);
}

int
chk_short(short **i)
{
    if (**i < 1)
	**i = 1;
    return (0);
}

int
chk_size(short **n)
{
    if (**n < 128)
	**n = 128;
    if (**n > maxnpts)
	**n = maxnpts;
    return (0);
}

int
dec_att(float **f)
{
    if (attenu.att1 < 0)
	attenu.att1 = 0;
    else if (attenu.att1 > 100)
	attenu.att1 = 100;
    attenu.intern_att = (float) pow(10, attenu.att1 / 20);
    attenu.att2 = attenu.att1;
    return (0);
}

void
dsp_vfs(void)
{
    float ad_vfs[2], da_vfs[2];

    dspvfs(ad_vfs, da_vfs);
    if ((Sys.AD == 0) && ad_vfs[0] != 0) {
	Sys.AD = (float)((pow(2, 23) - 1) / fabs(ad_vfs[0]));
    }
    if ((Sys.DA[0] == 0) && da_vfs[0] != 0) {
        Sys.DA[0] = (float)((pow(2, 15) - 1) / fabs(da_vfs[0]));
    }
    if ((Sys.DA[1] == 0) && da_vfs[1] != 0) {
        Sys.DA[1] = (float)((pow(2, 15) - 1) / fabs(da_vfs[1]));
    }
    Sen = Sys;
}

int
dsp_init(void)
{
    int     err;

    err = dspseta(dsppar.code);			// Set base address
    if(!err) {
	err = dspinit(dsp_code_file);	// Upload DSP code file
    } else {
	decide(0, 1, "Can't initialize DSP.");
    }
    if (!err) {
	dsp_vfs();
    }
    return (err);
}

/*
 * tok_init - Initialize tokenfile structure
 */
void
tok_init(int ntok, int mode)
{
    t->header.power10 = 1;
    t->header.isf = (int) (rate / 10);
    t->hitch.tokens = ntok;
    t->hitch.curtok = tcount = 0;
    t->hitch.toksiz = buflen;
    memcpy(t->hitch.comment, &patient, sizeof(PATIENT));
    t->hitch.target_level = (float) (target_voltage / Sen.MP);
    t->hitch.mode = mode;
    tinit(t);
    write_next_car();
}

/* rescale buffer from long to short */
static int32_t
rescale(int32_t *lbuf, short *sbuf, int n, int maxfs)
{
    double  scale;
    int32_t    maxval = 0L, absval;
    int     i;

    for (i = 0; i < n; i++) {
	absval = (int32_t) labs(lbuf[i]);
	if (absval > maxval)
	    maxval = absval;
    }
    if (maxval <= maxfs || maxval <= 0) {
	for (i = 0; i < n; i++)
	    sbuf[i] = (short) lbuf[i];
    } else {
	if(maxval == 0L) scale = 1.0;
	else scale = ((double) maxfs) / ((double) maxval);
	for (i = 0; i < n; i++)
	    sbuf[i] = nint(lbuf[i] * scale);
    }
    return (maxval);
}

/* find the absolute maximum of infbuf[0, ... , n-1] */
static int
findmax(short *sbuf, int n)
{
    int     i;
    int     maxval = 0, absval;

    for (i = 0; i < n; i++) {
	absval = abs(sbuf[i]);
	if (absval > maxval)
	    maxval = absval;
    }
    return (maxval);
}

/*
 * store the token
 */
void
tok_store(int c, int32_t *p)
{
    float   scale;
    int     tmax;
    int32_t    amax;
    short  *sbuf;

    if (c == 0)
	return;
    sbuf = (short *) calloc((size_t)(buflen), sizeof(short));
    amax = rescale(p, sbuf, buflen, MAXPOSINT);
    tmax = findmax(sbuf, buflen);
    c *= swp1set;
    scale = (tmax <= 0) ? 1 : ((float) amax / tmax) / (c * Sen.AD * Sen.MP);
    upperv = (int) (limit_mv * 0.001 * Sen.AD * Sen.MP);
    t->car.dccomp = 0;
    t->car.maxval = amax;
    t->car.tokmax = tmax;
    tcount++;
    strncpy(t->car.tokcmt, patient.Comment, 81);
    strncpy(t->car.sname, stmfn, FNAMESIZE);
    t->car.reps = c;
    t->car.atten1 = (int) (attenu.att1 * 100.);
    t->car.atten2 = (int) (attenu.att2 * 100.);
    t->car.swprej = 0;
    t->car.toktime = (int32_t) time(NULL);
    t->car.rejt1 = itime1;
    t->car.rejt2 = itime2;
    t->car.rejlo = -upperv;
    t->car.rejhi = upperv;
    t->car.scale = scale;
    t->car.adsen = Sen.AD;
    t->car.mpsen = Sen.MP;

    twrite(sbuf, tcount, t);
    free(sbuf);
}

/* generate sweep stimulus */

static void
gen_sweep(short *s, int n1, double f1, double f2)
{
    double am, f3, ff, df, dt, tp, dp, ph, sc;
    int i, n2;
    static double sl = 16;

    n2 = buflen;
    f3 = (f2 > 0 && (f2 * 2) < (rate * 0.5)) ? (f2 * 2) : (rate * 0.5);
    df = f3 / (n1 - 1);
    dt = 1.0 / rate;
    tp = 2 * M_PI;
    am = MAXPOSINT;
    ph = 0;
    for (i = 0; i < n1; i++) {
	ff = i * df;
	sc = 1;
	if (f1 > 0 && ff > 0)
	    sc /= 1 + pow(f1 / ff, sl);
	else if (ff == 0)
	    sc = 0;
	if (f2 > 0)
	    sc /= 1 + pow(ff / f2, sl);
	dp = tp * ff * dt;
	ph += dp;
	s[i] = nint(sc * am * sin(ph));
    }
    for (i = n1; i < n2; i++) {
	s[i] = 0;
    }
}

/* generate broad-band-noise stimulus */

static void
gen_noise(short *s, int n)
{
    double am, tpi, ph, v, sm, ss, sf;
    float *x;
    int i;
    static double cf = 4;   // crest factor

    srand(rand_seed);
    x = (float *) calloc(n, 2 * sizeof(float));
    x[0] = x[n] = 1;
    x[1] = x[n + 1] = 0;
    tpi = 2 * M_PI;
    for (i = 1; i < n / 2; i++) {
        ph = tpi * rand() / RAND_MAX;
        x[2 * i] = (float) cos(ph);
        x[2 * i + 1] = (float) sin(ph);
    }
    fsst(x, n);
    sm = ss = 0;
    for (i = 0; i < n; i++) {
	v = x[i];
        sm += v;
	ss += v * v;
    }
    // amplitude = rms * crest_factor
    am = MAXPOSINT;
    sf = (am / cf) / sqrt((ss - sm * sm / n) / n);
    for (i = 0; i < n; i++) {
        s[i] = nint(limit(-am, x[i] * sf, am));
    }
    free(x);
}

/* generate default stimulus */
static int
gen_def_stim(char *name, short *sbuf)
{
    int i, n;

    n = buflen;
    if (strlen(name) == 0) {
	for (i = 0; i < n; i++) {
	    sbuf[i] = 0;
	}
    } else if (strcmp(name, "~ssw") == 0) {
        n = nint(0.001 * rate);
        if (n) {
	    gen_sweep(sbuf, n, 125, 5657);
	}
    } else if (strcmp(name, "~lsw") == 0) {
        gen_sweep(sbuf, n, 0, 0);
    } else if (strcmp(name, "~bbn") == 0) {
        gen_noise(sbuf, n);
    }
    return (n);
}

/*
 * getstim - Read ils stimulus file
 */
int
getstim(char *stimfn, int np)
{
    int     n, i, valid = 0, k;
    static int32_t rrate, npts;
    static char last_stim[42] = {42 * '\0'};
    static int last_nvrt = 0;

    if (!strcmp(stimfn, last_stim) && (np == npts) && (invert == last_nvrt))
	return (0);
    if (gen_def_stim(stimfn, stmbuf)) {
	npts = buflen;
	rrate = rate;
	valid = 1;
    }
    while (!valid && strlen(stimfn) > 0 && !pgm_done()) {
    	if (!testexist(stimfn)) {
    	    valid = 0;
	} else {
	    valid = (rdils(stimfn, stmbuf, (int32_t) buflen, &rrate, &npts) > 0);
	}
	if (!valid) {
	    gr_beep();
	    k = open_text_w(&i);
	    gprintf(i, k, "stimulus file '%s' not found or open error.", stimfn);
	    n = gprintf(i, k + txtpar.font_height,
		"Please retype stimulus file name: ");
	    txtpar.text_wind_len -= n;
	    get_str(i + 8 * n, k + txtpar.font_height, 42, 0, STRING, stimfn);
	    close_text_w();
	}
    }
    if (!valid) {
	gr_beep();
	return (0);
    }
    if (rrate != rate) {
	if (!decide(1, 2, "Stimulus uses a different sampling rate",
	    "Do you still wish to use this stimulus?"))
	    return (0);
    }
    if (np < npts) {
	if (!decide(1, 2, "The stimulus is longer than the token.",
	    "Do you want to truncate the stimulus?"))
	    return (0);
    } else if (np > npts) {
	if (!decide(1, 2, "The stimulus is shorter than the token.",
	    "Do you want to extend the stimulus with zeros?"))
	    return (0);
	for (i = npts; i < np; i++)
	    stmbuf[i] = 0;
    }
    if (invert) {		/* invert the stimulus */
	for (i = 0; i < npts; i++)
	    stmbuf[i] = -stmbuf[i];
    }
    last_nvrt = invert;
    set_intern_att(1.0);
    strcpy(last_stim, stimfn);
    return (np);
}

void
read_next_car(void)
{
    if (tcount < ntokens) {
	trdcar(tcount + 1, t);
	strncpy(stmfn, t->car.sname, FNAMESIZE);
	acc_sets = t->car.reps;
	attenu.att1 = (float) (t->car.atten1 / 100.0);
	attenu.att2 = (float) (t->car.atten2 / 100.0);
	itime1 = t->car.rejt1;
	itime2 = t->car.rejt2;
	upperv = t->car.rejhi;
        limit_mv = (float) (upperv * 1000.0 / (Sen.AD * Sen.MP));
    }
}

void
write_next_car(void)
{
    if (tcount < ntokens) {
	strncpy(t->car.tokcmt, "", COMNTSIZE);
	strncpy(t->car.sname, stmfn, FNAMESIZE);
	t->car.reps = acc_sets;
	t->car.atten1 = (int) (attenu.att1 * 100.);
	t->car.atten2 = (int) (attenu.att2 * 100.);
	t->car.rejt1 = itime1;
	t->car.rejt2 = itime2;
	t->car.rejlo = -upperv;
	t->car.rejhi = upperv;
	t->car.swprej = 0;
	t->car.toktime = 0;
	twrcar(tcount + 1, t);
    }
}

static int
check_th_off(void)
{
    teoae.mvmax = mvmax;
    return (0);
}

int
check_t1_t2(void)
{
    return (0);
}

int
set_rej(char *no_use)
{
    mvmax = teoae.mvmax;

    simple_submenu(menu_x(), menu_y(), rej_menu);
    rate = teoae.rate;
    return (0);
}

static int
chk_rate_tone(void)
{
    tone.rate = adjust_rate(tone.rate);
    return (0);
}

int
chk_ramp_tone(void)
{
    int r;

    r = nint(tone.ramp_ms * (float) tone.rate / 1000.0);
    tone.ramp = limit(16, r, tone.size);
    tone.ramp_ms = (float) (tone.ramp * 1000.0 / tone.rate);

    return(0);
}

int
chk_ramp_dpoae(void)
{
    int r;

    r = nint(dpoae.ramp_ms * (float) dpoae.rate / 1000.0);
    dpoae.ramp = limit(16, r, dpoae.size);
    dpoae.ramp_ms = (float) (dpoae.ramp * 1000.0 / dpoae.rate);

    return(0);
}

void
set_test_opt(int t1, int t2, int t3)
{
    option[1].status = t1;	/* set DPOAE opts */
    option[2].status = t2;	/* set TEOAE opts */
    option[3].status = t3;	/* set TONE opts */
}

/* return index of selected distortion production frequency */
int
dpindx(void)
{
    int idp;

    if (datafmt.at == 0)
	idp = 0;
    else if (datafmt.at < 4)
	idp = dpftype.at;
    else
	idp = 0;

    return (idp);
}

/* compute the distortion production frequency */
double
dpfreq(double f1, double f2)
{
    int i;
    static float c1[] = {2, 3, 4, -1, -1};
    static float c2[] = {-1, -2, -3, 2, 1};

    i = dpindx();

    return (c1[i] * f1 + c2[i] * f2);
}

/* input_filter - configure high-pass filter on input */

void
input_filter(int filter_type, int o, float *f, int32_t sample_rate)
{
    float *a[2], *b[2], w;
    int i, nfo[2];
    static int pb = 1;  // pass_band: 0=low-pass, 1=high-pass

    // compute HPF coefficients
    for (i = 0; i < 2; i++) {
        a[i] = (float *) calloc((size_t)(o + 1), sizeof(float));
        b[i] = (float *) calloc((size_t)(o + 1), sizeof(float));
        if (f && (f[i] > 0) && (sample_rate > 0) && (o > 0)) {
            w = f[i] / sample_rate;        // normalized frequency
            if (filter_type == 0) {           // Butterworth
	        nfo[i] = butter(&w, b[i], a[i], o, pb);
            } else if (filter_type == 1) {    // Bessel
                nfo[i] = bessel(&w, b[i], a[i], o, pb);
            } else {                //default
                nfo[i] = 0;
            }
        } else {                    //default
            nfo[i] = 0;
        }
    }
    dspinpflt(a, b, nfo);
    for (i = 0; i < 2; i++) {
        free(a[i]);
        free(b[i]);
    }
}
