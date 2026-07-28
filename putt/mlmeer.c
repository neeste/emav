/* mlmeer.c - "Maximum Likelihood" and "Maximum Expected Entropy Reduction"
 * implemented by the functions pick_ml_lev() and pick_meer_lev().
 */

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#define NT          151         /* number of possible threshold levels */
#define NM            1         /* number of ML psych. func. per threshold*/
#define NC         (NT*NM)	/* total number of ML psychometric functions */

#define m_las(x)	(tflg ? 0.5 : 0.0)
#define m_uas(x)	(1.0)
#define m_slp(x)        (0.3)
#define m_lev(x)        ((double)(x)-20)

#define limit(a,b,c) (((b)<(a))?(a):((b)>(c))?(c):(b))

static double cpr[NC], npr[NC], tpr[NC];
static double so_m = 0, so_a = 0, so_b = 1, so_k = 0.3;
static double minlev = -60;
static double maxlev = 90;
static int32_t seed = 1;
static int eflg = 0;            /* MEER flag */
static int tflg = 0;            /* 2AFC flag */
static int uflg = 0;            /* UPDN flag */

/*
 * unirand - uniform random number generator
 *
 * From "Random Number Generators: Good Ones are Hard to Find",
 * by Stephen K. Park and Keith W. Miller,
 * Communications of the ACM, 31, 10 (Oct. 1988) pp. 1192-1201.
 *
 */
double
unirand()
{
    int32_t    hi, lo;
/*    static int32_t seed = 1; */
    static int32_t a = 16807;
    static int32_t m = 2147483647;	/* Mersenne prime 2^31 -1 */
    static int32_t q = 127773;	/* M div A (M / A) */
    static int32_t r = 2836;	/* M mod A (M % A) */

    hi = seed / q;
    lo = seed % q;
    if ((seed = a * lo - r * hi) <= 0)
	seed += m;
    return ((double) seed / m);
}

void
set_seed(int32_t s)
{
    seed = s;
}

double
pcor(double x, double m, double a, double b, double k)
{
    double p;

    p = a + (b - a) / (1 + exp(-k * (x - m)));
    return(p);
}

void
set_obser(double m, double a, double b, double k)
{
    so_m = m;     /* mid-level          */
    so_a = a;     /* lower asymptote    */
    so_b = b;     /* upper asymptote    */
    so_k = k;     /* slope of logistic  */
}

int
hear_it(double s)
{
    double p, r;

    p = pcor(s, so_m, so_a, so_b, so_k);
    r = unirand();
    return (p >= r);
}

void
init_cpr()
{
    int i, nc = NC;

    for (i = 0; i < nc; i++) 
	cpr[i] = 1;
}

void
comp_cpr()
{
    int i, nc = NC;

    for(i = 0; i < nc; i++) {
	cpr[i] *= tpr[i];
    }
}

int
max_cpr()
{
    int i, nc = NC, imax = 0;
    double max = 0;

    for(i = 0; i < nc; i++) {
	if (max < cpr[i]) {
	    max = cpr[i];
	    imax = i;
	}
    }
    return(imax);
}

void
init_tpr()
{
    int i, nc = NC;

    for(i = 0; i < nc; i++)
	tpr[i] = 1;
}

void
comp_tpr(double s, int y)
{
    int i, nc = NC;
    double m, a, b, k, p;

    for(i = 0; i < nc; i++) {
	m = m_lev(i);
	a = m_las(i);
	b = m_uas(i);
	k = m_slp(i);
	p = pcor(s, m, a, b, k);
	tpr[i] = y ? p : 1 - p;
    }
}

double
aver_tpr()
{
    int i, nc = NC;
    double sum1, sum2;

    sum1 = 0;
    for(i = 0; i < nc; i++)
	sum1 += cpr[i];
    sum2 = 0;
    for(i = 0; i < nc; i++) 
	sum2 += tpr[i] * cpr[i] / sum1;
    return (sum2);
}

double
entropy()
{
    int i, nc = NC;
    double sum;

    sum = 0;
    for(i = 0; i < nc; i++) {
	npr[i] = tpr[i] * cpr[i];
	sum += npr[i];
    }
    for(i = 0; i < nc; i++)
	npr[i] /= sum;

    sum = 0;
    for(i = 0; i < nc; i++)
	if (npr[i] > 0)
	    sum -= npr[i] * log(npr[i]);

    return (sum);
}

double
exp_entrpy(double s)
{
    double e, e0, e1, p;

    comp_tpr(s, 0);
    e0 = entropy();
    comp_tpr(s, 1);
    e1 = entropy();
    p = aver_tpr();
    e = (1 - p) * e0 + p * e1;
    return (e);
}

double
pick_ml_lev()
{
    return(m_lev(max_cpr()));
}

double
pick_meer_lev()
{
    double s, e, ep, em;

    s = pick_ml_lev();
    e = exp_entrpy(s);
    ep = exp_entrpy(s + 1);
    em = exp_entrpy(s - 1);
    while (e > ep || e > em) {
	if (ep < em) {
	    s += 1;
	    em = e;
	    e = ep;
	    ep = exp_entrpy(s + 1);
	} else if (em < ep) {
	    s -= 1;
	    ep = e;
	    e = em;
	    em = exp_entrpy(s - 1);
	}
	if (s <= minlev || s >= maxlev)
	    break;
    }
    max_cpr();

    return(s);
}

double
pick_exp_lev()
{
    int i;
    double s, sum1, sum2, x;

    sum1 = 0;
    for (i = 0; i < NC; i++)
	sum1 += cpr[i];
    sum2 = 0;
    for (i = 0; i < NC; i++) {
	s = m_lev(i);
	sum2 += s * cpr[i];
    }
    x = sum2 / sum1;
    /*if (!rflg)*/
	x = floor(x + 0.5);
    return(x);
}

/*****************************  updn  *************************************/

static double stp;
static int cnt, nco;

static void
init_updn(double s)
{
    stp = s;            /* initial step size                    */
    cnt = 0;            /* reset consec. correct response count */
    nco = tflg ? 3 : 1;  /* # correct before step down           */
}

static double
pick_updn_lev(double lev, int y)
{
    if (y == 0) {           /* incorrect response ?                 */
        lev += stp;         /* increase stimulus level              */
        cnt = 0;            /* reset consec. correct response count */
    } else if (++cnt >= nco) {/* increment counter, check if enough */
        lev -= stp;         /* decrease stimulus level              */
        cnt = 0;            /* reset consec. correct response count */
    }
    return (limit(minlev, lev, maxlev));  /* restrict level within limits */
}

/*****************************  ansi  *************************************/

static void
init_ansi(double s)
{
    stp = s;                /* initial step size       */
}

static double
pick_ansi_lev(double lev, int y)
{
    if (y == 0) {           /* incorrect response ?    */
        lev += stp;         /* increase stimulus level */
    } else {                /* correct reponse         */
        lev -= 2 * stp;     /* decrease stimulus level */
    }
    return (limit(minlev, lev, maxlev));  /* restrict level within limits */
}

static int
seq_match(char *rsp_hist, int trial, char *s, int n)
{
    int i, j;

    if (trial < n) {
        return (0);
    }
    // match sequence
    for (i = 0; i < n; i++) {
        j = trial - n + i;
        if (s[i] != rsp_hist[j]) {
            break;
        }
    }

    return (i == n);
} 

static int
ansi_stop(char *rsp_hist, char *max_hist, int trial)
{
    static char s1[] = {1, 0, 0, 1};
    static char s2[] = {1, 0, 1, 0, 0, 0, 1, 0, 1};
    static char s3[] = {0, 0};
    static int n1 = sizeof(s1) / sizeof(s1[0]);
    static int n2 = sizeof(s2) / sizeof(s2[0]);
    static int n3 = sizeof(s3) / sizeof(s3[0]);

    // check stopping rule #1
    if (seq_match(rsp_hist, trial, s1, n1)) {
        return (1);
    }
    // check stopping rule #2
    if (seq_match(rsp_hist, trial, s2, n2)) {
        return (1);
    }
    // check stopping rule #3
    if (seq_match(rsp_hist, trial, s3, n3)) {
        if (max_hist[trial - 1] && max_hist[trial - 2]) {
            return (1);
        }
    }

    return (0);
}

/*****************************  get_lev  *************************************/

double
get_first_lev(double updn_start, double updn_step)
{
    double s;
    init_cpr();
    if (uflg == 1) {                // UPDN
        init_updn(updn_step);
        s = updn_start;
    } else if (uflg == 2) {         // ANSI
        init_ansi(updn_step);
        s = updn_start;
    } else if (eflg) {              // MEER
        s = pick_meer_lev();
    } else {                        // ML
        s = pick_exp_lev();
    }

    return (s);
}

double
get_next_lev(double s, int y)
{
    double ml;

    comp_tpr(s, y);
    comp_cpr();
    ml = pick_ml_lev();
    if (uflg == 1) {                // UPDN
        s = pick_updn_lev(s, y);
    } else if (uflg == 2) {         // ANSI
        s = pick_ansi_lev(s, y);
    } else if (eflg) {              // MEER
        s = pick_meer_lev();
    } else {                        // ML
        s = ml;
    }

    return (s);
}

int
stop_check(char *rsp_hist, char *max_hist, int trial)
{
    int s = 0;

    if (uflg == 2) {                // ANSI
        s = ansi_stop(rsp_hist, max_hist, trial);
    }

    return (s);
}

/*****************************  ml_fit  *************************************/

void    simpfit();

static char *rh;
static float *lh;
static int ntrials = 0;

double
mlf_err(float *par)
{
    int i;
    double pc, cp;
    
    if (par[0] < -20 || par[0] > 120
        || par[1] < 0.1 || par[1] > 0.5
        || par[2] < 0.9 || par[2] > 1.0
        || par[3] < 0.9 || par[3] > 1.0)
        return (1e30);
    cp = 1;
    for (i = 0; i < ntrials; i++) {
        pc = pcor(lh[i], par[0], 1-par[2], par[3], par[1]);
        cp *= rh[i] ? pc : 1 - pc;
        cp *= 2;                    /* to avoid underflow */
    }
    return (1 / cp);
}

void
mlf_rep(float *par)
{
}

int
mlf_esc()
{
    return (0);
}

double
ml_fit(int nt, double ml, float *lev_hist, char *rsp_hist, 
    float *k, float *a, float *b)
{
    float par[4];
    static int npar = 4, niter = 1000;
    
    par[0] = (float) ml;
    par[1] = 0.2F;
    par[2] = 0.95F;
    par[3] = 0.95F;
    rh = rsp_hist;
    lh = lev_hist;
    ntrials = nt;
    simpfit(par, npar, niter, npar, mlf_err, mlf_rep, mlf_esc);
    *k = par[1];
    *a = 1 - par[2];
    *b = par[3];
    return (par[0]);
}

/*****************************  putt  **************************************/

double
cmp_sd_lev()
{
    int     i, nc = NC;
    double  a, b, c, s, mn, sd;

    a = b = c = 0.0;
    for (i = 0; i < nc; i++) {
	s = m_lev(i);
	a += cpr[i];
	b += cpr[i] * s;
	c += cpr[i] * s * s;
    }
    mn = (a > 0) ? b / a : 0;
    sd = (a > 0) ? sqrt(c / a - mn * mn) : 1000;
    return (sd);
}

void
set_algo(int a, int t)
{
    if (a == 0) {           // UPDN
        eflg = 0;
        uflg = 0;
    } else if (a == 1) {    // ML
        eflg = 1;
        uflg = 0;
    } else if (a == 2) {    // MEER
        eflg = 0;
        uflg = 1;
    } else {                // ANSI
        eflg = 0;
        uflg = 2;
    }
    tflg = t;
}

void
set_levlim(double mnlv, double mxlv)
{
    minlev = mnlv;
    maxlev = mxlv;
}
