/* fsstc.c */

#include <stdio.h>
#include <stdlib.h>

#define  MAXPOW2  31

void    fr2tr(int, float *, float *);
void    fr4syn(int, int, float *, float *, float *, float *,
            float *, float *, float *, float *);
void    fr4tr(int, int, float *, float *, float *, float *,
            float *, float *, float *, float *);
void    ford1(int, float *);
void    ford2(int, float *);

/*
 * fsst - Fourier synthesis subroutine
 */
void
fsst(float *b, int n)
{
    int     m, i, it, nn, jnt, n4pow;

    for (m = 1; m <= MAXPOW2 && n != (1 << m); m++)
	continue;
    if (m > MAXPOW2) {
	(void) fprintf(stderr, "fsst: n is not a power of two");
	exit(1);
    }
    b[1] = b[n];
/*
 * scale the input by n
 */
    for (i = 0; i < n; i++)
	b[i] = b[i] / n;
    n4pow = m / 2;
/*
 * scramble the inputs
 */
    for (i = 3; i < n; i += 2)
	b[i] = -b[i];
    ford2(m, b);
    ford1(m, b);
    if (n4pow != 0) {
	nn = 4 * n;
	for (it = 0; it < n4pow; it++) {
	    nn = nn / 4;
	    jnt = n / nn;
	    fr4syn(jnt, nn, b, b + jnt, b + 2 * jnt, b + 3 * jnt,
		b, b + jnt, b + 2 * jnt, b + 3 * jnt);
	}
    }
/*
 * do a radix 2 iteration if one is required
 */
    if (m > n4pow * 2) {
	jnt = n / 2;
	fr2tr(jnt, b, b + jnt);
    }
}
