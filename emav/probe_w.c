/* probe_w.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <dsp.h>
#include <menu.h>
#include <util.h>
#include <fftlib.h>
#include "savage.h"
#include "color.h"
#include "screen.h"
#include "emav.h"

#ifdef WIN32
#include <io.h>
#define PMODE       (_S_IREAD|_S_IWRITE)
#define OFLAG       (_O_RDWR|_O_CREAT|_O_BINARY)
#else
#include <unistd.h>
#define _access access
#define _close close
#define _open open
#define _write write 
#endif /* WIN32 */

#define MAXNPTS		8192	/* based on 65KW SMA */
#define FFRAC           2
#define MAX_NF          (MAXNPTS/FFRAC+1)
#define MAX_SM		32
#define free_null(x)	if(x){free(x);x=NULL;}
void    simpfit();

extern char line[], ths_file[];
extern float cal_attn, chk_attn;
extern int probe_counter, write_txt, rand_seed;
extern int lpsflg, fft_flag, probe_flag, dpoae_flag, cavit_test;
extern int maxnpts, cal_sets, cal_swps, chk_swps, contav;
extern short numsmo;
extern struct _dpoae dpoae;
extern struct _probe probe;
extern MENUITEM file_opt[];
extern TOGGLETYPE thv_itr, thv_ext, thv_xtk, z_cav_type, dec_len, stim_unit;

#ifndef WIN32
#define _hypot hypot
struct _complex {
    double  x;
    double  y;
};
#endif /* WIN32 */

typedef struct _complex complex;

static char load_cal[MAXNAME];
static double err[2];
static float *pc[MAXNCAV];
static float  fz1[2][MAXNCAV], fpk[2][MAXNCAV];
static float  ecv[2][MAXNCAV], lcv[2][MAXNCAV + 4], Rcv[2];
static float *ps, *px, *zs, *zl, *pl, *z0, *pr, *pf, *cd;
static float *qq1 = NULL, *st = NULL, *pzl = NULL, *pzs = NULL;
static float *padj = NULL, *iadj = NULL, *refl = NULL;
static int db_range = 40;
static int dis_cal_imp = 0;
static int extend = 0;
static int esc_flg = 0;
static int itr_flg = 0;
static int nadj = 0;
static int nftf = 0;
static int nthf = 0;
static int nthc = 0;
static int necv = 0;
static int os = 0;
static int ol = 0;

/* make the probe paramters current */

void
grab_probe()
{
    dis_units(Sen.AD, Sen.MP);
    buflen = limit(MINNPTS, dpoae.size, maxnpts);
    rate = dpoae.rate;
    acc_sets = dpoae.sets;
    numcav = probe.ncav;
    numsrc = probe.nsrc;
    fft_xrange = probe.fft_xrange;
    fft_yrange = probe.fft_yrange;
    fft_levref = probe.levref;
    cal_attn = dpoae.cal_atten;
    cal_swps = dpoae.cal_swps;
    chk_attn = dpoae.chk_atten;
    chk_swps = dpoae.chk_swps;
    contav = dpoae.contav;
    change_nic(dpoae.nic);
    input_filter(hpf_type, hpf_ord, dpoae.hpff, dpoae.rate);
    rand_seed = dpoae.seed;
}

static complex
cdiv(complex a, complex b)
{
    complex c;
    double  d;

    d = b.x * b.x + b.y * b.y;
    if (d < 1e-20)
    	d = 1e-20;
    c.x = (a.x * b.x + a.y * b.y) / d;
    c.y = (a.y * b.x - a.x * b.y) / d;
    return (c);
}

static complex
dbph(complex z)
{
    double zmg;
    complex zm;
    static double eps = 1e-40;

    zmg = _hypot(z.x, z.y);
    if (zmg > eps) {
        zm.x = 20 * log10(zmg);
        zm.y = atan2(z.y, z.x) * (180 / M_PI);
    } else {
        zm.x = -800;
        zm.y = 0;
    }
    return (zm);
}

/*
 * Thermodynamic constants:
 *
 *    rho   = 1.1769e-3 * (1-0.00335*deltemp)  g/cm^3
 *    eta   = 1.846e-4  * (1+0.0025*deltemp)   g/s/cm
 *    gamma = 1.4017    * (1-0.00002*deltemp)
 *    c     = 3.4723e4  * (1+0.00166*deltemp)  cm/s
 *    deltemp = temperature - 26.85 deg C
 *
 * Formulae:
 *
 *    Zcav  = Zchr * coth(Wavn * Lcav)
 *    Zchr  = Ro*(1+0.369/Rv) - i*Ro*(0.369+(1.1149+0.303/Rv)/Rv)/Rv
 *    Wavn  = (w/c)*(1.045+(1.080+0.750/Rv)/Rv)/Rv + i*(w/c)*(1+1.045/Rv)
 *    Ro    = rho*c/(pi*a^2)
 *    Rv    = a*sqrt(rho*w/eta)
 *    w     = 2*pi*f
 *
 * Where:
 *    a = radius of cavity
 *    coth(x+i*y) = (1+i*tanh(x)*tan(y))/(tanh(x)+i*tan(y))
 *
 * References:
 *
 *    Keefe (1984) JASA 75(1), p 58
 *    Keefe (1992) JASA 91(1), p 470
 *    Gradstien & Ryzhik (1965), p 24
 *
 */
static void
cav0(double f, double *zc, double *wn)
{
    double  Rv, Ro, c, rho, eta, w, dt, rc;

    rc = diacav / 2;        /* cavity radius         */
    dt = tmpcav - 26.85;    /* temperature deviation */
    w = 2 * M_PI * f;
    c = 3.4723e4 * (1 + 0.00166 * dt);
    rho = 1.1769e-3 * (1 - 0.00335 * dt);
    eta = 1.846e-4 * (1 + 0.0025 * dt);
    Ro = rho * c / (M_PI * rc * rc);
    Rv = rc * sqrt(rho * w / eta);
    // characteristic impedance
    zc[0] = Ro;
    zc[1] = 0;
    // propagration function
    wn[0] = (w / c) * ((1.045 + (1.080 + 0.750 / Rv) / Rv) / Rv);
    wn[1] = (w / c) * (1 + 1.045 / Rv);
}

static void
cav1(double f, double *zc, double *wn)
{
    double  c, rho, rcv;
    static double pa[2] = {0.003, 0};	// err = 0.350 0.516

    c = 3.44e4;
    rho = 1.17e-3;
    rcv = diacav / 2;		/* radius */
    zc[0] = rho * c / (M_PI * rcv * rcv);
    zc[1] = 0;
    wn[0] = pa[0];
    wn[1] = 2 * M_PI * f / c;
}

static void
z_chr(float *z)
{
    double  Ro, c, rho, rc, dt;

    rc = diacav / 2;        /* cavity radius         */
    dt = tmpcav - 26.85;    /* temperature deviation */
    c = 3.4723e4 * (1 + 0.00166 * dt);
    rho = 1.1769e-3 * (1 - 0.00335 * dt);
    Ro = rho * c / (M_PI * rc * rc);
    // characteristic impedance
    z[0] = (float) Ro;
    z[1] = 0;
}

static void
z_cav(double f, double lc, double rc, float *z)
{
    double zc[2], wn[2];
    double e, c, s, nr, ni, dr, di, dm, ctr, cti;

    if (f == 0) {
	z_chr(z);
	return;
    }
    if (z_cav_type.at == 0) {
        cav0(f, zc, wn);
    } else {
        cav1(f, zc, wn);
    }
    e = exp(-2 * wn[0] * lc) * rc;
    c = cos(-2 * wn[1] * lc) * e;
    s = sin(-2 * wn[1] * lc) * e;
    nr = 1 + c;
    ni = s;
    dr = 1 - c;
    di = -s;
    dm = dr * dr + di * di;
    ctr = (nr * dr + ni * di) / dm;
    cti = (ni * dr - nr * di) / dm;
    // cavity impedance
    z[0] = (float) (zc[0] * ctr - zc[1] * cti);
    z[1] = (float) (zc[0] * cti + zc[1] * ctr);
}

/***********************************************************************/

static double 
mean_real(double *z, int n)
{
    int i, ir;
    double sm;

    sm = 0;
    for (i = 0; i < n; i++) {
        ir = i * 2;
	sm += z[ir];
    }
    return (sm / n);
}

static void
c_ones(double *z, int n)
{
    int i, ir, ii;

    for (i = 0; i < n; i++) {
        ir = i * 2;
        ii = ir + 1;
        z[ir] = 1;
        z[ii] = 0;
    }
}

static void
fd_window(double *z, int n)
{
    double a, dp, p, w;
    int i, ir, ii;

    a = 0.16;           /* Blackman window */
    dp = M_PI / n;
    for (i = 0; i < n; i++) {
        ir = i * 2;
        ii = ir + 1;
        p = i * dp;
        w = (1 - a + cos(p) + a * cos(2 * p)) / 2;
        z[ir] *= w;
        z[ii] *= w;
    }
}

static void 
reflect(double *R, float *zl, double z0, int n)
{
    double rnr, rni, rdr, rdi,  rdm;
    int i, ir, ii;

    for (i = 0; i < n; i++) {
        ir = i * 2;
        ii = ir + 1;
        rnr = zl[ir] - z0;
        rni = zl[ii];
        rdr = zl[ir] + z0;
        rdi = zl[ii];
        rdm = rdr * rdr + rdi * rdi;
        R[ir] = (rnr * rdr + rni * rdi) / rdm;
        R[ii] = (rni * rdr - rnr * rdi) / rdm;
    }
}

static double
z_surge(float *zl, int na)
{
    double z0, w0, wr, *R;
    float zc[2];
    int k, nk = probe.surge;

    z_chr(zc);
    z0 = zc[0];
    R = (double *) calloc(na * 2, sizeof(double));
    c_ones(R, na);
    fd_window(R, na);
    w0 = mean_real(R, na);
    for (k = 0; k < nk; k++) {
        reflect(R, zl, z0, na);
        fd_window(R, na);
        wr = mean_real(R, na);
        z0 *= (1 + wr / w0);
    }
    free(R);

    return (z0);
}

/***********************************************************************/

static void
select_chan(int c)
{
    int i;

    ps = pzs + c * os;
    zs = ps + MAX_NF * 2;
    px = zs + MAX_NF * 2;

    pl = pzl + c * ol;
    zl = pl + MAX_NF * 2;
    pf = zl + MAX_NF * 2;
    z0 = pf + MAX_NF * 2;
    pr = z0 + MAX_NF * 2;
    cd = pr + MAX_NF * 2;

    pc[0] = pzl + c * MAX_NF * 2 * nthc;
    for (i = 1; i < nthc; i++)
        pc[i] = pc[i - 1] + MAX_NF * 2;
}


static int
thev_init()
{
    int m;

    qq1 = (float *) calloc(MAX_NF * 2 * 2, sizeof(float));
    if (qq1 == NULL) {
        decide(0, 1, "Can't allocate thev memory (qq1)");
        return (0);
    }

    os = MAX_NF * 2 * 3;	//allow for ps, zs, px
    pzs = (float *) calloc(os * 2, sizeof(float));
    if (pzs == NULL) {
        decide(0, 1, "Can't allocate thev memory (pzs)");
	free(qq1);
        return (0);
    }

    m = (nthc < 6) ? 6 : nthc;	//allow for pl, zl, pf, z0, pr, cd
    ol = MAX_NF * 2 * m;;
    pzl = (float *) calloc(ol * 2, sizeof(float));
    if (pzl == NULL) {
        decide(0, 1, "Can't allocate thev memory (pzl)");
	free(qq1);
	free(pzs);
        return (0);
    }

    select_chan(0);

    return (buflen / FFRAC);
}

static void
thev_free()
{

    free_null(pzs);
    free_null(pzl);
    free_null(qq1);
    nthf = 0;
}

/* thev_acc:  Z1 * P0 - P1 * Z0 = Z1 * P1 */

static void
thev_acc(float *p, float *z, double *a, double *q)
{
    double pir, pii, zir, zii, pms, zms;
    
    pir = p[0];
    pii = p[1];
    zir = z[0];
    zii = z[1];
    pms = pir * pir + pii * pii;
    zms = zir * zir + zii * zii;
    a[0] += pms;
    a[1] += zms;
    a[2] += pir * zir + pii * zii;
    a[3] += pii * zir - pir * zii;
    q[0] += pir * zms;
    q[1] += pii * zms;
    q[2] -= zir * pms;
    q[3] -= zii * pms;
}

static void
thev_zsps2(int n, double df, float *lc)
{
    int i, ir, ii;
    double z1p2r, z1p2i, z2p1r, z2p1i, z1z2r, z1z2i, p1p2r, p1p2i;
    double z1p2z2p1r, z1p2z2p1i, z1z2p1p2r, z1z2p1p2i, d, f;
    float z1[2], z2[2];
    static double eps = 1e-40;

    for (i = 1; i < n; i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
	f = i * df;
        z_cav(f, lc[0], rflcav, z1);
        z_cav(f, lc[1], rflcav, z2);
        z1p2r = z1[0] * pc[1][ir] - z1[1] * pc[1][ii];
        z1p2i = z1[1] * pc[1][ir] + z1[0] * pc[1][ii];
        z2p1r = z2[0] * pc[0][ir] - z2[1] * pc[0][ii];
        z2p1i = z2[1] * pc[0][ir] + z2[0] * pc[0][ii];
        z1p2z2p1r = z1p2r - z2p1r;
        z1p2z2p1i = z1p2i - z2p1i;
        d = z1p2z2p1r * z1p2z2p1r + z1p2z2p1i * z1p2z2p1i;
        if (fabs(d) < eps)
            d = eps;

        z1z2r = z1[0] * z2[0] - z1[1] * z2[1];
        z1z2i = z1[1] * z2[0] + z1[0] * z2[1];
        p1p2r = pc[0][ir] - pc[1][ir];
        p1p2i = pc[0][ii] - pc[1][ii];
        z1z2p1p2r = z1z2r * p1p2r - z1z2i * p1p2i;
        z1z2p1p2i = z1z2i * p1p2r + z1z2r * p1p2i;
        zs[ir] = (float) ((z1z2p1p2r * z1p2z2p1r + z1z2p1p2i * z1p2z2p1i) / d);
        zs[ii] = (float) ((z1z2p1p2i * z1p2z2p1r - z1z2p1p2r * z1p2z2p1i) / d);

        z1z2r = z1[0] - z2[0];
        z1z2i = z1[1] - z2[1];
        p1p2r = pc[0][ir] * pc[1][ir] - pc[0][ii] * pc[1][ii];
        p1p2i = pc[0][ii] * pc[1][ir] + pc[0][ir] * pc[1][ii];
        z1z2p1p2r = z1z2r * p1p2r - z1z2i * p1p2i;
        z1z2p1p2i = z1z2i * p1p2r + z1z2r * p1p2i;
        ps[ir] = (float) ((z1z2p1p2r * z1p2z2p1r + z1z2p1p2i * z1p2z2p1i) / d);
        ps[ii] = (float) ((z1z2p1p2i * z1p2z2p1r - z1z2p1p2r * z1p2z2p1i) / d);
    }
}

static void
thev_zsps(int n, double df, float *lc)
{
    int i, ir, ii, j, k;
    double d, f, a[4], q[4];
    float z[2], p[2];
    static float eps = 1e-20F;
    
    if (extend == 1) {          // T+D
        diacav = lc[nthc];
        tmpcav = lc[nthc+1];
    } else if (extend == 2) {   // R
        rflcav = lc[nthc];
    }
    ps[0] = px[0] = eps;
    z_chr(zs);
    if (nthc == 2) {
        thev_zsps2(n, df, lc);
	return;
    }
    for (i = 1; i < n; i++) {
	f = i * df;
	/* index real & imag */
        ir = 2 * i;
        ii = 2 * i + 1;
        /* zero summation variables */
	for (k = 0; k < 4; k++) {
            a[k] = q[k] = 0;
	}
	/* loop over cavity */
        for (j = 0; j < nthc; j++) {
            p[0] = pc[j][ir];
            p[1] = pc[j][ii];
            z_cav(f, lc[j], rflcav, z);
            thev_acc(p, z, a, q);
        }
	/* calculate determinant */
        d = a[1] * a[0] - (a[2] * a[2] + a[3] * a[3]);
        if (fabs(d) < eps) d = eps;
	/* solve for ps & zs */
        ps[ir] = (float) ((a[0] * q[0] + a[2] * q[2] - a[3] * q[3]) / d);
        ps[ii] = (float) ((a[0] * q[1] + a[2] * q[3] + a[3] * q[2]) / d);
        zs[ir] = (float) ((a[1] * q[2] + a[2] * q[0] + a[3] * q[1]) / d);
        zs[ii] = (float) ((a[1] * q[3] + a[2] * q[1] - a[3] * q[0]) / d);
    }
}

static double
thev_erf(int i1, int i2, int n, double df, float *lc, int cn)
{
    double  f, s1, s2, zzms;
    double zzr, zzi, pzr, pzi, ppr, ppi, pdr, pdi, par, pai;
    float   psr, psi, zsr, zsi, pcr, pci, zcr, zci, zc[2];
    int     i, j, ii, ir, er, j1, j2, xt;

    if (cn) {		// single cavity
	j1 = cn - 1;
	j2 = cn;
	xt = 0;
    } else {		// all cavities
	j1 = 0;
	j2 = nthc;
	xt = thv_xtk.at;
    }
    s1 = s2 = 0;
    for (i = 1; i < n; i++) {    
        er = (i >= i1 && i < i2);
        ir = 2 * i;
        ii = 2 * i + 1;
        psr = ps[ir];
        psi = ps[ii];
        zsr = zs[ir];
        zsi = zs[ii];
	par = 0;
	pai = 0;
	f = i * df;
        for (j = 0; j < nthc; j++) {
            pcr = pc[j][ir];
            pci = pc[j][ii];
            z_cav(f, lc[j], rflcav, zc);
            zcr = zc[0];
            zci = zc[1];
            zzr = zsr + zcr;
            zzi = zsi + zci;
            pzr = psr * zcr - psi * zci;
            pzi = psi * zcr + psr * zci;
            zzms = zzr * zzr + zzi * zzi;
            ppr = (pzr * zzr + pzi * zzi) / zzms;
            ppi = (pzi * zzr - pzr * zzi) / zzms;
            pdr = pcr - ppr;
            pdi = pci - ppi;
            par = par + pdr;
            pai = pai + pdi;
            if (er) {
		if (j >= j1 && j < j2) {
                    s1 += pdr * pdr + pdi * pdi;
		}
                s2 += pcr * pcr + pci * pci;
            }
        }
        if (xt) {		// cross-talk
       	    px[ir] = (float) (par / nthc);
       	    px[ii] = (float) (pai / nthc);
            if (er) {
                s1 -= (par * par + pai * pai) / nthc;
	    }
        }
    }
    return ((s2 > 0) ? 1e4 * s1 / s2 : 0);
}

static double
thev_err(float *lc)
{
    int i1, i2, nf;
    double df;

    df = (double) rate / buflen;
    i1 = limit(1, (int) (probe.f1_erf / df + 0.5), nthf);
    i2 = limit(1, (int) (probe.f2_erf / df + 0.5), nthf) + 1;
    nf = itr_flg ? i2 : nthf + 1;
    thev_zsps(nf, df, lc);
    if (rflcav>1.000001) return (1e4);
    return (thev_erf(i1, i2, nf, df, lc, 0));
}

static void
thev_err_cav()
{
    double df;
    float *lc;
    int i1, i2, nf, j, k;

    df = (double) rate / buflen;
    i1 = limit(1, (int) (probe.f1_erf / df + 0.5), nthf);
    i2 = limit(1, (int) (probe.f2_erf / df + 0.5), nthf) + 1;
    nf = nthf + 1;
    for (k = 0; k < numsrc; k++) {	// source
	select_chan(k);
	lc = lcv[k];
        thev_zsps(nf, df, lc);
	for (j = 0; j < nthc; j++) {	// cavity
	    ecv[k][j] = (float) thev_erf(i1, i2, nf, df, lc, j + 1);
	}
    }
    necv = nthc;
}

static void
thev_rep(float *lc)
{
    int i, j, k, m, fh, fw;
    double e;

    if (extend == 1) {
        e = lc[nthc+2];
    } else if (extend == 2) {
        e = lc[nthc+1];
    } else {
        e = lc[nthc];
    }
    j = w_info.xtop + 4;
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / 8 - 1;
    text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
    fw = txtpar.font_width;
    fh = txtpar.font_height;
    k = w_info.ytop + fh;
    gprintf(j + 23 * fw, k, "T=%.1fC  D=%.2fcm  R=%.2f  ", tmpcav, diacav, rflcav);
    text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
    m = (w_info.ybot - w_info.ytop - (3 + nthc) * fh) / 3;
    k += fh + ((m > 0) ? m : 0);
    for (i = 0; i < nthc; i++) {
        gprintf(j, k, "#%d: lc=%5.3f cm                     ", i + 1, lc[i]);
	k += fh;
    }
    k += (m > 0) ? m : 0;
    gprintf(j, k, "        error = %6.3f              ", e);
    k += fh;
}

static int
thev_esc()
{
    int c = 0;

    if (check_event()) {
        c = getevent();
        if (c == 'd')
            dis_cal_imp = !dis_cal_imp;
        if (c == 27)
            esc_flg = 1;
    }
    return (c == 27);
}

static void
divide_by_stim(float *pp, int n, int c, int s)
{
    int i, ii, ir;
    complex p0, p1, pr;
    double ppv;

    ppv = Sen.DA[c] / (Sen.AD * Sen.MP * s);  // removed 2 in 3.32
    for (i = 0; i < n; i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
        p0.x = st[ir];
        p0.y = st[ii];
        p1.x = pp[ir];
        p1.y = pp[ii];
        pr = cdiv(p1, p0);
        pp[ir] = (float) (pr.x * ppv);
        pp[ii] = (float) (pr.y * ppv);
    }
}

static void
atten_scale(float *pp, int n, int c, double attdif)
{
    float sc;
    int i;

    if (attdif != 0) {
        sc = (float) pow(10, attdif / 20);
        for (i = 0; i < (2 * n); i++) {
            pp[i] *= sc;
        }
    }
}

static void
smooth(float *pp, int n, int m)
{
    int i, ii, ir, i1, i2, j, jr, ji, mm;
    float t[MAX_SM * 2], si, sr;

    if (m < 2) {
	return;
    } else if (m > MAX_SM) {
	m = MAX_SM;
    }
    mm = m - 1;
    for (j = 0; j < m * 2; j++) {
	t[j] = pp[j];
    }
    i1 = m / 2;
    i2 = n - m + i1;
    for (i = i1; i < i2; i++) {
	sr = 0;
	si = 0;
	for (j = 0; j < m; j++) {
	    jr = 2 * j;
	    ji = 2 * j + 1;
	    sr = sr + t[jr];
	    si = si + t[ji];
	}
        ir = 2 * i;
        ii = 2 * i + 1;
        pp[ir] = sr / m;
        pp[ii] = si / m;
	for (j = 0; j < mm; j++) {
	    jr = 2 * j;
	    ji = 2 * j + 1;
	    t[jr] = t[jr + 2];
	    t[ji] = t[ji + 2];
	}
        ir = 2 * (mm + i - i1);
        ii = 2 * (mm + i - i1) + 1;
        jr = 2 * mm;
        ji = 2 * mm + 1;
        t[jr] = pp[ir];
        t[ji] = pp[ii];
    }
}

static void
info_msg(char *s, int n)
{
    text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / 8 - 1;
    gprintf(w_info.xtop + 4, w_info.ybot - 4 - txtpar.font_height * n, s);
    check_event();
}

static void
stat_msg(char *s)
{
    text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / 8 - 1;
    gprintf(w_info.xtop + 4, w_info.ybot - 4, s);
    check_event();
}

static void
save_stim_fft(int nset)
{
    int i;
    int32_t *stm, mx = 0;
    
    st = qq1 + MAX_NF * 2;
    stm = (int32_t *) st;
    for (i = 0; i < buflen; i++) {
        stm[i] = stmbuf[i];
        if (mx < stm[i])
            mx = stm[i];
        if (mx < -stm[i])
            mx = -stm[i];
    }
    if (mx == 0)
        stm[0] = 1;      /* default to unit impulse if stimulus is zero */
    nftf = fft_sig(stm, nset);
    retrieve_fft(st, nthf);
}

static void
dis_cal_tok(int32_t *buf)
{
    double  tms;
    extern WIND w_sig;

    dis_units(Sen.AD, Sen.MP);
    w_dis = &w_sig;
    w_sig.ybot -= txtpar.font_height;
    disply(cal_sets * swp1set * 2, buf, buflen, display_reset);
    w_sig.ybot += txtpar.font_height;
    tms = 1000 * (double) buflen / rate;
    w_dis->wfgc = scrn_c[C_WAVE];
    show_xlabel(0.0, tms, 0, "ms");
    fft_stim((int32_t *) buf, cal_sets, 1);
    show_fft(fft_xrange, fft_yrange, 0, dfgc, display_reset);
    display_reset = 0;
}

static int
dis_cal()
{
    int i, j, k, m, fh;

    draw_w(&w_info);
    j = w_info.xtop + 4;
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / 8 - 1;
    text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
    fh = txtpar.font_height;
    k = w_info.ytop + fh;
    gprintf(j, k, "Probe Calibration: D=%.2fcm T=%.1fC",
	diacav, tmpcav);
    m = (w_info.ybot - w_info.ytop - (3 + nthc) * fh) / 3;
    k += fh + ((m > 0) ? m : 0);
    text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
    for (i = 0; i < nthc; i++) {
	if (numsrc > 1) {
	    gprintf(j, k, "#%d: fzp= %4.0f %4.0f Hz, L=%.2f %.2f cm ", i + 1,
		(fz1[0][i]+fz1[1][i])/2, (fpk[0][i]+fpk[1][i])/2,
		lcv[0][i], lcv[1][i]);
	} else {
	    gprintf(j, k, "#%d: fzp= %4.0 %4.0 Hz, L=%.2f cm ", i + 1,
		fz1[0][i], fpk[0][i], lcv[0][i]);
	}
	k += fh;
    }
    k += (m > 0) ? m : 0;
    if (numsrc > 1) {
	gprintf(j, k, "    errors = %6.3f %6.3f      R = %4.2f %4.2f    ", 
            err[0], err[1], Rcv[0], Rcv[1]);
    } else {
	gprintf(j, k, "    error = %6.3f      R = %4.2f  ", err[0], Rcv[0]);
    }
    return (k);
}

static int
dis_cal_chan(double et, int c)
{
    int i, j, k, m, fh;

    err[c] = thev_err(lcv[c]);
    draw_w(&w_info);
    j = w_info.xtop + 4;
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / 8 - 1;
    text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
    fh = txtpar.font_height;
    k = w_info.ytop + fh;
    gprintf(j, k, "Channel %s Calibration:  T=%.1fC  D=%.2fcm  R=%.2f  ", 
	c ? "B" : "A", tmpcav, diacav, rflcav);
    m = (w_info.ybot - w_info.ytop - (3 + nthc) * fh) / 3;
    k += fh + ((m > 0) ? m : 0);
    text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
    for (i = 0; i < nthc; i++) {
        gprintf(j, k, "#%d: fzp= %4.0 %4.0 Hz, L=%.3f cm ", i + 1,
	    fz1[c][i], fpk[c][i], lcv[c][i]);
	k += fh;
    }
    k += (m > 0) ? m : 0;
    if (!et) {
	gprintf(j, k, "        error = %6.3f              ", err[c]);
    } else {
        gprintf(j, k, "        error = %6.3f (%.1f sec)   ", err[c], et);
    }
    return (k);
}

void
dis_cal_cav(int icav)
{
    int i, j, k, fh;

    if (icav < 0 || icav > necv)
	return;
    draw_w(&w_info);
    j = w_info.xtop + 4;
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / 8 - 1;
    text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
    fh = txtpar.font_height;
    k = w_info.ytop + fh;
    gprintf(j, k, "Probe Calibration: D=%.2fcm T=%.1fC", diacav, tmpcav);
    k += fh * 2;
    text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
    i = icav;
    gprintf(j, k, "Cavity #%d:", i + 1);
    k += fh;
    gprintf(j, k, "     fz1=%6.0f %6.0f Hz", fz1[0][i], fz1[1][i]);
    k += fh;
    gprintf(j, k, "     fpk=%6.0f %6.0f Hz", fpk[0][i], fpk[1][i]);
    k += fh;
    gprintf(j, k, "       L=%6.2f %6.2f cm", lcv[0][i], lcv[1][i]);
    k += fh;
    gprintf(j, k, " errors = %6.3f %6.3f  ", ecv[0][i], ecv[1][i]);
}

static void
write_thev_src_chan(int c, char *sfn)
{
    complex z, zm, p, pm;
    double df = 0;
    int i, ir, ii, n;
    FILE *ofp = NULL;
    static char *fn[2] = {"ths1.txt", "ths2.txt"};

    select_chan(c);
    df = (double) rate / buflen;
    ofp = fopen(fn[c], "w");
    fprintf(ofp, "; %s - %s\n", fn[c], sfn);
    fprintf(ofp, ";   f     |Ps|     Ps<    |Zs|     Zs<    |Px|     Px<\n");
    n = nthf;
    for (i = 1; i < n; i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
        p.x = ps[ir];
        p.y = ps[ii];
        z.x = zs[ir];
        z.y = zs[ii];
        pm = dbph(p);
        zm = dbph(z);
        fprintf(ofp, "%6.0f %7.1f %7.1f %7.1f %7.1f", 
            i * df, pm.x, pm.y, zm.x, zm.y);
        p.x = px[ir];
        p.y = px[ii];
        pm = dbph(p);
        fprintf(ofp, " %7.1f %7.1f\n", pm.x, pm.y);
    }
    fclose(ofp);
}

static void
write_thev_ld_chan(int c, char *lfn)
{
    complex z, zm, p, pm;
    double df = 0;
    int i, ir, ii, n;
    FILE *ofp = NULL;
    static char *fn[2] = {"thl1.txt", "thl2.txt"};

    select_chan(c);
    df = (double) rate / buflen;
    ofp = fopen(fn[c], "w");
    fprintf(ofp, "; %s - %s\n", fn[c], lfn);
    fprintf(ofp, ";   f     |Pl|     Pl<    |Zl|     Zl");
    fprintf(ofp,      "     |Pf|     Pf<    |Z0|     Z0<\n");
    n = nthf;
    for (i = 1; i < n; i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
        p.x = pl[ir];
        p.y = pl[ii];
        z.x = zl[ir];
        z.y = zl[ii];
        pm = dbph(p);
        zm = dbph(z);
        fprintf(ofp, "%6.0f %7.1f %7.1f %7.1f %7.1f ", 
            i * df, pm.x, pm.y, zm.x, zm.y);
        p.x = pf[ir];
        p.y = pf[ii];
        z.x = z0[ir];
        z.y = z0[ii];
        pm = dbph(p);
        zm = dbph(z);
        fprintf(ofp, "%7.1f %7.1f %7.1f %7.1f\n", 
            pm.x, pm.y, zm.x, zm.y);
    }
    fclose(ofp);
}

static void
write_thev_zcav(int type)
{
    char fn[MAXNAME];
    complex z, zm;
    double f, df = 0;
    float lc, zc[2];
    int i, j;
    FILE *ofp = NULL;

    sprintf(fn, "zcav%d.txt", type);
    ofp = fopen(fn, "w");
    fprintf(ofp, "; %s\n;\n", fn);
    fprintf(ofp, ";   length (cm) = ");
    for (j = 0; j < numcav; j++) {
        lc = (lcv[0][j] + lcv[1][j])/2;
        fprintf(ofp, " %6.2f", lc);
    }
    fprintf(ofp, "\n;\n");
    fprintf(ofp, ";   f     ");
    for (j = 0; j < numcav; j++) {
	fprintf(ofp,       "|Z%d|     Z%d<    ", j + 1, j + 1);
    }
    fprintf(ofp, "\n");
    df = (double) rate / buflen;
    for (i = 1; i < nthf; i++) {
	f = i * df;
        fprintf(ofp, "%6.0f", f);
	for (j = 0; j < numcav; j++) {
            lc = (lcv[0][j] + lcv[1][j])/2;
	    if (type < 2) {
		z_cav(f, lc, rflcav, zc);
	    } else {
		zc[0]=zl[i*2+0];
		zc[1]=zl[i*2+1];
	    }
	    z.x = zc[0];
            z.y = zc[1];
            zm = dbph(z);
            fprintf(ofp, " %7.1f %7.1f", zm.x, zm.y);
	}
        fprintf(ofp, "\n");
    }
    fclose(ofp);
}

static void
show_db(float *z, int n, WIND *wn, int levref, int reset, int db_range, int c)
{
    int     i, n1, nd;
    double   dt, lpsval;
    short   *ibuf;
    WIND    *wp;
    static double f1 = 100; // lowest frequency displayed;

    dfgc = c;
    display_reset = reset;
    dt = (nftf > 1) ? (2.0 * nftf) / rate : 1;
    lpsflg = levref;
    lpsval = (levref == 1) ? 10 * log10(dt) : 0;
    ibuf = (short *) z;
    ibuf[0] = -800;
    for (i = 0; i < n; i++) {
	ibuf[i] = nint(z[i] + lpsval);
    }
    n1 = nint(nftf * f1 / rate);
    for (i = 1; i < n1; i++) {
	ibuf[i] = -800;
    }
    wp = w_dis;
    w_dis = wn;
    if (display_reset > 0) {
	gr_rectf(w_dis->xtop + 1, w_dis->ytop + 1, w_dis->xbot - 1,
	    w_dis->ybot - 1, w_dis->wbgc);
	gr_recto(w_dis->xtop, w_dis->ytop, w_dis->xbot,
	    w_dis->ybot, menucolor.boxc);
    }
    fft_flag = 1;
    nd = nint(fft_xrange * dt * 1000);
    w_dis->ybot -= txtpar.font_height;
    display(ibuf, nd, display_reset, fft_yrange, fft_xrange);
    w_dis->ybot += txtpar.font_height;
    w_dis->wfgc = scrn_c[C_WAVE];
    show_xlabel(0.0, (double) fft_xrange, 0, "kHz");
    fft_flag = 0;
    w_dis = wp;

    display_reset = 0;
}

static void
show_pz(int c, int pz, int rs)
{
    complex p0, pm;
    int i, ir, ii, n;
    float *s, *p;
    WIND    wn;

    select_chan(c % 2);
    if (c < 2) {
        p = pz ? zs : ps;
        dfgc = (c % 2) ? scrn_c[C_WAVB] : scrn_c[C_WAVA];
    } else {
        p = pz ? zl : pl;
        dfgc = (c % 2) ? scrn_c[C_CHNB] : scrn_c[C_CHNA];
    }
    n = nthf;
    s = (float *) qq1 + MAX_NF;
    s[0] = -800;
    for (i = 1; i < n; i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
        p0.x = p[ir];
        p0.y = p[ii];
        pm = dbph(p0);
        s[i] = (float) pm.x;
    }

    wn = w_spec;
    wn.xtop = pz ? 0: w_sig.xbot / 2 + 1;
    wn.xbot = pz ? w_sig.xbot / 2 : w_sig.xbot;
    if (rs) {
        wn.ytop = w_sig.ybot + 1;
        wn.ybot = w_sig.ybot + txtpar.menu_height;
        wn.wfgc = scrn_c[C_WAVE];
        gprintf(draw_w(&wn), wn.ybot - 2, pz ? "Impedance:" : "Pressure:");
    }
    wn.ytop = w_sig.ybot + txtpar.menu_height + 1;
    wn.ybot = ypix - 1;
    wn.wfgc = dfgc;
    show_db(s, n, &wn, 2, rs, db_range, dfgc);
}

static void
show_pf(int c, int pp, int st, int rs)
{
    char ext[5];
    complex p0, pm;
    int i, ir, ii, n;
    float *s, *p;
    WIND    wn, ws;

    select_chan(c);
    if (pp == 1) {	    // FPL
        p = pf;
        dfgc = c ? scrn_c[C_WAVB] : scrn_c[C_WAVA];
	sprintf(ext,"+FPL");
    } else if (pp == 2) {   // SIL 
        p = pl;
        dfgc = c ? scrn_c[C_WAVB] : scrn_c[C_WAVA];
	sprintf(ext,"+SIL");
    } else {		    // SPL
        p = pl;
        dfgc = c ? scrn_c[C_CHNB] : scrn_c[C_CHNA];
	ext[0] = 0;
    }
    n = nthf;
    s = (float *) qq1 + MAX_NF;
    s[0] = -800;
    for (i = 1; i < n; i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
        p0.x = p[ir];
        p0.y = p[ii];
	if (pp == 2) {
	    p0.x *= cd[i];
	    p0.y *= cd[i];
	}
        pm = dbph(p0);
        s[i] = (float) pm.x;
    }

    ws = st ? w_stim_fft : w_spec;
    wn.wbgc = ws.wbgc;
    wn.xtop = ws.xtop;
    wn.xbot = ws.xbot;
    wn.ytop = ws.ytop;
    wn.ybot = ws.ytop + txtpar.menu_height;
    wn.wfgc = scrn_c[C_WAVE];
    gprintf(draw_w(&wn), wn.ybot - 2, "SPL%s:",ext);
    wn.ytop = ws.ytop + txtpar.menu_height;
    wn.ybot = st ? ws.ybot : ypix - 1;
    wn.wfgc = dfgc;
    show_db(s, n, &wn, 2, rs, db_range, dfgc);
}

static void
show_thev_src_chan(int c)
{
    show_pz(c, 0, 1);
    show_pz(c, 1, 1);
    check_event();
}

static void
show_thev_src()
{
    show_pz(0, 0, 1);
    show_pz(1, 0, 0);
    show_pz(0, 1, 1);
    show_pz(1, 1, 0);
    check_event();
}

static void
show_thev_ld(int sil)
{
    show_pz(0, 0, 1);
    show_pz(1, 0, 0);
    show_pz(2, 0, 0);
    show_pz(3, 0, 0);
    show_pz(0, 1, 1);
    show_pz(1, 1, 0);
    show_pz(2, 1, 0);
    show_pz(3, 1, 0);
    show_pf(0, 0, 0, 1);
    show_pf(1, 0, 0, 0);
    if (sil) {			// SIL
        show_pf(0, 2, 0, 0);
	show_pf(1, 2, 0, 0);
    } else {			// FPL
        show_pf(0, 1, 0, 0);
	show_pf(1, 1, 0, 0);
    }
    check_event();
}

static void
show_thev_adj(int sil, int st)
{
    show_pf(0, 0, st, 1);
    show_pf(1, 0, st, 0);
    if (sil) {			// SIL
        show_pf(0, 2, st, 0);
	show_pf(1, 2, st, 0);
    } else {			// FPL
        show_pf(0, 1, st, 0);
	show_pf(1, 1, st, 0);
    }
    check_event();
}

double
cavity_length(float *pc, int nf, double vs)
{
    double dl, ecl, l1, l2, d;
    float *p;
    int i, ii, ir, i1, i2, m, n;

    n = nf * 2 + 2;
    p = (float *) calloc(n, sizeof(float));
    for (i = 0; i < nf; i++) {
	ir = 2 * i;
	ii = 2 * i + 1;
	p[ir] = pc[ir] * pc[ir] + pc[ii] * pc[ii];
	p[ii] = 0;
    }
    p[0] = p[n - 2] = 1e-9F;
    p[1] = p[n - 1] = 0;
    fsst(p, nf * 2);		    // autocor. of resp/stim imp. resp.
    dl = (vs / rate) / 2;	    // half sound travel per sample
    l1 = diacav;
    l2 = probe.maxlen;
    i1 = nint(l1 / dl);
    i2 = nint(l2 / dl);
    m = (i1 < 1) ? 1 : i1;
    while ((m < i2) && (p[m + 1] < p[m])) {
        m++;
    }
    for (i = m + 1; i <= i2; i++) {
	if (p[m] < p[i]) {
	    m = i;
	}
    }
    d = (p[m-1]-p[m+1])/(p[m-1]-2*p[m]+p[m+1])/2;
    d = limit(-1, d, 1);
    ecl = (m + d) * dl;
    free(p);

    return (ecl);
}

int
thev_source(char *fn)
{
    char  sfn[MAXNAME], tfn[MAXNAME];
    double  et, vs;
    int32_t    stime;
    int32_t    *buf;
    int     i, j, nv, ev = 0;
    short   sets;
    WIND    ws, wf;

    strcpy(sfn,fn);
    newext(sfn, "PRB");
    if (_access(sfn,0)==0) {
	grab_probe();
	cal_read(fn, 0);	// read file to get buflen & numcav
        cal_mesg(fn);
        nthc = numcav;
	if (nthc < MINNCAV) {
	    sprintf(line, "Too few cavities: ncav = %d < %d", nthc, MINNCAV);
	    stat_msg(line);
	    return (0);
	}
	if (nthc > MAXNCAV) {
	    sprintf(line, "Too many cavities: ncav = %d > %d", nthc, MAXNCAV);
	    stat_msg(line);
	    return (0);
	}
        nthf = thev_init();
        if (nthf <= 0) {
	    stat_msg("thev freq. range too small");
            thev_free();
            return (0);
        }
        diacav = probe.diacav;
        tmpcav = probe.temp;
        rflcav = 1;
        vs = 3.4723e4 * (1 + 0.00166 * (tmpcav - 26.85));
	ws = w_sig;	// save w_sig
	wf = w_spec;	// save w_spec
	draw_w(&w_sig);
	w_sig.ybot = w_sig.ytop +(w_sig.ybot - w_sig.ytop) / 2;
	w_spec.ybot = ypix - 1;
	draw_w(&w_spec);
        probe_flag = 1;
	db_range = 40;
	for (j = 0; j < numsrc; j++) {
	    sprintf(line, "Reading pressures for source %d...", j + 1);
	    stat_msg(line);
	    select_chan(j);
	    for (i = 0; i < nthc; i++) {
		text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
		Sen.DA[0] = Sen.DA[1] = 0;
		if (cal_read(fn, i))
		    cal_disp(fn, 0);
		if (Sen.DA[0] == 0) {	// fix old CAL files
		    Sen.DA[0] = Sen.DA[1] = Sen.AD;
		}
		dfgc = j ? scrn_c[C_CHNB] : scrn_c[C_CHNA];
	        buf = j ? accbuf_b : accbuf_a;
		dis_cal_tok(buf);
		check_event();		    // flush graphics
		if (i == 0)                 // after swp1set is read 
		    save_stim_fft(1);
		nftf = fft_sig(buf, cal_sets);
                retrieve_fft(pc[i], nthf);
                divide_by_stim(pc[i], nthf, j, swp1set * cal_sets);
                atten_scale(pc[i], nthf, j, cal_attn);
		if (numsmo) {
                    smooth(pc[i], nthf, numsmo);
		}
	        modulesqr();
		lcv[j][i] = (float) cavity_length(pc[i], nthf, vs);
		fz1[j][i] = (float) (vs / lcv[j][i] / 4);
		fpk[j][i] = fz1[j][i] * 2;
	    }
	    (void) dis_cal_chan(0, j);
	    show_thev_src_chan(j);
	    decide_offset(-100, 40);
	    if((nthc > 2) && ((thv_itr.at == 1) || 
		((thv_itr.at == 2) && decide(1, 1, "Iterate?")))) {
		stat_msg("Iterating lengths ...      ");
		stime = clock();
		nv = nthc;
		esc_flg = 0;
		itr_flg = 1;
		simpfit(lcv[j], nv, probe.niter, nv, thev_err, thev_rep, thev_esc);
		if (!esc_flg && thv_ext.at) {
		    extend = thv_ext.at;
                    if (extend == 1) {
                        lcv[j][nv + 0] = diacav;
        	        lcv[j][nv + 1] = tmpcav;
                        ev = 2;
                    } else if (extend == 2) {
                        lcv[j][nv + 0] = rflcav;
                        ev = 1;
                    }
		    nv += ev;
		    simpfit(lcv[j], nv, probe.niter, nv, thev_err, thev_rep, thev_esc);
		    nv -= ev;
		    extend = 0;
		}
		itr_flg = 0;
		et = (clock() - stime) / (double) CLOCKS_PER_SEC;
	        dis_cal_chan(et, j);
            }
            Rcv[j] = rflcav;
            if (write_txt) {
                write_thev_src_chan(j, sfn);
	    }
	}
        dis_cal();
        show_thev_src();
        strcpy(tfn,fn);
	newext(tfn, "THS");
        select_chan(0);
        sets = acc_sets;
        if (thev_src_sav(tfn, 
	    zs, ps, px, lcv[0], 
	    zs+os, ps+os, px+os, lcv[1], 
	    nthf, nthc, diacav, tmpcav, cal_attn, sets)) {
          decide(0, 2, "Error writing Thevenin source file", tfn);
	} else {
            sprintf(line, "Wrote Thevenin source file %s", tfn);
            stat_msg(line);
	}
        decide_offset(0, 0);
        probe_flag = 0;
        w_sig = ws;	// restore w_sig
        w_spec = wf;	// restore w_spec
        if (write_txt) {
	    write_thev_zcav(0);
	    write_thev_zcav(1);
	    write_thev_zcav(2);
	}
	thev_err_cav();
        thev_free();
    }

    return (1);
}

/*****************************************************************************/

/* check cal & ths file names for wildcards */

static int
chkfn(char *calfn, char *thsfn)
{
    int j, k;

    if (strchr(calfn, '*') || strchr(calfn, '?')) {
        j = draw_w(&w_info);
        k = w_info.ytop + txtpar.font_height * 3;
        text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
        gprintf(j, k, "Select CAL file and press enter.");
        k += txtpar.font_height;
        gprintf(j, k, "Or press escape to cancel.");
        strcpy(o_file_name, calfn);
        if (open_file(NULL) == 0) {
	    return (0);
        }
        strcpy(cali_file, o_file_name);
        j = draw_w(&w_info);
    } else {
        strcpy(cali_file, calfn);
    }
    if (strchr(thsfn, '*') || strchr(thsfn, '?')) {
        j = draw_w(&w_info);
        k = w_info.ytop + txtpar.font_height * 3;
        text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
        gprintf(j, k, "Select THS file and press enter.");
        k += txtpar.font_height;
        gprintf(j, k, "Or press escape to cancel.");
        strcpy(o_file_name, thsfn);
        if (open_file(NULL) == 0) {
	    return (0);
        }
        strcpy(ths_file, o_file_name);
        j = draw_w(&w_info);
    } else {
        strcpy(ths_file, thsfn);
    }
    return (1);
}

/***********************************************************************/

static void
ldimp(float *zl, float *zs, float *ps, float *pl, int na, double sg)
{
    double  plr, pli, pdr, pdi, pdm, prr, pri, zlr, zli;
    double  psr, psi, zsr, zsi;
    int     i, ii, ir;
    static float eps = 1e-9F;
 
    zl[0] = eps;
    zl[1] = 0;
    for (i = 1; i < na; i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
        psr = ps[ir] * sg;
        psi = ps[ii] * sg;
        zsr = zs[ir];
        zsi = zs[ii];
        plr = pl[ir];
        pli = pl[ii];
        pdr = psr - plr;
        pdi = psi - pli;
        pdm = pdr * pdr + pdi * pdi;
        prr = (plr * pdr + pli * pdi) / pdm;
        pri = (pli * pdr - plr * pdi) / pdm;
        zlr = zsr * prr - zsi * pri;
        zli = zsi * prr + zsr * pri;
        zl[ir] = (float) zlr;
        zl[ii] = (float) zli;
    }
}

static double
surge_gain(float *zl, float *zs, float *ps, float *pl, int na)
{
    double sg = 1;
    float zc[2];
    int i, nk = probe.surge;

    z_chr(zc);
    for (i = 0; i < nk; i++) {
        ldimp(zl, zs, ps, pl, na, sg);
	sg *= z_surge(zl, na) / zc[0];
    }

    return (sg);
}

static void
thev_cmp_zl()
{
    double   plr, pli, pdr, pdi, pdm, prr, pri;
    double   z0r, z0i, zlr, zli, zlm, psr, psi, zsr, zsi, sg;
    float    zlrmin;
    int     i, ii, ir;
    static float eps = 1e-9F;

    sg = surge_gain(zl, zs, ps, pl, nthf);
    zl[0] = pl[0] = eps;
    zl[1] = pl[1] = 0;
    for (i = 1; i < nthf; i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
        psr = ps[ir] * sg;
        psi = ps[ii] * sg;
        zsr = zs[ir] * sg;
        zsi = zs[ii] * sg;
        plr = pl[ir];
        pli = pl[ii];
        pdr = psr - plr;
        pdi = psi - pli;
        pdm = pdr * pdr + pdi * pdi;
        prr = (plr * pdr + pli * pdi) / pdm;
        pri = (pli * pdr - plr * pdi) / pdm;
        zlr = zsr * prr - zsi * pri;
        zli = zsi * prr + zsr * pri;
        zl[ir] = (float) zlr;
        zl[ii] = (float) zli;
    }
    zlrmin = probe.minres;
    z0r = z_surge(zl, nthf);
    z0i = 0;
    for (i = 1; i < nthf; i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
        zlr = zl[ir];
        zli = zl[ii];
	if (zl[ir] < zlrmin) {
            zl[ir] = zlrmin;
	}
        zlm = zlr * zlr + zli * zli;
        prr = ((z0r * zlr + z0i * zli) / zlm + 1) / 2;
        pri = ((z0i * zlr - z0r * zli) / zlm) / 2;
        pr[ir] = (float) prr;
        pr[ii] = (float) pri;
    }
}

static void
thev_cmp_prz0()
{
    float   zt[2];
    int     i, ii, ir;
    double  z0r, z0i, zlr, zli, zlm;
    double  prr, pri;

;
    z0r = z_surge(zl, nthf);
    z0i = 0;
    z_chr(zt);
    for (i = 0; i < nthf; i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
        zlr = zl[ir];
        zli = zl[ii];
        zlm = zlr * zlr + zli * zli;
        prr = ((z0r * zlr + z0i * zli) / zlm + 1) / 2;
        pri = ((z0i * zlr - z0r * zli) / zlm) / 2;
	z0[ir] = zt[0];
	z0[ii] = zt[1];
	pr[ir] = (float) prr;
	pr[ii] = (float) pri;
    }
    zl[0] = (float) z0r;
    zl[1] = (float) z0i;
    for (i = 2; i < nthf - 1; i++) {	// fix outliers
        ir = 2 * i;
        ii = 2 * i + 1;
	prr = (pr[ir + 2] + pr[ir - 2]) / 2;
	pri = (pr[ii + 2] + pr[ii - 2]) / 2;
	if (_hypot(pr[ir], pr[ii]) > _hypot(prr, pri) * 2
	    || _hypot(pr[ir], pr[ii]) < _hypot(prr, pri) / 2) {
	    pr[ir] = (float) prr;
	    pr[ii] = (float) pri;
	}
    }
}

static void
thev_cmp_pfcd()
{
    int     i, ii, ir;
    double  zlr, zli, zlm, cdr, ac, nc;
    double  plr, pli, prr, pri, pfr, pfi;
    static float eps = 1e-9F;
    static double pi = M_PI;
    static double spl_ref = 2e-5;	// 20 uPa
    static double imp_ref = 1e5;	// 1 ohm = 10^5 Pa.s/m^3
    static double int_ref = 1e-12;	// pW/m^2 (F&M, 1933)

    //df = (double) rate / buflen;
    ac = pi * diacav * diacav / 4e4;	// area of cavity (m^2)
    nc = spl_ref / sqrt(ac * imp_ref * int_ref);
    pf[0] = cd[0] = eps;
    pf[1] = pr[1] = 0;
    for (i = 1; i < nthf; i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
        zlr = zl[ir];
        zli = zl[ii];
        plr = pl[ir];
        pli = pl[ii];
	//z0r = z0[ir];
	//z0i = z0[ii];
        prr = pr[ir];
        pri = pr[ii];
	// pf = pl * pr
        pfr = (plr * prr - pli * pri);
        pfi = (pli * prr + plr * pri);
	// cd = sqrt(real(1/zl))
        zlm = zlr * zlr + zli * zli;
	cdr = sqrt(zlr / zlm) * nc;
	//
	pf[ir] = (float) pfr;
	pf[ii] = (float) pfi;
	cd[i] = (float) cdr;
    }
}

int
thev_load(char *cfn, char *sfn)
{
    char tfn[MAXNAME];
    double vs;
    float   lc[2], ths_attn;
    int     j, n;
    short   sets;
    int32_t   *buf, ths_rate;

    if (!chkfn(cfn, sfn)) {
	return (0);
    }
    draw_w(&w_msg);
    draw_w(&w_sig);
    w_sig.ybot = w_sig.ytop +(w_sig.ybot - w_sig.ytop) / 2;
    w_spec.ybot = ypix - 1;
    draw_w(&w_spec);
    grab_probe();
    probe_flag = 1;
    db_range = 60;
    nthc = 5;
    vs = 3.4723e4 * (1 + 0.00166 * (tmpcav - 26.85));
    cal_read(cali_file, 0);	// read file to get buflen
    cal_mesg(cali_file);
    nthf = thev_init();
    if (!thev_src_read(ths_file, zs, ps, px, zs+os, ps+os, px+os)) {
        decide(0, 1, "Error reading THS file.");
	return (0);
    }
    ths_rate = rate;
    ths_attn = cal_attn;
    n = limit(0, numsrc * numadc, 2);	// tokens per channel
    for (j = 0; j < n; j++) {
        sprintf(line, "Reading pressure for token %d...", j + 1);
        stat_msg(line);
        select_chan(j);
        text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
	if (cal_read(cali_file, 0)) {
	    cal_disp(cali_file, 0);
	}
	save_stim_fft(1);
        dfgc = j ? scrn_c[C_CHNB] : scrn_c[C_CHNA];
        buf = j ? accbuf_b : accbuf_a;
        dis_cal_tok(buf);
        nftf = fft_sig(buf, cal_sets);
        retrieve_fft(pl, nthf);
        divide_by_stim(pl, nthf, j, swp1set * cal_sets);
        atten_scale(pl, nthf, j, cal_attn);
	lc[j] = (float) cavity_length(pl, nthf, vs);
        modulesqr();
	thev_cmp_zl();
	thev_cmp_prz0();
	thev_cmp_pfcd();
	if (write_txt) {
	    write_thev_ld_chan(j, cali_file);
	}
    }
    show_thev_ld(stim_unit.at == 4);
    line[0] = 0;
    if (ths_rate != rate) {
        sprintf(line, "rate = %d %d", ths_rate, rate);
	decide(0, 2, "Error THS & CAL mismatch:", line);
	return (0);
    }
    if (ths_attn != cal_attn) {
        sprintf(line, "attn = %6.0f %6.0f", ths_attn, cal_attn);
	decide(0, 2, "Error THS & CAL mismatch:", line);
	return (0);
    }
    strcpy(tfn, cali_file);
    newext(tfn, "THL");
    select_chan(0);
    sets = acc_sets;
    sprintf(line, "surge impedance = %6.1f %6.1f", zl[0], zl[ol]);
    info_msg(line, 1);
    sprintf(line, "  cavity length = %6.2f %6.2f cm", lc[0], lc[1]);
    info_msg(line, 2);
    if (thev_ld_sav(tfn, z0, zl, pl, pr, zl+ol, pl+ol, pr+ol, nthf, cal_attn, sets)) {
        decide(0, 1, "Error writing THL file.");
	stat_msg("Error writing THL file.");
    } else {
        sprintf(line, "Wrote Thevenin load file %s",tfn);
	stat_msg(line);
    }
    probe_flag = 0;
    return (1);
}

void
thev_load_show(char *cfn, int sil)
{
    char tfn[MAXNAME];

    strcpy(tfn, cfn);
    newext(tfn, "THL");
    nthf = thev_init();
    cal_read(cfn, 0);	// read file to get buflen
    save_stim_fft(1);
    if (thev_ld_read(tfn, z0, zl, pl, pr, zl+ol, pl+ol, pr+ol, nthf)) {
        thev_cmp_pfcd();
        select_chan(1);
        thev_cmp_pfcd();
        show_thev_adj(sil, 0);
    }
    thev_free();
}

int
thev_adjust(char *cfn, char *sfn, SAV_FFT *pft, int sil)
{
    double  rfm, prr, pri, prm, prp, cga, cgp, z0r, z0i, zlr, zli, zlm;
    int     i, ii, ir, j, n = 0;
    WIND    ws, wf;
    static double eps = 1e-9;

    ws = w_sig;	    // save w_sig
    wf = w_spec;    // save w_spec
    if (thev_load(cfn, sfn)) {
	// calculate FPL & SIL calibration adjustments
	for (j = 0; j < numsrc; j++) {
	    free_null(padj);
	    free_null(iadj);
	    free_null(refl);
	    n = (nthf < pft[j].npts) ? nthf : pft[j].npts;
	    padj = (float *) calloc(n, sizeof(float));
	    iadj = (float *) calloc(n, sizeof(float));
	    refl = (float *) calloc(n, sizeof(float));
	    for (i = 0; i < n; i++) {
		ir = 2 * i;
		ii = 2 * i + 1;
		prr = pr[ir];
		pri = pr[ii];
		prm = _hypot(prr, pri);
		prp = atan2(pri, prr);
		if (prm < eps) {
		    prm = eps;
		    prp = 0;
		}
		zlr = zl[ir];
		zli = zl[ii];
		zlm = _hypot(zlr, zli);
		if (zlm < eps) {
		    zlm = eps;
		}
		if (sil) {			// SIL adjust
		    cga = pft[j].rp[i] * cd[i];
		    cgp = pft[j].ph[i];	
		} else {			// FPL adjust
		    cga = pft[j].rp[i] * prm;
		    cgp = pft[j].ph[i] + prp;
		}
		// reflectance magnitude
		z0r = z0[ir];
		z0i = z0[ii];
		rfm = _hypot(zlr - z0r, zli - z0i) / _hypot(zlr + z0r, zli + z0i);
		// save for later reference
		pft[j].rp[i] = (float) cga;
		pft[j].ph[i] = (float) cgp;
		padj[i] = (float) (20 * log10(prm));
		iadj[i] = (float) (20 * log10(cd[i]));
		refl[i] = (float) (20 * log10(rfm));
	    }
        }
        show_thev_adj(sil, 1);
	nadj = n;
    }
    thev_free();
    w_sig = ws;	    // restore w_sig
    w_spec = wf;    // restore w_spec
    return (n);
}

/* adjust calibration gain for FPL or SIL */
void
thev_adj_cal(float *fst, float *pst, float *fpl, float *sil, float *rfl)
{
    double df;
    int i, j;

    df = (double) rate / buflen;
    for (i = 0; i < 4; i++) {
	j = nint(fst[i] / df);
	if (j > 0 && j < nadj) {
	    fpl[i] = pst[i] + padj[j];
	    sil[i] = pst[i] + iadj[j];
	    rfl[i] = refl[j];
	} else {
	    fpl[i] = pst[i];
	    sil[i] = pst[i];
	    rfl[i] = 0;
	}
    }
}

/* free calibration adjustment arrays */
void
thev_adj_free()
{
    if (padj) {		// forward pressure adjustment
	free(padj);
	padj = 0;
    }
    if (iadj) {		// intensity adjustment
	free(iadj);
	iadj = 0;
    }
}

void
thev_adj_show(char *cfn)
{
    char    tfn[MAXNAME];

    if (stim_unit.at != 3 && stim_unit.at != 4) {
	return;
    }
    strcpy(tfn, cfn);
    newext(tfn, "THL");
    nthf = thev_init();
    cal_read(cfn, 0);	// read file to get buflen
    save_stim_fft(1);
    if (thev_ld_read(tfn, z0, zl, pl, pr, zl+ol, pl+ol, pr+ol, nthf)) {
        thev_cmp_pfcd();
        select_chan(1);
        thev_cmp_pfcd();
        show_thev_adj(stim_unit.at == 4, 1);
    }
    thev_free();
}

/*****************************************************************************/

static void
create_load_file(void)
{
    WIND    ws, wf;

    ws = w_sig;	    // save w_sig
    wf = w_spec;    // save w_spec
    thev_load("*.cal", "*.ths");
    thev_free();
    w_sig = ws;	    // restore w_sig
    w_spec = wf;    // restore w_spec
}

int
create_load()
{
    set_trailer(create_load_file);
    return (27);
}

/*****************************************************************************/

int
stepper()
{
#ifdef WIN32
    char cmd[80];
    static char *pfn1 = "C:/Program Files/BTNRH/Stepper/StepperSigpro.exe";
    static char *pfn2 = "C:/Program Files/BTNRH/ER10XStepper/StepperSigpro.exe";

    if (_access(pfn1, 0) == 0) {
        if (decide(1, 1, "Launch HARP Stepper?")) {
            sprintf(cmd, "\"%s\"", pfn1);
            system(cmd);
            return(1);
        }
    }
    if (_access(pfn2, 0) == 0) {
        if (decide(1, 1, "Launch ER10X Stepper?")) {
            sprintf(cmd, "\"%s\"", pfn2);
            system(cmd);
            return(1);
        }
    }
#endif
    return (0);
}

/*****************************************************************************/

void
Cavity_wind()
{
    int cal_ok = 0;

    if (dsp_init() > 0)
	return;
    top_message("Cavity     ");
    dpoae.rate = adjust_rate(dpoae.rate);
    grab_probe();
    draw_w(&w_msg);
    draw_w(&w_info);
    FILE_SAVE.status = 0;
    dpoae_flag = 1;
    probe_flag = 1;
    cavit_test = 1;
    numcav = 1;
    cal_ok = chkfit_calibr();
    if (!cal_ok) {
        stat_msg(">>> Calibration Aborted.            ");
    } else {
	thev_load(cali_file, "*.ths");
	thev_free();
	strcpy(load_cal, cali_file);
    }
    probe_flag = 0;
    dpoae_flag = 0;
    cavit_test = 0;
    top_message("           ");
    dsprst(TRUE);
}

/*****************************************************************************/

void
Probe_wind()
{
    int cal_ok = 0;

    if (stepper())
	return;
    if (dsp_init() > 0)
	return;
    top_message("Probe      ");
    dpoae.rate = adjust_rate(dpoae.rate);
    grab_probe();
    draw_w(&w_msg);
    draw_w(&w_info);
    FILE_SAVE.status = 0;
    dpoae_flag = 1;
    probe_flag = 1;
    if (numsrc == 1 && !decide(1, 1, "Calibrate source ?")) {
	numcav = 1;
	numadc = 1;
    }
    cal_ok = chkfit_calibr();
    if (!cal_ok) {
        stat_msg(">>> Calibration Aborted.            ");
    } else if (numcav >= MINNCAV) {
        if (thev_source(cali_file))
	    probe_counter++;
    } else {
	if (decide(1, 2, "Calibrate load ?", cali_file)) {
	    thev_load(cali_file, "*.ths");
	    thev_free();
	    strcpy(load_cal, cali_file);
	} else if (_access(load_cal, 0) == 0) {
	    if (decide(1, 2, "Append to load file ?", load_cal)) {
		cal_save(load_cal, 1);
		cal_update(load_cal, 2);
		thev_load(load_cal, ths_file);
		thev_free();
	    }
	}
    }
    probe_flag = 0;
    dpoae_flag = 0;
    top_message("           ");
    dsprst(TRUE);
}
