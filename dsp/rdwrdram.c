/* rdwrdram.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <menu.h>
#include "dsp.h"
#include "savage.h"

extern int swp1set;
extern int maxnpts, accnpts, base_addr, dspprm;

void
downldpar(int mode, int np, int skip, int ramp)
{
    dsppar.mode = mode;		/* set channel mode */
    dsppar.npts = np;		/* number of points */
    dsppar.sweeps = swp1set;
    if(skip <= 0) {
        dsppar.skip = 0;
        dsppar.scale = 0;
        dsppar.range = 0;
    } else {
	dsppar.skip = skip;
	if(ramp <= 0) {
	    dsppar.scale = 0;
	    dsppar.range = 0;
        } else if(ramp < np) {
	    dsppar.scale = (int)(32768L / ramp);
	    dsppar.range = ramp - 1;
	} else { 
	    dsppar.scale = (int)(32768L / np);
	    dsppar.range = np - 1;
	}
    }

    dsppar.dac_a = (unsigned) base_addr;
    dsppar.dac_b = dsppar.dac_a + maxnpts;
    dsppar.acc = dsppar.dac_b + maxnpts;

    rate = dspsetrt(rate);
    /* down load parameter */
    (void) dspdarr(dspprm, (int) sizeof(dsppar) / 2, (short *) &dsppar);
}

void
ld_stim(int chan, int mode, short *wbuf, int nw)
{
    int i;
    int32_t *lbuf;
    unsigned int dac;

    dac = chan ? dsppar.dac_b : dsppar.dac_a;
    if (dsppar.code == 2) {
	lbuf = (int32_t *) calloc(nw, sizeof(int32_t));
	for (i = 0; i < nw; i++) {
	    lbuf[i] = wbuf[i] << 16;
	}
	(void) dspdarr(dac, nw * 2, (short *) lbuf);
	free(lbuf);
    } else {
	(void) dspdarr(dac, nw, wbuf);
    }
}

int32_t
adjust_rate(int32_t r)
{
    return (dspadjrt(r));
}
