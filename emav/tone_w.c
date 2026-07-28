/* ================================= tone_w.c =============================== */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <dsp.h>
#include <menu.h>
#include <util.h>
#include "savage.h"
#include "screen.h"
#include "emav.h"		/* function prototyes */

#define TWOPI		(2 * M_PI)

extern char line[MAXLINE];
extern int display_reset, swp1set, maxnpts, contav;
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
TOGGLETYPE dc_rmv = {
    1,
    { "No", "Yes", NULL },
};

static double dsp_scale[2] = {1, 1};
static double rms_out = 0;
static int esc_flg = 0;

/* make the tone option current */
void
grab_tone()
{
    mp_transfer(Sys.MPfn);
    Sen = Sys;
    dis_units(Sen.AD, Sen.MP);

    if (tone.size > maxnpts) {
	tone.size = (short)(maxnpts);
    }
    buflen = tone.size;
    rate = tone.rate = adjust_rate(tone.rate);
    fft_xrange = tone.fft_xrange;
    fft_yrange = tone.fft_yrange;
    fft_levref = tone.levref;
    contav = 0;
    chk_ramp_tone();
    change_nic(1);
    input_filter(hpf_type, hpf_ord, tone.hpff, tone.rate);
}

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
chk_due()
{
    float   a, d;
    int     i;

    a = (float) buflen / rate;
    d = tone.duration;
    if (d < a) {
	d = a;
    } else {
	i = (int) (d / a + 0.5);
	d = (float)(a * i);
    }
    tone.duration = d;
    return 0;
}

MENUITEM tone_menu[] = {
    {"&Start Tone", NULL, NONE, 0, 0, 1, do_tone},
    {"Tone &Frequency (Hz) :", (char *) &tone.freq, FLOAT, 7, 0, 1, adjust_tone_f},
    {"Sound &Level (dBSPL) :", (char *) &tone.level, FLOAT, 7, 0, 1, NULL},
    {"&Duration (sec) :", (char *) &tone.duration, FLOAT, 10, 3, 1, chk_due},
    {"&Channel =", (char *) &play_chan, TOGGLE, 0, 0, 1, NULL},
    {"&Remove DC =", (char *) &dc_rmv, TOGGLE, 0, 0, 1, NULL},
    {"&Mode =", (char *) &play_mode, TOGGLE, 0, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

static float mic_v  = 1, rec_v[2] = {1, 1};

static int
cmp_ad_sens(float **v)
{
    if (mic_v > 0)
        Sen.AD *= sig_v.rms / mic_v;
    return (0);
}

static int
cmp_da1_sens(float **v)
{
    if ((sig_v.rms > 0) && (rec_v[0] > 0)) {
        Sen.LS[0] = rec_v[0] * Sen.MP / sig_v.rms;
        Sen.DA[0] = (float) (rms_out / rec_v[0]);
    }
    return (0);
}

static int
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

    p = (int)(distype.at);
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
    gprintf(j, k, "Spectral Level");
    k += txtpar.font_height;
    gprintf(j, k, " %-5.1f dBSPL ", pickat(tone.freq));
    k += txtpar.font_height;
    gprintf(j, k, "P-P Level:");
    k += txtpar.font_height;
    gprintf(j, k, " %-5.1f ppdBSPL", vtodb(sig_v.vpp));
    k += txtpar.font_height;
    u = unit_select(sig_v.vpp, &v, &d);
    gprintf(j, k, "  %-6.*f %s", d, v, u);
    k += txtpar.font_height;
    gprintf(j, k, "RMS %s:", vp[distype.at]);
    k += txtpar.font_height;
    if (distype.at) {
        gprintf(j, k, " %-5.1lf dBSPL", vtodb(sig_v.rms));
    } else {
        u = unit_select(sig_v.rms, &v, &d);
        gprintf(j, k, "  %-6.*f %s", d, v, u);
    }
    k += txtpar.font_height;
    gprintf(j, k, "DC %s:", vp[distype.at]);
    k += txtpar.font_height;
    u = unit_select(sig_v.dco, &v, &d);
    gprintf(j, k, "  %-6.*f %s", d, v, u);
}
void
spec_volt()
{
    mic_v = sig_v.rms;
    rec_v[0] = (float) (rms_out / Sen.DA[0]);
    rec_v[1] = (float) (rms_out / Sen.DA[1]);
    preamp_voltage[1].status = (char) !play_chan.at;
    preamp_voltage[2].status = (char) play_chan.at;
    simple_submenu(menu_x(), 0, preamp_voltage);
}

static void
ramp_down()
{
    dspend();                       // tell dsp to ramp down
    msleep(nint(tone.ramp_ms));     // wait for ramp down
    dspdget();                      // check dsp status
}

/* graphical display of the tone */
void
show_tone()
{
    double  tms;
    int     cnt = 0, mx, my;
    int32_t   *lbuf;
    short   data;

    tms = 1000 * (double) buflen / rate;
    lbuf = ((int32_t *) calloc((size_t)(buflen), sizeof(int32_t)));
    dfgc = scrn_c[C_WAVE];
    dsprst(FALSE);
    while (cnt < acc_sets) {
	if (dspdget()) {
	    dspdread(&data);
	    if (data <= 0) {
                ramp_down();
		dsprst(TRUE);
		dspuarr(dsppar.acc, buflen * 2, (short *) lbuf);
		if (++cnt < acc_sets) {
		    dsprst(FALSE);
		}
		if (dc_rmv.at) {
		    take_the_aver(buflen, lbuf);
		}
	        w_sig.ybot -= txtpar.font_height;
		disply(swp1set, lbuf, buflen, 1);
		w_sig.ybot += txtpar.font_height;
		show_xlabel(0.0, tms, 0, "ms");
		fft_stim(lbuf, 1, 0);
		dis_tone();
		show_fft(10, 80, 0, scrn_c[C_WAVE], 1);
	    }
	}
	if (check_event()) {
	    data = (short)(getevent());
	    if (data == 3 || data == 27) {
		esc_flg = 1;
		break;
	    } else if (data == ' ') {
		R_clear();
	    } else if (data == 'v') {
		spec_volt();
	    } else if (data == SINGLE_CLICK) {
	    	mouse_position(&mx, &my);
		if (!in_wind(mx, my, w_sig)) {
		    esc_flg = 1;
		    break;
		}
	    }
	}
    }
    ramp_down();
    dsprst(TRUE);
    free(lbuf);
}

/* generate tone stimulus into a buffer */
static void
gen_tone(float f, int c)
{
    int     i;
    double  mag, osen, ref, dp, am, mxam;
    int32_t *sbuf = savbuf;

    osen = Sen.LS[c] * Sen.DA[c];	    // cnt/Pa
    ref = sqrt(2.0) * spl_ref * osen;	    // pk @ 0 dB SPL
    mxam = pow(2, 31) -1;		    // max amplitude
    mag = pow(10.0, tone.level / 20.0) * ref * (mxam / MAXPOSINT);
    dsp_scale[c] = dspsets(mag / mxam);
    am = limit(1, mag / dsp_scale[c], mxam);
    dp = TWOPI * f / rate;
    for (i = 0; i < buflen; i++) {
	sbuf[i] = nint(am * sin(dp * i));
    }
}

/* do the tone task */
int
do_tone()
{
    int     acc_sets_sav, swp1set_sav, chan, tone_cnt;
    float   a;

    swp1set_sav = swp1set;
    acc_sets_sav = acc_sets;
    a = (float)((tone.duration / buflen) * rate);
    acc_sets = 1;
    if (a < 32000) {
	swp1set = (int) (a + 0.5);
    } else {
	do {
	    acc_sets++;
	} while ((a / acc_sets) > 32000.0);
	swp1set = (float)((int) (a / acc_sets + 0.5));
    }
    R_clear();
    chan = (int)(play_chan.at);

    dsprst(TRUE);
    downldpar(2, buflen, 1, tone.ramp, swp1set);
    gen_tone(tone.freq, chan);
    dwnld_dac(chan, 1, 1, 1);
    dspsarr(dsp_scale[chan], chan);
    zero_out();
    dwnld_dac(1 - chan, 1, 1, 0);
    draw_w(&w_msg);
    esc_flg = 0;
    tone_cnt = 0;
    do {
	sprintf(line, "Tone %3d ", ++tone_cnt);
	top_message(line);
	show_tone();
	top_message("Tone Off   ");
      } while (esc_flg == 0 && play_mode.at == 0);
    swp1set = swp1set_sav;
    acc_sets = acc_sets_sav;
    return 0;
}

/* this function is associated to the mainmenu TONE item */
void 
Tone_wind()
{
    if (dsp_init() > 0)
	return;
    top_message("Tone       ");
    grab_tone();
    adjust_tone_f();
    draw_w(&w_spec);
    draw_w(&w_info);
    simple_submenu(menu_x(), menu_y(), tone_menu);
    top_message("           ");
    dsprst(TRUE);
}
