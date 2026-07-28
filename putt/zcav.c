/* zcav.c - compute cavity impedance given length of cavity */

#include <stdio.h>
#include <math.h>

//#ifndef MSDOS
#define _hypot hypot
//struct _complex {
//    double  x;
//    double  y;
//};
//#endif /* MSDOS */

#define TEMP 22.0
#define MXNP 2048
#define limit(min,val,max)	((val<min)?min:((val>max)?max:val))

double tanh(double);

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

void
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
    Zcr = Ro * (1 + 0.369 / Rv);
    Zci = -Ro * ((0.369 + (1.149 + 0.303 / Rv) / Rv) / Rv);
    wnr = (w / c) * ((1.045 + (1.080 + 0.750 / Rv) / Rv) / Rv);
    wni = (w / c) * (1 + 1.045 / Rv);
    thx = tanh(wnr * lcav);
    tgy = tan(wni * lcav);
    den = thx * thx + tgy * tgy;
    ctr = (thx + thx * tgy * tgy) / den;
    cti = (thx * thx * tgy - tgy) / den;
    z[0] = Zcr * ctr - Zci * cti;
    z[1] = Zcr * cti + Zci * ctr;
}

void
zcav(float *z, int nf, double df, double lcav, double dcav, double temp)
{
    int     i;

    z[0] = z[1] = 0;
    for (i = 1; i < nf; i++)
        z_cav(&z[i * 2], i * df, lcav, dcav, temp);
}

double
tanh(double x)
{
    return ((exp(x) - exp(-x)) / (exp(x) + exp(-x)));
}

typedef struct _complex complex;
static complex c_tmp, c_zero = {0, 0}, c_one = {1, 0}, c_two = {2, 0};

complex
clog(complex z)
{
    double  r, a;

    r = _hypot(z.x, z.y);
    a = atan2(z.y, z.x);
    z.x = r > 0 ? log(r) : -800;
    z.y = r > 0 ? a : 0;
    return (z);
}

complex
cadd(complex a, complex b)
{
    complex c;

    c.x = a.x + b.x;
    c.y = a.y + b.y;
    return (c);
}

complex
csub(complex a, complex b)
{
    complex c;

    c.x = a.x - b.x;
    c.y = a.y - b.y;
    return (c);
}

complex
cmul(complex a, complex b)
{
    complex c;

    c.x = a.x * b.x - a.y * b.y;
    c.y = a.y * b.x + a.x * b.y;
    return (c);
}

complex
cdiv(complex a, complex b)
{
    complex c;
    double  d;

    d = b.x * b.x + b.y * b.y;
    c.x = (a.x * b.x + a.y * b.y) / d;
    c.y = (a.y * b.x - a.x * b.y) / d;
    return (c);
}

complex
cctanh(complex z)
{
    double  thx, tgy, den;

    thx = tanh(z.x);
    tgy = tan(z.y);
    den = thx * thx + tgy * tgy;
    z.x = (thx + thx * tgy * tgy) / den;
    z.y = (thx * thx * tgy - tgy) / den;
    return (z);
}

complex
cactanh(complex z)
{
    return (cdiv(clog(cdiv(cadd(z, c_one), csub(z, c_one))), c_two));
}

complex
lcav(double *z, int nf, double df, double dcav, double temp)
{
    int     i, ns = 0;
    complex zt, ze, zr, rs, lc, sm, wn, wl;
    double  Rv, Ro, c, rho, eta, w, pi, deltemp;
    double  acav;
    static double stdtemp = 26.85;	/* std. temp. (celsius) */
    static double lmin = 1, wlrmax = 0.018;

    deltemp = temp - stdtemp;
    c = 3.4723e4 * (1 + 0.00166 * deltemp);
    rho = 1.1769e-3 * (1 - 0.00335 * deltemp);
    eta = 1.846e-4 * (1 + 0.0025 * deltemp);
    acav = dcav / 2;		/* radius */
    pi = 4 * atan(1.0);
    z[0] = z[1] = 0;
    sm = lc = c_zero;
    for (i = 1; i < nf; i++) {
	w = (2 * pi) * (i * df);
	Ro = rho * c / (pi * acav * acav);
	Rv = acav * sqrt(rho * w / eta);
	zt.x = Ro * (1 + 0.369 / Rv);
	zt.y = -Ro * ((0.369 + (1.149 + 0.303 / Rv) / Rv) / Rv);
	wn.x = (w / c) * ((1.045 + (1.080 + 0.750 / Rv) / Rv) / Rv);
	wn.y = (w / c) * (1 + 1.045 / Rv);
	ze.x = z[2 * i];
	ze.y = z[2 * i + 1];
	zr = cdiv(ze, zt);
	wl = cdiv(clog(cdiv(cadd(zr, c_one), csub(zr, c_one))), c_two);
	if (wl.x < wlrmax && wl.y < pi / 2) {
	    lc = cdiv(wl, wn);
            if (lc.x > lmin) {
                sm = cadd(sm, lc);
                printf("%.0f  ", i * df);
                printf("wl=(%.3f,%.3f)  ", wl.x, wl.y);
                printf("lc=(%.3f,%.3f)\n", lc.x, lc.y);
		ns++;
            }
	}
    }
    if (ns > 0) {
	lc.x = sm.x / ns;
	lc.y = sm.y / ns;
    }
    return (lc);
}

double
pkmn(double f1, double f2, double df, float *fbuf)
{
    double   f, c, mn;
    int     i, i1, i2, ii;

    i1 = (int) (f1 / df + 0.5);
    i2 = (int) (f2 / df + 0.5);
    ii = i1;
    mn = fbuf[i1];
    for (i = i1; i <= i2; i++) {
        if (mn > fbuf[i]) {
            mn = fbuf[i];
            ii = i;
        }
    }
    c = 2 * fbuf[ii] - (fbuf[ii+1] + fbuf[ii-1]);
    f = df * (ii + (fbuf[ii+1] - fbuf[ii-1]) / (2 * c));
    return (f);
}

#include <stdio.h>
#include <stdlib.h>

void
main(int ac, char **av)
{
    double  df, fr, fz1, fz2, r2d, cv, f1, f2, cnd;
    double  zmg, zph, zcr, zci, ycr, yci;
    int     i, n = MXNP;
    static float zc[MXNP];
    static float zm[MXNP / 4];
    static double eps = 1e-40;
    static double rate = 50000.0;
    static double fl = 8550;
    static double lencav = 2.0;
    static double diacav = 0.79375;	/* 5/16 in = 0.79375 cm */
/*
 *    1/4 in = 0.63500 cm
 *   5/16 in = 0.79375 cm
 *    3/8 in = 0.95250 cm
 */

    if (ac < 2) {
	printf("usage: zcav length [diameter]\n");
	printf("default: diameter=%.3f cm\n");
	exit(0);
    }
    lencav = atof(av[1]);
    if (ac > 2)
	diacav = atof(av[2]);

    c_tmp.x = lencav;
    c_tmp.y = diacav;
    printf("; zcav: len = %.1f (cm), dia = %.3f (cm), temp=%.0f deg C\n;\n", 
        lencav, diacav, TEMP);
    df = rate / n;
    zcav(zc, n / 4, df, lencav, diacav, TEMP);
    r2d = 45 / atan(1);
    for (i = 1; i < n / 4; i++) {
	zcr = zc[2 * i];
	zci = zc[2 * i + 1];
        ycr = zcr / (zcr * zcr + zci * zci);
        yci = -zci / (zcr * zcr + zci * zci);
	fr = i * df;
	zmg = _hypot(zcr, zci);
	if (zmg > eps) {
            zmg = 20 * log10(_hypot(zcr, zci));
            zph = atan2(zci, zcr) * r2d;
        } else {
            zmg = -800;
            zph = 0;
        }
        cnd = 10 * log10(max(eps, ycr));
	printf("%6.0f %9.3f %9.3f %9.3f\n", fr, zmg, zph, cnd);
	zm[i] = zmg;
    }
    for (i = 2; i < n / 4 - 1; i++) {
	if (zm[i] <= zm[i - 1] && zm[i] < zm[i + 1]) {
	    cv = 2 * zm[i] - (zm[i + 1] + zm[i - 1]);
	    fr = df * (i + (zm[i + 1] - zm[i - 1]) / (2 * cv));
	    break;
	}
    }
    fr = fl / lencav;
    f1 = limit(df, 0.5 * fr, rate / 4);
    f2 = limit(df, 1.5 * fr, rate / 4);
    fz1 = pkmn(f1, f2, df, zm);
    f1 = limit(df, 2.5 * fr, rate / 4);
    f2 = limit(df, 3.5 * fr, rate / 4);
    fz2 = pkmn(f1, f2, df, zm);
    printf("; fz1=%7.1f fz1*len=%7.1f\n", fz1, fz1 * lencav);
    printf("; fz2=%7.1f fz2*len=%7.1f\n", fz2, fz2 * lencav);
    for (i = 2; i < n / 4 - 1; i++) {
	if (zm[i] >= zm[i - 1] && zm[i] > zm[i + 1]) {
	    cv = 2 * zm[i] - (zm[i + 1] + zm[i - 1]);
	    fr = df * (i + (zm[i + 1] - zm[i - 1]) / (2 * cv));
	    break;
	}
    }
    printf("; fpk=%7.1f fpk*len=%7.1f\n", fr, fr * lencav);
}
