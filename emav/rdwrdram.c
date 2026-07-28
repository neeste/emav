/* ============================= rdwrdram.c =============================== */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <dsp.h>
#include <menu.h>
#include "savage.h"
#include "color.h"

double  dbv(double);

extern struct _dpoae dpoae;
extern int swp1set, base_addr, dspprm, maxnpts;

/* download parameters to the DSP */

void
downldpar(int mode, int np, int skip, int ramp, int swps)
{
    dsppar.mode = mode;		/* set channel mode */
    dsppar.npts = np;		/* number of points */
    dsppar.sweeps = swps;
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
    dspdarr(dspprm, (int) sizeof(dsppar) / 2, (short *) &dsppar);
}

/* download dac buffer */

void
dwnld_dac(int chan, int ns, double scale, int type)
{
    int i, nw;
    unsigned short dac;

    nw = ns * buflen;
    dac = chan ? dsppar.dac_b : dsppar.dac_a;
    if (type == 0) {
	for (i = 0; i < nw; i++) {
	    savbuf[i] = (int32_t) outbuf[i] << 16;	    // copy short to long
	}
    } else {
	for (i = 0; i < nw; i++) {
	    outbuf[i] = (short) (savbuf[i] >> 16);  // copy long to short
	}
    }
    if (dsppar.code == 2) {
	dspdarr(dac, nw * 2, (short *) savbuf);
    } else {
        dspdarr(dac, nw, outbuf);
    }
    dspsarr(scale, chan);
}

/* load the stimulus onto the soundcard */

void
ld_stim(int chan, int mode)
{
    int     i, j;
    short   v, ns, *wt;
    extern int reclen, rec[100];
    extern TOGGLETYPE RECor;

    if (mode < 0) {
        ns = 1;
    } else {
        ns = spm[mode].nset;
        wt = spm[mode].stim[chan];
        for (i = 0; i < buflen; i++) {
            v = outbuf[i];
            for (j = 0; j < ns; j++)
                outbuf[i + j * buflen] = v * wt[j];
            if (chan == 0 && mode == 3 && RECor.at) {
                for (i = 0; i < reclen; i++)
                   outbuf[i + 3 * buflen] -= rec[i];
            }
        }
    }

    dwnld_dac(chan, ns, 1, 0);
}

void
set_intern_att(double att)
{
    int     i = 0;

    for (i = 0; i < buflen; i++)
	outbuf[i] = (int) (stmbuf[i] / att + 0.5);
}

void
set_att(double att)
{
    attenu.att1 = (float) limit(0, att, 100);
    attenu.intern_att = (float) pow(10, attenu.att1 / 20);
    attenu.att2 = attenu.att1;
}

int
recalstim(int mode)
{
    int rv = 1;
    int min_att;

    min_att = spm[mode].smax;
    if (attenu.intern_att < min_att) {
	attenu.intern_att = (float) min_att;
	attenu.att1 = attenu.att2 = (float) dbv(attenu.intern_att);
	rv = 0;
    }
    set_intern_att(attenu.intern_att);

    return(rv);
}

/* adjust the sample rate to the closest possible rate */
int32_t
adjust_rate(int32_t r)
{
    return (dspadjrt(r));
}
