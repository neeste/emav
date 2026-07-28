/* tone_w.c */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <dsp.h>
#include <menu.h>
#include <util.h>
#include "savage.h"
#include "screen.h"
#include "abrav.h"		/* function prototyes */

#define TWOPI		(2 * M_PI)

int     disply(double, float *, int, int, double, MARK *mark, int);
int 	do_tone();
int     Option_wind();
int32_t    adjust_rate(int32_t);
void    clear_display(void);
void    downldpar(int, int, int, int);
void    dis_info();
void    text_color(int i, int n);
void    top_message(char *);

extern double dsp_scale;
extern int display_reset, swp1set, maxnpts;
extern struct tone tone;
extern TOGGLETYPE distype;

TOGGLETYPE play_mode = {
    0,
    { "Repeated", "Once", NULL },
};
TOGGLETYPE play_chan = {
    0,
    { "A", "B", NULL },
};

static int esc_flg = 0;

/* make the tone option current */
void
grab_tone()
{
    dis_units(Sen.AD, Sen.MP, 1.0);
    if (tone.size > maxnpts) {
	tone.size = maxnpts;
    }
    buflen = tone.size;
    rate = tone.rate = adjust_rate(tone.rate);
}

void
zero_all()
{
    int     i;

    for (i = 0; i < buflen; i++) {
	accbuf_1[i] = 0;
	accbuf_2[i] = 0;
	outbuf[i] = 0;
    }
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
	d = a * i;
    }
    tone.duration = d;
    return 0;
}

MENUITEM tone_menu[] = {
    {"&Start Tone", NULL, NONE, 0, 0, 1, do_tone},
    {"Tone &Frequency (Hz) :", (char *) &tone.freq, FLOAT, 10, 3, 1, adjust_tone_f},
    {"Sound &Level (dBSPL) :", (char *) &tone.level, FLOAT, 10, 2, 1, NULL},
    {"&Duration (sec) :", (char *) &tone.duration, FLOAT, 10, 3, 1, chk_due},
    {"AD &Channel =", (char *) &play_chan, TOGGLE, 0, 0, 1, NULL},
//    {"&Mode =", (char *) &play_mode, TOGGLE, 0, 0, 1, NULL},
//    {"&Options...", NULL, NONE, 0, 0, 1, Option_wind},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
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
    (void) gprintf(j, k, " %-5.1f dBSPL ", pickat(tone.freq, 0));
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

/* graphical display of the tone */
void
show_tone()
{
    double  tms;
    int     cnt = 0, mx, my;
    int32_t   *lbuf;
    short   data;

    tms = 1000 * (double) buflen / rate;
    lbuf = (int32_t *) calloc(buflen, sizeof(int32_t));
    dfgc = scrn_c[C_WAVE];
    dsprst(FALSE);
    while(cnt < nsweeps) {
	if (dspdget()) {
 	    dspdread(&data);
	    if (data <= 0) {
		dsprst(TRUE);
		dspuarr(dsppar.accbuf_a, buflen * 2, (short *) lbuf);
		if (++cnt < nsweeps) {
		    dsprst(FALSE);
		}
		long2float((int32_t *)lbuf, buflen);
		disply((double) swp1set, (float *) lbuf, buflen, 1, 0.0, NULL, D_RMDC);
		show_xlabel(0.0, tms, 0, "ms");
		fft_stim((float *) lbuf, 1, 0);
		dis_tone();
		show_fft(10, 80, 0, scrn_c[C_WAVE], 1);
	    }
	}
	if (check_event()) {
	    data = getevent();
	    if (data == 3 || data == 27) {
		esc_flg = 1;
		break;
	    } else if (data == ' ') {
		clear_display();
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
    dsprst(TRUE);
    free(lbuf);
}

/* generate tone stimulus into a buffer */
static void
gen_tone(float f, int c)
{
    int     i;
    double  mag, osen, ref, dp, am, scl, psc;

    osen = Sen.LS[c] * Sen.DA[c];                   /* cnt/Pa */
    ref = sqrt(2.0) * spl_ref * osen;               /* pk @ 0 dB SPL */
    mag = pow(10.0, tone.level / 20.0) * ref;
    tone.level = (float) (20 * log10(mag / ref));
    scl = mag / MAXPOSINT;
    dsp_scale = dspsets(scl);
    psc = scl / dsp_scale;
    am = limit(0, MAXPOSINT * psc, MAXPOSINT);
    dp = TWOPI * f / rate;
    for (i = 0; i < buflen; i++) {
	outbuf[i] = nint(am * sin(dp * i));
    }
}

/* do the tone task */
int
do_tone()
{
    int     nsweeps_s, swp1set_s, c;
    float   a;
    unsigned short dac0, dac1;

    swp1set_s = swp1set;
    nsweeps_s = nsweeps;
    a = (tone.duration / buflen) * rate;
    nsweeps = 1;
    if (a < 32000) {
	swp1set = (int) (a + 0.5);
    } else {
	do {
	    nsweeps++;
	} while ((a / nsweeps) > 32000.0);
	swp1set = (int) (a / nsweeps + 0.5);
    }
    clear_display();
    c = play_chan.at;

    dsprst(TRUE);
    downldpar(2, buflen, 1, 0);
    dac0 = c ? dsppar.dac_a : dsppar.dac_b;
    dac1 = c ? dsppar.dac_b : dsppar.dac_a;
    zero_all();
    dspdarr(dac0, buflen, outbuf);
    gen_tone(tone.freq, c);
    dspdarr(dac1, buflen, outbuf);
    dspsarr(dsp_scale, c);
    draw_w(&w_msg);
    esc_flg = 0;
    do {
	show_tone();
    } while (esc_flg == 0);
    swp1set = swp1set_s;
    nsweeps = nsweeps_s;
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
