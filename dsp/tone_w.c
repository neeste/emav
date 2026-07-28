/* tone_w.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <menu.h>
#include <util.h>
#include "dsp.h"
#include "savage.h"
#include "screen.h"
#include "tstdsp.h"		/* function prototyes */

#define TWOPI		(8 * atan(1.0))

extern char line[MAXLINE];
extern int display_reset, swp1set, dfgc;
extern struct _tone tone;
extern TOGGLETYPE distype;

TOGGLETYPE play_mode = {
    0,
    { "Repeated", "Once", NULL },
};
TOGGLETYPE play_chan = {
    0,
    { "A", "B", NULL },
};

static double rms_out = 0;
static int esc_flg = 0;

/* adjust the frequency to be the closest possible */
double
adjustf(double f)
{
    f = (floor((f * buflen) / rate + 0.5) * rate) / buflen;
    return (f);
}

/* adjust the freq for MENUITEM tone data structure */
int
adjust_tone_f()
{
    tone.freq = (float) adjustf(tone.freq);
    return (0);
}

/* check tone duration for MENUITEM tone */
int
chk_dur()
{
    float   a, d;
    int     i;

    a = (float) buflen / rate;
    d = tone.duration;
    if (d < a) {
	d = a;
    } else {
	i = (int) (d / a + 0.5);
	d = a * i;
    }
    tone.duration = d;
    return 0;
}

MENUITEM tone_menu[] = {
    {"&Start Tone", NULL, NONE, 0, 0, 1, do_tone},
    {"&Frequency (Hz) :", (char *) &tone.freq, FLOAT, 8, 1, 1, adjust_tone_f},
    {"&Level (dB SPL) :", (char *) &tone.level, FLOAT, 8, 2, 1, NULL},
    {"&Duration (sec) :", (char *) &tone.duration, FLOAT, 8, 1, 1, chk_dur},
    {"&Channel =", (char *) &play_chan, TOGGLE, 0, 0, 1, NULL},
    {"&Mode =", (char *) &play_mode, TOGGLE, 0, 0, 1, NULL},
    {"&Options...", NULL, NONE, 0, 0, 1, tone_opt},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

static float mic_v  = 1, rec_v[2] = {1, 1};

int
cmp_ad_sens(float **v)
{
    if (mic_v > 0) {
        Sen.AD *= sig_v.rms / mic_v;
    }
    return (0);
}

int
cmp_da1_sens(float **v)
{
    if ((sig_v.rms > 0) && (rec_v[0] > 0)) {
        Sen.LS[0] = rec_v[0] * Sen.MP / sig_v.rms;
        Sen.DA[0] = (float) (rms_out / rec_v[0]);
    }
    return (0);
}

int
cmp_da2_sens(float **v)
{
    if ((sig_v.rms > 0) && (rec_v[1] > 0)) {
        Sen.LS[1] = rec_v[1] * Sen.MP / sig_v.rms;
        Sen.DA[1] = (float) (rms_out / rec_v[1]);
    }
    return (0);
}

MENUITEM preamp_voltage[] = {
    {"&MIC (V)  :", (char *) &mic_v, FLOAT, 8, 4, 1, cmp_ad_sens},
    {"REC&1 (V) :", (char *) &rec_v[0], FLOAT, 8, 4, 1, cmp_da1_sens},
    {"REC&2 (V) :", (char *) &rec_v[1], FLOAT, 8, 4, 1, cmp_da2_sens},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

char *
unit_select(double in, double *out, int *dpl)
{
    double u, v;
    int p, r, d;
    static  char *units[2][5] = {
        {"pV ", "nV ", "uV ", "mV ", " V "},
        {"pPa", "nPa", "uPa", "mPa", " Pa"}, 
    };
    static double m[5] = {1e-12, 1e-9, 1e-6, 1e-3, 1};

    p = distype.at;
    if (p)
	in /= Sen.MP;
    v = fabs(in);
    if (v < m[1]) {
        r = 0;
    } else if (v < m[2]) {
        r = 1;
    } else if (v < m[3]) {
        r = 2;
    } else if (v < m[4]) {
        r = 3;
    } else {
        r = 4;
    }
    u = v / m[r];
    if (u < 10) {
	d = 3;
    } else if (u < 100) {
	d = 2;
    } else if (u < 1000) {
	d = 1;
    } else {
	d = 0;
    }
    *out = in / m[r];
    *dpl = d;
    return (units[p][r]);
}

/* display tone level and etc */
void
dis_tone()
{
    char *u;
    double v;
    int     j, k, d;
    extern SIG_V sig_v;
    static char *vp[2] = {"voltage", "pressure"};

    text_color(scrn_c[C_MSGF], scrn_c[C_MSGB]);
    txtpar.text_wind_len = (w_msg.xbot - w_msg.xtop) / 8;
    j = w_msg.xtop + 4;
    k = w_msg.ytop + txtpar.font_height;
    (void) gprintf(j, k, "Spectral Level");
    k += txtpar.font_height;
    (void) gprintf(j, k, " %-5.1f dBSPL ", pickat(tone.freq));
    k += txtpar.font_height;
    (void) gprintf(j, k, "P-P Level:");
    k += txtpar.font_height;
    (void) gprintf(j, k, " %-5.1f ppdBSPL", vtodb(sig_v.vpp));
    k += txtpar.font_height;
    u = unit_select(sig_v.vpp, &v, &d);
    (void)gprintf(j, k, "  %-6.*f %s", d, v, u);
    k += txtpar.font_height;
    (void) gprintf(j, k, "RMS %s:", vp[distype.at]);
    k += txtpar.font_height;
    if (distype.at) {
        (void) gprintf(j, k, " %-5.1lf dBSPL", vtodb(sig_v.rms));
    } else {
        u = unit_select(sig_v.rms, &v, &d);
        (void)gprintf(j, k, "  %-6.*f %s", d, v, u);
    }
    k += txtpar.font_height;
    (void) gprintf(j, k, "DC %s:", vp[distype.at]);
    k += txtpar.font_height;
    u = unit_select(sig_v.dco, &v, &d);
    (void)gprintf(j, k, "  %-6.*f %s", d, v, u);
}

void
spec_volt()
{
    mic_v = sig_v.rms;
    rec_v[0] = (float) (rms_out / Sen.DA[0]);
    rec_v[1] = (float) (rms_out / Sen.DA[1]);
    preamp_voltage[1].status = (char)!play_chan.at;
    preamp_voltage[2].status = (char)play_chan.at;
    (void) simple_submenu(menu_x(), 0, preamp_voltage);
}

/* graphical display of the tone signal */
void
show_tone()
{
    double  tms;
    int     cnt = 0, mxpos, mypos;
    short   data;

    if (dsprst(FALSE)) {
	esc_flg = 1;
	return;
    }
    while (cnt < nsets) {
	if (dspdget()) {
	    dspdread(&data);
	    if (data <= 0) {
		dspuarr(dsppar.acc, buflen * 2, outbuf);
		dsprst(TRUE);
		if (++cnt < nsets)
		    dsprst(FALSE);
                long2float((int32_t *) outbuf, buflen);
                dfgc = scrn_c[C_WAVE];
	        w_sig.ybot -= txtpar.font_height;
		disply(swp1set, (float *) outbuf, buflen, 1, 1);
		w_sig.ybot += txtpar.font_height;
		tms = 1000 * (double) buflen / rate;
		show_xlabel(0.0, tms, 0, "ms");
		fft_stim((float *) outbuf, 1, 0);
		dis_tone();
		show_fft(tone.fft_xrange, tone.fft_yrange, tone.levref, scrn_c[C_WAVE], 1);
	    }
	}
	if (check_event()) {
	    data = getevent();
	    if (data == 3 || data == 27) {
		esc_flg = 1;
		break;
	    } else if (data == ' ') {
		(void) R_clear();
	    } else if (data == 'v') {
		spec_volt();
	    } else if (data == SINGLE_CLICK) {
	    	mouse_position(&mxpos, &mypos);
		if (mxpos < w_sig.xtop || mxpos > w_sig.xbot ||
		    mypos < w_sig.ytop || mypos > w_sig.ybot) {
		    esc_flg = 1;
		    break;
		}
	    }
	}
    }
    dsprst(TRUE);
}

/* generate tone simulus into a buffer */
static double
gen_tone(float f, int c)
{
    int     i;
    double  mag, osen, ref, dp, amp;

    osen = Sen.LS[c] * Sen.DA[c];                   /* cnt/Pa */
    ref = sqrt(2.0) * spl_ref * osen;               /* pk @ 0 dB SPL */
    mag = pow(10.0, tone.level / 20.0) * ref;
    tone.level = (float) (20 * log10(mag / ref));
    amp = limit(0, mag, MAXPOSINT);
    dp = TWOPI * f / rate;
    for (i = 0; i < buflen; i++)
	outbuf[i] = nint(amp * sin(dp * i));
    return (amp / sqrt(2.0));
}

/* do the tone task */
int
do_tone()
{
    int     nsets_s, swp1set_s, chan, tone_cnt;
    float   a;

    swp1set_s = swp1set;
    nsets_s = nsets;
    a = tone.duration / buflen * rate;
    nsets = 1;
    if (a < 32000)
	swp1set = (int) (a + 0.5);
    else {
	do {
	    nsets++;
	} while (a / nsets > 32000.0);
	swp1set = (int) (a / nsets + 0.5);
    }
    R_clear();
    chan = play_chan.at;

    (void) dsprst(TRUE);
    downldpar(2, buflen, tone.skips, tone.ramp);
    rms_out = gen_tone(tone.freq, chan);
    ld_stim(chan, 0, outbuf, buflen);
    zero_all();
    ld_stim(1 - chan, 0, outbuf, buflen);
    rec_v[chan] = (float) (rms_out / Sen.DA[chan]);
    esc_flg = 0;
    tone_cnt = 0;
    do {
        if (rec_v[chan] != (rms_out / Sen.DA[chan])) {
           (void) dsprst(TRUE);
            rms_out = gen_tone(tone.freq, chan);
            ld_stim(chan, 0, outbuf, buflen);
            rec_v[chan] = (float) (rms_out / Sen.DA[chan]);
        }
	if (play_mode.at) {
	    top_message("Tone On ");
	} else {
	    sprintf(line, "Tone %3d", ++tone_cnt);
	    top_message(line);
	}
	show_tone();
	top_message("Tone Off");
    } while (esc_flg == 0 && play_mode.at == 0);
    swp1set = swp1set_s;
    nsets = nsets_s;
    return 0;
}

/* this function is associated to the mainmenu TONE item */
int 
Tone_wind()
{
    dis_units(Sen.AD, Sen.MP);
    rate = tone.rate = adjust_rate(tone.rate);
    buflen = tone.size;
    tone.freq = (float) adjustf(tone.freq);
    chk_ramp_tone();
    draw_w(&w_spec);
    if (dsp_init(0) > 0) {
	title("No device");
    } else {
        title(dspdev());
    }

    return (simple_submenu(menu_x(), 0, tone_menu));
}
