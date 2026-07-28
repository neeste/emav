/* probe_w.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <menu.h>
#include <util.h>
#include <fftlib.h>
#include "savage.h"
#include "color.h"
#include "screen.h"
#include "putt.h"

#ifndef WIN32
#define _hypot hypot
struct _complex {
    double  x;
    double  y;
};
#endif /* WIN32 */

#define MAXNPTS		8192	/* based on 65KW SMA */
#define FFRAC           2
#define MAX_NF          (MAXNPTS/FFRAC+2)
#define MAXCAV          8
#define FNSZ            42

double  pick_min(double, double);
double  pick_max(double, double);
double  pkmn(int, int, int, float *);
double  pkmx(int, int, int, float *);
int	dis_stim(int, void *, enum TYPE);
int     getstim(char *, int);
void    retrieve_fft(float *, int);
void    set_drspc(int);
void    simpfit();
void    out_msg(int, char[][16], int);

extern char line[];
extern char *probe_cal;
extern double cal_ppc, cal_vpc, lentst;
extern int lpsflg, fft_flag, prob_cal_cnt, rand_seed;
extern int32_t run_time;
extern ATTENUATION attenu;
extern MENUITEM file_opt[];
extern TOGGLETYPE thv_itr, thv_ext, thv_xtk, thv_dbg;

int     prb_flg = 0, prb_cav = 0;
int     g_ncav = 4;
char    th_src_file[FNSZ] = {0};

typedef struct _complex complex;

static char fnf[MAXPATH] = "probe.fnf";
static int icav = 0;
static int nftf = 0;
static int nthf = 0;
static int nthc = 0;
static int plsz = 0;
static int ncond = 0;
static int extend = 0;
static int esc_flg = 0;
static int itr_flg = 0;
static int dis_cav_imp = 0;
static float *cond = NULL;		// conductance (G10)
static float *fpladj = NULL;		// FPL adustment
static float *tmladj = NULL;		// SPL_TM adustment
static float *pc[MAXCAV];
static float   fz1[MAXCAV], fpk[MAXCAV];
static float   lcv[MAXCAV + 4];
static float   diacav = 1;
static float   tmpcav = 22;
static float *ps = NULL, *px, *zs, *pl = NULL, *zl, *pr;
static float *qq1 = NULL, *st;
static complex src_rfl = {0, 0};

void
grab_probe()
{
    buflen = probe.size;
    rate = probe.rate;
    swp1set = probe.chk_swps;
    diacav = probe.diacav;
    tmpcav = probe.temp;
    g_ncav = probe.ncav;
    rand_seed = probe.seed;
    Sen = Sys;
    dis_units(Sen.AD, Sen.MP);
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
z_cav(float *z, double f, double lcav, double dcav, double temp)
{
    double  Rv, Ro, c, rho, eta, w, tgy, thx, den, pi, deltemp;
    double  Zcr, Zci, wnr, wni, acav, ctr, cti;
    static double stdtemp = 26.85;    /* std. temp. (celsius) */

    deltemp = temp - stdtemp;
    c = 3.4723e4 * (1 + 0.00166 * deltemp);
    rho = 1.1769e-3 * (1 - 0.00335 * deltemp);
    eta = 1.846e-4 * (1 + 0.0025 * deltemp);
    acav = dcav / 2;		/* radius */
    pi = 4 * atan(1.0);
    w = 2 * pi * f;
    Ro = rho * c / (pi * acav * acav);
    Rv = acav * sqrt(rho * w / eta);
//    Zcr = Ro * (1 + 0.369 / Rv);
//    Zci = -Ro * ((0.369 + (1.149 + 0.303 / Rv) / Rv) / Rv);
    Zcr = Ro;
    Zci = 0;
    wnr = (w / c) * ((1.045 + (1.080 + 0.750 / Rv) / Rv) / Rv);
    wni = (w / c) * (1 + 1.045 / Rv);
    wnr += src_rfl.x;		// add source reflectance ???
    wni += src_rfl.y;
    thx = tanh(wnr * lcav);
    tgy = tan(wni * lcav);
    den = thx * thx + tgy * tgy;
    ctr = (thx + thx * tgy * tgy) / den;
    cti = (thx * thx * tgy - tgy) / den;
    z[0] = (float) (Zcr * ctr - Zci * cti);
    z[1] = (float) (Zcr * cti + Zci * ctr);
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
    while ((m < i2) && (p[m] < p[m - 1])) {
        m++;
    }
    for (i = m + 1; i <= i2; i++) {
	if (p[m] < p[i]) {
	    m = i;
	}
    }
    d = (p[m-1]-p[m+1])/(p[m-1]-2*p[m]+p[m+1])/2;
    ecl = (m + d) * dl;
    free(p);

    return (ecl);
}

static double
c_mgsq(complex z)
{
    return (z.x * z.x + z.y * z.y);
}

static double
c_abs(complex z)
{
    return (sqrt(z.x * z.x + z.y * z.y));
}

static complex
cadd(complex a, complex b)
{
    complex c;
    
    c.x = a.x + b.x;
    c.y = a.y + b.y;
    return (c);
}

static complex
csub(complex a, complex b)
{
    complex c;
    
    c.x = a.x - b.x;
    c.y = a.y - b.y;
    return (c);
}

static complex
cmul(complex a, complex b)
{
    complex c;
    
    c.x = a.x * b.x - a.y * b.y;
    c.y = a.y * b.x + a.x * b.y;
    return (c);
}

static complex
cdiv(complex a, complex b)
{
    complex c;
    double  d;
    static double eps = 1e-40;

    d = b.x * b.x + b.y * b.y;
    if (d < eps)
    	d = eps;
    c.x = (a.x * b.x + a.y * b.y) / d;
    c.y = (a.y * b.x - a.x * b.y) / d;
    return (c);
}

static complex
crecip(complex b)
{
    complex c;
    double  d;
    static double eps = 1e-40;

    d = b.x * b.x + b.y * b.y;
    if (d < eps)
    	d = eps;
    c.x = b.x / d;
    c.y = -b.y / d;
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
        zm.y = atan2(z.y, z.x) * (45 / atan(1));
    } else {
        zm.x = -800;
        zm.y = 0;
    }
    return (zm);
}

int
get_th_src_fn(char *fn)
{
    int     i;
    FILE   *ifp;
    static int msgcnt = 0;
    
    /* open the Thevenin source file-name file */
    ifp = fopen(fnf, "rt");
    if (ifp == NULL) {
        if (msgcnt == 0) {
            decide(0, 2, "Can't open Thevenin source file-name file", fnf);
            msgcnt++;
        }
        return (0);
    }
    fgets(fn, FNSZ, ifp);
    fclose(ifp);
    for (i = 0; fn[i] > ' '; i++)
        continue;
    fn[i] = '\0';

    return (i);
}

static int
thev_src_open()
{
    int     n;
    
    /* get the Thevenin source file-name */

    if (!get_th_src_fn(th_src_file))
        return (0);

    /* open the Thevenin source file */

    n = thev_src_read(th_src_file, zs, ps, px, MAX_NF);
    if (n <= 0 && thv_dbg.at)
    	decide(0, 2, "Error reading Thevenin source file", th_src_file);

    return(n);
}

int
thev_init()
{
    int i, n;
    
    ps = (float *) calloc(MAX_NF * 6, sizeof(float));
    if (ps == NULL) {
        decide(0, 1, "Can't allocate thev memory (ps)");
        return (0);
    }
    zs = ps + MAX_NF * 2;
    px = zs + MAX_NF * 2;

    qq1 = (float *) calloc(MAX_NF * 4, sizeof(float));
    if (qq1 == NULL) {
        decide(0, 1, "Can't allocate thev memory (qq1)");
        return (0);
    }

    n = (nthc < 2) ? 3 : nthc;	    // guarantee space for zl & pr
    if (plsz != n) {
	if (pl)
	    free(pl);
	pl = (float *) calloc(MAX_NF * 2 * n, sizeof(float));
	plsz = n;
    }
    if (pl == NULL) {
        decide(0, 1, "Can't allocate thev memory (pl)");
        return (0);
    }
    zl = pl + MAX_NF * 2;
    pr = zl + MAX_NF * 2;
    pc[0] = pl;
    for (i = 1; i < nthc; i++)
        pc[i] = pc[i - 1] + MAX_NF * 2;

    return (buflen / FFRAC);
}

/* thev_acc:  Z1 * P0 - P1 * Z0 = Z1 * P1 */

void
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

void
thev_cmp(int n, double df, float *lc)
{
    int i, ir, ii, j, k;
    double d, a[4], q[4];
    float z[2], p[2];
    static double eps = 1e-40;
    
    for (i = 1; i < n; i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
        for (k = 0; k < 4; k++)
            a[k] = q[k] = 0;
        for (j = 0; j < nthc; j++) {
            z_cav(z, i * df, lc[j], diacav, tmpcav);
            p[0] = pc[j][ir];
            p[1] = pc[j][ii];
            thev_acc(p, z, a, q);
        }
        d = a[1] * a[0] - (a[2] * a[2] + a[3] * a[3]);
        if (fabs(d) < eps)
            d = eps;
        ps[ir] = (float) ((a[0] * q[0] + a[2] * q[2] - a[3] * q[3]) / d);
        ps[ii] = (float) ((a[0] * q[1] + a[2] * q[3] + a[3] * q[2]) / d);
        zs[ir] = (float) ((a[1] * q[2] + a[2] * q[0] + a[3] * q[1]) / d);
        zs[ii] = (float) ((a[1] * q[3] + a[2] * q[1] - a[3] * q[0]) / d);
    }
}

void
thev_cmp2(int n, double df, float *lc)
{
    int i, ir, ii;
    double z1p2r, z1p2i, z2p1r, z2p1i, z1z2r, z1z2i, p1p2r, p1p2i;
    double z1p2z2p1r, z1p2z2p1i, z1z2p1p2r, z1z2p1p2i, d;
    float z1[2], z2[2];
    static double eps = 1e-40;
    

    for (i = 1; i < n; i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
        z_cav(z1, i * df, lc[0], diacav, tmpcav);
        z_cav(z2, i * df, lc[1], diacav, tmpcav);
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

double
thev_erf(int i1, int i2, int n, double df, float *lc)
{
    int     i, j, ii, ir, er;
    complex p0, z0, p1, z1, psum, perr, zero = {0,0};
    double   nm1, nm2;
    float    z[2];

    nm1 = nm2 = 0;
    for (i = 1; i < n; i++) {    
        er = (i >= i1 && i < i2);
        ir = 2 * i;
        ii = 2 * i + 1;
        p0.x = ps[ir];
        p0.y = ps[ii];
        z0.x = zs[ir];
        z0.y = zs[ii];
        psum = zero;
        for (j = 0; j < nthc; j++) {
            p1.x = pc[j][ir];
            p1.y = pc[j][ii];
            z_cav(z, i * df, lc[j], diacav, tmpcav);
            z1.x = z[0];
            z1.y = z[1];
            perr = csub(p1,cmul(p0,cdiv(z1,cadd(z0,z1))));
            psum = cadd(psum,perr);
            if (er) {
                nm1 += c_mgsq(perr);
                nm2 += c_mgsq(p1) / 10000;
            }
        }
        if (thv_xtk.at) {
       	    px[ir] = (float) (psum.x / nthc);
       	    px[ii] = (float) (psum.y / nthc);
            if (er)
                nm1 -= c_mgsq(psum) / nthc;
        }
    }
    return ((nm2 > 0) ? nm1 / nm2 : 0);
}

double
thev_err(float *lc)
{
    int i1, i2, nf;
    double df;

    if (extend == 1) {
        diacav = lc[nthc];
        tmpcav = lc[nthc+1];
    } else if (extend == 2) {
	src_rfl.x = lc[nthc];
	src_rfl.y = lc[nthc+1];
    }
    df = (double) rate / buflen;
    i1 = limit(1, (int) (probe.f1_erf / df + 0.5), nthf);
    i2 = limit(1, (int) (probe.f2_erf / df + 0.5), nthf) + 1;
    nf = itr_flg ? i2 : nthf + 1;
    
    if (nthc == 2) {
        thev_cmp2(nf, df, lc);
    } else {
        thev_cmp(nf, df, lc);
    }
    return (thev_erf(i1, i2, nf, df, lc));
}

void
thev_rep(float *lc)
{
    int i, j, k;
    double d, h, e, srr, sri;

    if (extend == 1) {
        d = lc[nthc];
        h = lc[nthc+1];
        e = lc[nthc+2];
	srr = 0;
	sri = 0;
    } else if (extend == 2) {
        d = diacav;
        h = tmpcav;
        e = lc[nthc+2];
	srr = lc[nthc];
	sri = lc[nthc+1];
    } else {
        d = diacav;
        h = tmpcav;
        e = lc[nthc];
	srr = 0;
	sri = 0;
    }
    j = w_info.xtop + 4;
    text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / 8 - 1;
    k = w_info.ytop + txtpar.font_height;
    gprintf(j, k, "Probe Calibration: D=%.3fcm T=%.2fC sr=%.4f,%.4f  ", d, h, srr, sri);
    k += txtpar.font_height;
    for (i = 0; i < nthc; i++) {
        gprintf(j, k, "#%d: lc=%5.3f cm       ", i + 1, lc[i]);
	k += txtpar.font_height;
    }

    gprintf(j, k, "error = %6.3f  ", e);
    k += txtpar.font_height;
}

int
thev_esc()
{
    int c = 0;

    if (check_event()) {
        c = getevent();
        if (c == 'd')
            dis_cav_imp = !dis_cav_imp;
        if (c == 27)
            esc_flg = 1;
    }
    return (c == 27);
}

static void
show_db(float *z, int n, WIND *wn, int levref, int reset, int db_range, int c)
{
    int xrange, yrange;
    int     i, nd;
    double   dt, lpsval;
    short   *ibuf;
    WIND    *wp;

    dfgc = c;
    display_reset = reset;
    dt = (nftf > 1) ? (2.0 * nftf) / rate : 1;
    lpsflg = levref;
    lpsval = (levref == 1) ? 10 * log10(dt) : 0;
    ibuf = (short *) z;
    ibuf[0] = -800;
    for (i = 1; i < n; i++) {
	ibuf[i] = nint(z[i] + lpsval);
    }
    wp = w_dis;
    w_dis = wn;
    if (display_reset > 0) {
	gr_rectf(w_dis->xtop + 1, w_dis->ytop + 1, w_dis->xbot - 1,
	    w_dis->ybot - 1, w_dis->wbgc);
	gr_recto(w_dis->xtop, w_dis->ytop, w_dis->xbot,
	    w_dis->ybot, menucolor.boxc);
    }
    xrange = probe.fft_xrange;
    yrange = db_range;
    fft_flag = 1;
    nd = nint(xrange * dt * 1000);
    display(ibuf, nd, display_reset, yrange, xrange);
    fft_flag = 0;
    w_dis = wp;

    display_reset = 0;
}

void
remove_dc(float *p)
{
    int     i, n;
    double  sum, avg;

    sum = 0;
    n = buflen;
    for (i = 0; i < n; i++)
    	sum += p[i];
    avg = (n > 0) ? sum / n : 0;
    for (i = 0; i < n; i++)
    	sum -= avg;
}

void
dis_cal_tok(int load, float *p)
{
    int     num;
    extern WIND w_sig;
    extern WIND w_spec;

    if (display_reset) {
	draw_w(&w_sig);
	draw_w(&w_spec);
    }
    w_dis = &w_sig;
    num = buflen;
    disply(swp1set, (float *) p, num, display_reset, 0);
    fft_sig((float *) p, 1);
    modulesqr();
    show_fft(probe.fft_xrange, probe.fft_yrange, probe.levref,
	dfgc, display_reset);
    display_reset = 0;
}

void
show_thev_src(int wrdf)
{
    complex z0, zm, p0, pm;
    double df = 0;
    int i, ir, ii, n, rs = 1;
    float *zsm, *psm;
    WIND    wn;
    FILE *ofp = NULL;
    static char *fn = "thev.dat";
    
    if (wrdf) {
        df = (double) rate / buflen;
        ofp = fopen(fn, "wt");
        fprintf(ofp, "; %s\n", fn);
        fprintf(ofp, ";   f     |Ps|     Ps<    |Zs|     Zs<\n");
    }
    n = nthf;
    zsm = (float *) qq1;
    psm = (float *) qq1 + MAX_NF;
    psm[0] = zsm[0] = -800;
    for (i = 1; i < n; i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
        p0.x = ps[ir];
        p0.y = ps[ii];
        z0.x = zs[ir];
        z0.y = zs[ii];
        pm = dbph(p0);
        zm = dbph(z0);
        psm[i] = (float) pm.x;
        zsm[i] = (float) zm.x;
        if (wrdf && i > 0)
            fprintf(ofp, "%6.0f %7.1f %7.1f %7.1f %7.1f\n", 
                i * df, pm.x, pm.y, zm.x, zm.y);
    }
    if (wrdf)
        fclose(ofp);

    wn.wfgc = w_spec.wfgc;
    wn.wbgc = w_spec.wbgc;
    wn.xtop = 0;
    wn.xbot = w_sig.xbot / 2;
    wn.ytop = w_sig.ybot + 1;
    wn.ybot = w_sig.ybot + txtpar.menu_height;
    gprintf(draw_w(&wn), wn.ybot - 3, "Source Impedance:");
    wn.ytop = w_sig.ybot + txtpar.menu_height + 1;
    wn.ybot = ypix - 1;
    show_db(zsm, n, &wn, 2, rs, 40, scrn_c[C_WAVE]);
    wn.xtop = w_sig.xbot / 2 + 1;
    wn.xbot = w_sig.xbot;
    wn.ytop = w_sig.ybot + 1;
    wn.ybot = w_sig.ybot + txtpar.menu_height;
    gprintf(draw_w(&wn), wn.ybot - 3, "Source Pressure:");
    wn.ytop = w_sig.ybot + txtpar.menu_height + 1;
    wn.ybot = ypix - 1;
    show_db(psm, n, &wn, 3, rs, 40, scrn_c[C_WAVE]);
}

int
thev_src_dsp(char *tfn)
{
    FILE *ofp;

    show_thev_src(1);
    if (decide(1, 3, "Use the 'Thevenin source' file", tfn,
        "for 'load impedance' calculations?")) {
        ofp = fopen(fnf, "wt");
        fprintf(ofp, "%s\n", tfn);
        fclose(ofp);
        return (1);
    };
    return (0);
}

void
thev_free()
{
    if (ps != NULL) {
        free(ps);
        ps = NULL;
    }
    if (qq1 != NULL) {
        free(qq1);
        qq1 = NULL;
    }
    if (pl != NULL) {
        free(pl);
        pl = NULL;
	plsz = 0;
    }
    nthf = 0;
}

static int
thev_ld_init()
{
    int     i, n;
    
/* set default conductance values, in case estimation fails */

    if (!cond)
        cond = (float *) calloc(MAX_NF, sizeof(float));
    for (i = 0; i < MAX_NF; i++)
        cond[i] = -400;
    if (!fpladj)
        fpladj = (float *) calloc(MAX_NF, sizeof(float));
    if (!tmladj)
        tmladj = (float *) calloc(MAX_NF, sizeof(float));
        
/* allocate memory for arrays */

    if (!ps)
        ps = (float *) calloc(MAX_NF * 6, sizeof(float));
    if (ps == NULL) {
        decide(0, 1, "Can't allocate thevenin memory (ps)");
        return (0);
    }
    zs = ps + MAX_NF * 2;
    px = zs + MAX_NF * 2;

    if (!qq1)
        qq1 = (float *) calloc(MAX_NF * 4, sizeof(float));
    if (qq1 == NULL) {
        decide(0, 1, "Can't allocate thev memory (qq1)");
        return (0);
    }

    n = (nthc < 2) ? 3 : nthc;	    // guarantee space for zl & pr
    if (plsz != n) {
	if (pl)
	    free(pl);
	pl = (float *) calloc(MAX_NF * 2 * n, sizeof(float));
	plsz = n;
    }
    if (pl == NULL) {
        decide(0, 1, "Can't allocate thevenin memory (pl)");
        return (0);
    }
    zl = pl + MAX_NF * 2;
    pr = zl + MAX_NF * 2;

    return (MAX_NF);
}

static void
thev_ld_cmp()
{
    complex z, y;
    double   plr, pli, pdr, pdi, pdm, prr, pri, pi, zchr, zlrmin;
    int     i, ii, ir;

    pi = 4 * atan(1.0);
    zchr = rho_c / (pi * diacav * diacav / 4);
    zlrmin = probe.minres;
    zl[0] = (float) zchr;
    zl[1] = 0;
    pr[0] = pl[0];
    pr[1] = pl[1];
    for (i = 1; i < nthf; i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
        plr = thv_xtk.at ? pl[ir] - px[ir] : pl[ir];
        pli = thv_xtk.at ? pl[ii] - px[ii] : pl[ii];
        pdr = ps[ir] - plr;
        pdi = ps[ii] - pli;
        pdm = pdr * pdr + pdi * pdi;
        prr = (plr * pdr + pli * pdi) / pdm;
        pri = (pli * pdr - plr * pdi) / pdm;
	zl[ir] = (float) (zs[ir] * prr - zs[ii] * pri);
	zl[ii] = (float) (zs[ii] * prr + zs[ir] * pri);
        if (zl[ir] < zlrmin)
            zl[ir] = (float) zlrmin;
	z.x = zl[ir];
        z.y = zl[ii];
	y = crecip(z);
	pr[ir] = (float) ((1 + zchr * y.x) / 2);
	pr[ii] = (float) (zchr * y.y / 2);
    }
}

static void
thev_ld_dsp(int savdat)
{
    complex z0, z1, zm, p0, p1, pm, zt, y1, a1, b1;
    double   df;
    float   *zlmg, *zlph = NULL;
    float   ztst[2];
    int     i, ii, ir, rs = 1;
    FILE   *ofp = NULL;
    WIND    wn;
    static char *fn = "load.dat";
    static double eps = 1e-40;

    df = (double) rate / buflen;
    zlmg = (float *) qq1;
    if (savdat) {
        zlph = (float *) qq1 + MAX_NF * 2;
        ofp = fopen(fn, "wt");
        fprintf(ofp, "; %s\n", fn);
        fprintf(ofp, "; lentst =%7.2f\n", lentst);
        fprintf(ofp, ";    f    |Zl|     Zl<     Gl     |Pl|  ");
        fprintf(ofp, "   Pl<    |Ps|     Ps<    |Zs|     Zs<  ");
        fprintf(ofp, "  |Zt|     Zt<\n");
        zlph[0] = 0;
    }
    zlmg[0] = -800;
    cond[0] = -400;
    for (i = 1; i < nthf; i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
        z0.x = zs[ir];
        z0.y = zs[ii];
        z0 = dbph(z0);
        p0.x = ps[ir];
        p0.y = ps[ii];
        p0 = dbph(p0);
        z1.x = zl[ir];
        z1.y = zl[ii];
        zm = dbph(z1);
        p1.x = thv_xtk.at ? pl[ir] - px[ir] : pl[ir];
        p1.y = thv_xtk.at ? pl[ii] - px[ii] : pl[ii];
        pm = dbph(p1);
        zlmg[i] = (float) zm.x;
        if (savdat)
            zlph[i] = (float) zm.y;
        //z1ms = c_mgsq(z1);
	y1 = crecip(z1);
        cond[i] = (float) ((y1.x < eps) ? -400 : (10 * log10(y1.x)));
	a1.x = pr[ir];
	a1.y = pr[ii];
        fpladj[i] = (float) (20 * log10(c_abs(a1)));		    // FPL adjustment
	b1.x = 1 - pr[ir];
	b1.y = -pr[ii];
        tmladj[i] = (float) (20 * log10(c_abs(a1) + c_abs(b1)));    //SPL_TM adjustment
        z_cav(ztst, i * df, lentst, diacav, tmpcav);
        zt.x = ztst[0];
        zt.y = ztst[1];
        zt = dbph(zt);
        if (savdat) {
            fprintf(ofp, "%6.0f %7.1f %7.1f %7.1f %7.1f ", 
                i * df, zlmg[i], zlph[i], cond[i], pm.x);
            fprintf(ofp, "%7.1f %7.1f %7.1f %7.1f %7.1f  ", 
                pm.y, p0.x, p0.y, z0.x, z0.y);
            fprintf(ofp, "%7.1f %7.1f \n", zt.x, zt.y);
        }
    }
    ncond = nthf;
    if (savdat)
        fclose(ofp);

    wn.wfgc = w_spec.wfgc;
    wn.wbgc = w_spec.wbgc;
    wn.xtop = 0;
    wn.xbot = w_sig.xbot / 2;
    wn.ytop = w_sig.ybot + 1;
    wn.ybot = w_sig.ybot + txtpar.menu_height;
    gprintf(draw_w(&wn), wn.ybot - 3, "Load Impedance:");
    wn.ytop = w_sig.ybot + txtpar.menu_height + 1;
    wn.ybot = ypix - 1;
    show_db(zlmg, nthf, &wn, 2, rs, 80, scrn_c[C_WAVE]);
    wn.xtop = w_sig.xbot / 2 + 1;
    wn.xbot = w_sig.xbot;
    wn.ytop = w_sig.ybot + 1;
    wn.ybot = w_sig.ybot + txtpar.menu_height;
    gprintf(draw_w(&wn), wn.ybot - 3, "Conductance:");
    wn.ytop = w_sig.ybot + txtpar.menu_height + 1;
    wn.ybot = ypix - 1;
    show_db(cond, nthf, &wn, 2, 1, 80, scrn_c[C_WAVE]);	    // data overwritten!
}

void
show_cond(char *fn, int flg)
{
    char     ldfn[FNSZ];
    double   z1ms, y1_r;
    int     i, ii, ir, n, c;
    WIND *wn;
    static char *ld_ext= "THL";
    static double eps = 1e-40;

    wn = &w_spec;
    if (flg == 0)
        gprintf(draw_w(wn), wn->ytop - 3, "Conductance:");

    n = (nthc < 2) ? 3 : nthc;	    // guarantee space for zl & pr
    if (plsz != n) {
	if (pl)
	    free(pl);
	pl = (float *) calloc(MAX_NF * 2 * n, sizeof(float));
	plsz = n;
    }
    if (pl == NULL) {
        decide(0, 1, "Can't allocate thevenin memory (pl)");
        return;
    }
    zl = pl + MAX_NF * 2;
    pr = zl + MAX_NF * 2;

    strcpy(ldfn, fn);
    newext(ldfn, ld_ext);
    n = thev_ld_read(ldfn, zl, pl, MAX_NF);
    if (n <= 0) {
        decide(0, 2, "Can't open load file", ldfn);
        return;
    }
    if (!cond)
        cond = (float *) calloc(MAX_NF, sizeof(float));

    cond[0] = -400;
    for (i = 1; i < n; i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
        z1ms = zl[ir] * zl[ir] + zl[ii] * zl[ii];
        y1_r = (z1ms < eps) ? 1 : (zl[ir] / z1ms);
        cond[i] = (float) ((y1_r < eps) ? -400 : (10 * log10(y1_r)));
    }

    ncond = nftf = n;
    c = (flg % 2) ? scrn_c[C_WAVB] : scrn_c[C_WAVA];
    show_db(cond, nftf, wn, 2, (flg == 0), 40, c);	    // data overwritten!
}

double
cond_peak(double f1, double f2)
{
    double  df;
    int     i1, i2;

    df = (double) rate / buflen;
    i1 = (int) (f1 / df + 0.5);
    i2 = (int) (f2 / df + 0.5);
    return (pkmx(i1, i2, 1, cond) * df);
}

static void
show_stim(char *file)
{
    stim_title(file);
    getstim(file, buflen);
    strncpy(ilstim, file, FNSZ);
    display_reset = 1;
    dfgc = scrn_c[C_WAVE];
    dis_stim(1, stimulus, INT);
}

void
open_stim_w(char *file)
{
    getstim(file, buflen);
    show_stim(file);
}

void
save_stim_fft(int nset)
{
    int i;
    float mx = 0;
    
    st = qq1 + MAX_NF * 2;
    for (i = 0; i < buflen; i++) {
        st[i] = stimulus[i];
        if (mx < st[i])
            mx = st[i];
        if (mx < -st[i])
            mx = -st[i];
    }
    if (mx == 0)
        st[0] = 1;      /* default to unit impulse if stimulus is zero */
    nftf = fft_sig(st, nset);
    retrieve_fft(st, nthf);
}

void
divide_by_stim(float *pp, int n)
{
    int i, ii, ir;
    complex p0, p1, p2;
    double ppv;

    ppv = cal_ppc / cal_vpc;
    for (i = 0; i < n; i++) {
        ir = 2 * i;
        ii = 2 * i + 1;
        p0.x = st[ir];
        p0.y = st[ii];
        p1.x = pp[ir];
        p1.y = pp[ii];
        p2 = cdiv(p1, p0);
        pp[ir] = (float) (p2.x * ppv);
        pp[ii] = (float) (p2.y * ppv);
    }
}

void
inf_msg(char *s)
{
    text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / 8 - 1;
    gprintf(w_info.xtop + 4, w_info.ybot - 4, s);
}

int
thev_source(char *fn1, char *fn2)
{
    char    tfn[80], sfn[80], hdr[12];
    char    outmsg[3][16] = {{0}};
    double  e, et, vs;
    float sr[2];
    int     i, j, k, nc, thv_flg, pf, nv, tk, used = 0;
    int32_t    stime;
    short   tm, ct;
    //static double fl = 8550;

    if (!cal_open(fn1, line, 0)) {
	decide(0, 2, line, fn1);
	return (0);
    } else {
    	strncpy(file_name, fn2, FNSZ);
        cal_info(hdr, sfn, outmsg, &tm, &ct);
        show_stim(sfn);
	pf = !strcmp(hdr, probe_cal);
        out_msg(tm, outmsg, 0);
        nc = (thv_dbg.at && decide(1,1,"Set nc=1?")) ? 1 : ct;
	thv_flg = pf && nc > 1;
        if (thv_flg) {
            nthc = limit(2, nc, MAXCAV);
            nthf = thev_init();
        } else {
            nthc = (ct > 0) ? 1 : 0;
            nthf = thev_ld_init() ? thev_src_open() : 0;
        }
        if (nthc <= 0 || nthf <= 0) {
            cal_close();
            thev_free();
            return (0);
        }
	display_reset = 1;
	//f0 = fl / probe.maxlen;
	diacav = probe.diacav;
	tmpcav = probe.temp;
        vs = 3.4723e4 * (1 + 0.00166 * (tmpcav - 26.85));
        inf_msg("Reading cavity pressures ...");
	for (i = 0; i < nthc; i++) {
            tk =i + 1;
            text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
            cal_read(tk);
            remove_dc(accbuf);
            if (i == 0)                 /* after swp1set is read */
                save_stim_fft(1);
	    dfgc = (i % 2) ? scrn_c[C_CHNB] : scrn_c[C_CHNA];
	    dis_cal_tok(0, accbuf);
	    nftf = fft_sig(accbuf, 1);
            if (thv_flg) {
                retrieve_fft(pc[i], nthf);
                divide_by_stim(pc[i], nthf);
	        lcv[i] = (float) cavity_length(pc[i], nthf, vs);
            } else {
                retrieve_fft(pl, nthf);
                divide_by_stim(pl, nthf);
	        lcv[i] = (float) cavity_length(pl, nthf, vs);
            }
	    modulesqr();
	    fz1[i] = (float) (vs / lcv[i] / 4);
	    fpk[i] = fz1[i] * 2;
	}
	dfgc = scrn_c[C_WAVE];
	cal_close();
    }

    draw_w(&w_info);
    j = w_info.xtop + 4;
    text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / 8 - 1;
    k = w_info.ytop + txtpar.font_height;
    gprintf(j, k, "%s Calibration", pf ? "Probe" : "Ear");
    k += txtpar.font_height;
    for (i = 0; i < nthc; i++) {
        gprintf(j, k, "#%d: fz/p=%.0f/%.0f Hz, L=%.3f cm ",
            i + 1, fz1[i], fpk[i], lcv[i]);
	k += txtpar.font_height;
    }

    if (thv_flg) {
        gprintf(j, k, "error = %6.3f  ", thev_err(lcv));
        show_thev_src(0);
        decide_offset(-100, 40);
        if((nthc > 2) && ((thv_itr.at == 1) || 
            ((thv_itr.at == 2) && decide(1, 1, "Iterate?")))) {
            inf_msg("Iterating lengths ...      ");
            stime = clock();
            nv = nthc;
            esc_flg = 0;
            itr_flg = 1;
	    src_rfl.x = 0;
	    src_rfl.y = 0;
	    extend = 0;
            simpfit(lcv, nv, probe.niter, nv, thev_err, thev_rep, thev_esc);
            if (esc_flg) {
                thev_free();
		return (0);
	    }
            extend = thv_ext.at;
            if (extend == 1) {
                lcv[nv] = diacav;
        	lcv[nv + 1] = tmpcav;
                nv += 2;
                simpfit(lcv, nv, probe.niter, nv, thev_err, thev_rep, thev_esc);
            } else if (extend == 2) {
                lcv[nv] = 0;
		lcv[nv + 1] = 0;
                nv += 2;
                simpfit(lcv, nv, probe.niter * 2, nv, thev_err, thev_rep, thev_esc);
		sr[0] = (float) src_rfl.x;
		sr[1] = (float) src_rfl.y;
            }
            itr_flg = 0;
            e = thev_err(lcv);
            et = (clock() - stime) / (double) CLOCKS_PER_SEC;
            text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
            txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / 8 - 1;
            gprintf(j, k, "error = %6.3f (%.0f sec)   ", e, et);
	    src_rfl.x = 0;
	    src_rfl.y = 0;
        }
        if (thev_src_sav(fn2, tfn, zs, ps, px, nthf, diacav, tmpcav, nthc, lcv, sr)) {
            decide(0, 2, "Error writing Thevenin source file", tfn);
	}
        inf_msg("                            ");
        used = thev_src_dsp(tfn);
        decide_offset(0, 0);
    } else {
        inf_msg("                            ");
        thev_ld_cmp();
        if (thev_ld_sav(fn2, th_src_file, zl, pl, pr, nthf))
            decide(0, 1, "Error writing load impedance file");
        thev_ld_dsp(1);
    }
    thev_free();
    return (used);
}

void
thev_load(char *fn)
{
    int sets;

    nthc = 1;
    nthf = thev_ld_init() ? thev_src_open() : 0;
    if (nthf <= 0) {                /* chk for proper thev_ld_init */
        thev_free();
        return;
    }
    sets = nsets * 2;
    //reps = sets * swp1set;
    cal_vpc = 1 / Sen.AD;           /* V/cnt  */
    cal_ppc = cal_vpc / Sen.MP;     /* Pa/cnt */
    save_stim_fft(sets);
    nftf = fft_sig(accbuf, sets);
    retrieve_fft(pl, nthf);
    divide_by_stim(pl, nthf);
    modulesqr();
    thev_ld_cmp();
    if (thev_ld_sav(fn, th_src_file, zl, pl, pr, nthf))
        decide(0, 1, "Error writing load impedance file");
    thev_ld_dsp(1);
    thev_free();
}

int
thev_quick_load()
{
    static int sets = 1;

    if(nthf <= 0) {                 /* initialize */
        nthc = 1;
        nthf = thev_ld_init() ? thev_src_open() : 0;
        if (nthf <= 0) {            /* chk for proper thv_ld_init */
            thev_free();
            return (0);
        }
        //reps = sets * swp1set;
        cal_vpc = 1 / Sen.AD;           /* V/cnt  */
        cal_ppc = cal_vpc / Sen.MP;     /* Pa/cnt */
        save_stim_fft(sets * 30);
        return (1);
    }
    nftf = fft_sig((float *) dspbuf, sets);
    retrieve_fft(pl, nthf);
    divide_by_stim(pl, nthf);
    modulesqr();
    thev_ld_cmp();
    thev_ld_dsp(0);
    return (1);
}

void
thev_reload(char *fn, char *src, int display)
{
    char    sfn[80], hdr[12];
    char    outmsg[3][16] = {{0}};
    int     i, tk;
    short   tm, ct;

    if (!cal_open(fn, line, 0)) {
	decide(0, 2, line, fn);
	return;
    } else {
    	strncpy(file_name, fn, FNSZ);
        cal_info(hdr, sfn, outmsg, &tm, &ct);
        show_stim(sfn);
        nthc = (ct > 0) ? 1 : 0;
        nthf = thev_ld_init() ? thev_src_read(src, zs, ps, px, MAX_NF) : 0;
        if (nthc <= 0 || nthf <= 0) {
            cal_close();
            thev_free();
            return;
        }

	for (i = 0; i < nthc; i++) {
            tk =i + 1;
            cal_read(tk);
            remove_dc(accbuf);
            if (i == 0)                 /* after swp1set is read */
                save_stim_fft(1);
	    nftf = fft_sig(accbuf, 1);
            retrieve_fft(pl, nthf);
            divide_by_stim(pl, nthf);
	}
	cal_close();
    }

    thev_ld_cmp();
    if (thev_ld_sav(fn, th_src_file, zl, pl, pr, nthf)) {
        decide(0, 1, "Error writing load impedance file");
    }
    if (display) {
        thev_ld_dsp(0);
    }
    thev_free();
    return;
}

double
thev_cond(double f)
{
    int i;
    double c;

    i = nint(buflen * f / rate);
    c = (cond && (i > 0) && (i < ncond)) ? cond[i] : -400;
    return (c);
}

void
thev_adjust(double f, double spl, double *sil, double *fpl, double *tml)
{
    int i;
    double c;

    i = nint(buflen * f / rate);
    c = (cond && (i > 0) && (i < ncond)) ? cond[i] : -400;
    *sil = spl + c + 10 * log10(rho_c);
    *fpl = fpladj ? spl + fpladj[i] : spl;	// FPL adjustment
    *tml = tmladj ? spl + tmladj[i] : spl;	// SPL_TM adjustment
}

int
dis_probe()
{
    int     i, k;

    i = w_info.xtop + 4;
    text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / 8 - 1;
    k = w_info.ytop + txtpar.font_height;
    gprintf(i, k, "Probe Calibration:  cavity # %d / %d",
	icav + 1, g_ncav);
    k += txtpar.font_height;
    gprintf(i, k, "D=%.2f cm, T=%.2fC", diacav, tmpcav);
    k += txtpar.font_height;
    return (k);
}

/*****************************************************************************/

int
cav_test()
{
    int     mode;

    if (dsp_init() > 0)
        return (0);
    top_message("Cavity  ");
    probe.rate = adjust_rate(probe.rate);
    prb_flg = 1;		/* start of cavity test */
    prb_cav = 0;
    option[1].status = 0;
    option[3].status = 0;
    grab_probe();
    dis_units(Sen.AD, Sen.MP);
    upperv = (int) (probe.rejlim * 0.001 * Sen.MP * Sen.AD);
    g_ncav = 1;
    icav = 0;
    draw_w(&w_msg);
    file_opt[4].status = 0;
    run_time = clock();
    for (;;) {
        open_stim_w(probe.calibrate);
        if (check_fit() == 0) {
            option[1].status = 1;
            option[3].status = 1;
            prb_flg = 0;
            return (0);
        }
        display_reset = 1;
        R_clear();
        open_stim_w(probe.calibrate);
        w_dis = &w_sig;
        mode = calibrate_it("P");
        if (mode != 0)
            break;
    }
    if (mode <= 0 || !decide(1, 2, "Save the file", cali_file)) {
	strcpy(cali_file, "Not Saved");
    } else {
	if (rename(tmp_cal_fn, cali_file)) {
	    decide(0, 3, "Can't rename file!", tmp_cal_fn, cali_file);
            return (0);
	}
        prob_cal_cnt++;
        write_counter_file();
    }
    option[1].status = 1;
    option[3].status = 1;
    prb_flg = 0;		/* end of cavity test */
    top_message("        ");

    return (0);
}

/*****************************************************************************/

int
probe_test()
{
    int     mode = 0;

    if (dsp_init() > 0)
        return (0);
    top_message("Probe   ");
    probe.rate = adjust_rate(probe.rate);
    prb_flg = 1;		/* start of probe test */
    prb_cav = 0;
    option[1].status = 0;
    option[3].status = 0;
    grab_probe();
    dis_units(Sen.AD, Sen.MP);
    upperv = (int) (probe.rejlim * 0.001 * Sen.MP * Sen.AD);
    draw_w(&w_msg);
    file_opt[4].status = 0;

    run_time = clock();
    for (icav = 0; icav < g_ncav; icav++) {
	for (;;) {
	    open_stim_w(probe.calibrate);
	    if (check_fit() == 0) {
		option[1].status = 1;
		option[3].status = 1;
		prb_flg = 0;
		return (0);
	    }
	    display_reset = 1;
	    R_clear();
	    open_stim_w(probe.calibrate);
	    w_dis = &w_sig;
	    mode = calibrate_it("P");
	    if (mode != 0)
		break;
	}
    }
    if (thev_source(tmp_cal_fn, cali_file))
        set_drspc(0);
    if (mode <= 0 || !decide(1, 2, "Save the file", cali_file)) {
	strcpy(cali_file, "Not Saved");
    } else {
	if (rename(tmp_cal_fn, cali_file)) {
	    decide(0, 3, "Can't rename file!", tmp_cal_fn, cali_file);
            return (0);
	}
        prob_cal_cnt++;
        write_counter_file();
    }
    option[1].status = 1;
    option[3].status = 1;
    prb_flg = 0;		/* end of probe test */
    top_message("        ");

    return (0);
}
