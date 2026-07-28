/* disadbuf.c */

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <menu.h>
#include "savage.h"
#include "color.h"

int     dsploadset(int);

SIG_V   sig_v;

extern int fft_flag;
extern int lpsflg;
extern TOGGLETYPE distype;
extern WIND *w_dis;

static double ad_sen = 1, mp_sen = 1;

void
dis_units(double ad, double mp)
{
    ad_sen = ad;
    mp_sen = mp;
}

double
dis_fullscale(double v)
{
    double m;
    int     r, d;
    static  char *units[2][3] = {
        {"uV ", "mV ", " V "},
        {"uPa", "mPa", " Pa"}, 
    };

    if (v < 1) {
        m = 1000;
        r = 0;
    } else if (v < 1000) {
        m = 1;
        r = 1;
    } else {
        m = 0.001;
        r = 2;
    }
    v *= m;
    if (v < 1) {
	d = 2;
    } else if (v < 10) {
	d = 1;
    } else {
	d = 0;
    }
    txtpar.text_wind_len = 10;
    text_color(w_dis->wfgc, w_dis->wbgc);
    gprintf(w_dis->xbot - 60, w_dis->ytop + txtpar.menu_height,
	"%4.*f%s", d, v, units[distype.at][r]);

    return (m);
}

double
dec_inc(double *s, double *lv)
{
    double  a, b, c, upv;
    int     om;
    int32_t    m;

    upv = distype.at ? 1000 / mp_sen : 1000;    /* units per volt */
    a = (*s) * (*lv) * upv;

    if(a < 1e-38) a = 1e-38;
    om = (int) log10(a);
    if (a < 1)
	om--;
    b = pow(10, om);
    a /= b;
    m = (int32_t) a;
    a -= m;
    a *= 10;
    om = (int) a;
    a -= om;
    if (a > 0)
	om++;
    a = (float) m + (float) om / 10;
    if (a > 5)
	c = 1;
    else if (a > 2)
	c = 0.5;
    else
	c = 0.2;
    a *= b;
    c *= b;
    m = (int32_t) *lv;
    while (((*s) * m * upv) < a)
	m++;
    *lv = m;
    *s *= m;
    return (c);
}

int
most_pos(int a, int b, int c, int d)
{
    if (a >= b && a >= c && a >= d)
	return (a);
    if (b >= c && b >= d)
	return (b);
    if (c >= d)
	return (c);
    return (d);
}

double
most_pos_f(double a, double b, double c, double d)
{
    if (a >= b && a >= c && a >= d)
	return (a);
    if (b >= c && b >= d)
	return (b);
    if (c >= d)
	return (c);
    return (d);
}

int
display(short *bptr, int np, int reset, int yrange, int xrange)
{
    char   *fmt;
    double  lval;
    double  v, sum, sumsq, upv;
    int     i, xrn, yrn, t;
    int     x0, y0, x1, y1;
    int     xofst, yofst;
    short     min, max, val;
    static double xgain, ygain, sc;
    static int fft_ref = 0;
    static int ref = 0;
    static char *levfmt[4] = {"%3ddBSPL", "%3ddBLps", "%3ddBcgs", "%3ddBP/V"};

    if (np <= 0)
	return (0);
    upv = distype.at ? 1000 / mp_sen : 1000;    /* units per volt */
    sum = sumsq = 0;
    min = max = bptr[0];
    for (i = 0; i < np; i++) {
	val = bptr[i];
	v = (float) val;
	sum += (float) val;
	if (val < min)
	    min = val;
	if (val > max)
	    max = val;
	sumsq = v * v;
    }

    xrn = w_dis->xbot - w_dis->xtop;
    yrn = w_dis->ybot - w_dis->ytop;
    xofst = w_dis->xtop + 1;
    yofst = (w_dis->ytop + w_dis->ybot) / 2;

    v = 1 / ad_sen;
    if (!fft_flag) {
        lval = (sumsq - sum * sum / np) / np;
        sig_v.rms = (float) ((lval > 0) ? sqrt(lval) * v : 0);
	sig_v.dco = (float) (sum / np * v);
	sig_v.ppk = (float) (max * v);
	sig_v.npk = (float) (min * v);
	sig_v.vpp = sig_v.ppk - sig_v.npk;
	if (sig_v.vpp <= 0)
	    sig_v.vpp = (float) 1e-30;
	sig_v.mvdc2 = sig_v.dco * 1000;
	if (reset > 0) {
	    lval = 2.0 * most_pos(min, max, -min, -max);
	    if (lval < 1)
		lval = 1;
	    sumsq = dec_inc(&v, &lval);
	    sc = v;
	    v = sc * upv;
	    sumsq *= (float) yrn / v;
	    i = yrn / 2;
	    if (sumsq > 1)
		for (sum = sumsq; (int) sum <= i; sum += sumsq) {
		    x0 = (int) sum;
		    y0 = yofst + x0;
		    gr_line(w_dis->xtop, y0, w_dis->xtop + 4, y0, menucolor.boxc);
		    gr_line(w_dis->xbot, y0, w_dis->xbot - 4, y0, menucolor.boxc);
		    y0 = yofst - x0;
		    gr_line(w_dis->xtop, y0, w_dis->xtop + 4, y0, menucolor.boxc);
		    gr_line(w_dis->xbot, y0, w_dis->xbot - 4, y0, menucolor.boxc);
		}
	} else {
	    v = sc * upv;
	}
	dis_fullscale(v);
    } else {
	if (reset > 0) {
	    fft_ref = max;
            text_color(w_dis->wfgc, w_dis->wbgc);
            fmt = levfmt[lpsflg];
            gprintf(w_dis->xbot - 68, w_dis->ytop - 3, fmt, fft_ref);
	    sumsq = (float) yrn * 10 / yrange;
	    sum = sumsq + w_dis->ytop;
	    for (y0 = (int) sum; y0 < w_dis->ybot; sum += sumsq) {
		y0 = (int) sum;
		gr_line(w_dis->xtop, y0, w_dis->xtop + 4, y0, menucolor.boxc);
		gr_line(w_dis->xbot, y0, w_dis->xbot - 4, y0, menucolor.boxc);
	    }
	    gr_line(w_dis->xtop, yofst, w_dis->xtop + 8, yofst, menucolor.boxc);
	    gr_line(w_dis->xbot - 8, yofst, w_dis->xbot, yofst, menucolor.boxc);
	}
	ref = fft_ref - yrange / 2;
	for (i = 0; i < np; i++)
	    bptr[i] -= ref;
	lval = yrange;
    }
    xgain = (float) np / (float) xrn;
    if (reset > 0)
	ygain = (float) lval / (float) yrn;
    if (fft_flag)
	sumsq = (float) xrn / xrange;
    else
	sumsq = (float) xrn / (float) np *(float) rate / 1000;

    x0 = xofst;
    y0 = yofst - (int) (bptr[0] / ygain);
    if (reset < 0)
	reset = 1;
    else
	reset = 0;
    mouse_exclude(w_dis->xtop, w_dis->ytop, w_dis->xbot, w_dis->ybot);
    min = w_dis->xbot - 1;
    xrn = w_dis->ytop + 1;
    yrn = w_dis->ybot - 1;
    y0 = limit(xrn, y0, yrn);
    for (i = 1; i < np; i++) {
	x1 = xofst + (int) (i / xgain);
	y1 = yofst - (int) (bptr[i] / ygain);
	x1 = limit(xofst, x1, min);
	y1 = limit(xrn, y1, yrn);
	gr_line(x0, y0, x1, y1, dfgc);
	x0 = x1;
	y0 = y1;
	if (reset) {
	    max = yrn - 8;
	    if (max > y1)
		gr_line(x1, max, x1, y1, dfgc);
	}
    }

    i = 0;
    for (sum = (float) xofst; (int) sum <= w_dis->xbot; sum += sumsq) {
	x0 = (int) sum;
	t = (i++ % 10) ? 4 : 8;
	gr_line(x0, w_dis->ytop, x0, w_dis->ytop + t, menucolor.boxc);
	gr_line(x0, w_dis->ybot, x0, w_dis->ybot - t, menucolor.boxc);
    }
    mouse_show(1);
    return (1);
}

int
disply(int N, float *p, int np, int reset, int rmdc)
{
    int     i, d, xrn, yrn, xt, xb, yt, yb, t;
    int     x0, y0, x1, y1;
    int     xofst, yofst;
    double   v, sum, sumsq, upv, vi, yi, yv;
    double   min, max, val;
    double   lval, dc_val = 0;
    static double xgain = 1, ygain = 1, sc = 1;

    if ((np <= 0) || (N <= 0))
	return (0);
    upv = distype.at ? 1000 / mp_sen : 1000;    /* units per volt */
    if(rmdc) {
        sum = 0;
	for (i = 0; i < np; i++)
	    sum += p[i];
        dc_val = sum / np;
	for (i = 0; i < np; i++)
	    p[i] -= (float) dc_val;
    }
    sum = sumsq = 0;
    min = max = p[0];
    for (i = 0; i < np; i++) {
	val = p[i];
	sum += val;
	if (val < min)
	    min = val;
	if (val > max)
	    max = val;
	sumsq += val * val;
    }
    v = 1 / (N * ad_sen);
    val = (sumsq - sum * sum / np) / np;
    sig_v.rms = (float) ((val > 0) ? sqrt(val) * v : 0);
    sig_v.dco = (float) (rmdc ? (dc_val * v) : ((sum / np) * v));
    sig_v.ppk = (float) (max * v);
    sig_v.npk = (float) (min * v);
    sig_v.vpp = sig_v.ppk - sig_v.npk;
    sig_v.mvdc2 = sig_v.dco * 1000;

    xrn = w_dis->xbot - w_dis->xtop;
    yrn = w_dis->ybot - w_dis->ytop;
    xofst = w_dis->xtop + 1;
    yofst = (w_dis->ytop + w_dis->ybot) / 2;

    mouse_exclude(w_dis->xtop, w_dis->ytop, w_dis->xbot, w_dis->ybot);
    if (reset) {
	lval = 2 * most_pos_f(min, max, -min, -max);
        sc = v * lval;
        if (reset > 0) {
            vi = dec_inc(&v, &lval);
            sc = v;
            v = sc * upv;
        } else {
	    v = sc * upv;
	    vi = v / 10;
	}
        yi = yrn * (vi / v);
	vi *= dis_fullscale(v);
	i = yrn / 2;
	if (yi > 1) {
	    x0 = yofst - (int) yi + txtpar.font_height / 2;
	    if (vi > 1)
		d = 0;
	    else if (vi > 0.1)
		d = 1;
	    else
		d = 2;
	    if (vi < 1000) {
                text_color(w_dis->wfgc, w_dis->wbgc);
		gprintf(w_dis->xtop + 6, x0, "%.*f", d, vi);
	    }
            xt = w_dis->xtop;
            xb = w_dis->xbot;
	    for (yv = yi; ((int) yv) <= i; yv += yi) {
		y0 = yofst + (int) yv;
		gr_line(xt, y0, xt + 4, y0, menucolor.boxc);
		gr_line(xb, y0, xb - 4, y0, menucolor.boxc);
		y0 = yofst - (int) yv;
		gr_line(xt, y0, xt + 4, y0, menucolor.boxc);
		gr_line(xb, y0, xb - 4, y0, menucolor.boxc);
	    }
	}
	ygain = lval / (float) yrn / N;
    }
    xgain = (float) np / (float) xrn;
    sumsq = (float) xrn / (float) np *(float) rate / 1000;

    xt = xofst;
    xb = w_dis->xbot - 1;
    yt = w_dis->ytop + 1;
    yb = w_dis->ybot - 1;
    sum = ygain * N;
    x0 = xofst;
    y0 = yofst - (int) (p[0] / sum);
    y0 = limit(yt, y0, yb);
    for (i = 0; i < (int) np; i++) {
	x1 = xofst + (int) (i / xgain);
	y1 = yofst - (int) (p[i] / sum);
	x1 = limit(xt, x1, xb);
	y1 = limit(yt, y1, yb);
	gr_line(x0, y0, x1, y1, dfgc);
	x0 = x1;
	y0 = y1;
    }

    gr_line(w_dis->xtop, yofst, w_dis->xtop + 8, yofst, menucolor.boxc);
    gr_line(w_dis->xbot - 8, yofst, w_dis->xbot, yofst, menucolor.boxc);
    i = 0;
    for (sum = (float) xofst; (int) sum <= w_dis->xbot; sum += sumsq) {
	x0 = (int) sum;
	t = (i++ % 10) ? 4 : 8;
	gr_line(x0, w_dis->ytop, x0, w_dis->ytop + t, menucolor.boxc);
	gr_line(x0, w_dis->ybot, x0, w_dis->ybot - t, menucolor.boxc);
    }
    mouse_show(1);
    return (1);
}
