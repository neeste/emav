/* abr_w.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include <ctype.h>
#include <time.h>
#include <dsp.h>
#include <menu.h>
#include <token.h>
#include <util.h>
#include "abrav.h"
#include "savage.h"
#include "color.h"
#include "screen.h"
#include "version.h"

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#define _access access
#endif /* WIN32 */

#define MAXLEVS     64

int     adjust_window(void);
int     check_list(char *, char **);
int     check_rej(float *);
int     check_timing();
int     display(short *, int, int, int, int);
int     disply(double, float *, int, int, double, MARK *, int);
int     dsp_init(void);
int     fft_short(short *);
int     fft_sig(float *, int);
int     fft_stim(float *, int, int);
int     num_params = 0;
int     open_file(char *use2chk);
int     Quit_wind(char *);
int32_t    adjust_rate(int32_t);
void    abr_aver(int);
void    cal_gain(SAV_FFT *, int, double);
void    dis_units(double, double, double);
void    downldpar(int, int, int, int);
void    get_token(int);
void    init_edit_msg();
void    pre_dis_ind(char *, char *), dis_artifact();
void    rd_cal_file(char *);
void    recalstim(int);
void    reset_timing();
void    show_fft(int, int, int, int, int);
void    text_color(int, int);
void    tok_init(int);
void    tok_store(int, int, float *);
void    top_message(char *);
void    wind_color(WIND *, int, int);
void    zero_all(), modulesqr();
void trim(char *s);
void	points_msg(void);
void	proc_event();

struct BINhdr binhdr;
struct BINwav current_wav;

extern char *abr_tmp_name;
extern char ilstim[];
extern char line[MAXLINE], tokstr[40], cali_file[MAXNAME];
extern double dsp_scale;
extern float time1_ms, time2_ms;
extern float ad_sens;
extern float max_level, nhl_level, test_level;
extern float repr_crit1, repr_crit2, Fsp_crit1, Fsp_crit2;
extern int ep_chan, ep_chan_cfg;
extern int swp1set;
extern int xpix, ypix;
extern int mic_chan, ad_pol;
extern int dc_offset_mic;
extern int ctrl_level;
extern int calibr_flag;
extern int cal_type;
extern int make_template, trkbuf;
extern int recflg, rej_mod, bin_count;
extern struct abr abr_;
extern struct stim stim_;
extern struct LEV_DEP L30, L50, L70;
extern struct AUX_INF ai;
extern FILE *binfp;
extern MENUITEM file_opt[];
extern REJ_IND t_ind;
extern TOGGLETYPE distype, r_mode, rejmod, sqm;
extern TOGGLETYPE stimType, windowType, maskerType, calType, epa_gain;
extern TOKENFILE *t;

double response_level[2];
float  *sav_stim;
float   tolerance = 3.0;	/* dB */
int     fft_flag = 0, stim_flag = 0, series_flag = 0;
int     display_reset = 0;
int     differ_flag = 0;
int     noreject = 0;
int32_t    check_fit_run_time = 0;
WIND    w_stim, w_sig, w_ind, w_info, w_text, w_msg, w_spec, w_stim_fft;

static char list_file[MAXPATH];
static char *cal_stim = "*chirp*";
static double chk_sec = 0.008; /* time span of chk_fit display */
static float msk_ramp = 0;
static int nreps = 0, nlevs = 0, stim_lev[MAXLEVS] = {0}, chk_dat = 0;
static int list_type = 0, list_items = 0;
static int32_t series_start_time = 0;
static MENUITEM lev_opt[] = {
    {"&Level (dBnHL) :", (char *) &test_level, FLOAT, 5, 1, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 1, NULL},
};
static SIG_V sav_v;
static SAV_FFT A_fft, B_fft;

void
grab_abr()
{
    static int stmtyp[3] = {1, 2, 3};
    static int msktyp[3] = {0, 5, 4};

    rate = adjust_rate(abr_.rate);
    ep_chan = ep_chan_cfg;
    ad_sens = Sen.AD * ad_pol;
    trkbuf = make_template ? 3 : 1;
    rej_mod = rejmod.at;
    test_level = abr_.stim_lev;
    stim_.rate = abr_.rate;    
    stim_.size = abr_.size;
    stim_.type = (short) stimType.at;    
    if(stim_.type == 0) {
        stim_.sig_level[0] = abr_.stim_lev;
	strcpy(ilstim, abr_.stim_file);
    } else if(stim_.type == 1) {
        stim_.sig_level[0] = abr_.clk_lev;
        stim_.sig_dur[0] = abr_.clk_dur;
	sprintf(ilstim, "Click:%.1fms", stim_.sig_dur[0]);
    } else if(stim_.type == 2) {
        stim_.sig_level[0] = abr_.sig_lev;
        stim_.sig_dur[0] = abr_.sig_dur;
	sprintf(ilstim, "Tone:%.1fms", stim_.sig_dur[0]);
    }
    stim_.sig_type[0] = stmtyp[stimType.at];
    stim_.sig_type[1] = msktyp[maskerType.at];
    stim_.sig_win[0] = (short) windowType.at;
    stim_.sig_win[1] = 0;
    stim_.sig_start[0] = abr_.sig_start;
    stim_.sig_start[1] = 0;
    stim_.sig_freq[0] = abr_.sig_freq;
    stim_.sig_freq[1] = abr_.msk_freq;
    stim_.sig_ramp[0] = 0;
    stim_.sig_ramp[1] = abr_.msk_ramp;
    stim_.sig_phase[0] = 0;
    stim_.sig_phase[1] = 0;
    stim_.sig_atten[0] = attenu.att1;
    stim_.sig_atten[1] = attenu.att2;
    strcpy(list_file, abr_.list_file);
}

double
stability(int show, float *p, float *q, int num)
{
    int     i, min;
    double   Sx = 0, Sy = 0, Sxx = 0, Syy = 0, Sxy = 0;
    double   u, v;

    min = buflen / 8;
    if (num < min)
	num = min;
    for (i = 0; i < num; i++) {
	v = p[i];
	u = q[i];
	Sy += u;
	Syy += u * u;
	Sx += v;
	Sxx += v * v;
	Sxy += v * u;
    }
    v = Sx / (float) num;
    u = Sy / (float) num;
    Sxx -= Sx * v;
    Syy -= Sy * u;
    Sxy -= Sx * u;
    if ((Sxx <= 0) || (Sy <= 0)) {
	v = 0;
    } else {
	v = Sxy / sqrt(Sxx * Syy);
	v *= 100.0;
    }
    if (v < 0)
	v = 0;
    i = (v > 99.9) ? 0 : 1;
    if (show) {
	text_color(w_stim.wfgc, w_stim.wbgc);
	(void) gprintf(w_stim.xbot - 80, w_stim.ybot - 2, "Stab %4.*f%%", i, v);
    }
    return (v);
}

int
dis_stim(int load, void *p, enum TYPE type)
{
    int     num, min;

    if (display_reset) {
	(void) draw_w(&w_stim);
	(void) draw_w(&w_stim_fft);
    }
    w_dis = &w_stim;
    num = nint(0.005 * rate);
    min = buflen / 8;
    if (type == LONG) {
	(void) disply((double) swp1set, p, num, display_reset, 0.0, NULL, 0);
	min = fft_sig(p, 1);
	modulesqr();
    } else if (type == INT) {
	(void) display(p, num, display_reset, 0, 0);
	min = fft_short(p);
    }
    w_dis = &w_sig;
    stim_flag = 1;
    show_fft(abr_.fft_xrange, abr_.fft_yrange, abr_.levref,
	dfgc, display_reset);
    stim_flag = 0;
    if (load) {
	memcpy(&sav_v, &sig_v, sizeof(SIG_V));
	if (type == LONG)
	    memcpy(sav_stim, p, buflen * sizeof(float));
    }
    return (min);
}

void
stim_title(char *file)
{
    int     i;

    w_stim.ytop = w_stim_fft.ytop = txtpar.menu_height + 1;
    w_stim.ybot = w_stim_fft.ybot = txtpar.menu_height * 2;

    i = w_stim.ybot - 3;
    (void) gprintf(draw_w(&w_stim), i, "Stim: %s", file);
    w_stim.ytop = w_stim.ybot + 1;
    w_stim.ybot = w_stim_fft.ybot = w_stim.ytop + 7 * txtpar.font_height + 1;
}

void
open_stim_w(char *file)
{
    stim_title(file);
    strcpy(ilstim, file);
    display_reset = 1;
    if (strcmp(file, cal_stim) == 0) {
	stim_.sig_type[0] = 6;
    } else if (*file >= ' ') {
	stim_.sig_type[0] = 1;
    } else {
	stim_.sig_type[0] = 0;
    }
    stim_.sig_atten[0] = 0;
    recalstim(0);
    dfgc = scrn_c[C_WAVE];
    display_reset = 1;
    (void) dis_stim(1, outbuf, INT);
}

void
init_spec()
{
    w_spec.ytop = w_msg.ybot + 1;
    (void) draw_w(&w_spec);
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
    (void) draw_w(&w_stim);

    w_stim_fft.xtop = w_stim.xbot + 1;
    w_stim_fft.xbot = (xpix * 7) / 16;
    w_stim_fft.ytop = w_stim.ytop;
    w_stim_fft.ybot = w_stim.ybot;
    wind_color(&w_stim_fft, scrn_c[C_STMF], scrn_c[C_STMB]);
    (void) draw_w(&w_stim_fft);

    w_info.xtop = w_stim_fft.xbot + 1;
    w_info.ytop = txtpar.menu_height + 1;
    w_info.xbot = xpix - 1;
    w_info.ybot = w_stim.ybot;
    wind_color(&w_info, scrn_c[C_INFF], scrn_c[C_INFB]);
    (void) draw_w(&w_info);

    w_sig.xtop = 0;
    w_sig.ytop = w_stim.ybot + 1;
    w_sig.xbot = (xpix * 13) / 16 - 1;
    w_sig.ybot = ypix - 1;
    wind_color(&w_sig, scrn_c[C_RSPF], scrn_c[C_RSPB]);
    (void) draw_w(&w_sig);

    w_ind.xtop = w_sig.xbot + 1;
    w_ind.xbot = xpix - 1;
    wind_color(&w_ind, scrn_c[C_INDF], scrn_c[C_INDB]);
    pre_dis_ind("Artifact", "Limit");
    w_dis = &w_sig;

    w_msg.xtop = w_ind.xtop;
    w_msg.xbot = w_ind.xbot;
    w_msg.ytop = w_info.ybot + 1;
    w_msg.ybot = w_msg.ytop + 8 * txtpar.menu_height;
    wind_color(&w_spec, scrn_c[C_RFTF], scrn_c[C_RFTB]);
    (void) draw_w(&w_msg);

    w_spec.xtop = w_ind.xtop;
    w_spec.xbot = w_ind.xbot;
    w_spec.ybot = w_ind.ytop - txtpar.menu_height - 1;
    wind_color(&w_msg, scrn_c[C_MSGF], scrn_c[C_MSGB]);
    init_spec();
}

double
vtodb(double v)
{
    return (dbv((v / Sen.MP) / spl_ref));
}

double
dbv(double v)
{
    if (v < 1e-40)
	return (-800.0);
    return (20 * log10(v));
}

double
db_(double p)
{
    if (p < 1e-40)
	return (-400.0);
    return (10 * log10(p));
}

void
cmul(float *a, float *b, float *c)
{				/* complex multiply */
    float   cr, ci;

    cr = (a[0] * b[0] - a[1] * b[1]);
    ci = (a[1] * b[0] + a[0] * b[1]);
    c[0] = cr;
    c[1] = ci;
}

void
cdiv(float *a, float *b, float *c)
{				/* complex divide */
    float   m, cr, ci;
    static float e = (float) 1e-9;

    m = b[0] * b[0] + b[1] * b[1];
    m = (m > e) ? m : e;
    cr = (a[0] * b[0] + a[1] * b[1]) / m;
    ci = (a[1] * b[0] - a[0] * b[1]) / m;
    c[0] = cr;
    c[1] = ci;
}

double
units_conv(double in)
{
    if (distype.at) {
	in /= Sen.MP;
    }
    in *= 1000;
    return (in);
}

int
dis_patient()
{
    int     i, k, j;
    char    str[82];

    text_color(w_info.wfgc, w_info.wbgc);
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / 8 - 1;
    i = w_info.xtop + 4;
    k = w_info.ytop + txtpar.font_height;
    sprintf(str, "Name:  %s, %s", patient.Lastname, patient.Firstname);
    for (j = strlen(str); j < txtpar.text_wind_len; j++)
	str[j] = ' ';
    str[j] = 0;
    (void) gprintf(i, k, str);
    k += txtpar.font_height;
    sprintf(str, "Ear: %c, ID: %s, DOB: %s", patient.Ear, patient.ID,
	patient.Dob);
    for (j = strlen(str); j < txtpar.text_wind_len; j++)
	str[j] = ' ';
    str[j] = 0;
    (void) gprintf(i, k, str);
    k += txtpar.font_height;
    sprintf(str, "%s", patient.Comment);
    for (j = strlen(str); j < (txtpar.text_wind_len - 10); j++)
	str[j] = ' ';
    str[j] = 0;
    (void) gprintf(i, k, "Comments: %s", str);
    k += txtpar.font_height;
    return (k);
}

void
dis_info(int n)
{
    int     k, i;
    double  vpk;

    i = w_info.xtop + 4;
    k = dis_patient() + 2;
    text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
    if(n) {
	if(calibr_flag)
	    (void) gprintf(i, k, "Calibrating ...... Press ENTER when ready");
	else 
	    (void) gprintf(i, k, "Checking Microphone Fit ..............");
    } else {
	(void) gprintf(i, k, "Display calibration result.");
    }
    text_color(w_info.wfgc, w_info.wbgc);
    k += txtpar.font_height;
    (void) gprintf(i, k, "Stimulus: %-13s Attenu: %4.1f dB",
	ilstim, attenu.att1);
    k += txtpar.font_height;
    vpk = (sig_v.ppk > -sig_v.npk) ? sig_v.ppk : -sig_v.npk;
    (void) gprintf(i, k, "pk =%5.1lf dBnHL =%5.1lf dBpSPL = %6.1f%s ",
	vtodb(vpk) - nhl_level, vtodb(vpk), units_conv(vpk), 
	distype.strs[distype.at]);
    k += txtpar.font_height;
    (void) gprintf(i, k, "rms = %5.1lf dB SPL  dc =%5.1f%s  ",	
        vtodb(sig_v.rms), units_conv(sig_v.dco), distype.strs[distype.at]);
    k += txtpar.font_height;
    (void) gprintf(i, k, "min = %6.1f%s  max = %6.1f%s  ",
	units_conv(sig_v.npk), distype.strs[distype.at], 
	units_conv(sig_v.ppk), distype.strs[distype.at]);
}

void dis_pos_info(double lcurr, double vcurr, double lv, double vv, double lvp, double vvp, double t0)
{
    int j, k;
    char st[30], mt[30], unit[10];
    double dis_vcurr, dis_vv, dis_vvp;
    double mul, off;
    double vctmp, vvtmp, vvptmp;

    if(vcurr < 0)
        vctmp = -vcurr;
    else
        vctmp = vcurr;
    if(vv < 0)
        vvtmp = -vv;
    else
        vvtmp = vv;
    if(vvp < 0)
        vvptmp = -vvp;
    else
        vvptmp = vvp;
    
    if( (vctmp >= 0.1 || vctmp == 0) && (vvtmp >= 0.1 || vvtmp == 0) && (vvptmp >= 0.1 || vvptmp == 0) )
    {
        mul = 1;
        strcpy(unit, "V ");
    }
    else if( (vctmp >= 1e-3 || vctmp == 0) && (vvtmp >= 1e-3 || vvtmp == 0) && (vvptmp >= 1e-3 || vvptmp == 0) )
    {
        mul = 1000;
        strcpy(unit, "mV");
    }
    else if( (vctmp >= 1e-6 || vctmp == 0) && (vvtmp >= 1e-6 || vvtmp == 0) && (vvptmp >= 1e-6 || vvptmp == 0) )
    {
        mul = 1e6;
        strcpy(unit, "uV");
    }
    else
    {
        mul = 1e9;
        strcpy(unit, "nV");

    }
    off = sig_v.dco;
    vcurr -= off;
    dis_vcurr = vcurr * mul;
    if(vv != 0)
        vv -= off;
    dis_vv = vv * mul;
    if(vv != 0)
        vvp -= off;
    dis_vvp = vvp * mul;

    if(binhdr.list_type == 0)
    {
        strcpy(st, "Click");
        strcpy(mt, "No Mask");
    }
    else if(binhdr.list_type == 1)
    {
        strcpy(st, "Ramp Tone");
        strcpy(mt, "Cont Tone");
    }
    else if(binhdr.list_type == 2)
    {
        strcpy(st, "Ramp Tone");
        strcpy(mt, "Ramp Mask");
    }
    draw_w(&w_info);
    txtpar.text_wind_len = 60;
    j = w_info.xtop + 4;
    k = w_info.ytop + txtpar.font_height;
    gprintf(j, k, "Stim Type: Sig = %s, Mask = %s", st, mt );
    k += txtpar.font_height;
    gprintf(j, k, "t0 = %8.3f", t0);
    k += txtpar.font_height;
    gprintf(j, k, "             Curr     V      V'     V-V'");
    k += txtpar.font_height;
    gprintf(j, k, "Lat  (ms)%8.2f%8.2f%8.2f%8.2f", lcurr, lv, lvp, lv-lvp);
    k += txtpar.font_height;
    gprintf(j, k, "Volts(%s)%8.2f%8.2f%8.2f%8.2f", unit, dis_vcurr, dis_vv, dis_vvp, dis_vv-dis_vvp);
}

void
download()
{
    int j, k, t0, t1, skip = 0;

    if (r_mode.at != 2) {
        (void) dsprst(TRUE);
	// skip the first sweep if stimulus contains a tone or chirp
	t0 = stim_.sig_type[0];
	t1 = stim_.sig_type[1];
	if (t0 == 5 || t0 == 6 || t1 == 5 || t1 == 6)
	    skip = 1;
	downldpar(2, buflen, skip, 0);
	stim_.sig_phase[0] = 0;
        recalstim(0);
	(void) dspdarr(dsppar.dac_a, buflen, outbuf);
	(void) dspsarr(dsp_scale, 0);
        stim_.sig_phase[1] = 0;
        recalstim(1);
	(void) dspdarr(dsppar.dac_b, buflen, outbuf);
	(void) dspsarr(dsp_scale, 1);
	(void) dsprst(FALSE);
	if (stim_.sig_type[1] != 0) {	// alternate phase of masker
	    stim_.sig_phase[1] = 180;
	    downldpar(2, buflen, skip, 1);
	    recalstim(0);
	    (void) dspdarr(dsppar.dac_a, buflen, outbuf);
	    (void) dspsarr(dsp_scale, 0);
	    recalstim(1);
	    (void) dspdarr(dsppar.dac_b, buflen, outbuf);
	    (void) dspsarr(dsp_scale, 1);
	    (void) dsprst(FALSE);
	}
    }
    txtpar.text_wind_len = 30;
    text_color(scrn_c[C_MSGF], scrn_c[C_MSGB]);
    j = w_msg.xtop + 4;
    k = w_msg.ytop + txtpar.font_height * 9;
    (void) gprintf(j, k, "Atten= %.0f dB  ", stim_.sig_atten[0]);
}

static void
set_level(int tstlev)
{
    test_level = (tstlev > max_level) ? max_level : tstlev;
    attenu.att1 = attenu.att2 = max_level - test_level;
    stim_.sig_atten[0] = attenu.att1;
    stim_.sig_atten[1] = attenu.att2;
    download();
}

void
clear_display()
{
    mouse_exclude(w_dis->xtop, w_dis->ytop, w_dis->xbot, w_dis->ybot);
    gr_rectf(w_dis->xtop + 1, w_dis->ytop + 1, w_dis->xbot - 1, w_dis->ybot - 1,
	w_dis->wbgc);
    mouse_show(1);
    display_reset = 1;
    reset_timing();
}

int
proc_art(int i)
{
    if (i & FN) {
	switch (i) {
	case FN | 77:		/* arrow right */
	    upperv += 2;
	    break;
	case FN | 116:		/* control right */
	    upperv += 20;
	    break;
	case FN | 75:		/* arrow left */
	    upperv -= 2;
	    break;
	case FN | 115:		/* contrl left */
	    upperv -= 20;
	    break;
	case FN | 71:
	    upperv = (int) (0.8 * t_ind.new);
	    break;
	case FN | 79:		/* control end */
	    upperv = (int) (t_ind.new + 0.5);
	    break;
	}
	if (upperv < 0)
	    upperv = 0;
	t_ind.lim = (float) (upperv * 1000.0 / Sen.AD);
	dis_artifact();
    } else if (i == 27)
	return (FN | 113);
    return (0);
}

void
take_the_aver(int len, float *p)
{
    float   tmp;
    int     i;

    tmp = 0;
    for (i = 0; i < len; i++)
	tmp += p[i];
    tmp /= len;
    for (i = 0; i < len; i++)
	p[i] -= tmp;
}

void
flush_dsp()
{
    short     data;

    while (dspdget())
	(void) dspdread(&data);
}

void
chk_fit_upld()
{
    float  *resp_ptr = (float *) outbuf;
    short   data;
    unsigned int acc_loc;

    acc_loc = (mic_chan == 0) ? dsppar.accbuf_a : dsppar.accbuf_b;
    if (check_timing())
	clear_display();
    if (dspdget()) {
	(void) dspdread(&data);
	if (data <= 0) {
	    (void) dspuarr(acc_loc, buflen * 2, (short *) outbuf);
	    (void) dsprst(TRUE);
	    (void) dsprst(FALSE);
	    long2float((int32_t *) outbuf, buflen);
	    chk_dat = 1;
	    data = nint(chk_sec * rate);
            dfgc = scrn_c[C_WAVE];
            text_color(scrn_c[C_RSPF], scrn_c[C_RSPB]);
            w_dis = &w_sig;
	    if (disply((double)swp1set, resp_ptr, data, display_reset, 0.0, NULL, 0))
		dis_info(1);
	    fft_stim(resp_ptr, 1, 0);
	    show_fft(abr_.fft_xrange, abr_.fft_yrange, abr_.levref,
		scrn_c[C_WAVE], display_reset);
	    display_reset = 0;
	}
    }
}

int
check_fit()
{
    int     i, j, k;

    top_message("ABR check    ");
    txtpar.text_wind_len = 30;
    text_color(scrn_c[C_MSGF], scrn_c[C_MSGB]);
    j = w_msg.xtop + 4;
    k = w_msg.ytop + txtpar.font_height * 1;
    (void) gprintf(j, k, "Check Fit");
    k += txtpar.font_height;
    (void) gprintf(j, k, "space = refr.");
    k += txtpar.font_height;
    (void) gprintf(j, k, "1 - 9 = level");
    k += txtpar.font_height;
    (void) gprintf(j, k, "Enter = cont.");

    clear_display();
    (void) draw_w(&w_info);
    if (!stim_flag)
	(void) draw_w(&w_spec);

    swp1set = abr_.chk_swps;
    buflen = abr_.chk_size;
    stim_.sig_atten[0] = abr_.chk_atten;
    stim_.sig_atten[1] = abr_.chk_atten;
    download();
    zero_all();

    fft_flag = 1;
    clear_display();
    chk_dat = 0;
    check_fit_run_time = clock();
    for (;;) {
        chk_fit_upld();
        if (check_event()) {
            i = getevent();
            if (i == 'c' || i == 'C' || i == '\r') {
            	if (chk_dat) {
            	    i = 0;
            	    break;
            	}
            } else if (i == 3 || i == 27 || i == SINGLE_CLICK) {
            	break;
            } else if (i == 'l') {
 	        (void) simple_submenu(150, 250, lev_opt);
                set_level(nint(test_level));
            } else if (i == ' ') {
        	clear_display();
            } else if (i == 'n') {
                set_level(-90);
            } else if (i == '-') {
                set_level(-10);
            } else if (i >= '0' && i <= '9') {
                set_level((i - '0') * 10);
            }
        }
    }
    check_fit_run_time = clock() - check_fit_run_time;
    (void) draw_w(&w_msg);
    fft_flag = 0;
    top_message("             ");
    return (i);
}

/* read in header in data file */
void
proc_hdr()
{
    int     i, k;
    static char *ky[] = {
		"repeat", "Stimulus_Type", "Items", "Masker_Ramp", 
		"Window_Type", "Signal_Phase", "Sweeps_per_Set", 
		"Size", ""
    };
    //static char *stm[] = {"SMT", "FMT", ""};

    get_token(1);
    k = check_list(tokstr, ky);
    get_token(0);
    i = atoi(tokstr);
    switch (k) {
    case 1:			
		if (i > 0)
		    nreps = i;
		break;
    case 2:                     // Stimulus_Type
        list_type = atoi(tokstr);
        break;
    case 3:                     // items
        list_items = atoi(tokstr);
        //set number of stimulus params we should exepect
        break;
    case 4:                     // Masker_Ramp
        msk_ramp = (float) atof(tokstr);
        break;
    case 5:                     // Window_Type
        stim_.sig_win[0] = atoi(tokstr);
        break;
    case 6:                     // Signal_Phase
        stim_.sig_phase[0] = (float) atof(tokstr);
        break;
    case 7:                     // Sweeps_per_Set
	if (i > 0)
	    swp1set = i;
    case 8:                     // Size
	if (i > 0)
	    buflen = i;
        break;
    }
}

/* add data value to stim_lev list */
int
add_data()
{
    int i, c;

    if (nlevs < MAXLEVS) {
        for(i = 0; i < MAXLINE && isspace(line[i]); i++)
            continue;
        c = line[i];
        if (c == '-' || c == '+' || (c >= '0' && c <= '9')) {
            stim_lev[nlevs++] = atoi(line + i);
            return (1);
        }
    }
    return (0);
}

//this will read some values, create a stimParam struct and add it to the linked list
int 
add_param_list(struct stimParam r) 
{
    struct stimParam *p;
    int i = 1;

    p = (struct stimParam *) calloc(1, sizeof(struct stimParam));
    if(!head_param) {
        head_param = p;
    } else {
        struct stimParam *tmp = head_param;
        i++;
        //find the last link
        while(tmp->next) {
            tmp = tmp->next;
            i++;
        }
        tmp->next = p;
    }
    p->next = NULL;
    p->dur1 = r.dur1;
    p->dur2 = r.dur2;
    p->freq1 = r.freq1;
    p->freq2 = r.freq2;
    p->level1 = r.level1;
    p->level2 = r.level2;
    p->start = r.start;
    p->sweeps = r.sweeps;
    p->index = i;       //we're doing 1 indexed with this
    num_params++;

    return 0;
}

//this will read the "line" value, get the values, and pass them along to the add_param_list function
void 
read_param_list()
{
    float f1, l1, d1, s1, f2, l2, d2;
    int swps;
    struct stimParam r;

    if(list_type == 0) {	    // simultaneous masking
        if(strlen(line) > 6)
        {
            sscanf(line, "%f %f %d", &l1, &d1, &swps);
	    r.dur1 = d1;
	    r.dur2 = 0;
	    r.freq1 = 0;
	    r.freq2 = 0;
	    r.level1 = l1;
	    r.level2 = 0;
	    r.start = 0;
	    r.sweeps = swps;
	    add_param_list(r);
        }
    } else if(list_type == 1) {	    // simultaneous masking
        if(strlen(line) > 12)
        {
            sscanf(line, "%f %f %f %f %f %d", 
	        &f1, &l1, &d1, &f2, &l2, &swps);
	    r.dur1 = d1;
	    r.dur2 = 0;
	    r.freq1 = f1;
	    r.freq2 = f2;
	    r.level1 = l1;
	    r.level2 = l2;
	    r.start = 0;
	    r.sweeps = swps;
	    add_param_list(r);
        }
    } else if(list_type == 2) {	    // forward masking
        if(strlen(line) > 16)
        {
            sscanf(line, "%f %f %f %f %f %f %f %d",
	        &f1, &l1, &d1, &s1, &f2, &l2, &d2, &swps);
	    r.dur1 = d1;
	    r.dur2 = d2;
	    r.freq1 = f1;
	    r.freq2 = f2;
	    r.level1 = l1;
	    r.level2 = l2;
	    r.start = s1;
	    r.sweeps = swps;
	    add_param_list(r);
        }
    }

}

void 
delete_param_list()
{
    struct stimParam *curr;
    struct stimParam *next;

    //only attempt to delete nodes if we have a list!
    if(head_param)
    {
        curr = head_param;
        //delete the current node untill we find the end
        while(curr) {
            next = curr->next;
            free(curr);
            curr = next;
        }
    }
    head_param = NULL;      //eh, hopefully we don't leak memory
    num_params = 0;
}

static void
stim_atten(int c)
{
    double db, f, ref, osen, mag, att, scl;
    int i, t;
    SAV_FFT *p;

    db = stim_.sig_level[c];
    t = stim_.sig_type[c];
    if ((calType.at == 1) && ((t == 3) 	|| (t == 4) || (t == 5))) {
	f = stim_.sig_freq[c];
        p = c ? &B_fft : &A_fft;
        i = nint(f * p->npts / p->rate);
        osen = Sen.MP * Sen.AD / p->rp[i];                  /* cnt/Pa */
        ref = sqrt(2.0) * spl_ref * osen;
        mag = pow(10.0, db / 20.0) * ref;
        scl = MAXPOSINT;
        att = 20 * log10(scl / mag);
    } else {
	att = max_level - db;
    }
    stim_.sig_atten[c] = (float) limit(0, att, 120);
}

void 
set_curr_param(struct stimParam *p)
{
    int s2s;

    curr_param = p;
    stim_.sig_freq[0] = (float)(floor((p->freq1 * buflen) / rate + 0.5) * rate) / buflen;
    stim_.sig_freq[1] = (float)(floor((p->freq2 * buflen) / rate + 0.5) * rate) / buflen;
    stim_.sig_level[0] = p->level1;
    stim_.sig_level[1] = p->level2;
    stim_.sig_dur[0] = p->dur1;
    stim_.sig_dur[1] = p->dur2;
    stim_.sig_start[0] = p->start;
    stim_.sig_ramp[1] = msk_ramp;
    s2s = swp1set * 2;			    // # sweeps in 2 sets
    nsweeps = (p->sweeps + s2s - 1) / s2s;  // # set pairs
    stim_atten(0);
    stim_atten(1);
    test_level = stim_.sig_level[0];
    attenu.att1 = stim_.sig_atten[0];
    attenu.att2 = stim_.sig_atten[1];
    download();
}

int 
numb_params()
{
    int rval = 0;
    struct stimParam *curr = head_param;
    while(curr)
    {
        rval++;
        curr = curr->next;
    }

    return rval;
}

/* read list file */
int
rdlsfile()
{
    FILE  *fptr;
    int j, k;

    if (*list_file == '\0') {
	return (0);
    }
    list_items = 0;
    list_type = 0;		// click level-series
    stim_.sig_type[0] = 1;	// click signal
    stim_.sig_type[1] = 0;	// no masker
    if(strchr(list_file,'*') || strchr(list_file, '?')) 
    {
        // open a window and allow user to select 
        top_message("ABR list");
        j = draw_w(&w_info) + 8;
	k = w_info.ytop + txtpar.font_height * 3;
	text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
	(void) gprintf(j, k, "Select list file and press enter.");
	strcpy(o_file_name, list_file);
        if (open_file(NULL) == 27) {
	    strcpy(list_file, o_file_name);
	} else {
            (void) draw_w(&w_info);
    	    return (0);
	} 
    }
    if (*list_file == '\0') {
	return (0);
    }
    fptr = fopen(list_file, "rt");
    if (fptr == NULL) {
        (void) decide(0, 2, list_file, "Stimlulus list file not found!");
	return (0);
    } else {
        nlevs = 0;
        nreps = 1;
        while (fgets(line, MAXLINE, fptr) != NULL) 
        {
            if (line[0] == ';') 
            {
                line[0] = ' ';
                proc_hdr();
	    } else 
            {
                trim(line);
                if(strlen(line) > 6)    //makes sure the line is a valid legnth (not a blank line)
                    (void) read_param_list();
            }
        }
        fclose(fptr);
    }
    (void) draw_w(&w_info);
    list_items = num_params;

    return (1);
}

int 
verify_info()
{
    char s1[82], s2[22], s3[42], s4[42];
    
    sprintf(s1, "Name:  %s, %s", patient.Lastname, patient.Firstname);
    sprintf(s2, "Ear: %c", patient.Ear);
    sprintf(s3, "ID: %s ", patient.ID);
    sprintf(s4, "DOB: %s", patient.Dob);
    return (decide(1, 5, s1, s2, s3, s4, "Is patient info correct?"));
}

void
do_abr_task(int file_type, int condition)
{
    extern int abr_count;

    if(file_type == 1) {
        if (!genfn("A", "ABR", file_name, &abr_count)) {
            // this should remove the creation of the token file unless we specify it...
            t = tcreat(abr_tmp_name);
            // if creating the token file fails, this removes any files of the same name and tries again...
	    if (t == NULL) {
                (void) remove(abr_tmp_name);
                t = tcreat(abr_tmp_name);
	    }
        }
    }
    if (t != NULL) {
	strcpy(t->header.pattern, "ABR");
    }
    abr_aver(condition);

    if (!sqm.at) 
    {
        /*
        txtpar.text_wind_len = 60;
        text_color(w_info.wfgc, w_info.wbgc);
        j = w_info.xtop + 4;
        k = w_info.ytop + txtpar.font_height;
        
        (void) gprintf(j, k, "stim = %.0f dB SPL    atten = %.1f dB", 
	    stim_.sig_level[1], stim_.sig_atten[1]);
	k += txtpar.font_height;
        (void) gprintf(j, k, "resp = %.0f dB SPL    error = %.1f dB", 
	    response_level[1], response_level[1] - stim_.sig_level[1]);
        */
    }
}

int initbinheader(int channel, char *filename)
{
    strcpy(binhdr.identity, "ABRAV");
    binhdr.bufflen = buflen;
    binhdr.srate = rate;
    binhdr.blocksize = sizeof(struct BINwav) + ((buflen * sizeof(float) ) * 4);
    binhdr.headsize = sizeof(struct BINwav);
    binhdr.numchan = 2;
    binhdr.numblocks = list_items;
    binhdr.list_type = list_type;
    binhdr.epa_gain = (float) atof(epa_gain.strs[epa_gain.at]);
    memcpy(&binhdr.patient, &patient, sizeof(PATIENT) );

    binfp = fopen("out.bin", "wb");
    if(!binfp)
        return 0;
    fwrite( &binhdr, sizeof(binhdr), 1, binfp );
    return 1;
}

void 
closebin(int num)
{
    extern char outmsg[7][16];
    binhdr.blockscompleted = num;
    binhdr.epa_gain = (float) atof(epa_gain.strs[epa_gain.at]);
    binhdr.totaltime = (short) ((clock() - series_start_time) / CLOCKS_PER_SEC);
    binhdr.version = VERSION_NUM;
    binhdr.sweepsperset = abr_.swp1set;
    strcpy(binhdr.date, outmsg[0]);
    strcpy(binhdr.list, list_file);
    memcpy(&binhdr.patient, &patient, sizeof(PATIENT) );
    memcpy(&binhdr.ai, &ai, sizeof(struct AUX_INF) );
    memcpy(&binhdr.sens, &Sen, sizeof(sensitivity) );
    fseek(binfp, 0, 0);
    fwrite( &binhdr, sizeof(binhdr), 1, binfp);
    fflush(binfp);
    fclose(binfp);
}

void
save_calibr()
{
    int scal = 0;

    if (!genfn("A", "CAL", file_name, &scal)) {
	t = tcreat(file_name);
	if (t == NULL) {
	    (void) remove(file_name);
	    t = tcreat(file_name);
	}
    }
    if (t == NULL) {
	decide(0, 1, "Can't create calibration file.");
    } else {
	strcpy(t->header.pattern, "ABR");
	tok_init(1);
	tok_store(1, 1, sav_stim);
    }
}

/******************************************************************************/

int
proc_accu()
{
    int     i, mx, my, np;
    int     pair = 0, cnt = 0, total = 0;
    float  *lp;
    short   data;

    while (cnt < nsweeps) {
	if (dspdget()) {
	    (void) dspdread(&data);
	    if (data <= 0) {
		np = buflen * 2;
		if (pair) {
		    (void) dspuarr(dsppar.accbuf_a, np, outbuf + np);
		    (void) dsprst(TRUE);
		    (void) dsprst(FALSE);
		    long2float((int32_t *) outbuf, np);
                    take_the_aver(np, (float *) outbuf);
		    lp = longptr + np;
		    for (i = 0; i < buflen; i++)
			lp[i] = longptr[i] - longptr[i + buflen];
		    if (!check_rej(lp) || noreject) {
			for (i = 0; i < buflen; i++) {
			    accbuf_1[i] += longptr[i];
			    accbuf_2[i] += longptr[i + buflen];
			}
			cnt++;
			if (!noreject) {
			    fft_A_B(accbuf_1, accbuf_2, 1, cnt, 1);
			    show_fft(abr_.fft_xrange, abr_.fft_yrange,
				abr_.levref, scrn_c[C_APBS], 1);
			    fft_A_B(accbuf_1, accbuf_2, 0, cnt, 1);
			    show_fft(abr_.fft_xrange, abr_.fft_yrange,
				abr_.levref, scrn_c[C_AMBS], 0);
//			    text_color(txtpar.gfg, txtpar.gbg);
//			    (void) gprintf(j, n, "S/N=%.0f/%.0f  ", dpp, dpn);
			}
		    }
		    pair = 0;
//		    (void) gprintf(j, k, "%5d", nsweeps - cnt);
		} else {
		    (void) dspuarr(dsppar.accbuf_a, np, outbuf);
		    (void) dsprst(TRUE);
		    (void) dsprst(FALSE);
		    pair = 1;
		}
		total++;
//		(void) gprintf(j, m, "%d / %d", 2 * cnt, total);
	    }
	}
	while (check_event()) {
	    i = getevent();
	    if (i == 27 || i == 3) {
		nsweeps = cnt;
		if (total >= 0)
		    total = -(total + 1);
	    } else if (i == SINGLE_CLICK) {
	    	mouse_position(&mx, &my);
		if (in_wind(mx, my, w_msg)) {
		    i = (my - w_msg.ytop) / txtpar.font_height;
		    if (i == 1) {
			nsweeps = cnt;
		    } else if (i == 2) {
			nsweeps = cnt;
			if (total >= 0)
			    total = -(total + 1);
		    }
		}
	    } else {
		(void) proc_art(i);
            }
	}
    }
    return (total);
}

int
sumup(int color, int reset)
{
    double  twt;
    int     i, np;

    dfgc = color;
    display_reset = reset;
    np = buflen;
    for (i = 0; i < np; i++)
	accbuf_1[i] += accbuf_2[i];
    text_color(scrn_c[C_RSPF], scrn_c[C_RSPB]);
    twt = swp1set * 2 * nsweeps;
    (void) disply(twt, accbuf_1, np, reset, 0.0, NULL, 0);
    (void) fft_stim(accbuf_1, 2 * nsweeps, 1); // save FFT in outbuf
    show_fft(abr_.fft_xrange, abr_.fft_yrange, abr_.levref, color, reset);
   return (np);
}

/* setup single channel calibration (0=A, 1=B) */
void
set_channel(int c)
{
    int d;

    d = c ? 0 : 1;
    stim_.sig_type[c] = 6;   // generate swept-tone on channel=c
    stim_.sig_type[d] = 0;
    stim_.sig_atten[0] = abr_.cal_atten;
    stim_.sig_atten[1] = abr_.cal_atten;
    download();
}

/* calibrate one channel (0=A, 1=B) */
int
cali_chan(int c)
{
    double att, scl;
    int     n, color, reset;
    SAV_FFT *pft;

    color = c ? scrn_c[C_CHNB] : scrn_c[C_CHNA];
    reset = c ? 0 : 1;
    pft = c ? &B_fft : &A_fft;
    att = c ? attenu.att2 : attenu.att1;

    set_channel(c);
    zero_all();
    if (proc_accu() <= 0) {
	return 0;
    }
    n = sumup(color, reset);
    scl = pow(10.0, -att / 20.0);
    cal_gain(pft, n, scl);
    if (c == 0) {
        memcpy(sav_stim, accbuf_1, buflen * sizeof(float));
    }
    if (c == 1) {
        memcpy(accbuf_2, accbuf_1, buflen * sizeof(float));
        memcpy(accbuf_1, sav_stim, buflen * sizeof(float));
    }
    return (1);
}

/* compute single channel calibration gain (0=A, 1=B) */
void
compute_cal_gain(int c)
{
    double att, scl, twt;
    int     np, color, reset;
    float  *buf;
    SAV_FFT *pft;


    color = c ? scrn_c[C_CHNB] : scrn_c[C_CHNA];
    reset = c ? 0 : 1;
    pft = c ? &B_fft : &A_fft;
    att = c ? attenu.att1 : attenu.att2;
    buf = c ? accbuf_2 : accbuf_1;

    dfgc = color;
    display_reset = reset;
    twt = swp1set * 2 * nsweeps;
    (void) disply(twt, buf, buflen, reset, 0.0, NULL, 0);
    np = fft_stim(buf, 2 * nsweeps, 1);
    show_fft(abr_.fft_xrange, abr_.fft_yrange, abr_.levref, color, reset);
    scl = pow(10.0, -att / 20.0);
    cal_gain(pft, np, scl);
}

/* save ITE calibration responses into a token file */
void
save_cali_tokens()
{
    t = tcreat(cali_file);
    if (t == NULL) {
	(void) remove(cali_file);
	t = tcreat(cali_file);
    }
    if (t == NULL) {
	decide(0, 1, "Can't create calibration file.");
    } else {
	strcpy(t->header.pattern, "ABR");
	tok_init(2);
	tok_store(1, 2 * nsweeps, accbuf_1);  // channel A
	tok_store(2, 2 * nsweeps, accbuf_2);  // channel B
	tclose(t);
	t = NULL;
    }
}

MENUITEM cfn[] = {
    {"&Calibration File :", cali_file, STRING, 42, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

/* read ITE calibration from file and compute gain */
int
rd_cal()
{
    int x, y;
    
    x = (w_sig.xbot + w_sig.xtop - 40 * 8) / 2;
    y = (w_sig.ybot + w_sig.ytop - txtpar.font_height) / 2;
    (void) simple_submenu(x, y, cfn);
    newext(cali_file, "CAL");
    if (_access(cali_file, 0) == 0) {
        decide(0, 2, "Reading calibration file", cali_file);
        rd_cal_file(cali_file);
        compute_cal_gain(0);
        compute_cal_gain(1);
        (void) check_event();
        noreject = 1;
    } else {
        decide(0, 2, "Can't open calibration file", cali_file);
        noreject = 0;
    }
        
    return (noreject);
}

/* in-the-ear (ITE) calibration */
int
calibrate_ite()
{
    int ok;

    strcpy(cali_file, "out.cal");

    attenu.att1 = attenu.att2 = abr_.cal_atten;
    swp1set = abr_.cal_swps;
    pre_dis_ind("Artifact", "Limit");
    dis_artifact();

    (void) draw_w(&w_info);

    open_stim_w(cal_stim);
    noreject = 1;
    nsweeps = 1;
    if (!cali_chan(0) || !cali_chan(1)) {
	(void) decide(0, 2, "Calibration has been aborted",
	    "Press any key or mouse, back to Checkfit");
	cali_file[0] = 0;
	return (0);
    }
    noreject = 0;
    dfgc = scrn_c[C_WAVE];
    (void) draw_w(&w_msg);
    ok = decide(1, 1, "Accept the Check Fit and Calibration ?");
    if (ok)
	save_cali_tokens();
    else
	cali_file[0] = 0;
    return (ok);
}

/******************************************************************************/

// this is called when we start a series
int 
begin_series()
{
    int j, k;

    if(genfn("A", "BIN", file_name, &bin_count) ) {
	return (0);
    }
    if (calType.at == 1 && list_type != 0) {
	if (!calibrate_ite()) {
	    (void) draw_w(&w_info);
	    text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
	    j = w_info.xtop + 4;
	    k = w_info.ytop + txtpar.font_height * 8;
	    (void) gprintf(j, k, " Calibration was rejected.");
	    return (0);
	}
    }
    if(list_type == 0) {        // click level-series:
        stim_.sig_type[0] = 2;   //   click signal
        stim_.sig_type[1] = 0;   //   no masker
    } else if(list_type == 1) { // simultaneous masking:
        stim_.sig_type[0] = 3;   //   windowed-tone signal
        stim_.sig_type[1] = 5;   //   continuous-tone masker
    } else if(list_type == 2) { // forward masking:
        stim_.sig_type[0] = 3;   //   windowed-tone signal
        stim_.sig_type[1] = 4;   //   ramped-tone masker
    }
    series_start_time = clock();
    return (1);
}

void
end_series()
{
    int res;
    char stuff[30];

    //delete's the parameter link list
    delete_param_list();
    
    if(decide(1, 1, "Save the BIN and CAL files?"))
    {
        genfn("A", "BIN", file_name, &bin_count);
        strcpy(cali_file, file_name);
        newext(cali_file, "CAL");
        res = rename("out.bin", file_name);
        if(res)
        {
            sprintf(stuff, "Could not save %s!", file_name);
            (void) decide(0, 1, stuff);
        }
        res = rename("out.cal", cali_file);
        if(res && (binhdr.list_type != 0))
        {
            sprintf(stuff, "Could not save %s!", cali_file);
            (void) decide(0, 1, stuff);
        }
    }

}

int 
ABR_wind()
{
    int j = 0;

    if (r_mode.at != 2) {
	if (dsp_init() > 0)
	    return (0);
    }
    grab_abr();
    points_msg();
    (void) draw_w(&w_msg);
    dis_units(ad_sens, Sen.MP, 0.0);
    file_opt[4].status = 0;
    file_opt[5].status = 0;
    distype.at = 0;
    if(r_mode.at != 2) {
	// open stimulus file only if the stim type is file
	if(stim_.type == 0) {
	    open_stim_w(ilstim);
        }
    }
    distype.at = 1;
    w_spec.ytop = w_msg.ybot + 1;
    (void) draw_w(&w_spec);
    if (r_mode.at != 2) {
	if (check_fit()) {
	    return (0);
	}
	memcpy(sav_stim, outbuf, buflen * sizeof(float));
	display_reset = 1;
	dfgc = scrn_c[C_WAVE];
	distype.at = 1;
	(void) dis_stim(1, sav_stim, LONG);
    }
    if(calibr_flag) {
        save_calibr();
	return (0);
    }
    clear_display();
    init_edit_msg();
    buflen = abr_.size;
    swp1set = abr_.swp1set;
    nsweeps = abr_.sets;
    w_dis = &w_sig;
    distype.at = 0;
    delete_param_list();    //make sure the list is 
    // when we do a series we start a series by reading a param 
    // list file, and do a complete do_abr_task for each rep
    series_flag = rdlsfile(); // if list file is valid, then do series
    if (!series_flag) {
        do_abr_task(1, 0);
    } else {
	if (begin_series()) 
        {
            if(initbinheader(2, file_name)) 
            {
	        curr_param = head_param;
		for(j = 0; j < list_items && series_flag && curr_param != NULL; ++j) 
                {
		    if(curr_param != NULL) 
                    {
		        set_curr_param(curr_param);
		        do_abr_task(0, 0);
		    }
		    curr_param = curr_param->next;
		}
	    }
	}
        closebin(j);
	end_series();
    }
    top_message("        ");
    return (0);
}
