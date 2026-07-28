/* =============================== teoae_w.c ============================= */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <memory.h>
#include <dsp.h>
#include <menu.h>
#include <token.h>
#include <util.h>
#include <fftlib.h>
#include "savage.h"
#include "color.h"
#include "emav.h"		/* function prototyes */
#include "screen.h"

extern char line[MAXLINE];
extern float resid_vpp, limit_mv, range_mv;
extern int dpoae_flag, probe_flag, scope, maxnpts, contav;
extern struct _teoae teoae;
extern struct _dpoae dpoae;
extern struct _probe probe;
extern int limits;
extern TOGGLETYPE distype, Stim_mode;
extern INDICATOR t_ind;

char   *tmp_tok_file = "out.tok";
char    auto_flag = 'M',  differ_flag = 0;
int     fft_flag = 0, stim_flag = 0;
int     display_reset = 0;
int     rd_cal_flag = 0;
float	tolerance = 1.0;	/* dB */

static void calibr_upld(void);

static SIG_V sav_v;
static int chk_fit_flag = 0;
static int cal_stim_flag = 0;

/* make the TEOAE paramters current ones */
void
grab_teoae()
{
    float  tmax;

    mp_transfer(Sys.MPfn);
    Sen = Sys;
    dis_units(Sen.AD, Sen.MP);

    if (teoae.size > maxnpts) {
	teoae.size = (short)(maxnpts);
    }
    buflen = teoae.size;
    acc_sets = teoae.sets;
    rate = teoae.rate;

    tmax = (float) (buflen * 1000.0 / teoae.rate);
    mstime1 = limit(0, mstime1, tmax);
    mstime2 = limit(0, mstime2, tmax);
    msramp = limit(0, msramp, tmax);
    itime1 = (int) (mstime1 * (teoae.rate / 1000.0));
    itime2 = (int) (mstime2 * (teoae.rate / 1000.0));
    iramp = (int) (msramp * (teoae.rate / 1000.0));
    if (iramp <= 0)
        iramp = 1;
    limit_mv = teoae.mvmax;
    fft_xrange = teoae.fft_xrange;
    fft_yrange = teoae.fft_yrange;
    fft_levref = teoae.levref;
    contav = 0;
    change_nic(1);
    input_filter(hpf_type, hpf_ord, teoae.hpff, teoae.rate);
}

/* calculate and display the stability */
double
stability(int show, int32_t *p, int32_t *q, int num)
{
    int     i, j, k, min;
    double  u, v, Sx = 0, Sy = 0, Sxx = 0, Syy = 0, Sxy = 0;

    min = buflen / 8;
    if (num < min)
	num = min;
    for (i = 0; i < num; i++) {
	v = (float) p[i];
	u = (float) q[i];
	Sy += u;
	Syy += u * u;
	Sx += v;
	Sxx += v * v;
	Sxy += v * u;
    }
    v = Sx / num;
    u = Sy / num;
    Sxx -= Sx * v;
    Syy -= Sy * u;
    Sxy -= Sx * u;
    if ((Sxx <= 0) || (Syy <= 0) || (Sxy <= 0)) {
	v = 0;
    } else {
	v = 100 * Sxy / sqrt(Sxx * Syy);
    }
    if (show) {
        text_color(w_stim.wfgc, w_stim.wbgc);
	//cw = txtpar.font_width;
        j = w_stim.xbot - 5 * 8;
	k = w_stim.ybot - txtpar.font_height;
	gprintf(j, k, "%3.0f%%", v);
    }
    return (v);
}

/* display the stimulus and the its FFT */
int
dis_stim(int load, void *p, int type, int swps, int sets)
{
    double  tms;
    float   vpp;
    int     num, min;

    if (display_reset) {
        draw_w(&w_stim);
        draw_w(&w_stim_fft);
    }
    w_dis = &w_stim;
    if (dpoae_flag) {
        num = (scope < buflen) ? scope : buflen;
    } else {
        num = itime1;
    }
    min = buflen / 8;
    if (num < min)
        num = min;
    w_stim.ybot -= txtpar.font_height;
    if (type == 1) {
        disply(sets * swps, p, num, display_reset);
        min = fft_sig(p, sets);
        modulesqr();
    } else if (type == 0) {
        display(p, num, display_reset, 1, 1);
        min = fft_short(p, 0);
    }
    w_stim.ybot += txtpar.font_height;
    tms = 1000 * (double) num / rate;
    show_xlabel(0.0, tms, 0, "ms");
    w_dis = &w_sig;
    stim_flag = 1;
    vpp = sig_v.vpp;	// save vpp
    show_fft(teoae.fft_xrange, teoae.fft_yrange, teoae.levref,
        dfgc, display_reset);
    stim_flag = 0;
    sig_v.vpp = vpp;	// restore vpp
    if (load) {
        memcpy(&sav_v, &sig_v, sizeof(SIG_V));
	if (type == LONG)
	    memcpy(savbuf, p, (size_t)(buflen) * sizeof(int32_t));
    }
    return(min);
}

/* show the stimulus title (file name) */
void
stim_title(char *file)
{
    int     i, yb;
    struct PATH path;

    yb = w_stim.ybot;
    w_stim.ytop = w_stim_fft.ytop = txtpar.menu_height + 1;
    w_stim.ybot = w_stim_fft.ybot = txtpar.menu_height * 2;
    i = w_stim.ybot - 3;
    split_path(file, path.drive, path.dir, path.name, path.ext);
    gprintf(draw_w(&w_stim), i, "Stim: %s%s", path.name, path.ext);
    w_stim.ytop = w_stim.ybot + 1;
    w_stim.ybot = w_stim_fft.ybot = yb;
}

/* open the stimulus file and display the simulus and its FFT */
void
open_stim_w(char *file)
{
    stim_title(file);
    strcpy(stmfn, file);
    getstim(file, buflen);
    display_reset = 1;
    dfgc = scrn_c[C_WAVE];
    dis_stim(1, outbuf, 0, 1, 1);
}

/* conversion between different units ( pascal and voltage ) */
double
units_conv(double in)
{
    if (distype.at) {
        in /= Sen.MP;
    }
    in *= 1000.0;
    return (in);
}

/* display patient (or subject) information */
int
dis_patient()
{
    int     i, k, j, cw, ch;
    char    str[82];

    cw = txtpar.font_width;
    ch = txtpar.font_height;
    text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / cw - 1;
    i = w_info.xtop + cw / 2;
    k = w_info.ytop + ch;
    sprintf(str, "Name:  %.13s, %.12s", patient.Lastname, patient.Firstname);
    for (j = strlen(str); j < txtpar.text_wind_len; j++)
	str[j] = ' ';
    str[j] = 0;
    gprintf(i, k, str);
    k += ch;
    sprintf(str, "Ear: %c, ID: %.12s, DOB: %.12s",
	patient.Ear, patient.ID, patient.Dob);
    for (j = strlen(str); j < txtpar.text_wind_len; j++)
        str[j] = ' ';
    str[j] = 0;
    gprintf(i, k, str);
    k += ch;
    sprintf(str, "%s", patient.Threshold);
    for (j = strlen(str); j < (txtpar.text_wind_len - 10); j++)
        str[j] = ' ';
    str[j] = 0;
    gprintf(i, k, "Threshold: %s", str);
    k += ch;
    sprintf(str, "%s", patient.Comment);
    for (j = strlen(str); j < (txtpar.text_wind_len - 10); j++)
        str[j] = ' ';
    str[j] = 0;
    gprintf(i, k, "Comment: %s", str);
    k += ch;
    return (k);
}

double
vtodb(double v)
{
    v /= Sen.MP;
    v /= 2E-5;
    return (dbv(v));
}

double
dbv(double v)
{
    if (v < 1e-39)
        return (-800.0);
    return (20 * log10(v));
}

static double
len_est(int32_t *acc, short *stm, int npt, double vs)
{
    double asr, asi, sms, ecl;
    float *a, *s, *p;
    int i, ii, ir, n;

    n = npt + 2;
    a = (unsigned long)((float *) calloc((size_t)(n), sizeof(float)));
    s = (unsigned long)((float *) calloc((size_t)(n), sizeof(float)));
    p = a;
    for (i = 0; i < npt; i++) {
        a[i] = (float) acc[i];
        s[i] = (float) stm[i];
    }
    fast(a, npt);
    fast(s, npt);
    for (i = 0; i <= (npt / 2); i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
        sms = s[ir] * s[ir] + s[ii] * s[ii];
        asr = (a[ir] * s[ir] + a[ii] * s[ii]) / sms;
        asi = (a[ii] * s[ir] - a[ir] * s[ii]) / sms;
        p[ir] = (float) (asr * asr + asi * asi);
        p[ii] = 0;
    }
    ecl = cavity_length(p, npt / 2, vs);
    free(a);
    free(s);

    return (ecl);
}

/* display the Check Fit information */
void
dis_info()
{
    double  val, lc, fp, vs;
    int     k, i, cw;

    // display checkfit info
    cw = txtpar.font_width;
    //ch = txtpar.font_height;
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / cw - 1;
    i = w_info.xtop + 4;
    if (probe_flag || dpoae_flag) {
        k = w_info.ybot - nint(txtpar.font_height * 2.5);
        // estimate cavity length
        vs = 3.4723e4 * (1 + 0.00166 * (tmpcav - 26.85));
        lc = (float) len_est(accbuf, stmbuf, buflen, vs);
        fp = vs / lc / 2;
        text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
        gprintf(i, k, "cavity length = %5.1f cm  ", lc);
        k += txtpar.font_height;
        gprintf(i, k, " notch & peak = %5.0f %5.0f Hz  ", fp / 2, fp);
    } else {
        k = dis_patient() + 2;
        text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
        gprintf(i, k, "Checking Microphone Fit ..............");
        k += txtpar.font_height;
        text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
        gprintf(i, k, "Stimulus: %-13s Attenu: %4.1f dB ", stmfn, attenu.att1);
        k += txtpar.font_height;
        val = units_conv(sig_v.vpp);
        gprintf(i, k, "Measured Value: %5.1lf dB ppSPL = %6.2f%s ",
	    vtodb(sig_v.vpp), val, distype.strs[distype.at]);
        k += txtpar.font_height;
        gprintf(i, k, "rms = %5.1lf dB SPL = %6.2f%s ",
	    vtodb(sig_v.rms), 
            units_conv(sig_v.rms), distype.strs[distype.at]);
        k += txtpar.font_height;
        gprintf(i, k, "min = %5.1f%s max = %5.1f%s dc =%5.1f%s ",
	    units_conv(sig_v.npk), distype.strs[distype.at], 
            units_conv(sig_v.ppk), distype.strs[distype.at], 
            units_conv(sig_v.dco), distype.strs[distype.at]);
    }
}

/* display the calibration info */
void
dis_cali()
{
    int     i, j, k;
    double  u;

    i = w_info.xtop + 4;
    j = dis_patient() + 2;
    u = units_conv(target_voltage);
    if (u < 1)
	k = 2;
    else if (u < 10)
	k = 1;
    else
	k = 0;
    gprintf(i, j, "Target Value: %5.1lf dB ppSPL = %4.*f%s",
	vtodb(target_voltage), k, u, distype.strs[distype.at]);
    j += txtpar.font_height;
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
    u = units_conv(resid_vpp);
    if (u < 1)
	k = 2;
    else if (u < 10)
	k = 1;
    else
	k = 0;
    gprintf(i, j, "Atten: %5.1lf dB  Resid: %4.*f%s ",
	attenu.att1, k, u, distype.strs[distype.at]);
    if(fabs(dbv(sig_v.vpp / target_voltage)) > tolerance)
	differ_flag = 1;
    else differ_flag = 0;
}

/* invokes option from the checkfit or calibrate phase */
int
O_options(char *no_use)
{
    void    (*func) ();
    int     mode;

    mode = (int)(Stim_mode.at);
    func = get_bg_exe();
    set_bg_exe((void (*)())0);
    Option_wind();
    set_bg_exe(func);

    if (cal_stim_flag && mode != Stim_mode.at) {
        mode = (int)(Stim_mode.at);
        dsprst(TRUE);
	downldpar(2, buflen * spm[mode].nset, 0, 0, swp1set);
        recalstim(mode);
        ld_stim(0, mode);
        recalstim(mode);
	ld_stim(1, mode);
        dsprst(FALSE);
    }
    return (' ');
}

/* invokes the Info window from the checkfit or calibrate phase */
int
I_info(char *no_use)
{
    void    (*func) ();

    func = get_bg_exe();
    set_bg_exe((void (*)())0);
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
    reset_refresh_time();
    display_reset = 1;
    return (FN | 72);
}

/* continuation function for the menus */
int
C_continue(char *no_use)
{
    if(dpoae_flag && !probe_flag) 
    	cali_file[0] = 0;
    chk_fit_flag = 1;
    return (get_resume());
}

int
E_continue(char *no_use)
{
    chk_fit_flag = 1;
    return (get_resume());
}

int
E_cancel(char *no_use)
{
    return (FN | 113);
}

int
F_continue(char *no_use)
{
    chk_fit_flag = 1;
    rd_cal_flag = 1;
    return (get_resume());
}

int
A_auto(char *s)
{
    if (auto_flag == 'A')
	auto_flag = 'M';
    else
	auto_flag = 'A';
    return (' ');
}

void
rd_target(char *str)
{
    int     i, c, db;
    double  convert, val;

    convert = Sen.MP;
    db = 1;
    i = (int) strlen(str);
    if (i) {
	while (i-- > 0) {
	    c = tolower(str[i]);
	    if (c == 'v') {
		convert = 1;
		db = 0;
		break;
	    } else if (c == 'p' && tolower(str[i - 1]) != 's') {
		db = 0;
		break;
	    }
	}
	if(i > 0) str[i] = 0;
	val = atof(str);
	if (val > 0) {
	    if (db) val = pow(10.0, val / 20.0) * spl_ref;
	    target_voltage = val * convert;
	}
    }
}

int
S_set_tar(char *no_use)
{
    int     j, k, n;
    char    str[20] = { 20*0 };

    set_bg_exe((void (*)())0);
    k = open_text_w(&j);
    gprintf(j, k, "Target p-p Stimulus = %.3fV, %.3fPas, %.1fdB SPL ", 
	target_voltage,	target_voltage / Sen.MP, vtodb(target_voltage));
    k += txtpar.font_height;
    n = gprintf(j, k, "Enter your value with units (V, Pas, dB): ");
    txtpar.text_wind_len -= n;
    get_str(j + n * 8, k, 20, 0, STRING, str);
    rd_target(str);
    close_text_w();
    set_bg_exe(calibr_upld);
    return (' ');
}

int
proc_art(int i)
{
    float ind, dmv;

    ind = (float) (t_ind.new * 1000.0 / (Sen.AD * Sen.MP));
    dmv = (float) (range_mv * 0.01);
    if (i & FN) {
	switch (i) {
	case FN | 77:		/* arrow right */
	    limit_mv += dmv;
	    break;
	case FN | 116:		/* control right */
	    limit_mv += dmv * 10;
	    break;
	case FN | 75:		/* arrow left */
	    limit_mv -= dmv;
	    break;
	case FN | 115:		/* control left */
	    limit_mv -= dmv * 10;
	    break;
	case FN | 71:           /* home */
	    limit_mv = (float) (ind * 0.8);
	    break;
	case FN | 79:		/* end */
	    limit_mv = ind;
	    break;
	}
	if (limit_mv < 0.1)
	    limit_mv = (float) 0.1;
	dis_artifact();
    } else if (i == 27) {
	return (FN | 113);
    }
    return (0);
}

static void
chk_fit_upld()
{
    double  tms;
    int     min;
    short   data;

    if (check_chkfit_time())
        putbackevent('C');
    if (check_refresh_time())
        R_clear();
    if (dspdget()) {
	dspdread(&data);
	if (data <= 0) {
	    dspuarr(dsppar.acc, buflen * 2, (short *) accbuf);
	    dsprst(TRUE);
	    dsprst(FALSE);
	    take_the_aver(buflen, accbuf);
	    if (dpoae_flag) {
		if (scope < buflen) 
		    data = (short)(scope);
		else 
		    data = (short)(buflen);
	    } else {
		data = (short)(itime1);
	    }
	    min = buflen / 8;
	    if (data < min)
		data = (short)(min);
            dfgc = scrn_c[C_WAVE];
            text_color(scrn_c[C_RSPF], scrn_c[C_RSPB]);
	    w_sig.ybot -= txtpar.font_height;
	    if (disply(swp1set, accbuf, data, display_reset)) {
                dis_info();
            }
	    w_sig.ybot += txtpar.font_height;
	    tms = 1000 * (double) data / rate;
	    show_xlabel(0.0, tms, 0, "ms");
	    fft_stim(accbuf, 1, 0);
	    show_fft(teoae.fft_xrange, teoae.fft_yrange, teoae.levref,
	        scrn_c[C_WAVE], display_reset);
	    display_reset = 0;
	}
    }
}

MENUITEM te_chk[] = {
    {"&Continue", NULL, NONE, 0, 0, 1, C_continue},
    {"& Refresh", NULL, NONE, 0, 0, 1, R_clear},
    {"Patient &Info...", NULL, NONE, 0, 0, 1, I_info},
    {"&Options...", NULL, NONE, 0, 0, 1, O_options},
    {NULL, NULL, NONE, 0, 0, 0, proc_art}
};

MENUITEM dp_chk[] = {
    {"&Continue", NULL, NONE, 0, 0, 1, C_continue},
    {"& Refresh", NULL, NONE, 0, 0, 1, R_clear},
    {"Patient &Info...", NULL, NONE, 0, 0, 1, I_info},
    {"&Options...", NULL, NONE, 0, 0, 1, O_options},
    {"&Test", NULL, NONE, 0, 0, 0, E_continue},
    {"&File", NULL, NONE, 0, 0, 1, F_continue},
    {NULL, NULL, NONE, 0, 0, 0, proc_art}
};

int
check_fit()
{
    MENUITEM *mi;

    R_clear();
    if(probe_flag || dpoae_flag) 
        swp1set = dpoae.chk_swps;
    else 
        swp1set = teoae.chk_swps;
    if(!probe_flag && !dpoae_flag) 
	draw_w(&w_info);
    if(!stim_flag) 
        draw_w(&w_spec);

    downldpar(1, buflen, 0, 0, swp1set);
    zero_acc();
    zero_out();
    dwnld_dac(1, 1, 1, 0);
    recalstim(0);
    dwnld_dac(0, 1, 1, 0);
    dsprst(TRUE);
    dsprst(FALSE);

    R_clear();
    if(dpoae_flag) {
	mi = dp_chk;
	mi[4].status = cali_file[0] && !probe_flag;
    } else {
	mi = te_chk;
    }
    fft_flag = 1;
    chk_fit_flag = cal_stim_flag = rd_cal_flag = 0;
    reset_chkfit_time();
    set_bg_exe(chk_fit_upld);
    simple_submenu(w_msg.xtop, w_msg.ytop, mi);
    set_bg_exe((void (*)())0);
    dsprst(TRUE);
    return (chk_fit_flag);
}

/* Residual Error Correction code */

#include <fftlib.h>
#define RECNFT  512
#define RESSIZ  (RECNFT + 2)
int     rec[100], reclen = 25;
int     reslen = 125, resrmp = 25;
static float recstm[RESSIZ], recrsp[RESSIZ], recrsd[RESSIZ];

void
zero_rec()
{
    int     i;

    for (i = 0; i < reclen; i++)
	rec[i] = 0;
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

void
compute_rec(int32_t *rsp, int n)
{
    float  *frsp, *frsd, *fstm, r, w;
    int     i, j, i1, i2;
    int32_t   *rsd;

    rsd = rsp + n;
    w = (float) resrmp;
    for (i = 0; i < RESSIZ; i++) {
	if (i < resrmp) {
	    r = (float)(i / w);
	    recrsp[i] = (float)(rsp[i] * r);
	    recrsd[i] = (float)(rsd[i] * r);
	} else if (i < (reslen - resrmp)) {
	    recrsp[i] = (float) rsp[i];
	    recrsd[i] = (float) rsd[i];
	} else if (i < reslen) {
	    r = (float)((reslen - i) / w);
	    recrsp[i] = (float)(rsp[i] * r);
	    recrsd[i] = (float)(rsd[i] * r);
	} else {
	    recrsp[i] = 0;
	    recrsd[i] = 0;
	}
    }
    fast(recrsp, RECNFT);
    fast(recrsd, RECNFT);
    i1 = 0;
    i2 = RECNFT / 3;
    for (i = 0; i < RESSIZ / 2; i++) {
	j = i * 2;
	if (i < i1 || i >= i2) {
	    recrsd[j] = recrsd[j + 1] = 0;
	} else {
	    fstm = recstm + j;
	    frsp = recrsp + j;
	    frsd = recrsd + j;
	    cdiv(fstm, frsp, frsp);	/* compute inverse response */
	    cmul(frsd, frsp, frsd);	/* apply inverse to residual */
	    frsd[0] *= 2;	/* correction for folding ??? */
	    frsd[1] *= 2;	/* correction for folding ??? */
	}
    }
    fsst(recrsd, RECNFT);	/* inverse FFT */
    for (i = 0; i < reclen; i++)/* add to error correction */
	rec[i] = (float)((int) (rec[i] + recrsd[i]));
}

static void
stim_rec(short *stm, int n)
{
    int     i;

    for (i = 0; i < RESSIZ; i++)
	recstm[i] = (float) ((i < reslen) ? stm[i] : 0);
    fast(recstm, RECNFT);
}

TOGGLETYPE RECor = {
    0,
    { "Off", "On", NULL },
};

MENUITEM bwt[] = {
    {"&Continue", NULL, NONE, 0, 0, 1, C_continue},
    {"& Refresh", NULL, NONE, 0, 0, 1, R_clear},
    {"Patient &Info...", NULL, NONE, 0, 0, 1, I_info},
    {"&Options...", NULL, NONE, 0, 0, 1, O_options},
    {"&Set Target...", NULL, NONE, 0, 0, 1, S_set_tar},
    {"&Auto/Manual=", (char *) &auto_flag, CHAR, 0, 0, 1, A_auto},
    {"Co&rrection", (char *) &RECor, TOGGLE, 6, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 0, proc_art}
};

void
calibr_upld()
{
    double  tms;
    int     i, ns;
    int32_t   *lp;
    short   data;

    
    if (check_refresh_time())
        R_clear();
    lp = swpbuf + buflen;
    if (dspdget()) {
	dspdread(&data);
	if (data <= 0) {
	    ns = spm[Stim_mode.at].nset;
	    dsprst(TRUE);
	    dspuarr(dsppar.acc, buflen * ns * 2, (short *) swpbuf);
	    dsprst(FALSE);
	    take_the_aver(ns * buflen, swpbuf);
	    i = fold_it();
	    if (RECor.at)
		compute_rec(swpbuf, buflen);
	    filter_it(lp, buflen);
	    compute_resid(lp);
            dfgc = scrn_c[C_WAVE];
            text_color(scrn_c[C_RSPF], scrn_c[C_RSPB]);
	    w_sig.ybot -= txtpar.font_height;
	    if (disply(i * swp1set, swpbuf, buflen, display_reset)) {
		if (!dpoae_flag && !probe_flag)
		    dis_cali();
		if (auto_flag == 'A') {
		    attenu.intern_att *= (float) (sig_v.vpp / target_voltage);
		    if(attenu.intern_att > 100000.0)
			attenu.intern_att = 100000.0;
		    set_att(dbv(attenu.intern_att));
		    dsprst(TRUE);
		    downldpar(2, buflen * ns, 0, 0, swp1set);
		    i = (int)(recalstim(Stim_mode.at));
		    ld_stim(0, Stim_mode.at);
		    i = (int)(recalstim(Stim_mode.at));
		    ld_stim(1, Stim_mode.at);
		    dsprst(FALSE);
		    text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
		    if(!i && differ_flag) {
			gprintf(w_info.xtop + 4, w_info.ybot - 4,
			    "Warning: Target level can't be reached");
		    } else {
			gprintf(w_info.xtop + 4, w_info.ybot - 4,
			    "Stimulus level has been adjusted      ");
		    }
		    stim_rec(outbuf, buflen);
		}
		display_reset = 0;
	    }
	    w_sig.ybot += txtpar.font_height;
	    tms = 1000 * (double) buflen / rate;
	    show_xlabel(0.0, tms, 0, "ms");
	}
    }
}

int
calculate_it()
{
    int     i;
    char    s[40];

    open_stim_w(teoae.stim_file);
    i = draw_w(&w_info);

    swp1set = teoae.swp1set;
    zero_rec();
    dsprst(TRUE);
    downldpar(2, spm[Stim_mode.at].nset * buflen, 1, 0, swp1set);
    recalstim(Stim_mode.at);
    ld_stim(0, Stim_mode.at);
    recalstim(Stim_mode.at);
    ld_stim(1, Stim_mode.at);
    dsprst(FALSE);
    stim_rec(outbuf, buflen);

    text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
    gprintf(i, w_info.ybot - 4, "You can calibrate the stimulus, now...");
    pre_dis_ind("Artifact", "Limit");
    dis_artifact();

    display_reset = 1;
    fft_flag = 0;
    cal_stim_flag = 1;
    set_bg_exe(calibr_upld);
    i = simple_submenu(w_msg.xtop, w_msg.ytop, bwt);
    set_bg_exe((void (*)())0);
    if(!i && differ_flag && !probe_flag) {
	sprintf(s, "by more than %.0f dB ppSPL", tolerance);
	i = !decide(1, 3, "Warning: Measured level differs from target level",
	    s,  "Do you still want to continue");
    }
    return (i);
}

static void
do_teoae_task()
{
    extern TOKENFILE *t;
    extern int teoae_counter;

    if (!genfn("T", "TOK", file_name, &teoae_counter)) {
	t = tcreat(tmp_tok_file);
	if (t == NULL) {
	    remove(tmp_tok_file);
	    t = tcreat(tmp_tok_file);
	}
    }
    if (t != NULL) {
	draw_w(&w_info);
	strcpy(t->header.pattern, "TEOAE");
	tok_init(ntokens, Stim_mode.at);
	teoae_aver();
    }
}

void
TEOAE_wind()
{
    extern MENUITEM file_opt[];

    if (dsp_init() > 0)
	return;
    init_wind();
    top_message("TEOAE      ");
    teoae.rate = adjust_rate(teoae.rate);
    grab_teoae();
    set_test_opt(0, 1, 0);
    draw_w(&w_msg);
    FILE_SAVE.status = 0;
    set_att(teoae.chk_atten);
    do {
        open_stim_w(teoae.checkfit);
	if (check_fit() == 0) {
	    set_test_opt(1, 1, 1);
	    top_message("           ");
	    dsprst(TRUE);
	    return;
	}
	R_clear();
        strcpy(stmfn, teoae.stim_file);
	w_dis = &w_sig;
    } while (calculate_it() != 0);

    memcpy(savbuf, outbuf, (size_t)(buflen) * sizeof(int32_t));

    swp1set = teoae.swp1set;
    do_teoae_task();

    set_test_opt(1, 1, 1);
    top_message("           ");
    dsprst(TRUE);
}
