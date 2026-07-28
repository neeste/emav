/* dofft.c */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fftlib.h>
#include <menu.h>
#include <util.h>
#include "savage.h"
#include "color.h"
#include "putt.h"

#define     EMIN	(1e-20)
#ifndef WIN32
#define     _hypot      hypot
#endif

void gr_rectf();

int     lpsflg = 0;

extern int fft_flag, display_reset, swp1set, dfgc;

static float *fbuf = NULL;
static int size = 0;

int
alloc_space(int create)
{
    int     n;

    if (create == 0) {
	if (fbuf != NULL) {
	    free(fbuf);
	    fbuf = NULL;
	}
	size = 0;
    } else {
	n = 1;
	while (n < buflen)
	    n *= 2;
	if (n != size) {
	    if (fbuf != NULL)
		free(fbuf);
	    fbuf = (float *) calloc(n + 2, sizeof(float));
	    size = (fbuf == NULL) ? 0 : n;
	}
    }
    return (size);
}

float
normalize(float v)
{
    float a;

    a = spl_ref * Sen.MP * Sen.AD * size;

    return(v / (a * a / 2));
}

void
retrieve_fft(float *p, int n)
{
    int     i;
    float   s;

    n *= 2;
    if (n > size)
        n = size;
    s = (float) (sqrt(2)/(spl_ref * Sen.MP * Sen.AD * size));
    s = (float) sqrt(normalize(1.0));
    for (i = 0; i < n; i++)
	p[i] = fbuf[i] * s;
}

void
modulesqr()
{
    double   a, b, c, tpi;
    int     i, ii, ir;
    static double eps = 1e-40;

    a = spl_ref * Sen.MP * Sen.AD * size;
    c = a * a / 2;
    tpi = 8 * atan(1.0);
    for (i = 0; i < size / 2; i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
       	a = fbuf[ir];
	b = fbuf[ii];
	fbuf[ir] = (float) ((a * a + b * b) / c);
	fbuf[ii] = (float) ((fbuf[ir] > eps) ? (atan2(b, a) / tpi) : 0);
    }
}

static void
save_fft(float *f, int n)
{
    int     i;
    static double eps = 1e-40;

    for (i = 0; i < n; i++) {
	if (fbuf[i * 2] < eps)
	    f[i] = (float) sqrt(eps);
	else
	    f[i] = (float) sqrt(fbuf[i * 2]);
    }
}

void
show_fft(int xrange, int yrange, int levref, int color, int reset)
{
    double  dt, a, lpsval;
    int     i, n;
    short  *ibuf;
    WIND   *w;
    extern int stim_flag;
    extern WIND w_stim_fft;

    dfgc = color;
    display_reset = reset;

    n = size / 2;
    if (n < 1)
        return;
    dt = ((float) n) / rate;
    lpsflg = levref;
    lpsval = levref ? 10 * log10(dt) : 0;

    ibuf = (short *) fbuf;
    for (i = 0; i < n / 2; i++) {
	a = fbuf[i * 4] + fbuf[i * 4 + 2];
	a = (i <= 1 || a < EMIN) ? EMIN : a;
	ibuf[i] = (int) (10 * log10(a) + lpsval + 0.5);
    }

    w = w_dis;
    w_dis = stim_flag ? &w_stim_fft : &w_spec;
    if (display_reset > 0) {
	gr_rectf(w_dis->xtop + 1, w_dis->ytop + 1, w_dis->xbot - 1,
	    w_dis->ybot - 1, w_dis->wbgc);
    }
    fft_flag = 1;
    a = xrange * dt * 1000;
    display(ibuf, (int) (a + 0.5), display_reset, yrange, xrange);
    fft_flag = 0;
    w_dis = w;

    display_reset = 0;
}

void
fft_A_B(float *p, float *q, int addflg, int sets)
{
    int     i;

    if (swp1set <= 0 || sets <= 0) 
        return;
    if (!alloc_space(1))
        return;

    if (addflg) {
	for (i = 0; i < buflen; i++)
	   fbuf[i] = (p[i] + q[i]) / sets / swp1set;
    } else {
	for (i = 0; i < buflen; i++)
	    fbuf[i] = (p[i] - q[i]) / sets / swp1set;
    }
    for (; i < size; i++)
	fbuf[i] = 0;

    fast(fbuf, size);
    modulesqr();
}

static void
r2p(float *f, int n)
{
    int i, ir, ii;
    double m, p;
    static double e = 1e-40;

    for (i = 0; i <= (n / 2); i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
        m = _hypot(f[ir], f[ii]);
        if (m > e) {
            m = log(m);
            p = atan2(f[ii], f[ir]);
        } else {
            m = -800;
            p = 0;
        }
        f[ir] = (float) m;
        f[ii] = (float) p;
    }
}

static void
p2r(float *f, int n)
{
    int i, ir, ii;
    double m;

    for (i = 0; i <= (n / 2); i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
        m = exp(f[ir]);
        f[ir] = (float) (m * cos(f[ii]));
        f[ii] = (float) (m * sin(f[ii]));
    }
}

void
minph_f(float *z, int nf, double fill)
{
    int     i, ii, ir, n;
    float  *gbuf, d;

    if (nf <= 0)
	return;
    gbuf = (float *) calloc(size + 2, sizeof(float));
    if (gbuf == NULL)
    	return;

    n = nf * 2;
    d = (float) log(fill);
    for (i = 0; i < n; i++)
        gbuf[i] = z[i];
    r2p(gbuf, n);
    for (i = 0; i <= size / 2; i++){
        ir = 2 * i;
        ii = 2 * i + 1;
        if (i >= nf)
            gbuf[ir] = d;
        gbuf[ii] = 0;
    }
    gbuf[0] = gbuf[size] = 0;
    fsst(gbuf, size);

    for (i = 1; i < size / 2; i++)
        gbuf[i] *= 2;
    for (i = size / 2; i < size; i++)
        gbuf[i] = 0;
    fast(gbuf, size);
    p2r(gbuf, n);
    for (i = 0; i < n; i++)
        z[i] = gbuf[i];

    free(gbuf);
    return;
}

int
fft_sig(float *p, int sets)
{
    int     i;

    if (!alloc_space(1))
	return (0);

    for (i = 0; i < buflen; i++)
	fbuf[i] = p[i] / (swp1set * sets);
    for (; i < size; i++)
	fbuf[i] = 0;
    fast(fbuf, size);

    return (size / 2);
}

int
fft_stim(float *p, int sets, int save)
{
    int     i;

    i = fft_sig(p, sets);
    modulesqr();
    if (save && i)
	save_fft((float *) outbuf, i);
    return (i);
}

int
fft_short(short *p, int div_flg)
{
    int     i;
    float   att = 1.0;

    if (!alloc_space(1))
	return (0);

    if (div_flg)
	att = attenu.intern_att;
    for (i = 0; i < buflen; i++)
	fbuf[i] = (float) p[i] / att;
    for (; i < size; i++)
	fbuf[i] = 0;
    fast(fbuf, size);
    i = size / 2;
    modulesqr();
    return (i);
}

void 
cal_gain(SAV_FFT *x_fft)
{
    int     i;

    i = fft_short(stimulus, 1);
    if (!i)
	return;
    save_fft(fbuf, i);
    for (i = 0; i < x_fft->npts; i++)
	x_fft->rp[i] /= fbuf[i];
}

double
pickat(double f)
{
    double  v;
    int     i;

    i = nint(size * (f / rate));
    v = fbuf[i * 2];
    if(v < EMIN) 
        v = EMIN;
    v = 10 * log10(v);
    return (v);
}

double
pkmn(int i1, int i2, int di, float *buf)
{
    double   f, c, mn;
    int     i, ii;

    ii = i1;
    mn = buf[i1*di];
    for (i = i1; i <= i2; i++) {
        if (mn > buf[i*di]) {
            mn = buf[i*di];
            ii = i;
        }
    }
    c = 2 * buf[ii*di] - (buf[(ii+1)*di] + buf[(ii-1)*di]);
    if (fabs(c) < 1e-40)
    	f = ii;
    else
        f = ii + (buf[(ii+1)*di] - buf[(ii-1)*di]) / (2 * c);
    return (f);
}

double
pkmx(int i1, int i2, int di, float *buf)
{
    double   f, c, mx;
    int     i, ii;

    ii = i1;
    mx = buf[i1*di];
    for (i = i1; i <= i2; i++) {
        if (mx < buf[i*di]) {
            mx = buf[i*di];
            ii = i;
        }
    }
    c = 2 * buf[ii*di] - (buf[(ii+1)*di] + buf[(ii-1)*di]);
    if (fabs(c) < 1e-40)
    	f = ii;
    else
        f = ii + (buf[(ii+1)*di] - buf[(ii-1)*di]) / (2 * c);
    return (f);
}

double
pick_min(double f1, double f2)
{
    double  df;
    int     i1, i2;

    df = ((double) rate) / size;
    i1 = (int) (f1 / df + 0.5);
    i2 = (int) (f2 / df + 0.5);
    return (pkmn(i1, i2, 2, fbuf) * df);
}

double
pick_max(double f1, double f2)
{
    double  df;
    int     i1, i2;

    df = ((double) rate) / size;
    i1 = (int) (f1 / df + 0.5);
    i2 = (int) (f2 / df + 0.5);
    return (pkmx(i1, i2, 2, fbuf) * df);
}

double
get_ph(double f, double lp)
{
    double  df, ph;
    int     i;

    df = ((float) rate) / size;
    i = nint(f / df);
    ph = fbuf[i*2+1];
    while ((lp - ph) > 0.5)
        ph += 1;
    while ((ph - lp) > 0.5)
        ph -= 1;
    return (ph);
}
