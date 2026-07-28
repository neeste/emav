/* savage.c */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include <dsp.h>
#include <menu.h>
#include <util.h>
#include "fftlib.h"
#include "savage.h"
#include "color.h"
#include "screen.h"
#include "putt.h"

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif /* WIN32 */

int 	pb_opt();
int 	p_c_opt();
int 	p_t_opt();
int     average();
int     do_meer_sel();
int     do_ml_sel();
int     naverage();
int     set_hear_wind();
int     storestim();
int     testexist(char *);
void    calibrate(int);
void    car_defaults();
void    clr_txt();
void    disadbuf();
void    get_data();
void    get_info();
void    grab_probe();

extern int rand_seed;

struct _upload dsppar;
int     buflen = 1024;
int32_t    rate = 32000;
int     nsets = 128;
int     upperv = MAXPOSINT / 1000;
int     swp1set = 8;

int     invert = 0;

TOGGLETYPE levref = {
    0,
    { "SPL", "HL", "SIL", "FPL", NULL },
};

TOGGLETYPE cond_thl = {
    1,
    { "TTH", "THL", NULL },
};

static TOGGLETYPE devtype = {
    1,
    { "EXTERNAL", "INTERNAL", NULL },
};

TOGGLETYPE f5type = {
    1,
    { "FIXED", "AUTO", "OFF", NULL },
};

TOGGLETYPE distype = {
    1,
    { "mV", "mPa", NULL },
};

TOGGLETYPE thresh_algo = {
    0,
    { "ML", "MEER", "UPDN", "ANSI", NULL },
};

TOGGLETYPE thresh_task = {
    0,
    { "Y/N", "2AFC", NULL },
};

TOGGLETYPE thresh_rand = {
    1,
    { "No", "Yes", NULL },
};

TOGGLETYPE thresh_cnct = {
    1,
    { "No", "Yes", NULL },
};

TOGGLETYPE thresh_hf = {
    1,
    { "No", "Yes", NULL },
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
    { "None", "Temp+Diam", "Src.Rfl.", NULL },
};

TOGGLETYPE thv_dbg = {
    0,
    { "Off", "On", NULL },
};

TOGGLETYPE thv_chk = {
    1,
    { "No", "Yes", NULL },
};

TOGGLETYPE sim_obs = {
    0,
    { "No", "Yes", NULL },
};

extern char dsp_code_file[];

int	chk_int(int **);
int	chk_neg( float **);
int     chk_rate(int32_t *);
int	chk_rate_tone(int32_t *);
int     dsp_force_init(char *);
int     sys_menu(char *);
int     sys_opt_1(char *);
int     ton_opt(char *);
int     hr_opt(char *);
int     hr_opt_1(char *);
int     hr_opt_2(char *);
int     hr_opt_3(char *);
int     hr_opt_4(char *);
int     hr_opt_5(char *);
int     hr_opt_6(char *);

MENUITEM option[] = {
    {"&System Options...", NULL, NONE, 0, 0, 1, sys_menu},
    {"&Hearing Options...", NULL, NONE, 0, 0, 1, hr_opt},
    {"&Probe Options...", NULL, NONE, 0, 0, 1, pb_opt},
    {"&Tone Options...", NULL, NONE, 0, 0, 1, ton_opt},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

MENUITEM h_option_1[] = {
    {"Frequency &Begin (Hz) :", (char *) &hear.f_start, FLOAT, 10, 3, 1, chk_neg},
    {"Frequency &End (Hz) :", (char *) &hear.f_end, FLOAT, 10, 3, 1, chk_neg},
    {"Points per &Octave :", (char *) &hear.pts_oct, SHORT, 4, 0, 1, chk_int},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM h_option_2[] = {
    {"&Start Level (dB) :", (char *) &hear.start, FLOAT, 6, 2, 1, chk_neg},
    {"&Initial Step Size (dB) :", (char *) &hear.step, FLOAT, 6, 2, 1, chk_neg},
    {"&Final Step Size (dB):", (char *) &hear.fin, FLOAT, 6, 2, 1, chk_neg},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM h_option_3[] = {
    {"Tone &Duration (sec) :", (char *) &hear.dur, FLOAT, 6, 2, 1, chk_neg},
    {"&Ramp Time (ms) :", (char *) &hear.ramp_ms, FLOAT, 6, 2, 1, NULL},
    {"S&kipped Sweeps :", (char *) &tone.skips, SHORT, 3, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM h_option_4[] = {
    {"&Min Delay Between Tones (sec): ", (char *)&hear.min_gap_s, FLOAT, 10, 1, 1, chk_neg},
    {"Max &Delay Between Tones (sec): ", (char *)&hear.max_gap_s, FLOAT, 10, 1, 1, chk_neg},
    {"Response &Timeout (sec): ", (char *)&hear.rsp_tim_s, FLOAT, 10, 1, 1, chk_neg},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM h_option_5[] = {
    {"&Maximum of Trials :", (char *) &hear.max_trial, SHORT, 4, 0, 1, chk_int},
    {"&Std. Dev. of Level (dB):", (char *) &hear.sd, FLOAT, 6, 2, 1, chk_neg},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM h_option_6[] = {
    {"&Procedure =", (char *) &thresh_algo, TOGGLE, 4, 0, 1, NULL},
    {"&Task =", (char *) &thresh_task, TOGGLE, 4, 0, 1, NULL},
    {"&Random =", (char *) &thresh_rand, TOGGLE, 4, 0, 1, NULL},
    {"&Connect =", (char *) &thresh_cnct, TOGGLE, 4, 0, 1, NULL},
    {"&High Frequency =", (char *) &thresh_hf, TOGGLE, 4, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM h_option[] = {
    {"Tone &Frequencies ...", NULL, NONE, 0, 0, 1, hr_opt_1},
    {"Tone &Levels ...", NULL, NONE, 0, 0, 1, hr_opt_2},
    {"Tone &Shape ...", NULL, NONE, 0, 0, 1, hr_opt_3},
    {"&Timing ...", NULL, NONE, 0, 0, 1, hr_opt_4},
    {"Stopping &Rules ...", NULL, NONE, 0, 0, 1, hr_opt_5},
    {"T&hreshold ...", NULL, NONE, 0, 0, 1, hr_opt_6},
    {"Le&vel Reference =", (char *) &levref, TOGGLE, 3, 0, 1, NULL},
    {"&Conductance =", (char *) &cond_thl, TOGGLE, 3, 0, 1, NULL},
    {"Si&muluate Listener =", (char *) &sim_obs, TOGGLE, 3, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM p_option[] = 
{
    {"&Size of Token :", (char *) &probe.size, SHORT, 10, 0, 1, NULL},
    {"&Rate of Clock (Hz) :", (char *) &probe.rate, LONG, 10, 0, 1, chk_rate},
    {"&Checkfit Sweeps :", (char *) &probe.chk_swps, SHORT, 10, 0, 1, chk_int},
    {"Spectrum Frequency Range (kH&z) :", (char *) &probe.fft_xrange, SHORT, 4, 0, 1, NULL},
    {"Spectrum Level Range (d&B) :", (char *) &probe.fft_yrange, SHORT, 6, 0, 1, NULL},
    {"C&alibration...", NULL, NONE, 0, 0, 1, p_c_opt},
    {"&Thevenin Computation...", NULL, NONE, 0, 0, 1, p_t_opt},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM p_c_option[] = 
{
    {"&Sweeps per Set :", (char *) &probe.cal_swps, SHORT, 10, 0, 1, chk_int},
    {"S&weep Sets :", (char *) &probe.cal_sets, SHORT, 10, 0, 1, NULL},
    {"&Stimulus File :", probe.calibrate, STRING, 42, 0, 1, NULL},
    {"&Attenuation :", (char *) &probe.cal_atten, FLOAT, 5, 0, 1, NULL},
    {"&Number of Cavities to Test :", (char*)&probe.ncav, SHORT, 4, 0, 1, chk_int},	
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM p_t_option[] = 
{
    {"&Compute Crosstalk :", (char *) &thv_xtk, TOGGLE, 5, 0, 1, NULL},
    {"&Iterate Length :", (char *) &thv_itr, TOGGLE, 0, 0, 1, NULL},
    {"&Also Iterate :", (char *) &thv_ext, TOGGLE, 0, 0, 1, NULL},
    {"&Debug :", (char *) &thv_dbg, TOGGLE, 0, 0, 1, NULL},
    {"f&1_erf (Hz) :", (char *) &probe.f1_erf, FLOAT, 6, 0, 1, NULL},
    {"f&2_erf (Hz) :", (char *) &probe.f2_erf, FLOAT, 6, 0, 1, NULL},
    {"Te&mperatue (deg C) :", (char *) &probe.temp, FLOAT, 5, 1, 1, NULL},
    {"Compute for Chec&kfit :", (char *) &thv_chk, TOGGLE, 0, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM t_option[] = {
    {"&Rate of Clock (Hz) :", (char *) &tone.rate, LONG, 10, 0, 1, chk_rate_tone},
    {"&Buffer Size :", (char *) &tone.size, SHORT, 10, 0, 1, NULL},
    {"&Ramp Time (ms) :", (char *) &tone.ramp_ms, FLOAT, 6, 2, 1, NULL},
    {"&Skipped Sweeps :", (char *) &tone.skips, SHORT, 3, 0, 1, NULL},
    {"Spectrum &Frequency Range (kHz) :", (char *) &tone.fft_xrange, SHORT, 4, 0, 1, NULL},
    {"Spectrum &Level Range (dB) :", (char *) &tone.fft_yrange, SHORT, 6, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM sys_opt[] = {
    {"Display &Units (mV or mPa) =", (char *) &distype, TOGGLE, 4, 0, 1, NULL},
    {"I/O &Sensitivity ...", NULL, NONE, 0, 0, 1, sys_opt_1},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

MENUITEM sen_opt[] = {
    {"&AD (cnt/V) :", (char *) &Sys.AD, FLOAT, 6, 1, 1, NULL},
    {"DA&1 (cnt/V):", (char *) &Sys.DA[0], FLOAT, 6, 0, 1, NULL},
    {"DA&2 (cnt/V):", (char *) &Sys.DA[1], FLOAT, 6, 0, 1, NULL},
    {"&MP (V/Pa)  :", (char *) &Sys.MP, FLOAT, 6, 3, 1, NULL},
    {"&LS1 (V/Pa) :", (char *) &Sys.LS[0], FLOAT, 6, 3, 1, NULL},
    {"L&S2 (V/Pa) :", (char *) &Sys.LS[1], FLOAT, 6, 3, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

extern WIND w_text;

static int text_wind_flag = 0;
static int drspc = 0;           /* DSP reset since probe calibration ? */

int
open_text_w(int *i)
{
    text_wind_flag = save_w(&w_text);
    *i = draw_w(&w_text);
    return (w_text.ytop + txtpar.font_height);
}

void
close_text_w()
{
    if (text_wind_flag)
	close_w(&w_text);
    text_wind_flag = 0;
}

int
Option_wind()
{
    return (simple_submenu(menu_x(), menu_y(), option));
}

int
sys_menu(char *no_use)
{
    int ret;

    ret = simple_submenu(menu_x(), menu_y(), sys_opt);
    attenu.att_dev = devtype.at;
    return (ret);
}

int
sys_opt_1(char *no_use)
{
    return (simple_submenu(menu_x(), menu_y(), sen_opt));
}

int
ton_opt(char *no_use)
{
    return (simple_submenu(menu_x(), menu_y(), t_option));
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

int
hr_opt_1(char *no_use)
{
    return (simple_submenu(menu_x(), menu_y(), h_option_1));
}

int
hr_opt_2(char *no_use)
{
    return (simple_submenu(menu_x(), menu_y(), h_option_2));
}

int
hr_opt_3(char *no_use)
{
    return (simple_submenu(menu_x(), menu_y(), h_option_3));
}

int
hr_opt_4(char *no_use)
{
    return (simple_submenu(menu_x(), menu_y(), h_option_4));
}

int
hr_opt_5(char *no_use)
{
    return (simple_submenu(menu_x(), menu_y(), h_option_5));
}

int
hr_opt_6(char *no_use)
{
    return (simple_submenu(menu_x(), menu_y(), h_option_6));
}

int
hr_opt(char *no_use)
{
    int ret;

    thresh_hf.at = hear.hfthr;
    ret = simple_submenu(menu_x(), menu_y(), h_option);
    hear.hfthr = (short) thresh_hf.at;
    return (ret);
}

int
chk_rate(int32_t *r)
{
    return (0);
}

int
chk_rate_tone(int32_t *r)
{
    return (0);
}

int
chk_int(int **i)
{
    if (**i < 1)
	**i = 1;
    return (0);
}

int
chk_neg(float **f)
{
    if (**f <= 0) 
        **f = 1.0;
    return (0);
}

void
set_drspc(int i)
{
    drspc = i;
}

int
chk_drspc()
{
    return (drspc);
}

void
dsp_vfs()
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
dsp_init()
{
    int     err;

    err = dspseta(0);	                // Set base adresses
    if (!err) {
	err = dspinit(dsp_code_file);   // Upload DSP code file
    }
    if (!err) {
	dsp_vfs();
        set_drspc(1);
    } else {
	decide(0, 1, "Can't initialize DSP.");
    }
    return (err);
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
    tp = 8 * atan(1.0);
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
    tpi = 8 * atan(1.0);
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
    } else if (strcmp(name, "~cal") == 0) {
        ;	// in CAL file
    } else {
	n = 0;	// not defined
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
    if (gen_def_stim(stimfn, outbuf)) {
	npts = buflen;
	rrate = rate;
	valid = 1;
    }
    while (!valid && strlen(stimfn) > 0 && !pgm_done()) {
    	if (!testexist(stimfn)) {
    	    valid = 0;
	} else {
	    valid = (rdils(stimfn, outbuf, (int32_t) buflen, &rrate, &npts) > 0);
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
	    outbuf[i] = 0;
    }
    if (invert) {		/* invert the stimulus */
	for (i = 0; i < npts; i++)
	    outbuf[i] = -outbuf[i];
    }
    last_nvrt = invert;
    memcpy(stimulus, outbuf, np * sizeof(short));
    strcpy(last_stim, stimfn);
    return (np);
}
