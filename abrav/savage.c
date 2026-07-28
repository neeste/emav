/* savage.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdint.h>
#include <memory.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <dsp.h>
#include <menu.h>
#include <token.h>
#include <util.h>
#include "savage.h"
#include "color.h"

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif /* WIN32 */

double  epamp_agn(double);
double  epamp_lpf(double);
int	testexit(char *);
int     average();
int     dis_art(char *);
int     naverage();
int     set_rej(char *);
int     testexist(char *), savedir(char **);
void    calibrate(int);
void    car_defaults();
void    clr_txt();
void    disadbuf();
void    get_info();
void    grab_abr();
void    points_msg(void);
void    pre_dis_ind(char *, char *);
void    tok_init(int);
void    write_next_car(int);
void    epamp_init(int, int, int, int);
int32_t rescale(float *, short *, int, int);

extern char dsp_code_file[128];
extern float time1_ms, time2_ms, rej_mv;
extern float ad_sens, max_level;
extern float F1_cutoff, F2_cutoff;
extern int min_sweeps;
extern int epa_flag;
extern struct abr abr_;

struct _upload dsppar;
char    ilstim[FNAMESIZE] = {"b4.ils"};
int     buflen = 2048;
int32_t    rate = 50000;
float   target_db = 68.0;
int     nsweeps = 128;
int     upperv = MAXPOSINT / 1000;
int     itime1 = 100, itime2 = 450;
int     swp1set = 8;

TOKENFILE *t = (TOKENFILE *) NULL;

TOGGLETYPE f5type = {
    1,
    { "FIXED", "AUTO", "OFF", NULL },
};

TOGGLETYPE distype = {
    1,
    { "V", "Pa", NULL },
};

TOGGLETYPE filter_type = {
    0,
    { "none", "zero-phase", NULL },
};

TOGGLETYPE r_mode = {
    0,
    { "REAL TIME DATA", "RECORD TO DISK", "PLAY BACK FROM DISK", NULL },
};

TOGGLETYPE pqm_mode = {
    0,
    { "No", "VR1 on A", "VR2 on A", "VR1 on B", "VR2 on B", NULL },
};

TOGGLETYPE ok_mode = {
    0,
    { "No", "Yes", NULL },
};

TOGGLETYPE epa_hpf = {
    1,
    { "1", "10", "30", "50", "100", "150", "300", "500", NULL },
};

TOGGLETYPE epa_lpf = {
    1,
    { "30", "100", "300", "500", "1000", "1500", "3000", "5000", NULL },
};

TOGGLETYPE epa_gain = {
    4,
    { "30", "50", "75", "100", "150", "200", "250", "300", NULL },
};


TOGGLETYPE epa_pgn = {
    0,
    { "1000", "50", NULL },
};

TOGGLETYPE epa_acc = {
    0,
    { "AC", "DC",  NULL },
};

TOGGLETYPE epa_notch = {
    1,
    { "In", "Out", NULL },
};

TOGGLETYPE weight = {
    2,
    { "NO", "RMS", "VAR", "SPVAR", "SPRMS", NULL },
};

TOGGLETYPE rejmod = {
    2,
    { "NONE", "MEAN", "RAW", NULL },
};

TOGGLETYPE monitor = {
    0,
    { "PQM", "AQM", "Nsp", "Nav", NULL },
};

TOGGLETYPE few = {
    0,
    { "ALL", "128", "256", "512", NULL },
};

TOGGLETYPE polar = {
    0,
    { "Normal", "Reversed", NULL },
};

TOGGLETYPE aqm = {
    0,
    { "Fsp", "Scor", "Repr", "SNR", "Fmp", NULL },
};

TOGGLETYPE pqm = {
    4,
    { "Fsp", "Scor", "Repr", "SNR", "Fmp", NULL },
};

TOGGLETYPE sqm = {
    0,
    { "No", "Yes", NULL },
};

TOGGLETYPE polarity = {
    0,
    { "ORIGINAL", "REVERSED", NULL },
};

TOGGLETYPE correlate = {
    0,
    { "TEMPLATE", "REPLICATION", NULL },
};

TOGGLETYPE stimType = 
{
    1,
    { "File", "Click", "Tone Burst" },
};

TOGGLETYPE maskerType =
{
    0,
    { "Not Present", "Cont. Tone", "Ramp. Tone" }
};

TOGGLETYPE windowType =
{
    1,
    { "Square", "Blackman" }
};

TOGGLETYPE calType =
{
    1,
    { "Cavity", "In-the-Ear" }
};

ATTENUATION attenu = {80.0, 80.0, 10000.0, 1};

static int chk_int(int **);
static int chk_rate(int32_t *);
static int sys_menu(char *), sen_menu(char *), epa_opt(char *);
static int abr_opt(char *);
static int abr_opt_2(char *), abr_opt_3(char *);
static int abr_opt_4(char *), abr_opt_5(char *), abr_opt_6(char *);
static int check_th_off(), check_t1_t2();
static int change_stim_type(char *);
static int adjust_tone_freq();
static int adjust_mask_freq();
static int adjust_stim_len();
static int change_masker(char *no_use);
static int adjust_window();
static int stim_opt(char *);
static int stim_opt_1(char *);
static int stim_opt_2(char *);
static int epa_reset(char *);

int
flip_polar()
{
    int i;

    for(i = 0; i < buflen; i++) 
    	stimulus[i] = -stimulus[i];
    return(0);
}

MENUITEM option[] = {
    {"&System Options...", NULL, NONE, 0, 0, 1, sys_menu},
    {"&ABR Options...", NULL, NONE, 0, 0, 1, abr_opt},
    {"S&timulus Options...", NULL, NONE, 0, 0, 1, stim_opt},
    {"Artifact &Rejection...", NULL, NONE, 0, 0, 1, set_rej},
    {"&EP Amp Options...", NULL, NONE, 0, 0, 1, epa_opt},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

MENUITEM abr_option[] = {
    {"&Averaging Options...", NULL, NONE, 0, 0, 1, abr_opt_2},
    {"&Quality Measure Options...", NULL, NONE, 0, 0, 1, abr_opt_3},
    {"&Display Options...", NULL, NONE, 0, 0, 1, abr_opt_4},
    {"&Run/Stop Options...", NULL, NONE, 0, 0, 1, abr_opt_5},
    {"&Filter Options...", NULL, NONE, 0, 0, 1, abr_opt_6},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM abr_option_2[] = {
    {"&Sweeps per Set :", (char *) &abr_.swp1set, SHORT, 4, 0, 1, chk_int},
    {"&Weighted Average =", (char *) &weight, TOGGLE, 6, 0, 1, NULL},
    {"&Minimum Set Pairs :", (char *) &min_sweeps, INT, 4, 0, 1, NULL},
    {"M&aximum Set Pairs :", (char *) &abr_.sets, SHORT, 6, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM abr_option_3[] = {
    {"Show &Quality Measure =", (char *) &sqm, TOGGLE, 3, 0, 1, NULL},
    {"&Primary Quality Measure =", (char *) &pqm, TOGGLE, 5, 0, 1, NULL},
    {"&Alternate Quality Measure =", (char *) &aqm, TOGGLE, 5, 0, 1, NULL},
    {"&First Track =", (char *) &monitor, TOGGLE, 4, 0, 1, NULL},
    {"&Start time (ms) :", (char *) &time1_ms, FLOAT, 10, 1, 1, check_t1_t2},
    {"&End time (ms) :", (char *) &time2_ms, FLOAT, 10, 1, 1, check_t1_t2},
    {"&Correlate with ", (char *) &correlate, TOGGLE, 0, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM abr_option_4[] = {
    {"&Points Displayed =", (char *) &few, TOGGLE, 4, 0, 1, NULL},
    {"&Waveform  Polarity =", (char *) &polar, TOGGLE, 9, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM abr_option_5[] = {
    {"&Run Mode =", (char *) &r_mode, TOGGLE, 0, 0, 1, NULL},
    {"Record &File :", (char *) record_file, STRING, 24, 0, 1, NULL},
    {"&PQM Stop Mode =", (char *) &pqm_mode, TOGGLE, 9, 0, 1, NULL},
    {"&OK Stop Mode =", (char *) &ok_mode, TOGGLE, 4, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM abr_option_6[] = {
    {"&Hi-Pass Filter (Hz) :", (char *) &F1_cutoff,
	FLOAT, 10, 1, 1, NULL},
    {"&Lo-Pass Filter (Hz) :", (char *) &F2_cutoff,
	FLOAT, 10, 1, 1, NULL},
    {"&Filter Type =", (char *) &filter_type, TOGGLE, 0, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM stim_option[] = {
    {"&Size of Buffer :", (char *) &abr_.size, SHORT, 5, 0, 1, NULL},
    {"&Rate of Clock (Hz) :", (char *) &abr_.rate, LONG, 6, 0, 1, chk_rate},
    {"Signal &Type = ", (char *) &stimType.at, TOGGLE, 0, 0, 1, change_stim_type},
    {"&Signal Options...", NULL, NONE, 0, 0, 1, stim_opt_1},
    {"&Masker Type = ", (char *) &maskerType.at, TOGGLE, 0, 0, 1, change_masker},
    {"Masker &Options...", NULL, NONE, 0, 0, 1, stim_opt_2},
    {"&Calibration Type = ", (char *) &calType.at, TOGGLE, 0, 0, 1, NULL},
    {"Ma&x Cav Level (dB SPL)) :", (char *) &max_level, FLOAT, 6, 1, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM stim_option_1[5] = 
{
    {"Stimulus &File :", abr_.stim_file, STRING, 42, 0, 1, NULL},
    {"Stimulus &Level (dB SPL):", (char *) &abr_.stim_lev, FLOAT, 6, 0, 1, NULL},
    {"Stimulus &Polarity =", (char *) &polarity, TOGGLE, 9, 0, 1, flip_polar},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

//this menu is the stimulus options when "click" is selected as stim type
MENUITEM stim_option_2[] = 
{
    {"Click &Duration (msec):", (char *) &abr_.clk_dur, FLOAT, 6, 1, 1, NULL},
    {"Click &Level (dB SPL):", (char *) &abr_.clk_lev, FLOAT, 6, 0, 1, NULL},
    {"Click &Polarity =", (char *) &polarity, TOGGLE, 9, 0, 1, flip_polar},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

//this menu is for the stimulus options when "tone" is selected as stim type
MENUITEM stim_option_3[] =
{
    {"Signal &Frequency (Hz) :", (char *) &abr_.sig_freq, FLOAT, 10, 2, 1, adjust_tone_freq},
    {"Signal &Level (dB SPL):", (char *) &abr_.sig_lev, FLOAT, 6, 0, 1, NULL},
    {"Signal &Duration (msec): ", (char *) &abr_.sig_dur, SHORT, 6, 0, 1, NULL},
    {"&Window Type : ", (char *) &windowType, TOGGLE, 0, 0, 1, NULL }, 
    {"Signal &Start (msec):", (char *) &abr_.sig_start, FLOAT, 6, 1, 1, adjust_window},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM stim_option_4[] =
{
    {"Masker &Frequency (Hz) : ", (char *) &abr_.msk_freq, FLOAT, 6, 3, 1, adjust_mask_freq},
    {"Masker &Level : ", (char *) &abr_.msk_lev, FLOAT, 6, 2, 1, NULL },
    {"Masker &Duration (msec): ", (char *) &abr_.msk_dur, SHORT, 6, 0, 1, NULL},
    {"Masker &ramp (msec): ", (char *) &abr_.msk_ramp, SHORT, 6, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};


MENUITEM rej_menu[] =
{
    {"&Rejection Threshold (mV) :", (char *) &rej_mv, FLOAT, 10, 0, 1, check_th_off},
    {"Re&ject Mode =", (char *) &rejmod, TOGGLE, 0, 0, 1, dis_art},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

MENUITEM epa_option[] = {
    {"&Hi-Pass Filter (Hz) =", (char *) &epa_hpf, TOGGLE, 4, 0, 1, NULL},
    {"&Lo-Pass Filter (Hz) :", (char *) &epa_lpf, TOGGLE, 6, 0, 1, NULL},
    {"&Amplifier Gain :", (char *) &epa_gain, TOGGLE, 6, 0, 1, NULL},
//    {"&Preamp Gain =", (char *) &epa_pgn, TOGGLE, 5, 0, 1, NULL},
//    {"&Coupling =", (char *) &epa_acc, TOGGLE, 3, 0, 1, NULL},
    {"&Notch Filter =", (char *) &epa_notch, TOGGLE, 4, 0, 1, NULL},
    {"&Reset Opti-Amp", NULL, NONE, 0, 0, 1, epa_reset},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

MENUITEM sys_opt[] = {
    {"Display &Units (V or Pa) =", (char *) &distype, TOGGLE, 4, 0, 1, NULL},
    {"I/O &Sensitivity ...", NULL, NONE, 0, 0, 1, sen_menu},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

MENUITEM sen_opt[] = {
    {"&AD (cnt/V) :", (char *) &Sen.AD, FLOAT, 6, 0, 7, NULL},
    {"DA&1 (cnt/V):", (char *) &Sen.DA[0], FLOAT, 6, 0, 1, NULL},
    {"DA&2 (cnt/V):", (char *) &Sen.DA[1], FLOAT, 6, 0, 1, NULL},
    {"&MP (V/Pa)  :", (char *) &Sen.MP, FLOAT, 6, 3, 1, NULL},
    {"&LS1 (V/Pa) :", (char *) &Sen.LS[0], FLOAT, 6, 3, 1, NULL},
    {"L&S2 (V/Pa) :", (char *) &Sen.LS[1], FLOAT, 6, 3, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

extern WIND w_text;

static int text_wind_flag = 0;

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
    int ret;

    ret = simple_submenu(menu_x(), menu_y(), option);
    points_msg();
    return (ret);
}

static int
sys_menu(char *no_use)
{
    int ret;

    ret = simple_submenu(menu_x(), menu_y(), sys_opt);
    return (ret);
}

static int
sen_menu(char *no_use)
{
    return (simple_submenu(menu_x(), menu_y(), sen_opt));
}

static int
abr_opt(char *no_use)
{
    int     ret;

    rate = abr_.rate;
    ret = simple_submenu(menu_x(), menu_y(), abr_option);
    grab_abr();
    return (ret);
}

static int
abr_opt_2(char *no_use)
{
    return(simple_submenu(menu_x(), menu_y(), abr_option_2));
}

static int
abr_opt_3(char *no_use)
{
    return(simple_submenu(menu_x(), menu_y(), abr_option_3));
}

static int
abr_opt_4(char *no_use)
{
    return(simple_submenu(menu_x(), menu_y(), abr_option_4));
}

static int
abr_opt_5(char *no_use)
{
    return(simple_submenu(menu_x(), menu_y(), abr_option_5));
}

static int
abr_opt_6(char *no_use)
{
    return(simple_submenu(menu_x(), menu_y(), abr_option_6));
}

static int
stim_opt(char *no_use)
{
    int     ret;

    rate = abr_.rate;
    ret = simple_submenu(menu_x(), menu_y(), stim_option);
    grab_abr();
    return (ret);
}

static int
stim_opt_1(char *no_use)
{
     if(stimType.at == 1)    //set the stim option window to reflect the stim type
    {
	//return the menu for the generated stimulus type
        return(simple_submenu(menu_x(), menu_y(), stim_option_2));
    }
    else if(stimType.at == 2)
    {
        adjust_tone_freq();
        adjust_stim_len();
        return(simple_submenu(menu_x(), menu_y(), stim_option_3));
    }
    else    //otherwise we just use the default stim file
    {
        return(simple_submenu(menu_x(), menu_y(), stim_option_1));
    }
}

static int 
stim_opt_2(char *no_use)
{
    return( simple_submenu(menu_x(), menu_y(), stim_option_4) );
}

static int 
change_stim_type(char *no_use)
{
    return 0;
}

static 
int change_masker(char *no_use)
{
    return (0);
}

static int
epa_opt(char *no_use)
{
    int     ret;
    ret = simple_submenu(menu_x(), menu_y(), epa_option);
    return (ret);
}

static int epa_reset(char *no_use)
{
    epamp_init(epa_gain.at, epa_hpf.at, epa_lpf.at, epa_notch.at);
    return 0;
}

//this funciton will make sure that the duration of the window will not
//exceed the legnth of the signal
static int 
adjust_window()
{
    return (0);
}

static int
chk_rate(int32_t *r)
{
    //unsigned int factor;

    if (abr_.rate < 100)
	abr_.rate = 100;
    abr_.rate = dspadjrt(abr_.rate);
    return (0);
}

static int
chk_int(int **i)
{
    if (**i < 1)
	**i = 1;
    return (0);
}

static int 
adjust_mask_freq()
{
    abr_.msk_freq = (float)(floor((abr_.msk_freq * abr_.size) / abr_.rate + 0.5) 
	* abr_.rate) / abr_.size;
    return (0);

}

static int 
adjust_tone_freq()
{
    abr_.sig_freq = (float)(floor((abr_.sig_freq * abr_.size) / abr_.rate + 0.5) 
	* abr_.rate) / abr_.size;
    return (0);
}

static int 
adjust_stim_len()
{
    return (0);
}

int
dsp_init()
{
    int     err;

    err = dspseta(dsppar.code);			// Set base address
    if(!err) {
	err = dspinit(dsp_code_file);	// Upload DSP code file
    } else {
	decide(0, 1, "Can't initialize DSP.");
    }
    return (err);
}

/*
 * tok_init - Initialize tokenfile structure
 */
void
tok_init(int ntokens)
{
    t->header.power10 = 1;
    t->header.isf = (int) (rate / 10);
    t->hitch.tokens = ntokens;
    t->hitch.curtok = 0;
    t->hitch.toksiz = buflen;
    memcpy(t->hitch.comment, &patient, sizeof(PATIENT));
    t->hitch.target_level = target_db;
    t->hitch.mode = 0;
    tinit(t);
    write_next_car(0);
}

int
findmax(short *intbuf, int n)
{
    int     i;
    int     maxval = 0, absval;

    for (i = 0; i < n; i++) {
	absval = abs(intbuf[i]);
	if (absval > maxval)
	    maxval = absval;
    }
    return (maxval);
}

/*
 * store the token
 */
void
tok_store(int tcount, int c, float *p)
{
    int     tmax;
    int32_t amax;
    float   scale;

    if (c == 0)
	return;
    amax = rescale(p, outbuf, buflen, MAXPOSINT);
    tmax = findmax(outbuf, buflen);
    c *= swp1set;
    scale = (float) ((tmax <= 0) ? 1 : ((double) amax / tmax) / (c * Sen.AD * Sen.MP));
    t->car.dccomp = 0;
    t->car.maxval = amax;
    t->car.tokmax = tmax;
    strncpy(t->car.tokcmt, patient.Comment, COMNTSIZE);
    strncpy(t->car.sname, ilstim, FNAMESIZE);
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
    t->car.adsen = ad_sens;
    t->car.mpsen = Sen.MP;

    (void) twrite(outbuf, tcount, t);
}

/*
 * getstim - Read ils stimulus file
 */
int
getstim(char *stimfn, int np)
{
    static int32_t rrate, npts;
    static char last_stim[42] = {42 * '\0'};
    int     n, i, valid, k;

    i = testexist(stimfn);
    if (!strcmp(stimfn, last_stim) && (np == npts)) return (0);
    do {
    	if(!i)
    	    valid = 0;
	else 
	    valid = (rdils(stimfn, (short *) outbuf, (int32_t) buflen, 
	        &rrate, &npts) > 0);
	if (!valid) {
	    gr_beep();
	    k = open_text_w(&i);
	    (void) gprintf(i, k, "stimulus file '%s' not found or open error.", stimfn);
	    n = gprintf(i, k + txtpar.font_height,
		"Please retype stimulus file name: ");
	    txtpar.text_wind_len -= n;
	    (void) get_str(i + 8 * n, k + txtpar.font_height, 42, 0, STRING, stimfn);
	    close_text_w();
	}
    } while (!valid && strlen(stimfn) > 0);
    k += txtpar.font_height + 4;
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
	for (i = npts; i < np; i++)
	    outbuf[i] = 0;
    }
    if (polarity.at) {		/* invert the stimulus */
	for (i = 0; i < npts; i++)
	    outbuf[i] = -outbuf[i];
    }
    memcpy(stimulus, outbuf, np * sizeof(short));
    strcpy(last_stim, stimfn);
    return (np);
}

void
write_next_car(int tcount)
{
    if (tcount < t->hitch.tokens) {
	strncpy(t->car.tokcmt, "", COMNTSIZE);
	strncpy(t->car.sname, ilstim, FNAMESIZE);
	t->car.reps = nsweeps;
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
check_th_off()
{
    rej_mv = (float) limit(0.001, rej_mv, (1000.0 * MAXPOSINT) / Sen.AD);
    return (0);
}

static int
check_t1_t2()
{
    static float tmax = 999;

    time1_ms = limit(0, time1_ms, tmax);
    time2_ms = limit(0, time2_ms, tmax);

    return (0);
}

int
set_rej(char *no_use)
{

    (void) simple_submenu(menu_x(), menu_y(), rej_menu);
    rate = abr_.rate;
    return (0);
}

int
dis_art(char *c)
{
    pre_dis_ind("Artifact", "Limit");
    return (0);
}
