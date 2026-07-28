// dsp_arsc.c - DSP function calls using ARSC

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <arsclib.h>
#include "bio.h"
#include "dsp.h"

#define ESC		27
#define MAXPOSINT   	32767
#define NBNK		2
#define NCHN		2
#define free_null(x)	{if(x){free(x);x=NULL;}}

/* DSP memory model for 16K words */
static int base_addr = 392 * 128;
static int dspmnmx =   510 * 128 + 64;
static int dspprm =    510 * 128;
static int dspsiz =    128 * 128;
static int dspspts =   506 * 128;
static int maxspts =   128;

static float sf0 = 0;	    /* scale factor with no attenuation */
static float sf_out[2];	    /* output scale factor */
static float sf_in = 0;	    /* output factor */
static int devset = 0;	    /* indicate active i/o */
static int dsp_type = 0;    /* select DSP code: 0=oae, 1=abr */
static int nbpc = NBNK;	    /* number of banks per channel */
static int ncio = NCHN;	    /* number of channels of I/O */
static int choi = 0;        /* input channel offset */
static int choo = 0;        /* output channel offset */
static int setoff = 0;      /* termination flag */
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
static unsigned int acc_a;
static unsigned int acc_b;
static unsigned int dac_a;
static unsigned int dac_b;

static float *a1 = NULL, *b1 = NULL, *z1 = NULL;
static float *a2 = NULL, *b2 = NULL, *z2 = NULL;
static int    nfo1 = 0;
static int    nfo2 = 0;

static int    datflg = 0;
static int    sioflg = 0;
static int    curseg = 0;

static double rate_set = 0;
//static float sf_in0 =  1.0 / 512;	    // ???
static float sf_in0 =  1.0 / 256;	    // Fix 6-dB error???
static int32_t iodev = -1;
static void *vbanki[NBNK * NCHN] = {NULL};
static void *vbanko[NBNK * NCHN] = {NULL};

/*************************************************************************/

static void
d0error(char *m, int e)
{
    char s[80], t[80];

    sprintf(s, "DSP error: %s.", m);
    sprintf(t, "error code = %d", e);
}

/*************************************************************************/

static int
sio_alloc()
{
    int i;
    static int nbps = sizeof(int32_t);

    for (i = 0; i < ncio * nbpc; i++) {
        vbanki[i] = (void *) calloc(npts, nbps); /* input bank */
        vbanko[i] = (void *) calloc(npts, nbps); /* output bank */
	if (!vbanko[i])
	    return (i + 1);
    }

    return (0);	// no errors
}

static void
sio_free()
{
    int i;

    for (i = 0; i < ncio * nbpc; i++) {
	free_null(vbanki[i]);
	free_null(vbanko[i]);
    }
}


/*************************************************************************/

// in_filt1 - implement real-time, IIR filter on input channel 1
static float __inline
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
static float __inline
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
oae_put_sswp(int32_t bo)
{
    double sf1, sf2, scl, rr;
    int i, i1, k, zf;
    int32_t *dac0, *dac1;

    k = bo % nbpc;
    dac0 = (int32_t *) vbanko[k * 2 + 0];
    dac1 = (int32_t *) vbanko[k * 2 + 1];
    sf1 = sf_out[0] * sf0;
    sf2 = sf_out[1] * sf0;
    if (setoff) {
        swpcnt = (short) (bo + 1);
        setoff = 0;
    }
    if (range && bo == 0) {
	i1 = range + 1;
	zf = 0;
	rr = 1.0 / i1;
	for (i = 0; i < i1; i++) {	    /* ramp up */
	    scl = i * rr;
	    dac0[i] = (int32_t) (dac_a_sbf[i] * sf1 * scl);
	    dac1[i] = (int32_t) (dac_b_sbf[i] * sf2 * scl);
	}
    } else if (range && bo == (swpcnt - 1)) {
	i1 = range + 1;
	zf = 1;
	rr = 1.0 / i1;
	for (i = 0; i < i1; i++) {	    /* ramp down */
	    scl = (range - i) * rr;
	    dac0[i] = (int32_t) (dac_a_sbf[i] * sf1 * scl);
	    dac1[i] = (int32_t) (dac_b_sbf[i] * sf2 * scl);
	}
    } else if (bo >= swpcnt) {
	i1 = 0;
	zf = 1;
    } else {
	i1 = 0;
	zf = 0;
    }
    if (zf) {
	for (i = i1; i < npts; i++) {		    /* zero amplitude */
	    dac0[i] = 0;
	    dac1[i] = 0;
	}
    } else {
        for (i = i1; i < npts; i++) {		    /* full amplitude */
	    dac0[i] = (int32_t) (dac_a_sbf[i] * sf1);
	    dac1[i] = (int32_t) (dac_b_sbf[i] * sf2);
	}
    }
}

static void
oae_put_lswp(int32_t bo)
{
    double sf1, sf2, scl, rr;
    int i, i1, k, zf;
    int32_t *dac0, *dac1;

    k = bo % nbpc;
    dac0 = (int32_t *) vbanko[k * 2 + 0];
    dac1 = (int32_t *) vbanko[k * 2 + 1];
    sf1 = sf_out[0] * sf0;
    sf2 = sf_out[1] * sf0;
    if (setoff) {
        swpcnt = (short) (bo + 1);
        setoff = 0;
    }
    if (range && bo == 0) {
	i1 = range + 1;
	zf = 0;
	rr = 1.0 / i1;
	for (i = 0; i < i1; i++) {	    /* ramp up */
	    scl = i * rr;
	    dac0[i] = (int32_t) (dac_a_lbf[i] * sf1 * scl);
	    dac1[i] = (int32_t) (dac_b_lbf[i] * sf2 * scl);
	}
    } else if (range && bo == (swpcnt - 1)) {
	i1 = range + 1;
	zf = 1;
	rr = 1.0 / i1;
	for (i = 0; i < i1; i++) {	    /* ramp down */
	    scl = (range - i) * rr;
	    dac0[i] = (int32_t) (dac_a_lbf[i] * sf1 * scl);
	    dac1[i] = (int32_t) (dac_b_lbf[i] * sf2 * scl);
	}
    } else if (bo >= swpcnt) {
	i1 = 0;
	zf = 1;
    } else {
	i1 = 0;
	zf = 0;
    }
    if (zf) {
	for (i = i1; i < npts; i++) {		    /* zero amplitude */
	    dac0[i] = 0;
	    dac1[i] = 0;
	}
    } else {
        for (i = i1; i < npts; i++) {		    /* full amplitude */
	    dac0[i] = (int32_t) (dac_a_lbf[i] * sf1);
	    dac1[i] = (int32_t) (dac_b_lbf[i] * sf2);
	}
    }
}

/*************************************************************************/

static void
oae_get_lswp(int32_t bo)
{
    float a, b;
    int32_t *adc0, *adc1;
    int i, k;

    k = bo % nbpc;
    adc0 = (int32_t *) vbanki[k * 2 + 0];
    adc1 = (int32_t *) vbanki[k * 2 + 1];
    if (!acc_b_lbf) {		    // single channel input
	if (swp_acc < sweeps) {
	    for (i = 0; i < npts; i++) {
		a = in_filt1((float)adc0[i]);
		if (bo >= skip) {
		    acc_a_lbf[i] += (int32_t) a;
		}
	    }
	    if (bo >= skip) {
    		swp_acc++;
		datflg++;
	    }
	}
    } else {			    // two-channel input
	if (swp_acc < sweeps) {
	    for (i = 0; i < npts; i++) {
		a = in_filt1((float)adc0[i]);
		b = in_filt2((float)adc1[i]);
		if (bo >= skip) {
		    acc_a_lbf[i] += (int32_t) a;
		    acc_b_lbf[i] += (int32_t) b;
		}
	    }
	    if (bo >= skip) {
    		swp_acc++;
		datflg++;
	    }
	}
    }
}

/* oae_init - initialize local variable from parameter area */

static void
oae_init()
{
    unsigned short  *p;

    // fetch DSP parameters
    p = (unsigned short *) (dspmem + dspprm - base_addr);
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
    int	    i, err = 0;
    static int32_t size[4];
    static int32_t fmt[2] = {ARSC_DATA_I4, 0};

    oae_init();
    if (npts <= 0 || sweeps <= 0)
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
    range = limit(0, range, npts - 1);
    if (range) {			// add sweep for ramp up
	skip++;
    }
    setoff = 0;
    swpcnt = sweeps + skip;
    if (range) {			// add sweep for ramp down
	swpcnt++;
    }
    for (i = 0; i < 4; i++) {
        size[i] = npts;
    }
    err = ar_set_fmt(iodev, fmt);
    err = ar_io_open_off(iodev, rate_set, ncio, ncio, choi, choo);
    if (err) {
        d0error("can't open I/O device", err);
	sio_free();
        return (3);
    }
    rate_set = ar_get_rate(iodev);
    if (dsp_type == 0) {
        sf0 = (float) ((pow(2,32)-1) / (pow(2,16)-1));
        err = ar_set_xfer(iodev, oae_get_lswp, oae_put_sswp);
    } else {
        sf0 = 1;
        err = ar_set_xfer(iodev, oae_get_lswp, oae_put_lswp);
    }
    if (err) {
        d0error("error in set_xfer", err);
	sio_free();
        return (4);
    }
    err = ar_io_prep(iodev, vbanki, vbanko, size, nbpc, 0);
    datflg = 0;
    swp_acc = 0;
    ar_io_start(iodev);
    sioflg = 1;

    return (0);
}

/*************************************************************************/

static void
abr_put_sswp(int32_t bo)
{
    double sf1, sf2;
    int i, k;
    int32_t *dac0, *dac1;

    k = bo % nbpc;
    dac0 = (int32_t *) vbanko[k * 2 + 0];
    dac1 = (int32_t *) vbanko[k * 2 + 1];
    if (bo < swpcnt) {
	sf1 = sf_out[0] * sf0;
	sf2 = sf_out[1] * sf0;
    } else {
	sf1 = 0;
	sf2 = 0;
    }
    if (mode == 2) {
	for (i = 0; i < npts; i++) {
	    dac0[i] = (int32_t) (dac_a_sbf[i] * sf1);
	    dac1[i] = (int32_t) (dac_b_sbf[i] * sf2);
	}
    } else {
	for (i = 0; i < npts; i++) {
	    dac0[i] = (int32_t) (dac_a_sbf[i] * sf1);
	    dac1[i] = 0;
	}
    }
}

static void
abr_get_lswp(int32_t bo)
{
    double v1, v2;
    int i, k;
    int32_t *adc0, *adc1;
    short s1, s2;

    k = bo % nbpc;
    adc0 = (int32_t *) vbanki[k * 2 + 0];
    adc1 = (int32_t *) vbanki[k * 2 + 1];
    if (swp_acc < sweeps) {
	for (i = 0; i < npts; i++) {
	    v1 = adc0[i] * sf_in;
	    v2 = adc1[i] * sf_in;
	    acc_a_lbf[i] += (int32_t) v1;
	    acc_b_lbf[i] += (int32_t) v2;
	    s1 = (short) v1;
	    s2 = (short) v2;
	    if (spctr == 0 && i == 0) {
	        mnmx_sbf[0] = mnmx_sbf[1] = (short) v1;
		mnmx_sbf[2] = mnmx_sbf[3] = (short) v2;
	    } else {
		if (mnmx_sbf[0] < s1) {
		    mnmx_sbf[0] = s1;
		}
		if (mnmx_sbf[1] > s1) {
		    mnmx_sbf[1] = s1;
		}
		if (mnmx_sbf[2] < s2) {
		    mnmx_sbf[2] = s2;
		}
		if (mnmx_sbf[3] > s2) {
		    mnmx_sbf[3] = s2;
		}
	    }
	    if (spctr < maxspts && i == spidx) {
	        spts_sbf[spctr] = s1;
	        spts_sbf[spctr + maxspts] = s2;
	        spctr++;
	    }
	}
	if (bo >= skip) {
    	    swp_acc++;
	    datflg++;
	}
    }
}

/* abr_init - initialize local variable from parameter area */

static void
abr_init()
{
    unsigned short  *p;

    /* fetch DSP parameters */
    p = (unsigned short *) (dspmem + dspprm - base_addr);
    mode = p[0];
    npts = p[1];
    sweeps = p[2];
    dac_a = p[3];
    dac_b = p[4];
    acc_a = p[5];
    acc_b = p[6];
    greg = p[7];
    spidx = p[8];
    skip = p[9];
    code = p[10];
    nic = 2;
}

/* abr_start - emulates DSP code for ABR recording */

static int
abr_start()
{
    int	    i, err = 0;
    static int32_t size[4];
    static int32_t fmt[2] = {ARSC_DATA_I4, 0};

    abr_init();
    if (npts <= 0 || sweeps <= 0)
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

    err = sio_alloc();
    if (err) {
        d0error("can't allocate memory", err);
	sio_free();
        return (2);
    }
    err = ar_set_fmt(iodev, fmt);
    err = ar_io_open_off(iodev, rate_set, ncio, ncio, choi, choo);
    rate_set = ar_get_rate(iodev);
    if (err) {
        d0error("can't open I/O device", err);
	sio_free();
        return (3);
    }
    setoff = 0;
    swpcnt = sweeps + skip;    /* compute sweep count */
    for (i = 0; i < 4; i++) {
        size[i] = npts;
    }
    sf0 = (float) ((pow(2,32)-1) / (pow(2,16)-1));
    err = ar_set_xfer(iodev, abr_get_lswp, abr_put_sswp);
    err = ar_io_prep(iodev, vbanki, vbanko, size, nbpc, 0);
    datflg = 0;
    swp_acc = 0;
    ar_io_start(iodev);
    sioflg = 1;

    return (0);
}

/*************************************************************************/

static void
dsp_init()
{
    if (dsp_type == 1) {
	abr_init();
    } else { 
	oae_init();
    }
}

static int
dsp_start()
{
    if (dsp_type == 1) {
	return (abr_start());
    } else {
	return (oae_start());
    }
}

static void
dsp_halt()
{
    ar_io_stop(iodev);
    sioflg = 0;
    ar_io_close(iodev);
    sio_free();
}

static void
dsp_check()
{
    if (sioflg) {
	curseg = ar_io_cur_seg(iodev);
	if (curseg > swpcnt) {
	    sweeps = swp_acc;
	    dsp_halt();
	}
    }
}

static void
dsp_clr_acc()
{
    if (nic > 0) { 
	// clear accumulate buffer A
        memset(acc_a_lbf, 0, npts * sizeof(int32_t));
    }
    if (nic > 1) { 
	// clear accumulate buffer B
        memset(acc_b_lbf, 0, npts * sizeof(int32_t));
    }
    if (sioflg) { 
	// allow opportunity for i/o buffer transition
	ar_io_cur_seg(iodev);
    }
}

static void
dsp_set_swpcnt(int c)
{
    sweeps = c;
    swpcnt = sweeps + skip;
    if (dsp_type != 1 && range) {
	// add sweep for OAE ramp down
	swpcnt++;
    }
}

/*************************************************************************/

/* F1: (DINIT) Initialize Soundcard */
int
dspinit(char *hex_file)
{
    static char *default_device = "CardDeluxe Analog";

    if (ar_num_devs()) {
	if (dspmem == NULL) {
	    dspmem = (short *) calloc(dspsiz, sizeof(short));
	}
	if (hex_file == NULL || strlen(hex_file) == 0) {
	    hex_file = default_device;
	}
	iodev = atoi(hex_file) - 1;
	if (iodev < 0) {
	    iodev = ar_find_dev_name(hex_file);
	}
#ifdef WIN32
	if (iodev <= 0) {
	    iodev = ar_find_dev(ARSC_PREF_ASIO);
	}
#endif
	if (iodev <= 0) {
	    iodev = ar_find_dev(ARSC_PREF_SYNC);
	}
	sf_out[0] = sf_out[1] = 1;
	sf_in = sf_in0;
	return (0);
    }
    return (1);
}

/* F2: (DSETA) Set Address. */

int
dspseta(int num)
{
    dsp_type = num;	    /* select DSP code: 0=oae, 1=abr */
    return (ar_num_devs() <= 0);
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
    int prev_devset = devset;

    if (reset && devset) {		/* Stop I/O */
	dsp_halt();
	devset = 0;
    } else if (!reset && !devset) {	/* Start I/O */
	if (dsp_start() == 0) {
	    devset = 1;
	}
    }
    return (prev_devset);
}

/* F30: (DGET) Get Read Status. */

int
dspdget(void)
{
    if (devset) {
	dsp_check();
    }
    return (datflg);
}

/* F31: (DREAD) Read Immediate. */

int
dspdread(short *data)
{
    *data = sweeps - swp_acc;
    datflg = 0;
    return (0);
}

/* dspdev - get current device name */

char *
dspdev(void)
{
    static char name[80];

    ar_dev_name(iodev, name, 80);
    return (name);
}

/* dspname - get device name by number */

char *
dspname(int num)
{
    static char name[80];

    if ((num >= 0) && (num < ar_num_devs())) {
        ar_dev_name(num, name, 80);
	return (name);
    }
    return (NULL);
}

/* dspsets - set output scale factor */

double
dspsets(double sf)
{
    //return (limit(1e-9, sf, 1));
    return (1);     // [2-Nov-2010 STN]
}

/* dspsatt - set input attenuator */

double
dspsatt(double at)
{
    at = limit(0, at, 120);
    sf_in = (float) (pow(10.0, -at / 20) * sf_in0);
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
    return ((int32_t) ar_adjust_rate(iodev, (double)r)); // <- needs fixing
}

/* dspsetrt - set DSP sample rate */

int32_t
dspsetrt(int32_t r)
{
    if (r != (int32_t) rate_set) {
        ar_io_close(iodev);
	if (ar_io_open_off(iodev, (double) r, ncio, ncio, choi, choo)) {
	    rate_set = 0; /* device open failure */
	} else {
            rate_set = ar_get_rate(iodev);
	}
    }
    return ((int32_t) rate_set);
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

/* dspinpflt - set DSP input filter */

void
dspinpflt(float **a, float **b, int *nfo)
{
    int i, nc;

    // channel 1
    free_null(a1);
    free_null(b1);
    free_null(z1);
    nfo1 = nfo[0];
    nc = nfo1 + 1;
    a1 = (float *) calloc(nc, sizeof(float));
    b1 = (float *) calloc(nc, sizeof(float));
    z1 = (float *) calloc(nc, sizeof(float));
    for (i = 0; i < nc; i++) {
        a1[i] = a[0][i];
        b1[i] = b[0][i];
    }
    // channel 2
    free_null(a2);
    free_null(b2);
    free_null(z2);
    nfo2 = nfo[1];
    nc = nfo2 + 1;
    a2 = (float *) calloc(nc, sizeof(float));
    b2 = (float *) calloc(nc, sizeof(float));
    z2 = (float *) calloc(nc, sizeof(float));
    for (i = 0; i < nc; i++) {
        a2[i] = a[1][i];
        b2[i] = b[1][i];
    }
}

/*************************************************************************/

/* dspapi - return soundcard API */

char *
dspapi(void)
{
    return (ar_version());
}

/* dspvfs - return ARSC vfs, if needed */

void
dspvfs(float *advfs, float *davfs)
{
    double adv[MAXNDC], dav[MAXNDC];
    int i;

    ar_get_vfs(iodev, dav, adv);
    for (i = 0; i < 2; i++) {
	advfs[i] = (float) adv[i];
	davfs[i] = (float) dav[i];
    }
}

/* dspclr - zero accumulate buffers */

void
dspclr(void)
{
    dsp_clr_acc();
}

/* dspcnt - reset sweep count */

void
dspcnt(int c)
{
    dsp_set_swpcnt(c);
}

/* dspcho - channel offset */

void
dspcho(int chnoff_in, int chnoff_out)
{
    choi = chnoff_in;
    choo = chnoff_out;
}

/* dspend - set termination flag */

void
dspend()
{
    if (devset) {
        setoff = 1;
	dsp_check();
    }
}

/*************************************************************************/
