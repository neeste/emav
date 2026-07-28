/* var.c */

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <token.h>
#include <menu.h>
#include "savage.h"

#define NCH 2

void    zpf(float *, int, double, double);

extern int nmp, ifsp;

static float smmp[NCH][MAXNMP] = {{0}};
static float ssmp[NCH][MAXNMP] = {{0}};
static int wmvflg = 0;          /* use "wide-mean variance" in var_sum */
static double vmp_min = 0;      /* minimum variance */

double
var_sum(float *buf1, float *buf2)
{
    int     i, i1, i2, n1 = 0, n2 = 0;
    float   sa1 = 0, sa11 = 0;
    float   A1, ea11;

    if (wmvflg) {
	i1 = 0;
	i2 = buflen;
    } else {
	i1 = itime1;
	i2 = itime2;
    }
    for (i = i1; i < i2; i++) {
	n1++;
	A1 = (buf1[i] + buf2[i]);
	sa1 += A1;
        if (i >= itime1 && i < itime2) {
	    n2++;
	    sa11 += A1 * A1;
	}
    }
    if (n2 > 0) {
	ea11 = (sa11 / n2) - (sa1 / n1) * (sa1 / n1);
	if (ea11 > 0)
	    return (ea11);
    }
    return (1);
}

double
var_dif(float *buf1, float *buf2)
{
    int     i, i1, i2, n1 = 0, n2 = 0;
    float   sa1 = 0, sa11 = 0;
    float   A1, ea11;

    if (wmvflg) {
	i1 = 0;
	i2 = buflen;
    } else {
	i1 = itime1;
	i2 = itime2;
    }
    for (i = i1; i < i2; i++) {
	n1++;
	A1 = (buf1[i] - buf2[i]);
	sa1 += A1;
        if (i >= itime1 && i < itime2) {
	    n2++;
	    sa11 += A1 * A1;
	}
    }
    if (n2 > 0) {
	ea11 = (sa11 / n2) - (sa1 / n1) * (sa1 / n1);
	if (ea11 > 0)
	    return (ea11);
    }
    return (1);
}

#ifdef NEVER
double
var_dif(float *a1, float *a2)
{
    int     i, n;
    float   sa1 = 0, sa11 = 0;
    float   A1, ea11, rms;

    for (i = itime1; i < itime2; i++) {
	A1 = (a1[i] - a2[i]) / 2;
	sa1 += A1;
	sa11 += A1 * A1;
    }
    n = itime2 - itime1;
    if (n > 0) {
	ea11 = (sa11 - sa1 * sa1 / n) / n;
	if (ea11 > 0)
	    return (ea11);
    }
    return (1);
}

double
var_sum(float *a1, float *a2)
{
    int     i, n;
    float   sa1 = 0, sa11 = 0;
    float   A1, ea11, rms;

    for (i = itime1; i < itime2; i++) {
	A1 = (a1[i] + a2[i]) / 2;
	sa1 += A1;
	sa11 += A1 * A1;
    }
    n = itime2 - itime1;
    if (n > 0) {
	ea11 = (sa11 - sa1 * sa1 / n) / n;
	if (ea11 > 0)
	    return (ea11);
    }
    return (1);
}
#endif /* NEVER */

double
cor_sum(float *a1, float *a2)
{
    int     i, n;
    double   sa1, sa11, ea11, A1 = 0;
    double   sa2, sa22, ea22, A2 = 0;
    double   sa12, ea12, cor;
    //extern float F1_cutoff, F2_cutoff;
    extern TOGGLETYPE correlate;
    static double cor_mul = 1;
    //static double tmpl_f1 = 0, tmpl_f2 = 0, tr = 50000;

    sa1 = sa2 = sa11 = sa12 = sa22 = 0;
    for (i = itime1; i < itime2; i++) {
        if (correlate.at == 0) {
//            A1 = a1[i] + a2[i];
//            A2 = template[i * j];
        } else {
            A1 = a1[i];
            A2 = a2[i];
        }
	sa1 += A1;
	sa2 += A2;
	sa11 += A1 * A1;
	sa12 += A1 * A2;
	sa22 += A2 * A2;
    }
    n = itime2 - itime1;
    if (n > 0) {
	ea11 = (sa11 - sa1 * sa1 / n);
	ea12 = (sa12 - sa1 * sa2 / n);
	ea22 = (sa22 - sa2 * sa2 / n);
	if (ea11 > 0 && ea12 > 0 && ea22 > 0) {
	    cor = ea12 / sqrt(ea11 * ea22);
	    return (cor * cor_mul);
	}
    }
    return (1e-9);
}

void
init_mp()
{
    int i, j;

    for (i = 0; i < MAXNMP; i++)
        for (j = 0; j < NCH; j++)
            smmp[j][i] = ssmp[j][i] = 0;
}

double
var_mp(float *buf1, float *buf2, double wt, double twt, int ns, int c)
{
    double  vmp;
    int     i, j, imp, jmp, ct;
    double  mn, ms, sm;
    double  a1, a2;
    static int prflg = 1;

    if (nmp == 1) {
        imp = 0;
        jmp = ifsp;
    } else {
        imp = buflen / nmp;
        jmp = imp / 2;
    }
    for (i = 0; i < nmp; i++) {
        j = i * imp + jmp;
        a1 = buf1[j] * wt;
        a2 = buf2[j] * wt;
        smmp[c][i] += (float) (a1 + a2);
        if (prflg) {
            sm = a1 + a2;
	    ssmp[c][i] += (float) (sm * sm);
        } else {
            ssmp[c][i] += (float) (a1 * a1 + a2 * a2);
        }
    }
    sm = 0;
    wt = twt * 2;
    ct = ns * (prflg ? 1 : 2);
    for (i = 0; i < nmp; i++) {
        mn = smmp[c][i] / wt;
        ms = ssmp[c][i] * ct / (wt * wt);
        sm += (ct > 1) ? ms - mn * mn : ms;
    }
    vmp = sm / nmp;

    if (vmp < vmp_min)
        vmp = vmp_min;

    return (vmp / ct);
}
