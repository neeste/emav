/* oba2.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <fftlib.h>
#include <token.h>

#define MAXNPTS	8192
#define DSIZE	128

static float a[MAXNPTS + 2];
static float b[MAXNPTS + 2];
static short nto2;
static float r1_scale, r2_scale, st_scale;
static short oflg = 0, cflg = 0, iflg = 0, kflg = 0;
static short aflg = 0, sflg = 0, rflg = 0;;
static short *r1, *r2, *st;

TOKENFILE *t;
int     npts;
int32_t    rate;
int     itime1, itime2;

/* give a new extension to a filename */
void
newext(char *f, char *e)
{
    while (*f != '.' && *f != '\0')
	f++;
    *f++ = '.';
    while (*e != '\0')
	*f++ = *e++;
    *f++ = '\0';
}

void
usage()
{
    printf("usage:\n");
    printf("    oba2 [-option] file ...\n");
    printf("\n");
    printf("where file is a TOKEN data file.  Default analysis uses\n");
    printf("four one-octave bands centered at 0.5, 1, 2, and 4 kHz.\n");
    printf("options:\n");
    printf("    -c  use 12 third-octave bands\n");
    printf("    -i  write patient info\n");
    printf("    -k  use 6 1-kHz bands\n");
    printf("    -r  report rms in bands (default is level per cycle)\n");
    printf("    -s  write overall SPL (time-domain calculation)\n");
    exit(0);
}

void
wrinfo()
{
    float   ndb;

    printf("patient name: %s, %s\n", t->hitch.lastname, t->hitch.firstname);
    printf("ear: %s\n", t->hitch.ear == 'L' ? "Left" : "Right");
    printf("comment: %s\n", t->hitch.comment);
    printf("date: %s\n", t->hitch.outmsg[0]);
    printf("Test Time: %d sec\n", t->hitch.time);
    printf("Accepted: %s\n", t->hitch.outmsg[1]);
    printf("Sets x Sweeps: %s\n", t->hitch.outmsg[2]);
    ndb = 20.0 * log10(t->hitch.target_level / 0.00002);
    printf("Target Level=%.1f dB   %.1f mPa\n", ndb,
	t->hitch.target_level * 1000);
    ndb = 20.0 * log10(t->hitch.rejlmt / 0.02);
    printf("Noise:  Limit=%.1fmPa    %.1f dB pSPL\n", t->hitch.rejlmt, ndb);
    printf("Stimulus:  Stability=%4.1f%%  Mode=%d\n", t->hitch.stab,
	t->hitch.mode);
    printf("Attenuation=%f dB  Name=%s\n", (float) t->car.atten1 / 100.0,
	t->car.sname);
}

void
oba(short *aa, short *bb, float aa_scale, float bb_scale, short op, short n, 
    float *d, float *f, short nf)
{
    double  sum, dt, v, rms, dc, db;
    short   i, j, k, m, i1, i2;

    for (i = 0; i < n; i++) {
	if (op == 0)
	    a[i] = (float) aa[i] * aa_scale + (float) bb[i] * bb_scale;
	else if (op == 1)
	    a[i] = (float) aa[i] * aa_scale - (float) bb[i] * bb_scale;
	else
	    a[i] = (i < DSIZE) ? (float) aa[i] * aa_scale
		+ (float) bb[i] * bb_scale : 0;
    }

    if (sflg) {
	dc = 0;
	sum = 0;
	for (i = itime1; i < itime2; i++) {
	    v = a[i];
	    dc += v;
	    sum += v * v;
	}
	i1 = itime2 - itime1;
	rms = sqrt((sum - dc * dc / i1) / i1) / 2.0;
	db = 20 * log10(rms / 0.020);
	printf("1-2 pressure = %6.3f mPa (rms) = %4.1f dB SPL\n", rms, db);
	dc = 0;
	sum = 0;
	for (i = 0; i < n; i++) {
	    v = a[i];
	    dc += v;
	    sum += v * v;
	}
	rms = sqrt((sum - dc * dc / n) / n) / 2;
	db = 20 * log10(rms / 0.020);
	printf("tot pressure = %6.3f mPa (rms) = %4.1f dB SPL\n", rms, db);
    }
    fast(a, n);
    dt = n * 1000.0 / rate;
    for (j = 0; j < nf; j++) {
	sum = 0;
	m = 0;
	i1 = (j == 0) ? 1 : f[j - 1] * dt + 0.5;
	i2 = (j == 0) ? (n / 2) : f[j] * dt + 0.5;
	for (i = 2 * i1; i < 2 * i2; i += 2) {
	    k = i + 1;
	    sum += a[i] * a[i] + a[k] * a[k];
	    m++;
	}
	sum /= (float) m;
	if (sum <= 0) {
	    d[j] = -99;
	} else {
	    d[j] = 10.0 * log10(sum / 2.0) - 20.0 * log10(0.020 * n);
            if (rflg == 0)
                d[j] -= 10.0 * log10(1000.0 / dt);  /* level per cycle */
        }
    }
}

void
cross(short *aa, short *bb, float aa_scale, float bb_scale, int op, int n, float *d, float *f, int nf)
{
    double  dt, sum;
    short   i, j, k, m, i1, i2;

    for (i = 0; i < n; i++) {
	a[i] = (float) aa[i] * aa_scale;
	b[i] = (float) bb[i] * bb_scale;
    }
    fast(a, n);
    fast(b, n);
    dt = n * 1000.0 / rate;
    for (j = 0; j < nf; j++) {
	sum = 0;
	m = 0;
	i1 = (j == 0) ? 1 : f[j - 1] * dt + 0.5;
	i2 = (j == 0) ? (n / 2) : f[j] * dt + 0.5;
	for (i = 2 * i1; i < 2 * i2; i += 2) {
	    k = i + 1;
	    sum += a[i] * b[i] + a[k] * b[k];
	    m++;
	}
	sum /= (float) m;
	if (op == 0) {
	    d[j] = sum;
	} else {
	    if (sum > 1e-39) {
		d[j] = 10.0 * log10(sum * 2.0) - 20.0 * log10(0.020 * n);
		if (rflg == 0)
		    d[j] -= 10.0 * log10(1000.0 / dt);  /* level per cycle */
	    } else d[j] = -400;
	}
    }
}

double
cgd(double ar, double ai, double br, double bi, double dt)
{
    double  ph, uw;
    static double tpi = 6.2832;

    if ((ar * ar + ai * ai) == 0 || (br * br + bi * bi) == 0)
	return (0.0);
    ph = (atan2(ar, ai) - atan2(br, bi)) / tpi;
    uw = (ph < -0.5) ? (ph + 1) : ((ph > 0.5) ? (ph - 1) : ph);
    return (dt * uw);
}

void
gdelay(float *a, short n, float *d, float *f, short nf)
{
    double  sume, sumd, dt, dly, nrg;
    short   i, j, ii, ir, i1, i2;

    fast(a, n);
    a[0] = a[1] = a[n] = a[n + 1] = 0;
    dt = n * 1000.0 / rate;
    for (j = 0; j < nf; j++) {
	sume = sumd = 0;
	i1 = (j == 0) ? 1 : f[j - 1] * dt + 0.5;
	i2 = (j == 0) ? (n / 2) : f[j] * dt + 0.5;
	for (i = i1; i < i2; i++) {
	    ir = i * 2;
	    ii = ir + 1;
	    nrg = a[ir] * a[ir] + a[ii] * a[ii];
	    dly = cgd(a[ir], a[ii], a[ir - 2], a[ii - 2], dt) * nrg;
	    sumd += dly;
	    sume += nrg;
	}
	d[j] = sumd / sume;
    }
}

void
wrhdln(char *s, float *d, int n, char *u)
{
    int     i;

    printf("            %s", s);
    for (i = 1; i < n; i++)
	printf("  %5.2f", d[i]);
    printf("  %s\n", u);
}

void
wrdtln(char *s, float *d, short n, char *u)
{
    int     i;

    printf("  %s", s);
    for (i = 0; i < n; i++)
	printf(" %6.1f", d[i]);
    printf("  %s\n", u);
}

void
wrdata()
{
    float  *cf, *f;
    float   dbs[13], dbn[13], dbr[13], dly[13];
    float   dba[13], dbb[13], dbc[13], dbd[13];
    short   i, nf;
    static float cf1[2] = {0, 1.5};
    static float f1[2] = {0.5, 2.5};
    static float cf4[5] = {0.0, 0.5, 1.0, 2.0, 4.0};
    static float f4[5] = {0.354, 0.707, 1.414, 2.828, 5.657};
    static float cf5[6] = {0, 1, 2, 3, 4, 5};
    static float f5[6] = {0.488, 1.465, 2.441, 3.418, 4.395, 5.371};
    static float cf12[13] =
    {
	0.000, 0.397, 0.500, 0.630, 0.794, 1.000, 1.260,
	1.587, 2.000, 2.520, 3.175, 4.000, 5.040
    };
    static float f12[13] =
    {
	0.354, 0.445, 0.561, 0.707, 0.891, 1.122, 1.414,
	1.782, 2.245, 2.828, 3.564, 4.490, 5.657
    };

    if (cflg) {
	nf = 13;
	cf = cf12;
	f = f12;
    } else if (kflg) {
	nf = 6;
	cf = cf5;
	f = f5;
    } else if (aflg) {
	nf = 2;
	cf = cf1;
	f = f1;
    } else {
	nf = 5;
	cf = cf4;
	f = f4;
    }
    wrhdln("ALL", cf, nf, "kHz");

    cross(r1, r1, r1_scale, r1_scale, 0, nto2, dba, f, nf);
    cross(r2, r2, r2_scale, r2_scale, 0, nto2, dbb, f, nf);
    cross(r1, r2, r1_scale, r2_scale, 0, nto2, dbc, f, nf);
    for (i = 0; i < nf; i++) {
	dbd[i] = sqrt(dba[i] * dbb[i]);
	dbr[i] = 100 * dbc[i] / dbd[i];
    }
    wrdtln("  REP:", dbr, nf, "% ");

    cross(r1, r2, r1_scale, r2_scale, 1, nto2, dbc, f, nf);
    wrdtln("  AxB:", dbc, nf, rflg ? "dB SPL" : "dB Lps");

    oba(r1, r2, r1_scale, r2_scale, 0, nto2, dbs, f, nf);
    wrdtln("  A+B:", dbs, nf, rflg ? "dB SPL" : "dB Lps");

    oba(r1, r2, r1_scale, r2_scale, 1, nto2, dbn, f, nf);
    wrdtln("  A-B:", dbn, nf, rflg ? "dB SPL" : "dB Lps");

    for (i = 0; i < nf; i++)
	dbr[i] = dbs[i] - dbn[i];
    wrdtln("  S/N:", dbr, nf, rflg ? "dB SPL" : "dB Lps");

    for (i = 0; i < nto2; i++)
	a[i] = (float) r1[i] * r1_scale + (float) r2[i] * r2_scale;
    while (i < (2 * nto2))
	a[i++] = 0;
    gdelay(a, (nto2 * 2), dly, f, nf);
    wrdtln("delay:", dly, nf, "ms");

    for (i = 0; i < DSIZE; i++)
	st[i] -= 128;
    oba(st, st, st_scale, st_scale, 3, nto2, dbs, f, nf);
    wrdtln(" stim:", dbs, nf, rflg ? "dB SPL" : "dB Lps");
}

int
main(int ac, char **av)
{
    char    dfn[80];
    short   i;
    FILE   *fp;

    if (ac < 2)
	usage();
    while (ac > 1) {
	if (av[1][0] == '-') {
	    if (av[1][1] == 'o') {
		oflg++;
	    } else if (av[1][1] == 'c') {
		cflg++;
	    } else if (av[1][1] == 'i') {
		iflg++;
	    } else if (av[1][1] == 'k') {
		kflg++;
	    } else if (av[1][1] == 'r') {
		rflg++;
	    } else if (av[1][1] == 'a') {
		aflg++;
	    } else if (av[1][1] == 's') {
		sflg++;
	    }
	} else {
	    strcpy(dfn, av[1]);
	    newext(dfn, "tok");
	    if ((fp = fopen(dfn, "rb")) == NULL) {
		fprintf(stderr, "can't open %s\n", dfn);
	    } else {
		t = topen(dfn);
		i = (int32_t) t->header.power10;
		rate = (int32_t) t->header.isf;
		while (i-- > 0)
			rate *= 10;
		npts = (int) t->hitch.toksiz;
		if (iflg)
		    wrinfo();
		printf("\nData file: %s\n", dfn);
		nto2 = 1;
		while (nto2 < npts)
		    nto2 *= 2;
		r1 = (short *) malloc(sizeof(short) * nto2);
		r2 = (short *) malloc(sizeof(short) * nto2);
		st = (short *) malloc(sizeof(short) * nto2);
		tread(st, 1, t);
		st_scale = t->car.scale * 1000.0;
		tread(r1, 2, t);
		r1_scale = t->car.scale * 1000.0;
		itime1 = t->car.rejt1;
		itime2 = t->car.rejt2;
		tread(r2, 3, t);
		r2_scale = t->car.scale * 1000.0;
		i = npts;
		while (i < nto2) {
		    r1[i] = r2[i] = st[i] = 0;
		    i++;
		}
		wrdata();
		fclose(fp);
	    }
	}
	ac--;
	av++;
    }
    printf("\n");
    return (0);
}

