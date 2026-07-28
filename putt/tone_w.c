/* tone_w.c */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <dsp.h>
#include <menu.h>
#include <util.h>
#include "savage.h"
#include "screen.h"
#include "putt.h"

#define TWOPI		(8 * atan(1.0))

double  units_conv(double);
int     do_tone(), ton_opt();

extern int display_reset, swp1set, dfgc;
extern TOGGLETYPE distype;

TOGGLETYPE play_mode = {
    0,
    { "Repeated", "Once", NULL },
};
TOGGLETYPE play_chan = {
    0,
    { "A", "B", NULL },
};

static double dsp_scale[2] = {1, 1};
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
adjust_tone_f(float **f)
{
    **f = (float) adjustf(**f);
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
	d = a * i;
    }
    tone.duration = d;
    return 0;
}

MENUITEM tone_menu[] = {
    {"&Start Tone", NULL, NONE, 0, 0, 1, do_tone},
    {"&Frequency (Hz) :", (char *) &tone.freq, FLOAT, 8, 1, 1, adjust_tone_f},
    {"&Level (dB SPL) :", (char *) &tone.level, FLOAT, 8, 2, 1, NULL},
    {"&Duration (sec) :", (char *) &tone.duration, FLOAT, 8, 1, 1, chk_due},
    {"&Channel =", (char *) &play_chan, TOGGLE, 0, 0, 1, NULL},
    {"&Mode =", (char *) &play_mode, TOGGLE, 0, 0, 1, NULL},
    {"&Options...", NULL, NONE, 0, 0, 1, ton_opt},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

/* display tone signal level and etc */
void
dis_tone()
{
    double  fft_lev;
    int     j, k;

    text_color(scrn_c[C_MSGF], scrn_c[C_MSGB]);
    txtpar.text_wind_len = (w_msg.xbot - w_msg.xtop) / 8;
    j = w_msg.xtop + 4;
    k = w_msg.ytop + txtpar.font_height;
    fft_lev = pickat(tone.freq);
    gprintf(j, k, "FFT lev = %-4.1f dBSPL ", fft_lev);
    k += txtpar.font_height;
    gprintf(j, k, "P-P lev = %-4.1lf ppdBSPL ", vtodb(sig_v.vpp));
    k += txtpar.font_height;
    gprintf(j, k, "        = %-6.2f %s ", units_conv(sig_v.vpp),
    distype.strs[distype.at]);
    k += txtpar.font_height;
    gprintf(j, k, "DC val  = %-6.2f %s ", units_conv(sig_v.dco),
    distype.strs[distype.at]);
    k += txtpar.font_height;
    gprintf(j, k, "RMS lev = %-4.1lf dBSPL", vtodb(sig_v.rms));
    k += txtpar.font_height;
    gprintf(j, k, "RMS in  = %7.1f cnt ", sig_v.rms * Sen.AD);
    k += txtpar.font_height;
    gprintf(j, k, "        = %-6.2f %s ", units_conv(sig_v.rms),
    distype.strs[distype.at]);
    k += txtpar.font_height;
    gprintf(j, k, "RMS out = %7.1f cnt ", rms_out);
}

static float mic_v  = 1, rec_v[2] = {1, 1};

int
cmp_ad_sens(float **v)
{
    if ((sig_v.rms > 0) && (mic_v > 0)) {
        Sen.AD *= sig_v.rms / mic_v;
        sig_v.rms = mic_v;
        dis_units(Sen.AD, Sen.MP);
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

void
spec_volt()
{
    mic_v = sig_v.rms;
    rec_v[0] = (float) (rms_out / Sen.DA[0]);
    rec_v[1] = (float) (rms_out / Sen.DA[1]);
    preamp_voltage[1].status = (char) !play_chan.at;
    preamp_voltage[2].status = (char) play_chan.at;
    simple_submenu(menu_x(), menu_y(), preamp_voltage);
}

/* graphical display of the tone signal */
void
show_tone()
{
    int     cnt = 0, mx, my;
    short   data;

    dsprst(FALSE);
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
		disply(swp1set, (float *) outbuf, buflen, 1, 1);
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
		R_clear();
	    } else if (data == 'v') {
		spec_volt();
	    } else if (data == SINGLE_CLICK) {
	    	mouse_position(&mx, &my);
		if (mx < w_sig.xtop || mx > w_sig.xbot ||
		    my < w_sig.ytop || my > w_sig.ybot) {
		    esc_flg = 1;
		    break;
		}
	    }
	}
    }
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
    dsp_scale[c] = dspsets(mag / MAXPOSINT);
    amp = limit(1, mag / dsp_scale[c], MAXPOSINT);
    dp = TWOPI * f / rate;
    for (i = 0; i < buflen; i++)
	outbuf[i] = nint(amp * sin(dp * i));
    return (amp * dsp_scale[c] / sqrt(2.0));
}

/* do the tone task */
int
do_tone()
{
    int     nsets_s, swp1set_s, chan, ramp;
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

    dsprst(TRUE);
    ramp = nint(tone.ramp_ms * rate / 1000);
    downldpar(2, buflen, tone.skips, ramp);
    rms_out = gen_tone(tone.freq, chan);
    ld_stim(chan, outbuf, buflen);
    dspsarr(dsp_scale[chan], chan);
    zero_all();
    ld_stim(1 - chan, outbuf, buflen);
    dsprst(FALSE);
    rec_v[chan] = (float) (rms_out / Sen.DA[chan]);
    draw_w(&w_msg);
    esc_flg = 0;
    while (esc_flg == 0) {
//        if (rec_v[chan] != (rms_out / Sen.DA[chan])) {
//           dsprst(TRUE);
//            rms_out = gen_tone(tone.freq, chan);
//            ld_stim(chan, outbuf, buflen);
//            dspsarr(dsp_scale[chan], chan);
//            dsprst(FALSE);
//            rec_v[chan] = (float) (rms_out / Sen.DA[chan]);
//        }
	show_tone();
    }
    swp1set = swp1set_s;
    nsets = nsets_s;
    return 0;
}

/* this function is associated to the mainmenu TONE item */
void 
Tone_wind()
{
    if (dsp_init() > 0)
        return;
    top_message("Tone    ");
    Sen = Sys;
    dis_units(Sen.AD, Sen.MP);
    rate = tone.rate = adjust_rate(tone.rate);
    buflen = tone.size;
    tone.freq = (float) adjustf(tone.freq);
    draw_w(&w_spec);
    draw_w(&w_info);
    simple_submenu(menu_x(), menu_y(), tone_menu);
    top_message("        ");
}
