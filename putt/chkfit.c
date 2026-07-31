#include <stdlib.h>
/* chkfit.c */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include <dsp.h>
#include <menu.h>
#include <util.h>
#include "savage.h"
#include "color.h"
#include "screen.h"
#include "putt.h"

WIND    w_stim;
WIND    w_sig;
WIND    w_ind;
WIND    w_info;
WIND    w_text;
WIND    w_msg;
WIND    w_spec;
WIND    w_stim_fft;

extern float resid_vpp;
extern int swp1set;
extern int xpix, ypix;
extern int scope;

double  cond_peak(double, double);
double  pick_min(double, double);
double  get_ph(double, double);
int     alloc_space(int);
int     check_timing();
int     dis_probe();
int     fft_short(short *, int);
int     Info_wind(void);
int     Option_wind(void);
int     recalstim(int);
int     thev_quick_load();
void    filter_it(int32_t *, int);
void    reset_timing(void);
void    take_the_aver(int, int32_t *);

extern int prb_flg, prb_cav, g_ncav;
extern int limits;
extern TOGGLETYPE distype, thv_chk;

float   z1_freq = 0, notch_freq = 0;
int     fft_flag = 0, stim_flag = 0;
int     display_reset = 0;
char    auto_flag = 'M';

int check_fit_flag = 0;

static double f10 = 10000, ph10 = 0, d10 = 0;
static double hob = 0.7071067811865;
static double ln2 = 0.6931471805599;
static int chk_ramp = 0;

int
dis_stim(int load, void *p, enum TYPE type)
{
    int     num, min;

    if (display_reset) {
	draw_w(&w_stim);
	draw_w(&w_stim_fft);
    }
    w_dis = &w_stim;
    if (scope < buflen)
        num = scope;
    else
        num = buflen;
    min = buflen / 8;
    if (num < min)
	num = min;
    if (type == FLOAT) {
	disply(swp1set, p, num, display_reset, 1);
	min = fft_sig(p, 1);
	modulesqr();
    } else if (type == INT) {
	display(p, num, display_reset, 1, 1);
	min = fft_short((short *) p, 0);
    }
    w_dis = &w_sig;
    stim_flag = 1;
    show_fft(probe.fft_xrange, probe.fft_yrange, probe.levref,
	dfgc, display_reset);
    stim_flag = 0;
    if (load && type == FLOAT)
	    memcpy(sav_stim, p, buflen * sizeof(float));

    return (min);
}

void
stim_title(char *file)
{
    int     i;

    w_stim.ytop = w_stim_fft.ytop = txtpar.menu_height + 1;
    w_stim.ybot = w_stim_fft.ybot = txtpar.menu_height * 2;

    i = w_stim.ybot - 3;
    gprintf(draw_w(&w_stim), i, "Stim: %s", file);
    gprintf(draw_w(&w_stim_fft), i, "Spec:");
    w_stim.ytop = w_stim_fft.ytop = w_stim.ybot + 1;
    w_stim.ybot = w_stim_fft.ybot = w_stim.ytop + 7 * txtpar.font_height + 1;
}

void
init_wind()
{
    w_text.xtop = 100;
    w_text.xbot = xpix - 100;
    w_text.ybot = ypix - txtpar.menu_height;
    w_text.ytop = w_text.ybot - 7 * txtpar.font_height;
    wind_color(&w_text, scrn_c[C_TXTF], scrn_c[C_TXTB]);

    w_stim.xtop = 0;
    w_stim.xbot = xpix / 4;
    w_stim.ytop = txtpar.menu_height * 2 + 1;
    w_stim.ybot = w_stim.ytop + 7 * txtpar.font_height + 1;
    wind_color(&w_stim, scrn_c[C_STMF], scrn_c[C_STMB]);
    draw_w(&w_stim);

    w_stim_fft.xtop = w_stim.xbot + 1;
    w_stim_fft.xbot = (xpix * 7) / 16;
    w_stim_fft.ytop = w_stim.ytop;
    w_stim_fft.ybot = w_stim.ybot;
    wind_color(&w_stim_fft, scrn_c[C_STMF], scrn_c[C_STMB]);
    draw_w(&w_stim_fft);

    w_info.xtop = w_stim_fft.xbot + 1;
    w_info.ytop = txtpar.menu_height + 1;
    w_info.xbot = xpix - 1;
    w_info.ybot = w_stim.ybot;
    wind_color(&w_info, scrn_c[C_INFF], scrn_c[C_INFB]);
    draw_w(&w_info);

    w_sig.xtop = 0;
    w_sig.xbot = (xpix * 2) / 3 - 1;
    w_sig.ytop = w_stim.ybot + 1;
    w_sig.ybot = (w_sig.ytop + ypix) / 2 - 1;
    wind_color(&w_sig, scrn_c[C_RSPF], scrn_c[C_RSPB]);
    draw_w(&w_sig);

    w_ind.xtop = w_sig.xbot + 1;
    w_ind.xbot = xpix - 1;
    wind_color(&w_ind, scrn_c[C_INDF], scrn_c[C_INDB]);
    pre_dis_ind("Artifact", "Limit");
    w_dis = &w_sig;

    w_spec.xtop = w_sig.xbot + 1;
    w_spec.xbot = xpix - 1;
    w_spec.ytop = w_sig.ytop;
    w_spec.ybot = w_sig.ybot;
    wind_color(&w_spec, scrn_c[C_RFTF], scrn_c[C_RFTB]);
    draw_w(&w_spec);
    gprintf(w_spec.xtop + 4, w_spec.ytop + txtpar.font_height, "Spec:");
    w_spec.ytop += txtpar.menu_height;

    w_msg.xtop = w_spec.xtop;
    w_msg.xbot = w_spec.xbot;
    w_msg.ytop = w_spec.ybot + 1;
    w_msg.ybot = w_ind.ytop - 1;
    wind_color(&w_msg, scrn_c[C_MSGF], scrn_c[C_MSGB]);
    draw_w(&w_msg);
}

double
units_conv(double in)       /* in = V */
{
    if (distype.at)
	in /= Sen.MP;       /* Sen.MP = V/Pa */

    return (in * 1000);     /* return = mV or mPa */
}

int
dis_patient()
{
    int     i, k, j;
    char    str[82];

    text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / 8 - 1;
    i = w_info.xtop + 4;
    k = w_info.ytop + txtpar.font_height;
    sprintf(str, "Name:  %s, %s", patient.Lastname, patient.Firstname);
    for (j = strlen(str); j < txtpar.text_wind_len; j++)
	str[j] = ' ';
    str[j] = 0;
    gprintf(i, k, str);
    k += txtpar.font_height;
    sprintf(str, "Ear: %c, ID: %s, DOB: %s", patient.Ear, patient.ID,
	patient.Dob);
    for (j = strlen(str); j < txtpar.text_wind_len; j++)
	str[j] = ' ';
    str[j] = 0;
    gprintf(i, k, str);
    k += txtpar.font_height;
    sprintf(str, "%s", patient.Comment);
    for (j = strlen(str); j < (txtpar.text_wind_len - 10); j++)
	str[j] = ' ';
    str[j] = 0;
    gprintf(i, k, "Comments: %s", str);
    return (k);
}

void
dis_cali()
{
    double  u, hobl, chng;
    int     i, j, k;

    i = w_info.xtop + 4;
    if (prb_flg)
	j = dis_probe();
    else
	j = dis_patient() + 2;

    u = units_conv(sig_v.vpp);
    if (u < 1)
	k = 2;
    else if (u < 10)
	k = 1;
    else
	k = 0;
    gprintf(i, j, "Measured Value: %5.1lf dB ppSPL = %4.*f%s ",
	vtodb(sig_v.vpp), k, u, distype.strs[distype.at]);
    j += txtpar.font_height;
    gprintf(i + 192, j, " fz1=%5.0f Hz", z1_freq);
    j += txtpar.font_height;
    if (notch_freq > 0) {
        hobl = hob * notch_freq;
        chng = (z1_freq  > 0) ? log(z1_freq / notch_freq) / ln2 : 0;
        gprintf(i, j, "hobl = %5.0f Hz  chng=%5.2f oct  ", hobl, chng);
    }
}

void
dis_info()
{
    int     k, i;
    double  val, hobl, chng;

    i = w_info.xtop + 4;
    if (!prb_flg)
	k = dis_patient() + 2;
    else
	k = dis_probe();

    text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
    gprintf(i, k, "Check Microphone Fit ..............");
    text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
    k += txtpar.font_height;
    gprintf(i, k, "Stimulus: %-13s Attenu: %4.1f dB ",
	ilstim, attenu.att1);
    k += txtpar.font_height;
    val = units_conv(sig_v.vpp);
    gprintf(i, k, "pk  = %6.2f%s = %5.1lf dB ppSPL ",
	val, distype.strs[distype.at], vtodb(sig_v.vpp));
    k += txtpar.font_height;
    gprintf(i, k, "rms = %6.2f%s = %5.1lf dB SPL ",
	units_conv(sig_v.rms), distype.strs[distype.at], vtodb(sig_v.rms));
    k += txtpar.font_height;
    gprintf(i, k, "min/max/dc = %5.1f/%5.1f/%5.1f%s ",
	units_conv(sig_v.npk), units_conv(sig_v.ppk),
	units_conv(sig_v.dco), distype.strs[distype.at]);
    k += txtpar.font_height;
    gprintf(i, k, "fz1 = %5.0f Hz ", z1_freq);
    if (prb_flg) {
        gprintf(i + 120, k, "    d10 = %5.1f usec ", d10);
    } else if (notch_freq > 0) {
        hobl = hob * notch_freq;
        chng = (z1_freq  > 0) ? log(z1_freq / notch_freq) / ln2 : 0;
        gprintf(i + 120, k, " hobl = %5.0f chng=%5.2f ", hobl, chng);
    }
}

/* invokes option from the checkfit or calibrate phase */
int
O_options(char *no_use)
{
    void    (*func) ();

    func = get_bg_exe();
    set_bg_exe((void (*) ()) 0);
    Option_wind();
    set_bg_exe(func);
    dsprst(TRUE);
    downldpar(1, buflen, probe.chk_skps, chk_ramp);
    recalstim(0);
    ld_stim(0, outbuf, buflen);
    dsprst(FALSE);
    return (' ');
}

/* invokes the Info window from the checkfit or calibrate phase */
int
I_info(char *no_use)
{
    void    (*func) ();

    func = get_bg_exe();
    set_bg_exe((void (*) ()) 0);
    Info_wind();
    set_bg_exe(func);
    draw_w(&w_info);
    return (' ');
}

/* clears the current display box */
int
R_clear()
{
    mouse_exclude(w_dis->xtop, w_dis->ytop, w_dis->xbot, w_dis->ybot);
    gr_rectf(w_dis->xtop + 1, w_dis->ytop + 1, w_dis->xbot - 1, w_dis->ybot - 1,
	w_dis->wbgc);
    mouse_show(1);
    reset_timing();
    display_reset = 1;
    return (FN | 72);
}

/* continuation function for the menus */
int
C_continue(char *no_use)
{
    if (!prb_flg || !prb_cav)
        cali_file[0] = 0;
    check_fit_flag = 1;
    return (get_resume());
}

int
E_continue(char *no_use)
{
    check_fit_flag = 1;
    return (get_resume());
}

int
E_cancel(char *no_use)
{
    return (FN | 113);
}

MENUITEM bwt[] = {
    {"&Continue", NULL, NONE, 0, 0, 1, C_continue},
    {"Space& Refresh", NULL, NONE, 0, 0, 1, R_clear},
    {"Patient &Info...", NULL, NONE, 0, 0, 1, I_info},
    {"&Options...", NULL, NONE, 0, 0, 1, O_options},
    {"&Test...", NULL, NONE, 0, 0, 1, E_continue},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

void
chk_fit_upld()
{
    short     data;

    while (dspdget()) {
	dspdread(&data);
	if (data <= 0) {
	    dspuarr(dsppar.acc, buflen * 2, (short *) dspbuf);
	    dsprst(TRUE);
	    dsprst(FALSE);
            long2float((int32_t *) dspbuf, buflen);

            data = limit(buflen / 8, scope, buflen);
            if (check_timing())
                R_clear();
            dfgc = scrn_c[C_WAVE];
            text_color(scrn_c[C_RSPF], scrn_c[C_RSPB]);
	    if (disply(swp1set, (float *) dspbuf, data, display_reset, 0))
		dis_info();
	    fft_stim((float *) dspbuf, 1, 0);
	    z1_freq = (float) pick_min(hear.minfz, hear.maxfz);
            if (prb_flg) {
                ph10 = get_ph(f10, ph10);
                d10 = 1e6 * ph10 / f10;
            }
            if (thv_chk.at)
                display_reset = 1;
	    show_fft(probe.fft_xrange, probe.fft_yrange, probe.levref,
	        scrn_c[C_WAVE], display_reset);
            if (thv_chk.at) {
                display_reset = 1;
	        if (thev_quick_load())
                    z1_freq = (float) cond_peak(hear.minfz, hear.maxfz);
	    }
            display_reset = 0;
	}
    }
}

int
check_fit()
{
    swp1set = probe.chk_swps;
    attenu.att1 = probe.cal_atten;
    ph10 = 0;
    R_clear();
    gr_rectf(w_sig.xtop, w_sig.ybot + 1, w_sig.xbot, ypix - 1, txtpar.cbg);
    gr_recto(w_sig.xtop, w_sig.ybot + 1, w_sig.xbot, ypix - 1, menucolor.boxc);
    draw_w(&w_info);
    if (!stim_flag)
	draw_w(&w_spec);

    zero_all();
    recalstim(0);
    dsprst(TRUE);
    downldpar(1, buflen, probe.chk_skps, chk_ramp);
    dspdarr(dsppar.dac_a, buflen, outbuf);
    dsprst(FALSE);

    fft_flag = 1;
    R_clear();
    thev_free();
    set_bg_exe(chk_fit_upld);
    if (prb_flg) {
	bwt[4].status = 0;
    } else {
	bwt[4].status = cali_file[0];
    }
    check_fit_flag = 0;
    simple_submenu(w_msg.xtop, w_msg.ytop, bwt);
    set_bg_exe((void (*) ()) 0);
    fft_flag = 0;

    dsprst(TRUE);
    return (check_fit_flag);
}

double
dbv(double v)
{
    if (v < 1e-39)
	return (-800.0);
    return (20 * log10(v));
}

double
vtodb(double v)
{
    return (dbv(v / (spl_ref * Sen.MP)));
}
