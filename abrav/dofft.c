/* dofft.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <fftlib.h>
#include <menu.h>
#include <token.h>
#include "savage.h"
#include "color.h"

#define     EMIN	(1e-20)

int     display(short *, int, int, int, int);
int     nint(double);
void    show_xlabel(double, double, int, char *);

int     lpsflg = 0;

extern int display_reset, fft_flag, swp1set, dfgc;
static float *fbuf = NULL;
static int size = 0;

int
alloc_space(int create)
{
    int     n;

    if (!create && fbuf != NULL) {
	free(fbuf);
	fbuf = NULL;
	return (0);
    }
    n = 1;
    while (n < buflen)
	n *= 2;
    if (fbuf != NULL)
	free(fbuf);
    fbuf = (float *) calloc((n + 2), sizeof(float));
    if (fbuf == NULL)
	return (0);
    size = n;
    return (n);
}

double
normalize(double v)
{
    double   a, b;

    a = spl_ref * Sen.MP * Sen.AD * size;
    b = a * a / 2;

    return (v / b);
}

void
modulesqr()
{
    double  a, b, c, d;
    int     i;

    a = spl_ref * Sen.MP * Sen.AD * size;
    b = a * a / 2.0;
    for (i = 0; i < size; i++) {
	c = fbuf[i++];
	d = fbuf[i];
	fbuf[i / 2] = (float) ((c * c + d * d) / b);
    }
}

void
save_fft(float *f, int n)
{
    int     i;

    for (i = 0; i < n; i++) {
	if (fbuf[i] <= 1E-80)
	    f[i] = (float) 1e-40;
	else
	    f[i] = (float) sqrt(fbuf[i]);
    }
}

void
show_fft(int xrange, int yrange, int levref, int color, int reset)
{
    double  dt, a, lpsval;
    int     i;
    short  *ibuf;
    WIND   *w;
    extern int stim_flag;
    extern WIND w_stim_fft;

    dfgc = color;
    display_reset = reset;

    if (size < 2)
	return;
    ibuf = (short *) fbuf;

    dt = ((float) size) / rate;
    lpsval = 10 * log10(dt);
    if (xrange <= 1) {
        for (i = 0; i < size; i++) {
            a = fbuf[i];
            a = (i == 0 || a < EMIN) ? EMIN : a;
            ibuf[i] = (int) (10 * log10(a) + lpsval + 0.5);
        }
    } else {
        dt /= 2;
        lpsval = 10 * log10(dt);
        for (i = 0; i < size; i++) {
            a = fbuf[i++];
            a += fbuf[i];
            a = (i == 1 || a < EMIN) ? EMIN : a;
            ibuf[i / 2] = (int) (10 * log10(a) + lpsval + 0.5);
        }
    }

    w = w_dis;
    w_dis = stim_flag ? &w_stim_fft : &w_spec;
    if (display_reset > 0) {
	gr_rectf(w_dis->xtop + 1, w_dis->ytop + 1, w_dis->xbot - 1,
	    w_dis->ybot - 1, w_dis->wbgc);
    }
    fft_flag = 1;
    i = (int) (rate / 2000);
    if (xrange > i)
	xrange = i;
    a = xrange * dt * 1000;
    w_dis->ybot -= txtpar.font_height;
    (void) display(ibuf, (int) (a + 0.5), display_reset, yrange, xrange);
    w_dis->ybot += txtpar.font_height;
    show_xlabel(0.0, (double) xrange, 0, "kHz");
    fft_flag = 0;
    w_dis = w;

    display_reset = 0;
}

void
gen_sweep(short *sbuf, double scl)
{
    double tpi, a, p, amp, s;
    int i, n;

    if (!alloc_space(1))
	return;
    tpi = 8 * atan(1.0);
    n = buflen;
    a = (-tpi / n) / 4;
    fbuf[0] = fbuf[1] = 1;
    i = 2;
    while(i < n) {
	p = i * a * i;
        fbuf[i++] = (float) cos(p);
        fbuf[i++] = (float) sin(p);
    }
    fsip(fbuf, n);
    a = 0;
    for (i = 0; i < n; i++) {
        if (a < fabs(fbuf[i])) {
	    a = fabs(fbuf[i]);
	}
    }
    amp = MAXPOSINT / a;
    for (i = 0; i < n; i++) {
        s = amp * fbuf[i];
        sbuf[i] = (short) nint(s * scl);
        stimulus[i] = (short) nint(s);
    }
}

void
fft_A_B(float *p, float *q, int sets, int addflg, int msflg)
{
    int     i;

    if (swp1set <= 0 || !sets)
	return;
    if (!alloc_space(1))
	return;
    if (addflg) {
	for (i = 0; i < buflen; i++)
	    fbuf[i] = (float) (p[i] + q[i]) / sets / swp1set;
    } else {
	for (i = 0; i < buflen; i++)
	    fbuf[i] = (float) (p[i] - q[i]) / sets / swp1set;
    }
    for (; i < size; i++)
	fbuf[i] = 0;
    faip(fbuf, size);
    if (msflg)
	modulesqr();
}

int
fft_sig(float *p, int sets)
{
    int     i;

    if (!alloc_space(1))
	return (0);

    for (i = 0; i < buflen; i++)
	fbuf[i] = p[i] / swp1set / (float) sets;
    for (; i < size; i++)
	fbuf[i] = 0;
    faip(fbuf, size);
    i = size / 2;
    return (i);
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
fft_short(short *p)
{
    int     i;
    float   att = 1.0;

    if (!alloc_space(1))
	return (0);
    for (i = 0; i < buflen; i++)
	fbuf[i] = (float) p[i] / att;
    for (; i < size; i++)
	fbuf[i] = 0;
    faip(fbuf, size);
    i = size / 2;
    modulesqr();
    return (i);
}

/* compute and store gain for one channel
   gain = (FFT of ear response / FFT of original stimulus)
*/
void
cal_gain(SAV_FFT *pft, int npts, double scl)
{
    float  *b, s;
    int     i, n;

    pft->rate = rate;
    if (pft->rp == NULL || pft->npts != npts) {
        if (pft->rp != NULL)
            free(pft->rp);
	pft->rp = (float *) malloc(npts * sizeof(float));
	pft->npts = npts;
    }
    memcpy(pft->rp, outbuf, npts * sizeof(float));

    n = fft_short(stimulus);
    if (n <= 0)
	return;
    save_fft(fbuf, n);
    b = pft->rp;
    n = pft->npts;
    s = (float) scl;
    for (i = 0; i < n; i++)
	b[i] /= fbuf[i] * s;
}

void
pickcomplex(float f, float *r, float *c)
{
    int     i;

    f *= (float) size / rate;
    i = (int) (f + 0.5);
    i *= 2;
    *r = fbuf[i++];
    *c = fbuf[i];
}

double
pickat(double f, int nsb)
{
    double  v;
    int     i, c;

    i = (int) (f * size / rate + 0.5);
    v = fbuf[i];
    if (i - nsb < 0)
	nsb = 0;
    for (c = 1; c <= nsb; c++)
	v += fbuf[i - c] + fbuf[i + c];
    v /= (float) (2 * nsb + 1);
    if (v < EMIN)
	v = EMIN;
    v = 10 * log10(v);
    return (v);
}

double
getcomplex(float f, float *c)
{
    double  r;
    int     i;

    f *= (float) size / rate;
    i = (int) (f + 0.5);
    i *= 2;
    c[0] = fbuf[i++];
    c[1] = fbuf[i];
    r = (double) c[0] * c[0] + (double) c[1] * c[1];
    return (r);
}
