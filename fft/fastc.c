/* fastc.c */

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

/* fast -  replaces the real vector b with its Fourier transform
 */
void
fast(float *b, int n)
{
    int     iint, nn, n4pow, it, m;

    for (m = 1; m <= MAXPOW2 && n != (1 << m); m++)
	continue;
    if (m > MAXPOW2) {
	(void) fprintf(stderr, "fast: n is not a power of two\n");
	exit(1);
    }
    n4pow = m / 2;
/*
 * do a radix 2 iteration first if one is required.
 */
    if (m <= n4pow * 2)
	nn = 1;
    else {
	nn = 2;
	iint = n / nn;
	fr2tr(iint, b, b + iint);
    }
/*
 * perform radix 4 iterations.
 */
    if (n4pow != 0)
	for (it = 0; it < n4pow; it++) {
	    nn = nn * 4;
	    iint = n / nn;
	    fr4tr(iint, nn, b, b + iint, b + 2 * iint, b + 3 * iint,
		b, b + iint, b + 2 * iint, b + 3 * iint);
	}
/*
 * perform in-place reordering.
 */
    ford1(m, b);
    ford2(m, b);
    for (it = 3; it < n; it += 2)
	b[it] = -b[it];
    b[n] = b[1];
    b[1] = 0.0;
    b[n + 1] = 0.0;
}
