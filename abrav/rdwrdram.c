/* rdwrdram.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <dsp.h>
#include <menu.h>
#include <token.h>
#include <util.h>
#include "savage.h"
#include "screen.h"
#include "color.h"

double  dbv(double);
int     getstim(char *, int);
int     dis_stim(int, void *, enum TYPE);
void    gen_sweep(short *, double);

extern double dsp_scale;
extern int swp1set, ifsp, display_reset, check_signal;
extern unsigned int maxnpts, accnpts, base_addr, dspprm;
extern struct stim stim_;
extern TOGGLETYPE stimType;
extern TOGGLETYPE windowType;
extern TOGGLETYPE maskerType;

void
downldpar(int mode, int np, int skip, int out)
{
    static int32_t rate_set = 0;

    dsppar.mode = mode;		/* set channel mode */
    dsppar.npts = np;		/* number of points */
    dsppar.sweeps = swp1set;
    dsppar.spidx = ifsp;
    dsppar.skip = skip;
    dsppar.dac_a = base_addr + out * np;
    dsppar.dac_b = base_addr + out * np + maxnpts;
    dsppar.accbuf_a = base_addr + maxnpts * 2;
    dsppar.accbuf_b = base_addr + maxnpts * 4;

    /* down load parameters */
    (void) dspdarr(dspprm, (int) sizeof(dsppar) / 2, (short *) &dsppar);

    /* set rate (only if it differs from last time) */
    if (rate != rate_set) {
	rate = rate_set = dspsetrt(rate);
    }
}

void
zero_out()
{
    int     i;

    for (i = 0; i < buflen; i++) {
	outbuf[i] = 0;
    }
}

void
read_stimulus(double scl)
{
    int     i;

    getstim(ilstim, buflen);
    for (i = 0; i < buflen; i++) {
	outbuf[i] = (short) nint(stimulus[i] * scl);
    }
}

void 
create_click(int c, double scl)
{
    double dur;
    int i, pts;
    short amp;

    //calculates the number of points the click will be
    dur = stim_.sig_dur[c];
    amp = (short) nint(MAXPOSINT * scl);
    pts = (int) (0.001 * dur * rate);
    for(i = 0; i < buflen; i++) {
        if(i < pts) 
	outbuf[i] = amp;
	    else
	outbuf[i] = 0;
    }
}

// create a windowed tone in outbuf
void 
create_wind_tone(int c, double scl)
{
    int     i, wt;
    double amp, p, dp, p0, twopi, freq;
    double dpw, pw, w, s;
    double startpt, stoppt;

    if (scl == 0 || stim_.sig_freq[c] <= 0 || stim_.sig_dur[c] <= 0) {
	zero_out();
	return;
    }
    startpt = 0.001 * stim_.sig_start[c] * rate;
    stoppt = 0.001 * (stim_.sig_start[c] + stim_.sig_dur[c]) * rate;
    twopi = 8 * atan(1.0);
    freq = stim_.sig_freq[c];
    amp = MAXPOSINT * scl;
    dp = twopi * freq / rate;
    p0 = stim_.sig_phase[c] * twopi / 360 - dp * (stoppt + startpt) / 2;
    wt = stim_.sig_win[c];
    dpw = twopi / (stoppt - startpt);
    for (i = 0; i < buflen; i++) {
        if((i >= startpt) && (i <= stoppt)) {
	    if(wt) {			    // blackman window
                pw = dpw * (i - startpt);
	        w = 0.42 - 0.5 * cos(pw) + 0.08 * cos(2 * pw);
	    } else {			    // rectangular window
		w = 1;
	    }
	    p = dp * i + p0;
	    s = (float) (w * amp *  sin(p));
	} else {
	    s = 0;
	}
	outbuf[i] = (short) nint(s);
    }
}

// create a ramped tone in outbuf
void 
create_ramp_tone(int c, double scl)
{
    double t0, t1, t2, t3;
    double pi, amp, dp, p0, w;
    int     i;

    if (scl == 0 || stim_.sig_freq[c] <= 0 || stim_.sig_dur[c] <= 0) {
	zero_out();
	return;
    }
    t0 = stim_.sig_start[c];
    t1 = t0 + 0.001 * stim_.sig_ramp[c] * rate;
    t2 = t0 + 0.001 * stim_.sig_dur[c] * rate;
    t3 = t0 + 0.001 * (stim_.sig_dur[c] + stim_.sig_ramp[c]) * rate;
    pi = 4 * atan(1.0);
    dp = 2 * pi * stim_.sig_freq[c] / rate;
    p0 = stim_.sig_phase[c] * pi / 180;
    amp = MAXPOSINT * scl;
    for (i = 0; i < buflen; i++) {
        if(i < t3) {
	    if (i < t1) {
		w = (1 - cos((i - t0) * pi / (t1 - t0))) / 2;
	    } else if (i > t2) {
		w = (1 - cos((t3 - i) * pi / (t3 - t2))) / 2;
	    } else {
		w = 1;
	    }
	    outbuf[i] = (short) nint(w * amp * sin(dp * i + p0));
	} else {
	    outbuf[i] = 0;
	}
    }
}

// create a continuous tone in outbuf
void 
create_cont_tone(int c, double scl)
{
    int     i;
    double pi, amp, dp, p0;

    if (scl == 0 || stim_.sig_freq[c] <= 0) {
	zero_out();
	return;
    }
    pi = 4 * atan(1.0);
    dp = 2 * pi * stim_.sig_freq[c] / rate;
    p0 = stim_.sig_phase[c] * pi / 180;
    amp = MAXPOSINT * scl;
    for (i = 0; i < buflen; i++) {
	outbuf[i] = (short) nint(amp * sin(dp * i + p0));
    }

}

// create a swept tone in outbuf
void 
create_swpt_tone(int c, double scl)
{
    gen_sweep(outbuf, scl);
}

void 
show_stim(int c)
{
    display_reset = 1;
    dfgc = scrn_c[C_WAVE];
    dis_stim(0, outbuf, INT);
    decide_offset(-200, 100);
    decide(0, 1, c ? "stimulus B" : "stimulus A");
    decide_offset(0, 0);
}

void 
recalstim(int c)
{
    double att, scl, psc;
    int typ;

    typ = stim_.sig_type[c];
    att = stim_.sig_atten[c];
    scl = pow(10.0, -att / 20.0);
    dsp_scale = dspsets(scl);
    psc = scl / dsp_scale;	// reduce pre-scaling by amount deferred
    switch(typ) {
    case 0:	    // zero the buffer
	zero_out();
	break;
    case 1:	    // read stimulus from file
        read_stimulus(psc);
	break;
    case 2:	    // generate click
        create_click(c, psc);
        break;
    case 3:	    // generate windowed tone
        create_wind_tone(c, psc);
        break;
    case 4:	    // generate ramped tone
        create_ramp_tone(c, psc);
	break;
    case 5:     // generate continuous-tone
        create_cont_tone(c, psc);
        break;
    case 6:     // generate swept-tone
        create_swpt_tone(c, psc);
        break;
    }
    if (check_signal)
	show_stim(c);
}

/* adjust the sample rate to the closest possible rate */
int32_t
adjust_rate(int32_t r)
{
    return (dspadjrt(r));
}
