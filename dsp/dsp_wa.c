// dsp_wa.c - DSP function calls using ARSC

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "bio.h"
#include "dsp.h"
#include "anafilt.h"

#define DEVICE	    "[Win32 wave audio]"

void derror(char *, char *);

BIO bio_;

/* DSP memory model for 16K words */
static int base_addr = 392 * 128;
static int dspmnmx =   510 * 128 + 64;
static int dspprm =    510 * 128;
static int dspsiz =    128 * 128;
static int dspspts =   506 * 128;
static int maxspts =   128;

static double mxsv[2];
static float sf0 = 0;	    /* scale factor with no attenuation */
static float sf_out[2];	    /* output scale factor */
static float sf_in = 0;	    /* output factor */
static int devset = 0;	    /* indicate active i/o */
static int dsp_type = 0;    /* select DSP code: 0=oae, 1=abr */
static int hpf_type = 1;    /* select filter type: 0=butter, 1=bessel */
static int nbpc = 2;	    /* number of banks per channel */
static int ncio = 2;	    /* number of channels of I/O */
static int nspb = 0;	    /* number of sweeps per bank */
static int32_t  *acc_a_lbf;
static int32_t  *acc_b_lbf;
static int32_t  *dac_a_lbf;
static int32_t  *dac_b_lbf;
static short *dac_a_sbf;
static short *dac_b_sbf;
static short *mnmx_sbf;
static short *spts_sbf;
static short *dspmem = NULL;
static short  greg = 1;
static short  mode;
static short  code;
static short  nic = 0;
static short  npts;
static short  range;
static short  scale;
static short  skip;
static short  spctr;
static short  spidx;
static short  sweeps;
static short  swp_acc;
static short  swpcnt;
static short  swp_in;
static short  swp_out;
static unsigned int acc_a;
static unsigned int acc_b;
static unsigned int dac_a;
static unsigned int dac_b;

static float a1[4], b1[4], z1[4];
static float a2[4], b2[4], z2[4];
static float fco[2] = {0,0};
static int    nfo1 = 0;
static int    nfo2 = 0;
static int32_t   rate = 0;

int ad_type = 0;        /* Will A/D clock fail to synchronize? */
int dsp_select = 0;     /* Which DSP is installed ? */
int dsp_err_msg = 0;    /* Allow DSP error messages ? */

/*************************************************************************/

static void
d0error(char *m, int e)
{
    char s[80], t[80];

    sprintf(s, "WaveAudio error: %s.", m);
    sprintf(t, "error code = %d", e);
    derror(s, t);
}

/*************************************************************************/

static int
sio_alloc()
{
    int i;

    for (i = 0; i < ncio; i++) {
	bio_.stim_i[i] = 1;
	bio_.stim_b[i] = (int32_t *) calloc((size_t) npts, sizeof(int32_t));
	if (!bio_.stim_b[i])
	    return (i + 1);
    }
    bio_.bank_b = (int32_t *) calloc((size_t) npts, sizeof(int32_t) * ncio);
    if (!bio_.bank_b)
	return (i + 1);

    return (0);	// no errors
}

static void
sio_free()
{
    int i;

    for (i = 0; i < ncio; i++) {
	bio_.stim_i[i] = 0;
        if(bio_.stim_b[i]) {
	    free(bio_.stim_b[i]);
	    bio_.stim_b[i] = NULL;
	}
    }
    if (bio_.bank_b) {
        free(bio_.bank_b);
	bio_.bank_b = NULL;
    }
}

/*************************************************************************/

static void
oae_put_sswp(int bo)
{
    int i, i1, j;
    double sf1, sf2, scl;
    static int ss = 0;

    for (j = 0; j < nspb; j++) {
        sf1 = sf_out[0] * sf0;
        sf2 = sf_out[1] * sf0;
	if (swp_out == 0) {
	    if (range > 0) {
		for (i = 0; i < range; i++) {	    /* ramp up */
		    scl = (double) i / range;
		    bio_.stim_b[0][i] = (int32_t) (dac_a_sbf[i] * sf1 * scl);
		    bio_.stim_b[1][i] = (int32_t) (dac_b_sbf[i] * sf2 * scl);
		}
	    }
	    i1 = range;
	    ss = 1;
	} else if (swpcnt == -1) {
	    if (range > 0) {
		for (i = 0; i < range; i++) {	    /* ramp down */
		    scl = (double) (range - i - 1) / range;
		    bio_.stim_b[0][i] = (int32_t) (dac_a_sbf[i] * sf1 * scl);
		    bio_.stim_b[1][i] = (int32_t) (dac_b_sbf[i] * sf2 * scl);
		}
	    }
	    i1 = range;
	    ss = 0;
	} else {
	    i1 = 0;
	}
	if (!ss) {
	    sf1 = 0;
	    sf2 = 0;
	}
        for (i = i1; i < npts; i++) {		    /* full amplitude */
	    bio_.stim_b[0][i] = (int32_t) (dac_a_sbf[i] * sf1);
	    bio_.stim_b[1][i] = (int32_t) (dac_b_sbf[i] * sf2);
	}
        wa_bank_put(bo, j * npts, npts, 1, 0);
	swp_out++;
	swpcnt--;
    }
    wa_bank_ready(bo);
}

static void
oae_put_lswp(int bo)
{
    int i, i1, j;
    double sf1, sf2, scl;
    static int ss = 0;

    for (j = 0; j < nspb; j++) {
        sf1 = sf_out[0] * sf0;
        sf2 = sf_out[1] * sf0;
	if (swp_out == 0) {
	    if (range > 0) {
		for (i = 0; i < range; i++) {	    /* ramp up */
		    scl = (double) i / range;
		    bio_.stim_b[0][i] = (int32_t) (dac_a_lbf[i] * sf1 * scl);
		    bio_.stim_b[1][i] = (int32_t) (dac_b_lbf[i] * sf2 * scl);
		}
	    }
	    i1 = range;
	    ss = 1;
	} else if (swpcnt == -1) {
	    if (range > 0) {
		for (i = 0; i < range; i++) {	    /* ramp down */
		    scl = (double) (range - i - 1) / range;
		    bio_.stim_b[0][i] = (int32_t) (dac_a_lbf[i] * sf1 * scl);
		    bio_.stim_b[1][i] = (int32_t) (dac_b_lbf[i] * sf2 * scl);
		}
	    }
	    i1 = range;
	    ss = 0;
	} else {
	    i1 = 0;
	}
	if (!ss) {
	    sf1 = 0;
	    sf2 = 0;
	}
        for (i = i1; i < npts; i++) {		    /* full amplitude */
	    bio_.stim_b[0][i] = (int32_t) (dac_a_lbf[i] * sf1);
	    bio_.stim_b[1][i] = (int32_t) (dac_b_lbf[i] * sf2);
	}
        wa_bank_put(bo, j * npts, npts, 1, 0);
	swp_out++;
	swpcnt--;
    }
    wa_bank_ready(bo);
}

/*************************************************************************/

// hpf_coef - second-order, high-pass filter coefficients
static int
hpf_coef(float *wn, float *b, float *a)
{
    double  rr, aa, bb;
    float cc;
    int i, n = 0;
    static double pi = 3.1415927;

    if (wn[0] > 0) {
	n = 2;	    // filter order
	rr = tan(pi * wn[0]);
	bb = rr * rr;
        aa = 2 * rr * sin(pi / 4);
	a[0] = (float) (1 + aa + bb);
	a[1] = (float) (2 * (bb - 1));
	a[2] = (float) (1 - aa + bb);
	b[0] = 1;
	b[1] = -2;
	b[2] = 1;
	cc = a[0];
	for (i = 0; i <= n; i++) {
	    a[i] /= cc;
	    b[i] /= cc;
	}
    }

    return (n);
}

static void
zeroz(float *z, int n)
{
    int   i;

    for (i = 0; i <= n; i++) {
	z[i] = 0;
    }
}

// hpf_init - initialize high-pass filters on both input channels
static void
hpf_init(float *fco)
{
    int   no = 3, ft = 1;	// third-order, high-pass
    float w1, w2;

    w1 = fco[0] / (float) rate;
    w2 = fco[1] / (float) rate;
    if (hpf_type == 1) {
	nfo1 = bessel(&w1, b1, a1, no, ft);
	nfo2 = bessel(&w2, b2, a2, no, ft);
    } else {
	nfo1 = hpf_coef(&w1, b1, a1);
	nfo2 = hpf_coef(&w2, b2, a2);
    }
    zeroz(z1, nfo1);
    zeroz(z2, nfo1);
}

// in_filt1 - implement real-time, IIR filter on input channel 1
static float _inline
in_filt1(float yy)
{
    float xx;
    int j, k;

    if (nfo1) {
	xx = yy;
	yy = b1[0] * xx + z1[0];
	for (j = 0; j < nfo1; j++) {
	    k = j + 1;
	    z1[j] = z1[k] + b1[k] * xx - a1[k] * yy;
	}
    }
    return (yy * sf_in);
}

// in_filt2 - implement real-time, IIR filter on input channel 2
static float _inline
in_filt2(float yy)
{
    float xx;
    int j, k;

    if (nfo2) {
	xx = yy;
	yy = b2[0] * xx + z2[0];
	for (j = 0; j < nfo2; j++) {
	    k = j + 1;
	    z2[j] = z2[k] + b2[k] * xx - a2[k] * yy;
	}
    }
    return (yy * sf_in);
}

/*************************************************************************/

static void
oae_get_lswp(int bo)
{
    float a, b;
    int i, j, k;

    if (!acc_b_lbf) {		    // single channel input
        for (j = 0; j < nspb; j++) {
	    if (swp_acc < sweeps) {
		wa_bank_get(bo, j * npts, npts);
		for (i = 0, k = 0; i < npts; i++, k += ncio) {
		    a = in_filt1((float)bio_.bank_b[k]);
		    if (swp_in >= skip) {
			acc_a_lbf[i] += (int32_t) a;
		    }
		}
		if (swp_in >= skip) {
    		    swp_acc++;
		}
	    }
	    swp_in++;
	}
    } else {			    // two-channel input
        for (j = 0; j < nspb; j++) {
	    if (swp_acc < sweeps) {
		wa_bank_get(bo, j * npts, npts);
		for (i = 0, k = 0; i < npts; i++, k += ncio) {
		    a = in_filt1((float)bio_.bank_b[k + 0]);
		    b = in_filt2((float)bio_.bank_b[k + 1]);
		    if (swp_in >= skip) {
			acc_a_lbf[i] += (int32_t) a;
			acc_b_lbf[i] += (int32_t) b;
		    }
		}
		if (swp_in >= skip) {
    		    swp_acc++;
		}
	    }
	    swp_in++;
	}
    }
}

/* oae_init - initialize local variable from parameter area */

static void
oae_init()
{
    unsigned short  *p;

    // compute HPF coefficients
    hpf_init(fco);

    // fetch DSP parameters
    p = dspmem + dspprm - base_addr;
    mode = p[0];
    npts = p[1];
    sweeps = p[2];
    skip = p[3];
    dac_a = p[4];
    dac_b = p[5];
    acc_a = p[6];
    greg = p[7];
    scale = p[8];
    range = p[9];
    nic = p[10];
    code = p[11];
}

/* oae_start - emulates DSP code for OAE recording */

static int
oae_start()
{
    int	    i, err, bs;

    oae_init();
    swpcnt = sweeps + skip;    /* compute sweep count */
    if (npts <= 0 || swpcnt <= 0)
	return (1);

    if (dsp_type == 0) {
        dac_a_sbf = dspmem + dac_a - base_addr;
	dac_b_sbf = dspmem + dac_b - base_addr;
    } else {
        dac_a_lbf = (int32_t *) (dspmem + dac_a - base_addr);
	dac_b_lbf = (int32_t *) (dspmem + dac_b - base_addr);
    }
    acc_a_lbf = (int32_t *) (dspmem + acc_a - base_addr);
    if (nic < 2) {			// single-channel input
        acc_b_lbf = 0;
	for (i = 0; i < npts; i++) {
	    acc_a_lbf[i] = 0;
	}
    } else {				// two-channel input
        acc_b_lbf = acc_a_lbf + npts;
	for (i = 0; i < npts; i++) {
	    acc_a_lbf[i] = 0;
	    acc_b_lbf[i] = 0;
	}
    }

    err = sio_alloc();
    if (err) {
        d0error("can't allocate memory", err);
	sio_free();
        return (2);
    }
    err = wa_open(NULL, NULL, mxsv);
    if (err) {
        d0error("can't open I/O device", err);
	sio_free();
        return (3);
    }
    bs = wa_reset_io(npts * swpcnt);
    if (bs == 0) {
        d0error("failed to reset_io", err);
	sio_free();
        return (4);
    }
    nspb = bs / npts;
    swp_acc = swp_out = swp_in = 0;
    if (dsp_type == 0) {
        sf0 = (float) (mxsv[0] / (pow(2,16)-1));
        for (i = 0; i < nbpc; i++) {
	    oae_put_sswp(i);
	}
    } else {
        sf0 = (float) (mxsv[0] / (pow(2,32)-1));
        for (i = 0; i < nbpc; i++) {
	    oae_put_lswp(i);
        }
    }
    wa_begin_io();

    return (0);
}

/*************************************************************************/

static void
abr_put_sswp(int bo)
{
    int i, j;
    double sf1, sf2;

    for (j = 0; j < nspb; j++) {
	sf1 = (swpcnt > 0) ? sf_out[0] * sf0 : 0;
	sf2 = (swpcnt > 0) ? sf_out[1] * sf0 : 0;
	if (mode == 2) {
            for (i = 0; i < npts; i++) {
    		bio_.stim_b[0][i] = (int32_t) (dac_a_sbf[i] * sf1);
		bio_.stim_b[1][i] = (int32_t) (dac_b_sbf[i] * sf2);
	    }
	} else {
	    for (i = 0; i < npts; i++) {
		bio_.stim_b[0][i] = (int32_t) (dac_a_sbf[i] * sf1);
		bio_.stim_b[1][i] = 0;
	    }
	}
        wa_bank_put(bo, j * npts, npts, 1, 0);
	swp_out++;
	swpcnt--;
    }
    wa_bank_ready(bo);
}

static void
abr_get_lswp(int bo)
{
    double v1, v2;
    int i, j, k;
    short s1, s2;

    for (j = 0; j < nspb; j++) {
	if (swp_in >= skip && swp_acc < sweeps) {
            wa_bank_get(bo, j * npts, npts);
	    for (i = 0, k = 0; i < npts; i++, k += ncio) {
 	        v1 = bio_.bank_b[k + 0] * sf_in;
 	        v2 = bio_.bank_b[k + 1] * sf_in;
	        acc_a_lbf[i] += (int32_t) v1;
	        acc_b_lbf[i] += (int32_t) v2;
		s1 = (short) v1;
		s2 = (short) v2;
		if (spctr == 0 && i == 0) {
		    mnmx_sbf[0] = mnmx_sbf[1] = (short) v1;
		    mnmx_sbf[0] = mnmx_sbf[1] = (short) v2;
		} else {
		    if (mnmx_sbf[0] < s1)
			mnmx_sbf[0] = s1;
		    if (mnmx_sbf[1] > s1)
			mnmx_sbf[1] = s1;
		    if (mnmx_sbf[2] < s2)
			mnmx_sbf[2] = s2;
		    if (mnmx_sbf[3] > s2)
			mnmx_sbf[3] = s2;
		}
		if (spctr < maxspts && i == spidx) {
		    spts_sbf[spctr] = s1;
		    spts_sbf[spctr + maxspts] = s2;
		    spctr++;
		}
	    }
    	    swp_acc++;
	}
	swp_in++;
    }
}

/* abr_init - initialize local variable from parameter area */

static void
abr_init()
{
    unsigned short  *p;

    /* fetch DSP parameters */
    p = dspmem + dspprm - base_addr;
    mode = p[0];
    npts = p[1];
    sweeps = p[2];
    dac_a = p[3];
    acc_a = p[4];
    acc_b = p[5];
    greg = p[6];
    spidx = p[7];
    skip = p[8];
    dac_b = p[9];
}

/* abr_start - emulates DSP code for ABR recording */

static int
abr_start()
{
    int	    i, err, bs;

    abr_init();
    swpcnt = sweeps + skip;    /* compute sweep count */
    if (npts <= 0 || swpcnt <= 0)
	return (1);

    dac_a_sbf = dspmem + dac_a - base_addr;
    dac_b_sbf = dspmem + dac_b - base_addr;
    acc_a_lbf = (int32_t *) (dspmem + acc_a - base_addr);
    acc_b_lbf = (int32_t *) (dspmem + acc_b - base_addr);
    mnmx_sbf = dspmem + dspmnmx - base_addr;
    spts_sbf = dspmem + dspspts - base_addr;
    for (i = 0; i < npts; i++) {
        acc_a_lbf[i] = 0;
        acc_b_lbf[i] = 0;
     }
    spctr = 0;

    sio_alloc();
    err = wa_open(NULL, NULL, mxsv);
    if (err) {
        d0error("can't open I/O device", err);
	sio_free();
        return (2);
    }
    sf0 = (float) mxsv[0] / 32767;
    bs = wa_reset_io(npts * swpcnt);
    nspb = bs / npts;
    swp_acc = swp_out = swp_in = 0;
    for (i = 0; i < nbpc; i++) {
        abr_put_sswp(i);
    }
    wa_begin_io();

    return (0);
}

/*************************************************************************/

static void
dsp_init()
{
    if (dsp_type == 1) 
	abr_init();
    else 
	oae_init();
}

static int
dsp_start()
{
    if (dsp_type == 1) 
	return (abr_start());
    else 
	return (oae_start());
}

static int 
dsp_check()
{
    int b;

    if (b = wa_check_bank()) {
	if (dsp_type == 0) {
	    oae_get_lswp(b - 1);
            oae_put_sswp(b - 1);
	} else if (dsp_type == 1) {
	    abr_get_lswp(b - 1);
            abr_put_sswp(b - 1);
	} else {
	    oae_get_lswp(b - 1);
            oae_put_lswp(b - 1);
	}
    }
    
    return (b);
}

static void
dsp_halt()
{
    wa_stop_io();
    wa_close();
    sio_free();
}

/*************************************************************************/

/* F1: (DINIT) Initialize Soundcard */
int
dspinit(char *hex_file)
{
//    static char *default_device = "CardDeluxe Analog";
    static char *default_device = "Indigo";

    if (wa_test()) {
        if (dspmem == NULL)
	    dspmem = (short *) calloc(dspsiz, sizeof(short));
	if (!*hex_file)
	    hex_file = default_device;
        wa_devsel_byname(hex_file);
	sf_in = sf_out[0] = sf_out[1] = 1;
	return (0);
    }
    return (1);
}

/* F2: (DSETA) Set Address. */

int
dspseta(int num)
{
    dsp_type = num;	    /* select DSP code: 0=oae, 1=abr */
    return (wa_test() == 0);
}

/* F4: (DSSR) Set Sample Rate. */

int
dspssr(int inp_rate, int out_rate, int sh_delay, int io_sync)
{
    return(0);
}
    
/* F11: (DDARR) Download array. */

int
dspdarr(unsigned int ofst, int nwords, short *parr)
{
    memcpy(dspmem + ofst - base_addr, parr, nwords * 2);
    // initialize parameters & scale factors
    if ((int) ofst == dspprm) {
	dsp_init();
    } else if (ofst == dac_a) {
	sf_out[0] = 1;
    } else if (ofst == dac_b) {
	sf_out[1] = 1;
    }
    return (0);
}

/* F12: (DUARR) Upload array. */

int
dspuarr(unsigned int ofst, int nwords, short *parr)
{
    memcpy(parr, dspmem + ofst - base_addr, nwords * 2);
    return (0);
}

/* F16: (DRST) I/O Control: 0 = Start, 1 = Stop */

int
dsprst(int reset)
{
    if (reset && devset) {		/* Stop I/O */
	dsp_halt();
	devset = 0;
    } else if (!reset && !devset) {	/* Start I/O */
	if (dsp_start() == 0) {
	    devset = 1;
	}
    }
    return (0);
}

/* F30: (DGET) Get Read Status. */

int
dspdget(void)
{
    if (devset)
	return (dsp_check());
    return (0);
}

/* F31: (DREAD) Read Immediate. */

int
dspdread(short *data)
{
    *data = sweeps - swp_acc;
    return (0);
}

/* dspdev - get device name */

char *
dspdev(void)
{
    return (wa_devnam());
}

/* dspsets - set output scale factor */

double
dspsets(double sf)
{
    return (limit(1e-9, sf, 1));
}

/* dspsatt - set input attenuator */

double
dspsatt(double at)
{
    at = limit(0, at, 120);
    sf_in = (float) pow(10.0, -at / 20);
    return (at);
}

/* dspsarr - scale DSP array */

void
dspsarr(double sf, int chan)
{
    sf_out[chan] = (float) limit(1e-9, sf, 1);
}

/* dspadjrt - adjust given sample rate to nearest possible value */

int32_t
dspadjrt(int32_t r)
{
    r = rate = (int32_t) wa_adjust_rate((double) r);
    return (r);
}

/* dspsetrt - set DSP sample rate */

int32_t
dspsetrt(int32_t r)
{
    rate = (int32_t) wa_set_rate((double) r);
    return (rate);
}

/* dspgreg - set SMA size */

int
dspgreg(int g)
{
    switch (g) {
    case 1 :	    /* DSP memory model for 16K words */
        base_addr = 392 * 128;
	dspspts =   506 * 128;
	dspprm =    510 * 128;
	dspmnmx =   510 * 128 + 64;
	dspsiz =    120 * 128;
	maxspts =   128;
	greg = 1;
	break;
    case 2 :	    /* DSP memory model for 65K words */
        base_addr = 0;
	dspprm =    512 * 128;
	dspmnmx =   dspprm + 64;
	dspspts =   dspprm + 512;
	dspsiz =    dspprm + 1024;
	maxspts =   128;
	greg = 2;
	break;
    case 3 :	    /* DSP memory model for 97K words */
        base_addr = 0;
	dspprm =    768 * 128;
	dspmnmx =   dspprm + 64;
	dspspts =   dspprm + 512;
	dspsiz =    dspprm + 1024;
	maxspts =   128;
	greg = 3;
	break;
    case 4 :	    /* DSP memory model for 129K words */
        base_addr = 0;
	dspprm =    1023 * 128;
	dspmnmx =   dspprm + 64;
	dspspts =   dspprm + 512;
	dspsiz =    dspprm + 1024;
	maxspts =   128;
	greg = 4;
	break;
    }

    return (greg);
}

/* dsphpf - set DSP high-pass filter: type, order, & frequency */

void
dsphpf(int t, int o, float *f)
{
    if (f) {
	fco[0] = f[0];
	fco[1] = f[1];
    } else {
	fco[0] = 0;
	fco[1] = 0;
    }
}

/*************************************************************************/

// dsptest - test which DSP card is present

void
dsptest(char *dsp_file, char *file0, char *file1, char *file2)
{
    dsp_err_msg = 0;
    dsp_select = 0;
    strcpy(dsp_file, file0);
    dspseta(0);
    if (dspinit(dsp_file) == 0)
	return;
    dsp_select = -1;
    dsp_err_msg = 1;
}

/*************************************************************************/

/* dspapi - return soundcard API */

char *
dspapi(void)
{
    return ("Wave Audio API");
}

/*************************************************************************/
