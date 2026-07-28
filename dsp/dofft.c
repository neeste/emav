/* dofft.c */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fftlib.h>
#include <menu.h>
#include <util.h>
#include "savage.h"

#define     EMIN	((float)1e-20)

int     display(short *, int, int, int, int);

int     lpsflg = 0;

extern int fft_flag, display_reset, swp1set, dfgc;

static float *fbuf = NULL;
static int size = 0;

static int
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
	if (n > size) {
	    if (fbuf != NULL)
		free(fbuf);
	    fbuf = (float *) calloc(n + 2, sizeof(float));
	    size = (fbuf == NULL) ? 0 : n;
	}
    }
    return (size);
}

static void
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
	fbuf[ir] = (float)((a * a + b * b) / c);
	fbuf[ii] = (float)((fbuf[ir] > eps) ? (atan2(b, a) / tpi) : 0);
    }
}

static void
save_fft(float *f, int n)
{
    int     i;
    static float eps = (float)1e-40;

    for (i = 0; i < n; i++) {
	if (fbuf[i * 2] < eps)
	    f[i] = (float)sqrt(eps);
	else
	    f[i] = (float)sqrt(fbuf[i * 2]);
    }
}

void
show_fft(int xrange, int yrange, int levref, int color, int reset)
{
    int     i, n;
    float   dt, a, lpsval;
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
    lpsval = levref ? (float) (10 * log10(dt)) : 0;

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
    (void) display(ibuf, (int) (a + 0.5), display_reset, yrange, xrange);
    fft_flag = 0;
    w_dis = w;

    display_reset = 0;
}

static int
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
