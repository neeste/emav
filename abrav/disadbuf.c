/* disadbuf.c */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <menu.h>
#include <util.h>
#include "savage.h"
#include "color.h"

int     dsploadset(int);
void    text_color(int, int);
void    clr_scr();

SIG_V   sig_v;

extern int fft_flag;
extern int lpsflg;
extern TOGGLETYPE distype, few;
extern WIND *w_dis;

static double ad_sen = 1, mp_sen = 1, ep_sen = 1;

void
dis_units(double ad, double mp, double ep)
{
    ad_sen = ad;
    mp_sen = mp;
    ep_sen = ep;
}

double
dis_fullscale(double v)
{
    double m;
    int     r, d;
    static  char *units[2][4] = {
        {"nV ", "uV ", "mV ", " V "},
        {"nPa", "uPa", "mPa", " Pa"}, 
    };

    if (v < 0)
        v = -v;
    if (v < 1e-3) {
        m = 1e6;
        r = 0;
    } else if (v < 1) {
        m = 1e3;
        r = 1;
    } else if (v < 1e3) {
        m = 1;
        r = 2;
    } else {
        m = 1e-3;
        r = 3;
    }
    v *= m;
    if (v < 1) {
	d = 2;
    } else if (v < 10) {
	d = 1;
    } else {
	d = 0;
    }
    txtpar.text_wind_len = 8;
    text_color(w_dis->wfgc, w_dis->wbgc);
    (void) gprintf(w_dis->xbot - 8 * txtpar.font_width, 
	w_dis->ytop + txtpar.menu_height,
	"%4.*f%s", d, v, units[distype.at][r]);

    return (m);
}

double
dec_inc(float *s, float *lv)
{
    double  a, b, c, sen, upv;
    int     om;
    int32_t    m;

   // upv = distype.at ? 1000 / mp_sen : 1000;    /* units per volt */
    upv = distype.at ? 1000 / mp_sen : 1000;    /* units per volt */
    sen = fabs(*s);
    a = sen * (*lv) * upv;

    if(a < 1e-40) a = 1e-40;
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
    while ((sen * m * upv) < a)
	m++;
    *lv = (float) m;
    *s *= (float) m;
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
    double  sum, sumsq, upv;
    float   v, lval;
    int     i, xrn, yrn, t;
    int     x0, y0, x1, y1;
    int     xofst, yofst;
    int     min, max, val;
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

    v = (float) (1 / ad_sen);
    if (!fft_flag) {
        lval = (float) ((sumsq - sum * sum / np) / np);
        sig_v.rms = (float) ((lval > 0) ? sqrt(lval) * fabs(v) : 0);
	sig_v.dco = (float) ((sum / np) * v);
	sig_v.ppk = (float) ((v > 0) ? max * v : min * v);
	sig_v.npk = (float) ((v > 0) ? min * v : max * v);
	sig_v.vpp = sig_v.ppk - sig_v.npk;
	if (sig_v.vpp <= 0)
	    sig_v.vpp = (float) 1e-30;
	sig_v.mvdc2 = sig_v.dco * 1000;
	if (reset) {
	    lval = (float) (2 * most_pos(min, max, -min, -max));
	    if (lval < 1)
		lval = 1;
	    sumsq = dec_inc(&v, &lval);
	    sc = v;
	    v = (float) (sc * upv);
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
	    v = (float) (sc * upv);
	}
	(void) dis_fullscale(v);
    } else {
	if (reset) {
            x0 = w_dis->xbot - 68;
            y0 = w_dis->ytop + txtpar.menu_height;
            fmt = levfmt[lpsflg];
	    fft_ref = max + 6;
	    text_color(w_dis->wfgc, w_dis->wbgc);
            (void) gprintf(x0, y0, fmt, fft_ref);
	    sumsq = (float) yrn * 10 / yrange;
	    sum = sumsq + w_dis->ytop;
	    for (y0 = (int) sum; y0 < w_dis->ybot; sum += sumsq) {
		y0 = (int) sum;
		gr_line(w_dis->xtop, y0, w_dis->xtop + 4, y0, menucolor.boxc);
		gr_line(w_dis->xbot, y0, w_dis->xbot - 4, y0, menucolor.boxc);
	    }
	    gr_line(w_dis->xtop, yofst, w_dis->xtop + 8, yofst, menucolor.boxc);
	    gr_line(w_dis->xbot - 8, yofst, w_dis->xbot, yofst, menucolor.boxc);
	    gr_line(w_dis->xtop, w_dis->ybot, w_dis->xbot, w_dis->ybot, menucolor.boxc);
	}
	ref = fft_ref - yrange / 2;
	for (i = 0; i < np; i++)
	    bptr[i] -= ref;
	lval = (float) yrange;
    }
    xgain = (double) np / xrn;
    if (reset)
	ygain = (float) lval / (float) yrn;
    if (fft_flag)
	sumsq = (float) xrn / xrange;
    else
	sumsq = (float) xrn / (float) np *(float) rate / 1000;

    x0 = xofst;
    y0 = yofst - (int) (bptr[0] / ygain);
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
disply(double Nope, float *p, int np, int reset, double offset, MARK *mark, int flags)
{
    int     i, d, xrn, yrn, xt, xb, j, cw, rmv_dc = 1;
    int     x0, y0, x1, y1;
    int     xofst, yofst;
    double  sum, sumsq, upv, vi, yi, yv, zofst;
    double  min, max, val, dc_val;
    float   v, lval;
    static double xgain = 1, ygain = 1, sc = 1;


    //return 0;
    if ((np <= 0) || (Nope <= 0.0))
	return (0);
    upv = distype.at ? 1000 / mp_sen : 1000;    /* units per volt */
    v = (float) (1 / (Nope * ad_sen));
    if (flags & D_GAIN)
        v /= (float) ep_sen;
    if (reset && (flags & D_NOUP)) 
    {
	max = (sig_v.vpp / fabs(v) / 2.0);
	min = -max;
    } 
    else 
    {
	sum = sumsq = 0;
	min = max = p[0];
	for (i = 0; i < np; i++)    //find the min, max, and the sum
        {
	    val = p[i];
	    sum += val;
	    if (min > val)
		min = val;
	    if (max < val)
		max = val;
	    sumsq += val * val;
	}
        dc_val = sum / np;          //average value of entire buffer
        if (flags & D_RMDC) {
	    for (i = 0; i < np; i++)
		p[i] -= (float) dc_val;
	    dc_val = 0;
	} else if (!rmv_dc) {
	    dc_val = 0;
	}
        val = (sumsq - sum * sum / np) / np;
        sig_v.rms = (float) ((val > 0) ? sqrt(val) * fabs(v) : 0);
	sig_v.dco = (float) ((sum / np) * v);
	sig_v.ppk = (float) ((v > 0) ? max * v : min * v);
	sig_v.npk = (float) ((v > 0) ? min * v : max * v);
	sig_v.vpp = sig_v.ppk - sig_v.npk;
	sig_v.mvdc2 = sig_v.dco * 1000;
    }
    if (rmv_dc) {
	sum = 0;
	for (i = 0; i < np; i++)
	    sum += p[i];
        dc_val = sum / np;
    } else 
    {
        dc_val = 0;
    }
    xrn = w_dis->xbot - w_dis->xtop;
    yrn = w_dis->ybot - w_dis->ytop;
    xofst = w_dis->xtop + 1;
    yofst = (w_dis->ytop + w_dis->ybot) / 2;
    zofst = (w_dis->ytop + w_dis->ybot) * ((1 - offset) / 2);

    mouse_exclude(w_dis->xtop, w_dis->ytop, w_dis->xbot, w_dis->ybot);
    if (reset) 
    {
	lval = (float) (2 * most_pos_f(min, max, -min, -max));
        sc = v * lval;
        if (flags & D_NOUP) 
        {
	    v = (float) fabs(sc * upv);
	    vi = v / 10;
        } else 
        {
            vi = dec_inc(&v, &lval);
            sc = v;
            v = (float) (sc * upv);
	}
        yi = yrn * (vi / v);
	vi *= dis_fullscale(v);
	i = yrn / 2;
	if (yi > 1) 
        {
	    x0 = yofst - (int) yi + txtpar.font_height / 2;
	    if (vi > 1)
		d = 0;
	    else if (vi > 0.1)
		d = 1;
	    else
		d = 2;
	    if (vi < 1000) 
            {
                text_color(w_dis->wfgc, w_dis->wbgc);
		(void) gprintf(w_dis->xtop + 6, x0, "%.*f", d, vi);
	    }
            xt = w_dis->xtop;
            xb = w_dis->xbot;
	    for (yv = yi; ((int) yv) <= i; yv += yi)
            {
		y0 = yofst + (int) yv;
		gr_line(xt, y0, xt + 4, y0, menucolor.boxc);
		gr_line(xb, y0, xb - 4, y0, menucolor.boxc);
		y0 = yofst - (int) yv;
		gr_line(xt, y0, xt + 4, y0, menucolor.boxc);
		gr_line(xb, y0, xb - 4, y0, menucolor.boxc);
	    }
	}
	ygain = lval / (float) yrn / Nope;
    }
    xgain = (float) np / (float) xrn;
    sumsq = (float) rate / xgain / 1000.0;

    max = w_dis->xbot - 1;
    xrn = w_dis->ytop + 1;
    yrn = w_dis->ybot - 1;

    val = 1.0;
    if(few.at) 
    {
	switch(few.at) {
	case 1:
	    x0 = 128;
	    break;
	case 2:
	    x0 = 256;
	    break;
	default:
	    x0 = 512;
	    break;
	}
	val = (float) np / (float) x0;
	/* if less points found just display them all */
	if(val < 1.0) val = 1.0;
    }

    sum = (flags & D_FLIP) ? -ygain * Nope : ygain * Nope;
    zofst += dc_val / sum;
    x0 = xofst;
    y0 = (int) (zofst - p[0] / sum);
    y0 = limit(xrn, y0, yrn);
    cw = txtpar.font_width;

    for (vi = 0.0, i = 0; i < (int) np; vi += val, i = nint(vi)) //this is where we draw the graph
    {
	x1 = xofst + (int) (i / xgain);
	y1 = (int) (zofst - p[i] / sum);
	x1 = limit(xofst, x1, (int) max);
	y1 = limit(xrn, y1, yrn);
	gr_line(x0, y0, x1, y1, dfgc);
	x0 = x1;
	y0 = y1;
    }
    if(mark != NULL)    //this is where we mark the graph (0 to V')
    {
	for(j = 0; j < 7; j++) {
	    x0 = mark->loc[j] - mark->beg;
	    x1 = xofst + (int) (x0 / xgain);
	    if(x1 > xofst && x1 < max) {
		y1 = (int) (zofst - p[x0] / sum);
		if(y1 > xrn && y1 < yrn) {
		    i = strlen(mark->sym[j]);
		    x0 = x1 - i * 4;
		    if(x0 <= xofst)
		    	x0 = xofst + 1;
		    else if((x1 + i * 4) >= max) 
		    	x0 = (int) max - i * cw - 2;
		    y0 = y1 - 12;
		    y1 += 12;
		    if(y0 < xrn) y0 = xrn;
		    if(y1 > yrn) y1 = yrn;
		    gr_line(x1, y0, x1, y1, dfgc);
		    if(y0 - txtpar.font_height <= xrn)
			y0 = y1 + txtpar.font_height;
		    (void) gprintf(x0, y0, mark->sym[j]);
		}
	    }
	}
    }

    if (reset) {
	gr_line(w_dis->xtop, yofst, w_dis->xtop + 8, yofst, menucolor.boxc);
	gr_line(w_dis->xbot - 8, yofst, w_dis->xbot, yofst, menucolor.boxc);
	x1 = 0;
	i = (int) ((float) np * 1000.0 / (float) rate);
	if (i <= 10) {
	    y0 = 10;
	    sumsq /= 10.0;
	} else {
	    y0 = 5;
	    if(sumsq < 2.0) sumsq *= 2.0;
	}
        //this is where we draw the tick marks
	for (sum = (float) xofst; (int) sum <= w_dis->xbot; sum += sumsq)
        {
	    x0 = (int) sum;
	    if (x1 % y0 == 0)
		y1 = 6;
	    else
		y1 = 4;
	    gr_line(x0, w_dis->ytop, x0, w_dis->ytop + y1, menucolor.boxc);
	    gr_line(x0, w_dis->ybot, x0, w_dis->ybot - y1, menucolor.boxc);
	    x1++;
	}
    }
    mouse_show(1);
    return (1);
}

void
show_xlabel(double vb, double vr, int bv, char *u)
{
    double  xx, dx, dv, xv, x0;
    int     j, k, y, rs, wx, cw, rm = 0;

    y = w_dis->ybot - 1;
    rs = w_dis->xbot;
    cw = txtpar.font_width;
    text_color(w_dis->wfgc, w_dis->wbgc);
    if (u && *u) {
        rm = strlen(u) * cw + 4;
        (void) gprintf(rs - rm + 2, y, "%s", u);
        rs -= rm;
    }
    dx = (float) (w_dis->xbot - w_dis->xtop) / vr;
    k = 0;
    if (dx < 2) {
        dx *= 20;
        dv = 20;
    } else if (dx < 4) {
        dx *= 10;
        dv = 10;
    } else if (dx < 8) {
        dx *= 5;
        dv = 5;
    } else if (dx < 20) {
        dx *= 2;
        dv = 2;
    } else if (dx < 80) {
        dv = 1.0;
    } else {
        dx /= 2.0;
        dv = 0.5;
        k = 1;
    }
    xv = vb;
    x0 = w_dis->xtop;
    if (!bv) {
        x0 += dx;
        xv += dv;
    }
    for (xx = x0; (int) xx < rs; xx += dx) {
        wx = ((xv < 10) ? 1 : 2) * cw;
        if (k)
            wx += (k + 1) * cw;
        j = (int) (xx - wx / 2) + 1;
        if ((j + wx) > rs) {
            wx = ((xv < 10) ? 1 : 2) * cw;
            if ((rs - xx) > wx / 2)
                j = (int) (xx - wx / 2) + 1;
            else
                j = (int) (rs - wx);
            (void) gprintf(j, y, "%.0f", xv);
        }
        xv += dv;
    }
}

