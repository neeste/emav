/* hf.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#ifndef PI
#define PI	((double) 3.1415926535897932)
#endif

void
long2float(int32_t *lht, int n)
{
    int     i;
    float  *fht;

    fht = (float *) lht;
    for (i = 0; i < n; i++)
	fht[i] = (float) lht[i];
}

void
float2long(int32_t *lht, int n)
{
    int     i;
    float  *fht;

    fht = (float *) lht;
    for (i = 0; i < n; i++)
	lht[i] = (int32_t) fht[i];
}

/* bwfilt - Butterworth low-pass/high-pass filter (bilinear transform IIR)
 *
 *      ht    - pointer to float array
 *      npts  - number of points in array
 *      r     - ratio of cut-off frequency to sample frequency
 *      npole - number of complex poles (must be even)
 *      hpf   - high-pass flag (0=low-pass, 1=high-pass)
 */

void
bwfilt(float *ht, int npts, double r, int npole, int hpf)
{
    int     i, j;
    double  aa, bb, rr;
    double  a0, a1, a2;
    double  b0, b1, b2;
    double  X0, X1, X2;
    double  Y0, Y1, Y2;

    if (npole > 1 && r > 0) {
	rr = tan(PI * r);
	bb = rr * rr;
	for (j = 0; j < npole / 2; j++) {
	    aa = 2 * rr * sin((j + 0.5) * PI / npole);
	    b0 = 1 + aa + bb;
	    b1 = 2 * (bb - 1) / b0;
	    b2 = (1 - aa + bb) / b0;
	    if (hpf) {
		a0 = 1 / b0;
		a1 = -2 / b0;
		a2 = 1 / b0;
	    } else {
		a0 = bb / b0;
		a1 = 2 * bb / b0;
		a2 = bb / b0;
	    }
            X0 = Y0 = 0;
            X1 = Y1 = 0;
	    for (i = 0; i < npts; i++) {
		X2 = X1;
		X1 = X0;
		X0 = ht[i];
		Y2 = Y1;
		Y1 = Y0;
		Y0 = a0 * X0 + a1 * X1 + a2 * X2 - b1 * Y1 - b2 * Y2;
	    }
	    for (i = 0; i < npts; i++) {
		X2 = X1;
		X1 = X0;
		X0 = ht[i];
		Y2 = Y1;
		Y1 = Y0;
		Y0 = a0 * X0 + a1 * X1 + a2 * X2 - b1 * Y1 - b2 * Y2;
		ht[i] = (float) Y0;
	    }
	}
    }
}

#ifdef NEVER

/*
 * bw - computes Blackman window function 
 */
void
bw(float *wc, int m)
{
    double  dp, ph, w, sum;
    int     i, k;
    static double a[3] = {0.42, 0.5, 0.08};
    static double pi = 3.14159265;
    static short nc = 3;

    dp = pi / m;
    sum = 0;
    for (i = 0; i < m; i++) {
	ph = i * dp;
	w = a[0];
	for (k = 1; k < nc; k++)
	    w = w + a[k] * cos(ph * k);
        sum += (i == 0) ? w : 2 * w;
	wc[i] = (float) w;
    }
    for (i = 0; i < m; i++)
	wc[i] /= (float) sum;
}

#endif // NEVER

