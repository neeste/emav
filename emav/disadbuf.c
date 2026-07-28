/* ============================= disadbuf.c =========================== */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <menu.h>
#include "savage.h"
#include "color.h"
#include "emav.h"		/* function prototyes */

extern WIND *w_dis;

SIG_V   sig_v;

extern int swp1set, lpsflg, fft_flag;
extern TOGGLETYPE distype;
static double ad_sen = 1, mp_sen = 1;

void
dis_units(double ad, double mp)
{
    ad_sen = ad;
    mp_sen = mp;
}

static void
dis_fullscale(double v)
{
    int     r, d, cw;
    static  char *units[2][3] = {
        {"uV ", "mV ", " V "},
        {"uPa", "mPa", " Pa"}, 
    };

    if (v < 1) {
        v *= 1000;
        r = 0;
    } else if (v < 1000) {
        r = 1;
    } else {
        v /= 1000;
        r = 2;
    }
    if (v < 1) {
	d = 2;
    } else if (v < 10) {
	d = 1;
    } else {
	d = 0;
    }
    txtpar.text_wind_len = 9;
    cw = txtpar.font_width;
    text_color(w_dis->wfgc, w_dis->wbgc);
    gprintf(w_dis->xbot - 9 * cw, w_dis->ytop + txtpar.menu_height,
	"%5.*f%s", d, v, units[distype.at][r]);
}

double
dec_inc(double *s, int32_t *lv)
{
    double  a, b, c, upv;
    int     om;
    int32_t    m;

    upv = distype.at ? 1000 / mp_sen : 1000;
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

double
chkint(double d)
{
    double m;
    int x, y;

    gr_scrsiz(&x, &y);
    m = x / 100;	    // min. interval
    if (d < m * 0.001)
	d = m;
    else if (d < m * 0.002)
	d *= 1000;
    else if (d < m * 0.004)
	d *= 500;
    else if (d < m * 0.01)
	d *= 200;
    else if (d < m * 0.02)
	d *= 100;
    else if (d < m * 0.04)
	d *= 50;
    else if (d < m * 0.1)
	d *= 20;
    else if (d < m * 0.2)
	d *= 10;
    else if (d < m * 0.4)
	d *= 5;
    else if (d < m * 1)
	d *= 2;
    else if (d < m * 2)
	d *= 1;
    else if (d < m * 4)
	d *= 0.5;
    else if (d < m * 10)
	d *= 0.2;
    else if (d < m * 20)
	d *= 0.1;
    else if (d < m * 40)
	d *= 0.05;
    return (d);
}

int
display(short *bptr, int np, int reset, int yrange, int xrange)
{
    char   *fmt;
    double  v, sum, sumsq, dx, dy, xx, yy;
    int     i, xrn, yrn;
    int     x0, y0, x1, y1, cw, ch;
    int     xofst, yofst;
    int32_t    lval;
    short   min, max, val;
    static double xgain, ygain, sc;
    static int ref = 0;    
    static int fft_ref = 0;

    if (np <= 0)
	return (0);
    sum = sumsq = 0;
    min = max = bptr[0];
    for (i = 0; i < np; i++) {
	val = bptr[i];
	v = val;
	sum += val;
	if (val < min)
	    min = val;
	if (val > max)
	    max = val;
	sumsq = v * v;
    }

    v = 1 / ad_sen;
    xrn = w_dis->xbot - w_dis->xtop;
    yrn = w_dis->ybot - w_dis->ytop;
    xofst = w_dis->xtop + 1;
    yofst = (w_dis->ytop + w_dis->ybot) / 2;

    sig_v.dco = (float) (sum / np * v);
    sig_v.rms = (float) (sqrt(sumsq / np) * v);
    sig_v.ppk = (float) (max * v);
    sig_v.npk = (float) (min * v);
    sig_v.vpp = (sig_v.ppk > sig_v.npk) ? (sig_v.ppk - sig_v.npk) : (float) 1e-30;
    sig_v.mvdc2 = sig_v.dco * 1000;

    if (!fft_flag) {
	if (reset > 0) {
	    lval = 2L * most_pos(min, max, -min, -max);
	    if (lval < 1)
		lval = 1;
	    sumsq = dec_inc(&v, &lval);
	    sc = v;
	    v = units_conv(sc);
	    x0 = w_dis->xtop;
	    x1 = w_dis->xbot;
	    dy = chkint(sumsq * (float) yrn / v);
	    for (yy = dy; yy <= (double) yrn / 2; yy += dy) {
	        y0 = yofst + (int) yy;
	        gr_line(x0, y0, x0 + 4, y0, menucolor.boxc);
	        gr_line(x1, y0, x1 - 4, y0, menucolor.boxc);
	        y0 = yofst - (int) yy;
	        gr_line(x0, y0, x0 + 4, y0, menucolor.boxc);
	        gr_line(x1, y0, x1 - 4, y0, menucolor.boxc);
	    }
	} else {
	    v = units_conv(sc);
	}
	dis_fullscale(v);
    } else {
	if (xrange < 1)
	    xrange = 1;
	if (yrange < 1)
	    yrange = 1;
	if (reset > 0) {
	    cw = txtpar.font_width;
	    ch = txtpar.font_height;
            x0 = w_dis->xbot - cw * 8 - cw / 2;
            y0 = w_dis->ytop + ch + 4;
            fmt = lpsflg ? "%3ddBLps" : "%3ddBSPL";
	    fft_ref = max + 6;
            text_color(w_dis->wfgc, w_dis->wbgc);
            gprintf(x0, y0, fmt, fft_ref);
	    x0 = w_dis->xtop;
	    x1 = w_dis->xbot;
	    dy = chkint((float) yrn * 10 / yrange);
	    for (yy = dy + w_dis->ytop; yy < (double) w_dis->ybot; yy += dy) {
		y0 = (int) yy;
		gr_line(x0, y0, x0 + 4, y0, menucolor.boxc);
		gr_line(x1, y0, x1 - 4, y0, menucolor.boxc);
	    }
	    y0 = yofst;
	    y1 = w_dis->ybot;
	    gr_line(x0, y0, x0 + 8, y0, menucolor.boxc);
	    gr_line(x1 - 8, yofst, x1, yofst, menucolor.boxc);
	    gr_line(x0, y1, x1, y1, menucolor.boxc);
            for (x0 = w_dis->xtop + 2; x0 < w_dis->xbot - 4; x0 += 4)
                gr_setpix(x0, y0, menucolor.boxc);
	}
	ref = fft_ref - yrange / 2;
	for (i = 0; i < np; i++)
	    bptr[i] -= ref;
	lval = yrange;
    }
    xgain = (float) xrn / (float) np;
    if (reset > 0)
	ygain = (float) yrn / (float) lval;

    x0 = xofst;
    y0 = yofst - (int) (bptr[0] * ygain);
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
	x1 = xofst + (int) (i * xgain);
	y1 = yofst - (int) (bptr[i] * ygain);
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

    xrn = w_dis->xbot - w_dis->xtop;
    if (fft_flag)
	dx = chkint((double) xrn / xrange);
    else
	dx = chkint(xgain * (double) rate / 1000);
    x0 = w_dis->xtop;
    x1 = w_dis->xbot;
    y0 = w_dis->ytop;
    y1 = w_dis->ybot;
    gr_line(x0, y1, x1, y1, menucolor.boxc);
    for (xx = (double) xofst; xx <= (double) w_dis->xbot; xx += dx) {
	x0 = (int) (xx + 0.5);
	gr_line(x0, y0, x0, y0 + 4, menucolor.boxc);
	gr_line(x0, y1, x0, y1 - 4, menucolor.boxc);
    }
    mouse_show(1);
    return (1);
}

int
disply(int N, int32_t *p, int np, int reset)
{
    double  v, sum, sumsq, dx, dy, xx, yy;
    int     i, xrn, yrn;
    int     x0, y0, x1, y1;
    int     xofst, yofst;
    int32_t    min, max, val;
    int32_t    lval;
    static double xgain, ygain, sc;

    if ((np <= 0) || (N == 0))
	return (0);
    sum = sumsq = 0;
    min = max = p[0];
    for (i = 0; i < np; i++) {
	val = p[i];
	v = (float) val;
	sum += (float) val;
	if (val < min)
	    min = val;
	if (val > max)
	    max = val;
	sumsq += v * v;
    }

    val = (int32_t) (sum / np);
    v = 1 / (N * ad_sen);

    sig_v.dco = (float) (sum / np * v);
    sig_v.rms = (float) (sqrt(sumsq / np) * v);
    sig_v.ppk = (float) (max * v);
    sig_v.npk = (float) (min * v);
    sig_v.vpp = sig_v.ppk - sig_v.npk;
    sig_v.mvdc2 = sig_v.dco * 1000;

    xrn = w_dis->xbot - w_dis->xtop;
    yrn = w_dis->ybot - w_dis->ytop;
    xofst = w_dis->xtop + 1;
    yofst = (w_dis->ytop + w_dis->ybot) / 2;

    if (reset) {
	lval = 2L * most_pos_L(min, max, -min, -max);
	if (lval < 1)
	    lval = 1;
	sum = dec_inc(&v, &lval);
	sc = v;
	v = units_conv(sc);
        i = 3;
        if (sum > 0.5)
	    i = 0;
	else if (sum > 0.05)
	    i = 1;
	else if (sum > 0.005)
	    i = 2;
	x0 = w_dis->xtop + 6;
        y0 = yofst - (int) sumsq + txtpar.font_height / 2;
	if (sum <= 10) {
            text_color(w_dis->wfgc, w_dis->wbgc);
	    gprintf(x0, y0, "%.*f", i, sum);
	}
        x0 = w_dis->xtop;
        x1 = w_dis->xbot;
	dy = chkint(sum * (float) yrn / v);
	for (yy = dy; yy <= (double) yrn / 2; yy += dy) {
	    y0 = yofst + (int) yy;
	    gr_line(x0, y0, x0 + 4, y0, menucolor.boxc);
	    gr_line(x1, y0, x1 - 4, y0, menucolor.boxc);
	    y0 = yofst - (int) yy;
	    gr_line(x0, y0, x0 + 4, y0, menucolor.boxc);
	    gr_line(x1, y0, x1 - 4, y0, menucolor.boxc);
	}
	ygain = (float) yrn / (float) lval;
    } else {
	v = units_conv(sc);
    }
    dis_fullscale(v);
    xgain = (float) xrn / (float) np;

    max = w_dis->xbot - 1;
    xrn = w_dis->ytop + 1;
    yrn = w_dis->ybot - 1;
    x0 = xofst;
    y0 = yofst - (int) (p[0] / sum);
    y0 = limit(xrn, y0, yrn);
    mouse_exclude(w_dis->xtop, w_dis->ytop, w_dis->xbot, w_dis->ybot);
    for (i = 0; i < np; i++) {
	x1 = xofst + (int) (i * xgain);
	y1 = yofst - (int) (p[i] * ygain);
	x1 = limit(xofst, x1, max);
	y1 = limit(xrn, y1, yrn);
	gr_line(x0, y0, x1, y1, dfgc);
	x0 = x1;
	y0 = y1;
    }
    x0 = w_dis->xtop;
    x1 = w_dis->xbot;
    y0 = yofst;
    y1 = w_dis->ybot;
    gr_line(x0, y0, x0 + 8, y0, menucolor.boxc);
    gr_line(x1 - 8, y0, x1, y0, menucolor.boxc);
    gr_line(x0, y1, x1, y1, menucolor.boxc);
    dx = chkint(xgain * (double) rate / 1000);
    y0 = w_dis->ytop;
    y1 = w_dis->ybot;
    for (xx = (double) xofst; xx <= (double) w_dis->xbot; xx += dx) {
	x0 = (int) (xx + 0.5);
	gr_line(x0, y0, x0, y0 + 4, menucolor.boxc);
	gr_line(x0, y1, x0, y1 - 4, menucolor.boxc);
    }
    mouse_show(1);
    return (1);
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

int32_t
most_pos_L(int32_t a, int32_t b, int32_t c, int32_t d)
{
    if (a >= b && a >= c && a >= d)
	return (a);
    if (b >= c && b >= d)
	return (b);
    if (c >= d)
	return (c);
    return (d);
}

void
show_xlabel(double vb, double vr, int bv, char *u)
{
    double  xx, dx, dv, xv, x0;
    int     j, dp, y, rs, wx, rm = 0, cw;

    y = w_dis->ybot - 1;
    rs = w_dis->xbot;
    cw = txtpar.font_width;
    text_color(w_dis->wfgc, w_dis->wbgc);
    if (u && *u) {
        rm = strlen(u) * cw + cw / 2;
        gprintf(rs - rm + 2, y, "%s", u);
        rs -= rm;
    }
    if (vr <= 0)
	return;
    dx = (w_dis->xbot - w_dis->xtop) / vr;
    wx = ((vr < 20) ? 1 : ((vr < 200) ? 2 : 3)) * cw;
    if (dx < wx * 0.05) {
        dv = 100;
    } else if (dx < wx * 0.1) {
        dv = 50;
    } else if (dx < wx * 0.25) {
        dv = 20;
    } else if (dx < wx * 0.5) {
        dv = 10;
    } else if (dx < wx) {
        dv = 5;
    } else if (dx < wx * 2.5) {
        dv = 2;
    } else if (dx < (wx + 2 * cw) * 5) {
        dv = 1;
    } else if (dx < (wx + 2 * cw) * 10) {
        dv = 0.5;
    } else if (dx < (wx + 2 * cw) * 25) {
        dv = 0.2;
    } else {
        dv = 0.1;
    }
    dx *= dv;
    dp = ( dv < 1) ? 1 : 0; // decimal places
    xv = vb;
    x0 = w_dis->xtop + 1;
    if (!bv) {
        x0 += dx;
        xv += dv;
    }
    for (xx = x0; (int) xx < rs; xx += dx) {
        wx = ((xv < 10) ? 1 : ((xv < 100) ? 2 : 3)) * cw;
        if (dp)
            wx += (dp + 1) * cw;
        j = (int) (xx - wx / 2.0);
        if ((j + wx) < rs) {
            gprintf(j, y, "%.*f", dp, xv);
        } else if (dp) {
            wx = ((xv < 10) ? 1 : ((xv < 100) ? 2 : 3)) * cw;
            j = (int) (xx - wx / 2.0);
            if ((j + wx) < rs)
                gprintf(j, y, "%.0f", xv);
        }
        xv += dv;
    }
}

