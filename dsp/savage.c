/* savage.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include <menu.h>
#include "dsp.h"
#include "savage.h"
#include "screen.h"

int	chk_int(int **);
int     chk_neg( float **);
int     chk_ramp_tone();
int     dsp_force_init(char *);
int     Quit_wind();
int     sys_menu(char *);
int     sys_opt_1(char *);
int     testexist(char *);
int     tone_opt(char *);
int32_t    adjust_rate();

extern char dsp_code_file[80];
extern int dsp_code;
extern struct _tone tone;
extern WIND w_text;

float   target_voltage = (float)0.3162;    /* 70dB SPL (80dB SPL = 1v) */
int     buflen = 1024;
int     option_end = 0;
int     nsets = 128;
int     upperv = MAXPOSINT / 1000;
int     swp1set = 8;
int     invert = 0;
int32_t    rate = 50000;
struct _upload dsppar;

TOGGLETYPE distype = {
    1,
    { "mV", "mPa", NULL },
};

static int chk_rate_tone(int32_t *);

static MENUITEM tone_option[] = {
    {"&Rate of Clock (Hz) :", (char *) &tone.rate, LONG, 10, 0, 1, chk_rate_tone},
    {"&Buffer Size :", (char *) &tone.size, SHORT, 10, 0, 1, NULL},
    {"Ramp &Time (ms) :", (char *) &tone.ramp_ms, FLOAT, 6, 1, 1, chk_ramp_tone},
    {"&Skipped Sweeps :", (char *) &tone.skips, SHORT, 3, 0, 1, NULL},
    {"Spectrum &Frequency Range (kHz) :", (char *) &tone.fft_xrange, SHORT, 4, 0, 1, NULL},
    {"Spectrum &Level Range (dB) :", (char *) &tone.fft_yrange, SHORT, 6, 0, 1, NULL},
    {"E&xit ( ^C )", NULL, NONE, 0, 0, 1, Quit_wind},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

int
tone_opt(char *no_use)
{
    return (simple_submenu(menu_x(), menu_y(), tone_option));
}

int
chk_ramp_tone()
{
    int i, n, r, m = 16;

    r = (int) (tone.ramp_ms * (float) tone.rate / 1000.0 + 0.5);
    if(r < m) {
    	i = 0;
    } else if (r >= tone.size) {
	i = tone.size;
    } else {
	i = m;
	n = r;
	for(;;) {
	    n /= 2;
	    if(n < m) break;
	    i *= 2;
	}
	n = r - i;
	n *= 2;
	if(n > i && r < 32768)
	    i *= 2;
    }
    tone.ramp = i;
    tone.ramp_ms = (float) (tone.ramp * 1000.0 / tone.rate);

    return(0);
}

static int
chk_rate_tone(int32_t *r)
{
    tone.rate = adjust_rate(tone.rate);
    return (0);
}

int
dsp_init(int force)
{
    int     status = 0;

    status = dspseta(dsppar.code);		/* Set base address     */
    if(!status)
	status = dspinit(dsp_code_file);	/* Upload DSP code file */
    if(status)
	decide(0, 1, "Can't initialize DSP.");
    return (status);
}
