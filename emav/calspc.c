/* calspc.c - computes spectrum of CAL file */

#include <stdio.h>
#include <stdint.h>
#include <io.h>
#include <math.h>
#include <token.h>

#define MAXNPTS	8192
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

double a_scale, b_scale;
float a[MAXNPTS + 2];
float b[MAXNPTS + 2];
int     npts;
int32_t    rate;
TOKENFILE *t;

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
spec(float *a, int k)
{
    int     i, n, nf, ir, ii;
    double  mg, db, ph, sc, rd;
    short  *ia;

    ia = (short *) a;
    tread(ia, k, t);
    sc = t->car.scale * 1000.0;
    rd = 180 / M_PI;
    n = npts;
    for (i = n - 1; i >= 0; i--)
        a[i] = ia[i] * sc;
    fast(a, n);
    nf = n / 2;
    for (i = 0; i < nf; i++) {
        ir = 2 * i;
        ii = ir + 1;
        mg = hypot(a[ir], a[ii]);
        if (mg < 1e-20) {
            db = -400;
            ph = 0;
        } else {
            db = 20 * log10(mg);
            ph = atan2(a[ii], a[ir]) * rd;
        }
        a[ir] = db;
        a[ii] = ph;
    }
}

void
wrdata()
{
    double df;
    int i, ir, ii, nf;

    df = rate / (double) npts;
    nf = npts / 2;
    printf("; freq     Adb     Aph     Bdb     Bph\n");
    for (i = 0; i < nf; i++) {
        ir = i * 2;
        ii = ir + 1;
        printf("%6.0f %7.1f %7.1f %7.1f %7.1f\n", i * df,
            a[ir], a[ii], b[ir], b[ii]);
    }
}

void
usage()
{
    printf("usage:\n");
    printf("    calspc file ...\n");
    printf("\n");
    printf("where file is an EmAv CAL file.\n");
    exit(0);
}

int
main(int ac, char **av)
{
    char    dfn[80];
    int   i, nto2;

    if (ac < 2)
	usage();
    while (ac > 1) {
	if (av[1][0] == '-') {
            ;
	} else {
	    strcpy(dfn, av[1]);
	    newext(dfn, "cal");
	    if (_access(dfn, 0) != 0) {
		fprintf(stderr, "can't open %s\n", dfn);
	    } else {
		t = topen(dfn);
		i = (int32_t) t->header.power10;
		rate = (int32_t) t->header.isf;
		while (i-- > 0)
			rate *= 10;
		npts = (int) t->hitch.toksiz;
		nto2 = 1;
		while (nto2 < npts)
		    nto2 *= 2;
                if (npts != nto2) {
                    fprintf(stderr, "npts (=%d) is not a power of 2\n", npts);
                    exit(1);
                }
		printf("; %s - npts=%d\n", dfn, npts);
                spec(a, 1);
                spec(b, 2);
		wrdata();
	    }
	}
	ac--;
	av++;
    }
    return (0);
}
