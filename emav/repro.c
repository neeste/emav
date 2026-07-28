/* repro.c */

#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <menu.h>
#include <util.h>
#include "savage.h"

#define NFB	5

static double f1[]= {707.1, 353.6, 707.1, 1414.2, 2828.4};
static double f2[]= {5659.9, 707.1, 1414.2, 2828.4, 5656.9};

void fast(float *, int);

/* repro - compute reproducibility in octave frequency bands
 *
 *	inputs:
 *		a[n]  - waveform A
 *		b[n]  - waveform B
 *		n     - number of elements in A and B
 *		rate  - sampling rate (samples / second)
 *	output:
 *		rb[4] - reproducibility (percent) in four octave bands
 */
void
repro(int32_t *a, int32_t *b, int n, float *rb)
{
    float   dt, eab;
    float   ea[NFB], eb[NFB], ec[NFB], *af, *bf;
    int     i, j, k, i1, i2;

    if (n > 0 && rate > 0) {
	af = (float *) malloc((n + 2) * sizeof(float) * 2);
	bf = af + n + 2;
	for (i = 0; i < n; i++) {
	    af[i] = (float) a[i];
	    bf[i] = (float) b[i];
	}
	fast(af, n);
	af[0] = af[1] = af[n] = af[n + 1] = 0;
	fast(bf, n);
	bf[0] = bf[1] = bf[n] = bf[n + 1] = 0;
	dt =(float) n / rate;
	for (j = 0; j < NFB; j++) {
	    ea[j] = eb[j] = ec[j] = 0;
	    i1 = nint(f1[j] * dt) * 2;
	    i2 = nint(f2[j] * dt) * 2;
	    for (i = i1; i < i2; i += 2) {
		k = i + 1;
		ea[j] += af[i] * af[i] + af[k] * af[k];
		eb[j] += bf[i] * bf[i] + bf[k] * bf[k];
		ec[j] += af[i] * bf[i] + af[k] * bf[k];
	    }
	    eab = (float) sqrt(ea[j] * eb[j]);
	    rb[j] = (eab > 0 && ec[j] > 0) ? 100 * ec[j] / eab : 0;
	    if(rb[j] > 99.9) rb[j] = (float) 99.9;
	}
	free(af);
    } else {
	for (j = 0; j < NFB; j++)
	    rb[j] = 0;
    }
}
