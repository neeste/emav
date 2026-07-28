/* hf.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "putt.h"

#ifndef PI
#define PI	((double) 3.1415926535897932)
#endif

/* bwfilt - Butterworth low-pass/high-pass filter (bilinear transform IIR)
 *
 *      ht    - pointer to float array
 *      npts  - number of points in array
 *      r     - ratio of cut-off frequency to sample frequency
 *      npole - number of complex poles (must be even)
 *      hpf   - high-pass flag (0=low-pass, 1=high-pass)
 */

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

void
bwfilt(float *ht, int npts, double r, int npole, int hpf)
{
    int     i, j;
    float   aa, bb, rr;
    float   a0, a1, a2;
    float   b0, b1, b2;
    float   X0, X1, X2;
    float   Y0, Y1, Y2;

    if (npole > 1 && r > 0) {
	rr = (float) tan(PI * r);
	bb = rr * rr;
	for (j = 0; j < npole / 2; j++) {
	    aa = (float) (2 * rr * sin((j + 0.5) * PI / npole));
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
	    Y0 = Y1 = 0;
	    X0 = X1 = 0;
	    for (i = 0; i < npts; i++) {
		X2 = X1;
		X1 = X0;
		X0 = ht[i];
		Y2 = Y1;
		Y1 = Y0;
		Y0 = a0 * X0 + a1 * X1 + a2 * X2 - b1 * Y1 - b2 * Y2;
		ht[i] = Y0;
	    }
	}
    }
}
