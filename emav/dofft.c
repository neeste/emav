/* =============================== dofft.c =============================== */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <fftlib.h>
#include <menu.h>
#include <util.h>
#include "savage.h"
#include "color.h"
#include "emav.h"

#define EMIN		(1e-20)
#define SPSZ		(2*MAXNNSB+1)*2
#define free_zero(x)	{if(x){free(x);x=0;}}

extern float red_thr;
extern int display_reset, fft_flag, swp1set, dfgc, stim_flag, probe_flag;
extern WIND w_stim_fft;

int     lpsflg = 0;

static double  mp_sens(double, float *);

static float *abuf = NULL;
static float *bbuf = NULL;
static float *fbuf = NULL;
static float *phbf = NULL;
static float *sbuf = NULL;
static int size = 0;

/* allocate space for FFT buffer fbuf and set the size to be power of two */
int
alloc_space(int create)
{
    int     n;

    if (create == 0) {
	if (fbuf != NULL) {
	    free(abuf);
	    free(bbuf);
	    free(fbuf);
	    free(phbf);
	    free(sbuf);
	    fbuf = NULL;
	}
	size = 0;
    } else {
	n = 1;
	while (n < buflen)
	    n *= 2;
	if (n != size) {
	    if (fbuf != NULL) {
		free(abuf);
		free(bbuf);
		free(fbuf);
		free(phbf);
		free(sbuf);
	    }
	    size = 0;
	    abuf = (unsigned long)((float *) calloc((size_t)((n + 2)), sizeof(float)));
	    bbuf = (unsigned long)((float *) calloc((size_t)((n + 2)), sizeof(float)));
	    fbuf = (unsigned long)((float *) calloc((size_t)((n + 2)), sizeof(float)));
	    phbf = (unsigned long)((float *) calloc((size_t)((n + 2)), sizeof(float)));
	    sbuf = (unsigned long)((float *) calloc((size_t)((n + 2)), sizeof(float)));
	    if (fbuf != NULL)
		size = n;
	}
    }
    return (size);
}

static double
press_ref(double fr, float *ph)
{
    double b;

    b = spl_ref * Sen.AD * mp_sens(fr, ph) * size; // 3.22

    return (b * b / 2);
}

void
modulesqr()
{
    double  a, b, c, d, fr;
    float   ph;
    int     i, j;

    d = (double) rate / size;
    for (i = 0; i < size; i++) {
        a = fbuf[i++];
        b = fbuf[i];
        j = i / 2;
        fr = j * d;
        c = press_ref(fr, &ph);
        fbuf[i / 2] = (float) ((a * a + b * b) / c);
        phbf[i / 2] = (float) (atan2(b, a) - ph);
    }
}

/* save the FFT result into float array f */
static void
save_fft(float *f, int n)
{
    int     i;

    for (i = 0; i < n; i++) {
        if (fbuf[i] <= 1e-80)
            f[i] = (float) 1e-40;
        else
            f[i] = (float) sqrt(fbuf[i]);
    }
}

double
pkmn(int i1, int i2, int di, float *buf)
{
    double   f, c, mn;
    int     i, ii;

    ii = i1;
    mn = buf[i1*di];
    for (i = i1; i <= i2; i++) {
        if (mn > buf[i*di]) {
            mn = buf[i*di];
            ii = i;
        }
    }
    c = 2 * buf[ii*di] - (buf[(ii+1)*di] + buf[(ii-1)*di]);
    if (c > -0.05 || c < -20) {
    	f = ii;
    } else {
        f = ii + (buf[(ii+1)*di] - buf[(ii-1)*di]) / (2 * c);
    }
    f = limit(i1, f, i2);
    return (f);
}

double
pkmx(int i1, int i2, int di, float *buf)
{
    double   f, c, mx;
    int     i, ii;

    ii = i1;
    mx = buf[i1*di];
    for (i = i1; i <= i2; i++) {
        if (mx < buf[i*di]) {
            mx = buf[i*di];
            ii = i;
        }
    }
    c = 2 * buf[ii*di] - (buf[(ii+1)*di] + buf[(ii-1)*di]);
    if (c < 0.05 || c > 20) {
    	f = ii;
    } else {
        f = ii + (buf[(ii+1)*di] - buf[(ii-1)*di]) / (2 * c);
    }
    f = limit(i1, f, i2);
    return (f);
}

double
pick_min(double f1, double f2)
{
    double  df;
    int     i1, i2;

    df = ((double) rate) / size;
    i1 = (int) (f1 / df + 0.5);
    i2 = (int) (f2 / df + 0.5);
    return (pkmn(i1, i2, 1, fbuf) * df);
}

double
pick_max(double f1, double f2)
{
    double  df;
    int     i1, i2;

    df = ((double) rate) / size;
    i1 = (int) (f1 / df + 0.5);
    i2 = (int) (f2 / df + 0.5);
    return (pkmx(i1, i2, 1, fbuf) * df);
}

void
retrieve_fft(float *p, int n)
{
    int     i;
    float   s;

    n *= 2;
    if (n > size)
        n = size;
    s = (float) (sqrt(2)/(spl_ref * Sen.MP * Sen.AD * size));	// MP_transfer ???
    for (i = 0; i < n; i++)
        p[i] = fbuf[i] * s;
}

#ifdef DUMP_FFT
static void
dump_fft()
{
    int i, n;
    double a, dt, lpsval, f, m;

    printf("; size=%d rate=%d\n", size, rate);
    n = size / 2;
    dt = ((double) size) / rate;
    lpsval = 10 * log10(dt);
    for (i = 0; i < n; i++) {
        a = fbuf[i];
        a = (i == 0 || a < EMIN) ? EMIN : a;
        m = 10 * log10(a) + lpsval;
        f = i / dt;
        printf("%8.1f %8.1f\n", f, m);
    }
}
#endif

/* display FFT result in the correct units */
void
show_fft(int xrange, int yrange, int levref, int color, int reset)
{
    double  dt, a, lpsval;
    int     i, n, np;
    short  *ibuf;
    WIND   *w;

    dfgc = color;
    display_reset = reset;
    lpsflg = levref;
    ibuf = (short *) fbuf;
    if (size < 2)
	return;

    if (xrange < 3) {
	n = size;
	dt = (float)(((float) n) / rate);
	lpsval = lpsflg ? 10 * log10(dt) : 0;
	for (i = 0; i < n; i++) {
	    a = fbuf[i];
	    a = (i == 0 || a < EMIN) ? EMIN : a;
	    ibuf[i] = (short)((int) (10 * log10(a) + lpsval + 0.5));
	}
    } else {
	n = size / 2;
	dt = (float)(((float) n) / rate);
	lpsval = lpsflg ? 10 * log10(dt) : 0;
	for (i = 0; i < n; i++) {
	    a = fbuf[i++];
	    a += fbuf[i];
	    a = (i == 1 || a < EMIN) ? EMIN : a;
	    ibuf[i / 2] = (short)((int) (10 * log10(a) + lpsval + 0.5));
	}
    }
    w = w_dis;
    w_dis = stim_flag ? &w_stim_fft : &w_spec;
    mouse_exclude(w_dis->xtop, w_dis->ytop, w_dis->ytop, w_dis->ybot);
    if (display_reset > 0) {
	gr_rectf(w_dis->xtop + 1, w_dis->ytop + 1, w_dis->xbot - 1,
	    w_dis->ybot - 1, w_dis->wbgc);
    }
    if (probe_flag)
	display_reset = 1;
    fft_flag = 1;
    np = (int) (xrange * dt * 1000 + 0.5);
    w_dis->ybot -= txtpar.font_height;
    display(ibuf, np, display_reset, yrange, xrange);
    w_dis->ybot += txtpar.font_height;
    show_xlabel(0.0, (double) xrange, 0, "kHz");
    mouse_show(1);
    fft_flag = 0;
    w_dis = w;

    display_reset = 0;
}

/* take the FFT of the sum/diff of two long buffer p and q */
void
fft_A_B(int32_t *p, int32_t *q, int addflg, int sets, int flag, int chan)
{
    int     i, n;

    if (swp1set <= 0 || !sets)
	return;

    if (!alloc_space(1))
	return;

    if (chan) {
	p += buflen;
	q += buflen;
    }

    n = sets * swp1set;
    if (addflg) {
	for (i = 0; i < buflen; i++) {
	    abuf[i] = (float) p[i] / n;
	    bbuf[i] = (float) q[i] / n;
	}
        for (; i < size; i++) {
	    abuf[i] = 0;
	    bbuf[i] = 0;
	}
        fast(abuf, size);
        fast(bbuf, size);
	for (i = 0; i < size; i++)
	    fbuf[i] = (abuf[i] + bbuf[i]) / 2;
    } else {
	for (i = 0; i < size; i++)
	    fbuf[i] = (abuf[i] - bbuf[i]) / 2;
    }
    if (flag)
	modulesqr();
}

/* FFT of the p[0], ..., p[buflen - 1] */
int
fft_sig(int32_t *p, int sets)
{
    int     i;

    if (!alloc_space(1))
	return (0);

    for (i = 0; i < buflen; i++)
	fbuf[i] = (float) (p[i] / (swp1set * sets * 2));
    for (; i < size; i++)
	fbuf[i] = 0;
    fast(fbuf, size);
    i = size / 2;
    return (i);
}

/* FFT long buffer p[0], ..., p[buflen-1] with save option */
int
fft_stim(int32_t *p, int sets, int save)
{
    int     n;

    n = fft_sig(p, sets);
    modulesqr();
    if (save && n)
	save_fft((float *) sbuf, n);
    return (n);
}

/* FFT short int buffer p[0], ..., p[buflen -1] */
int
fft_short(short *p, int div_flg)
{
    int     i;
    float   att = 1.0;

    if (!alloc_space(1))
	return (0);

    if (div_flg)
	att = attenu.intern_att;
    for (i = 0; i < buflen; i++)
	fbuf[i] = (float) p[i] / att;
    for (; i < size; i++)
	fbuf[i] = 0;
    fast(fbuf, size);
    i = size / 2;
    modulesqr();
    return (i);
}

/* compute and store gain for one channel
   gain = (FFT of ear response / FFT of original stimulus)
*/
void
cal_gain(SAV_FFT *pft, int npts)
{
    double fr, df;
    float  *a, *b, mph = 0;
    int     i, n;

    pft->rate = rate;
    if (pft->rp == NULL || pft->npts != npts) {
        if (pft->rp != NULL)
            free(pft->rp);
        if (pft->ph != NULL)
            free(pft->ph);
	pft->rp = (unsigned long)((float *) malloc((size_t)(npts) * sizeof(float)));
	pft->ph = (unsigned long)((float *) malloc((size_t)(npts) * sizeof(float)));
	pft->npts = (short)(npts);
    }
    memcpy(pft->rp, sbuf, (size_t)(npts) * sizeof(float));
    memcpy(pft->ph, phbf, (size_t)(npts) * sizeof(float));

    i = fft_short(stmbuf, 1);
    if (!i)
	return;
    save_fft(fbuf, i);
    a = pft->rp;
    b = pft->ph;
    n = pft->npts;
    df = rate / 2.0 / npts;
    for (i = 0; i < n; i++) {
	fr = i * df;
	a[i] /= fbuf[i] * (float) (spl_ref * Sen.AD * mp_sens(fr, &mph));
	b[i] -= phbf[i] + mph;
    }
}

/* pick out the real and imaginary components of the FFT at frequency f */
void
pick_A_B(double f, float *ar, float *ai, float *br, float *bi)
{
    int     i, j;

    i = nint((f * size) / rate) * 2;
    if (i <= 0 || i > size) {
        *ar = *ai = *br = *bi = 0;
    } else {
	j = i + 1;
        *ar = abuf[i];
        *ai = abuf[j];
        *ar = bbuf[i];
        *ai = bbuf[j];
    }
}

/* pickat - pick out the magnitude of the FFT at frequency f */

double
pickat(double f)
{
    double  v;
    int     i;

    i = nint((f * size) / rate);
    if (i <= 0 || i > size / 2)
        return (-400.0);
    v = fbuf[i];
    if (v < EMIN)
	v = EMIN;
    v = 10 * log10(v);
    return (v);
}

/* fetch_spec - retrieve a range of complex values from the FFT */

static void
fetch_spec(double f, int ns, float *aspc, float *bspc)
{
    int i, j, n, ii;

    i = 2 * (nint((f * size) / rate) - ns);
    n = 2 * (2 * ns + 1);
    for (j = 0; j < n; j++) {
	ii = i + j;
	if (ii > 0 && ii < size) {
	    aspc[j] = abuf[ii];
	    bspc[j] = bbuf[ii];
	} else {
	    aspc[j] = 0;
	    bspc[j] = 0;
	}
    }
}

/* artred1 - artifact reduction (first method) removes mean value */

void
artred1(float *p, int d, int o, int n, int ns, int j, double rt)
{
    double v, sm, ss, mn, ms, rs, rr;
    int i, k, m;

    // compute mean and variance
    sm = 0;
    ss = 0;
    m = 0;
    for (i = 0; i < n; i++) {
	if (((i - ns) % j) != 0) {
	    k = i * d + o;
	    v = p[k];
	    sm += v;
	    ss += v * v;
	    m++;
	}
    }
    if (m > 0) {
	mn = sm / m;	// mean value
	ms = ss / m;	// mean squared value
        ss = 0;
	for (i = 0; i < n; i++) {
	    if (((i - ns) % j) != 0) {
		k = i * d + o;
		v = p[k] - mn;
		ss += v * v;
	    }
	}
	rs = ss / m;	// reduced mean squared value;
	rr = rs / ms;   // ratio of reduced to initial mean squared value;
	if (rr < rt) {
	    for (i = 0; i < n; i++) {
		k = i * d + o;
		v = p[k] - mn;
		p[k] = (float) v;
	    }
	}
    }
}

/* artred2 - artifact reduction (second method) removes linear regression */

void
artred2(float *p, int d, int o, int n, int ns, int j, double rt)
{
    double pv, psm, pss, pms, rs, rr;
    double xv, xsm, xss, xps, detx, a, b;
    int i, k, m;

    // compute mean and variance
    xsm = 0;
    psm = 0;
    xss = 0;
    pss = 0;
    xps = 0;
    m = 0;
    for (i = 0; i < n; i++) {
	if (((i - ns) % j) != 0) {
	    k = i * d + o;
	    xv = i;
	    pv = p[k];
	    xsm += xv;
	    psm += pv;
	    xss += xv * xv;
	    pss += pv * pv;
	    xps += xv * pv;
	    m++;
	}
    }
    if (m > 0) {
	pms = pss / m;
	detx = m * xss - xsm * xsm;
        a = (psm * xss - xsm * xps) / detx;
        b = (m * xps - xsm * psm) / detx;
        pss = 0;
	for (i = 0; i < n; i++) {
	    if (((i - ns) % j) != 0) {
		k = i * d + o;
		pv = p[k] - (a + b * i);
		pss += pv * pv;
	    }
	}
	rs = pss / m;	// reduced mean squared value;
	rr = rs / pms;   // ratio of reduced to initial mean squared value;
	if (rr < rt) {
	    for (i = 0; i < n; i++) {
		k = i * d + o;
		pv = p[k] - (a + b * i);
		p[k] = (float) pv;
	    }
	}
    }
}

/* get_level - primary function for determining spectral level */

void
get_level(double fr, int nsb, int typ, double df, float *slv, float *nlv, float *sph)
{
    double  cr, ci, sr, ms, er, rt, raddeg;
    float   *asp, *bsp,dbmin, ph;
    int     i, j, m, n, ir, ii, ns;
    static double emin = 1e-40;

/* zero frequency returns minimum level */

    dbmin = (float) (10 * log10(emin));
    if (fr <= 0) {
        *slv = dbmin;
	*sph = 0;
        *nlv = dbmin;
	return;
    }

/* fetch sidebands */

    if (nsb <= 0) {
	ns = -nsb;
	j = ns + 1;
    } else {
	ns = nsb;
	j = nint((df * size) / rate);
    }
    n = 1 + 2 * ns;
    asp = (unsigned long)((float *) calloc(n, 2 * sizeof(float)));
    bsp = (unsigned long)((float *) calloc(n, 2 * sizeof(float)));
    fetch_spec(fr, ns, asp, bsp);

/* remove correlated noise to reduce artifacts */

    if (typ == 1 && ns > 0) {
        er = 1 - 0.5 / ns;
	rt = (red_thr > 0) ? red_thr : er;
        artred1(asp, 2, 0, n, ns, j, rt);
        artred1(asp, 2, 1, n, ns, j, rt);
        artred1(bsp, 2, 0, n, ns, j, rt);
        artred1(bsp, 2, 1, n, ns, j, rt);
        //ff = 10 * log10(rt);
    } else if (typ == 2 && ns > 1) {
        er = 1 - 1.0 / ns;
	rt = (red_thr > 0) ? red_thr : er;
        artred2(asp, 2, 0, n, ns, j, rt);
        artred2(asp, 2, 1, n, ns, j, rt);
        artred2(bsp, 2, 0, n, ns, j, rt);
        artred2(bsp, 2, 1, n, ns, j, rt);
    } else {
        er = 1;
    }

/* compute signal level & phase */

    sr = press_ref(fr, &ph); 
    ir = 2 * ns;
    ii = 2 * ns + 1;
    cr = (asp[ir] + bsp[ir]) / 2;
    ci = (asp[ii] + bsp[ii]) / 2;
    ms = (cr * cr + ci * ci) / sr; 
    if (ms < emin) {
        *slv = dbmin;
	*sph = 0;
    } else {
        raddeg = 180 / M_PI;
        *slv = (float) (10 * log10(ms));
	*sph = (float) ((atan2(ci, cr) - ph) * raddeg);
    }

/* compute noise level */

    m = 0;
    ms = 0;
    for (i = 0; i < n; i++) {
	ir = 2 * i;
	ii = 2 * i + 1;
	if (nsb <= 0) {
	    cr = (asp[ir] - bsp[ir]) / 2;
	    ci = (asp[ii] - bsp[ii]) / 2;
	} else if (((i - ns) % j) == 0) {
	    continue;
	} else {
	    cr = (asp[ir] + bsp[ir]) / 2;
	    ci = (asp[ii] + bsp[ii]) / 2;
	}
	ms += cr * cr + ci * ci; 
	m++;
    }
    if (m > 0)
	ms /= m * sr * er;
    if (ms < emin) {
        *nlv = dbmin;
    } else {
        *nlv = (float) (10 * log10(ms));
    }
    free(asp);
    free(bsp);
}

/* check_noise - check for noise floor separation */

int
check_noise(double f, double df, double nf_max_dif, short nfsb)
{
    double  cr, ci, ms1, ms2, dbdif;
    float   *asp, *bsp;
    int     i, j, m, n, ir, ii, ns;
    static double emin = 1e-40;

    if (f <= 0 || nfsb <= 0)
	return (0);

/* fetch sidebands */

    ns = nfsb;
    j = nint((df * size) / rate);
    n = 1 + 2 * ns;
    asp = (unsigned long)((float *) calloc(n, 2 * sizeof(float)));
    bsp = (unsigned long)((float *) calloc(n, 2 * sizeof(float)));
    fetch_spec(f, ns, asp, bsp);

/* compute noise level */

    m = 0;
    ms1 = ms2 = 0;
    for (i = 0; i < n; i++) {
	if (((i - ns) % j) == 0)    // skip signal frequencies
	    continue;
	ir = 2 * i;
	ii = 2 * i + 1;
        cr = (asp[ir] - bsp[ir]) / 2;
        ci = (asp[ii] - bsp[ii]) / 2;
	ms1 += cr * cr + ci * ci; 
        cr = (asp[ir] + bsp[ir]) / 2;
        ci = (asp[ii] + bsp[ii]) / 2;
	ms2 += cr * cr + ci * ci; 
	m++;
    }
    free(asp);
    free(bsp);
    if (m > 0 && ms1 > emin && ms2 > emin) {
	dbdif = fabs(10 * log10(ms1 / ms2));
	if (dbdif > nf_max_dif)
	    return (1);
    }

    return (0);
}

/* =============================== mp_transfer =============================== */

int  access(char *, int);

extern char	line[];

static float	mps_df, mps_fm;
static int	mps_nf = 0;
static float   *mps_mg, *mps_ph;

static void
mp_transfer_write(char *fn)
{
    double tpi, fr, mg, ph;
    int i;
    FILE *fp;

    if (mps_nf > 0) {
        fp = fopen(fn, "wt");
	fprintf(fp, "; %s\n", fn);
	fprintf(fp, "; rate = %d (Hz)\n", rate);
	fprintf(fp, "; MP_transfer = %s\n", Sen.MPfn);
	fprintf(fp, "; MP_sensitivity = %.3f\n", Sen.MP);
	fprintf(fp, ";    f     mg     ph\n");
        tpi = 2 * M_PI;
	for (i = 1; i <mps_nf; i++) {
	    fr = (float)(mps_df * i);
	    mg = mps_mg[i];
	    ph = mps_ph[i] / tpi;	// convert rad to cyc
	    fprintf(fp, " %6.0f %7.4f %6.3f\n", fr, mg, ph);
	}
	fclose(fp);
    }
}

static void
mp_transfer_set(int nf, float *fr, float *mg, float *ph)
{
    double f, a;
    int  i, j;

    free_zero(mps_mg);
    mps_nf = buflen / 2;
    mps_df = rate / (float) buflen; // df=Hz
    mps_fm = (float)(mps_df * mps_nf);
    mps_mg = (unsigned long)((float *) calloc((size_t)(mps_nf * 2), sizeof(float)));
    mps_ph = mps_mg + mps_nf;
    j = 0;
    for (i = 0; i < mps_nf; i++) {
	f = (float)(i * mps_df);
	if (nf < 1) {
	    mps_mg[i] = 0;
	    mps_ph[i] = 0;
	} else if (f <= fr[0]) {
	    mps_mg[i] = mg[0];
	    mps_ph[i] = ph[0];
	} else if (f >= fr[nf - 1]) {
	    mps_mg[i] = mg[nf - 1];
	    mps_ph[i] = ph[nf - 1];
	} else {
	    while ((j + 1) < (nf - 1) && f > fr[j + 1])
		j++;
	    a = (fr[j + 1] - f) / (fr[j + 1] - fr[j]);
	    mps_mg[i] = (float) (a * mg[j] + (1 - a) * mg[j + 1]);
	    mps_ph[i] = (float) (a * ph[j] + (1 - a) * ph[j + 1]);
	}
    }
}

static int
mp_transfer_mat(char *fn)
{
    float *fr, *mg, *ph;
    int  fd, n, nf = 0;

    if (access(fn, 0) == 0) {	// read mp_sens from file
	fd = mptrans_open(fn, &n);
	if (fd > 0 && n > 0) {
	    nf = n / 2 + 1;
	    fr = (unsigned long)((float *) calloc((size_t)(nf * 3), sizeof(float)));
	    mg = fr + nf;
	    ph = mg + nf;
	    mptrans_read(fd, fr, mg, ph);
	    mp_transfer_set(nf, fr, mg, ph);
	    free(fr);
	}
    }
    return (nf);
}

static int
mp_transfer_txt(char *fn)
{
    float tpi, *fr, *mg, *ph;
    int  nf = 0;
    FILE *fp;

    if (access(fn, 0) == 0) {	// read mp_sens from file
	fp = fopen(fn, "rt");
	while (fgets(line, MAXLINE, fp) != NULL && nf < MAXPOSINT) {
	    if (line[0] > 0 && line[0] != ';') {
		nf++;
    	    }
	}
	fclose(fp);
	if (nf > 0) {
	    fr = (unsigned long)((float *) calloc((size_t)(nf * 3), sizeof(float)));
	    mg = fr + nf;
	    ph = mg + nf;
	    tpi = (float) (2 * M_PI);
	    nf = 0;
	    fp = fopen(fn, "rt");
	    while (fgets(line, MAXLINE, fp) != NULL && nf < MAXPOSINT) {
		if (line[0] > 0 && line[0] != ';') {
		    fr[nf] = mg[nf] = ph[nf] = 0;
		    // assume: fr=Hz, ph=cyc
		    sscanf(line, " %f %f %f", &fr[nf], &mg[nf], &ph[nf]);
		    ph[nf] *= tpi;   // convert phase to radians
		    nf++;
    		}
	    }
	    fclose(fp);
	    mp_transfer_set(nf, fr, mg, ph);
	    free(fr);
	}
    }
    return (nf);
}

void
mp_transfer(char *tfn)
{
    if (mp_transfer_mat(tfn)) {		    // read mp_sens from mat file
        strncpy(Sen.MPfn, tfn, 41);
    } else if (mp_transfer_txt(tfn)) {	    // read mp_sens from txt file
        strncpy(Sen.MPfn, tfn, 41);
    } else {
	if (atof(tfn) > 0) {		    // set mp_sens from filename
	    Sen.MP = (float) atof(tfn);
	}
        free_zero(mps_mg);
	sprintf(Sen.MPfn, "%.4g", Sen.MP);
        mps_nf = 0;
    }
    mp_transfer_write("mptrans.txt");	    // debug
}

static double
mp_sens(double fr, float *pp)
{
    double mg = 0, ph = 0;
    int n;

    if (mps_nf > 0) {
        if (fr <= 0) {
            n = 0;
        } else if (fr >= mps_fm) {
            n = mps_nf - 1;
        } else {
            n = nint(fr / mps_df);
        }
        mg = mps_mg[n];
        ph = mps_ph[n];
    }
    if (mg == 0) {
        mg = Sen.MP;   // (V/Pa)
        ph = 0;
    }

    *pp = (float) ph;
    return (mg);
}
