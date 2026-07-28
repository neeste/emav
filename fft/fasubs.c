/* fsubs.c */
#include <math.h>

static double pii = 3.1415926535898;
static double p7 = 0.70710678118655;

int
bitrev(int ii, int m)
{
    register int jj;

    jj = ii & 1;
    --m;
    while (--m > 0) {
	ii >>= 1;
	jj <<= 1;
	jj |= ii & 1;
    }
    return (jj);
}

/*
 * fr2tr - radix 2 iteration subroutine
 */
void
fr2tr(int iint, float *b0, float *b1)
{
    int     k;
    float   t;

    for (k = 0; k < iint; k++) {
	t = b0[k] + b1[k];
	b1[k] = b0[k] - b1[k];
	b0[k] = t;
    }
}

/*
 * fr4tr - radix 4 iteration subroutine
 */
void
fr4tr(int iint, int nn, float *b0, float *b1, float *b2, float *b3,
    float *b4, float *b5, float *b6, float *b7)
{
    double  arg, tpiovn;
    float   c1, c2, c3, s1, s2, s3, pr, pi, r1, r5;
    float   t0, t1, t2, t3, t4, t5, t6, t7;
    int     ii, int4, j, j0, ji, jl, jr, jlast, k, k0, kl, m, n, ni;

/*
 * jr steps two at a time to locate the real parts of intermediate results,
 * and ji locates the imaginary part corresponding to jr.
 */
    n = nn / 4;
    for (m = 1; (1 << m) < n; m++)
	continue;
    tpiovn = 2 * pii / nn;
    ji = 3;
    jl = 2;
    jr = 2;
    ni = (n + 1) / 2;
    for (ii = 0; ii < ni; ii++) {
	if (ii == 0) {
	    for (k = 0; k < iint; k++) {
		t0 = b0[k] + b2[k];
		t1 = b1[k] + b3[k];
		b2[k] = b0[k] - b2[k];
		b3[k] = b1[k] - b3[k];
		b0[k] = t0 + t1;
		b1[k] = t0 - t1;
	    }
	    if (nn > 4) {
		k0 = iint * 4;
		kl = k0 + iint;
		for (k = k0; k < kl; k++) {
		    pr = (float) (p7 * (b1[k] - b3[k]));
		    pi = (float) (p7 * (b1[k] + b3[k]));
		    b3[k] = b2[k] + pi;
		    b1[k] = pi - b2[k];
		    b2[k] = b0[k] - pr;
		    b0[k] += pr;
		}
	    }
	} else {
	    arg = tpiovn * bitrev(ii, m);
	    c1 = (float) cos(arg);
	    s1 = (float) sin(arg);
	    c2 = c1 * c1 - s1 * s1;
	    s2 = c1 * s1 + c1 * s1;
	    c3 = c1 * c2 - s1 * s2;
	    s3 = c2 * s1 + s2 * c1;
	    int4 = iint * 4;
	    j0 = jr * int4;
	    k0 = ji * int4;
	    jlast = j0 + iint;
	    for (j = j0; j < jlast; j++) {
		k = k0 + j - j0;
		r1 = b1[j] * c1 - b5[k] * s1;
		r5 = b1[j] * s1 + b5[k] * c1;
		t2 = b2[j] * c2 - b6[k] * s2;
		t6 = b2[j] * s2 + b6[k] * c2;
		t3 = b3[j] * c3 - b7[k] * s3;
		t7 = b3[j] * s3 + b7[k] * c3;
		t0 = b0[j] + t2;
		t4 = b4[k] + t6;
		t2 = b0[j] - t2;
		t6 = b4[k] - t6;
		t1 = r1 + t3;
		t5 = r5 + t7;
		t3 = r1 - t3;
		t7 = r5 - t7;
		b0[j] = t0 + t1;
		b7[k] = t4 + t5;
		b6[k] = t0 - t1;
		b1[j] = t5 - t4;
		b2[j] = t2 - t7;
		b5[k] = t6 + t3;
		b4[k] = t2 + t7;
		b3[j] = t3 - t6;
	    }
	    jr += 2;
	    ji -= 2;
	    if (ji <= jl) {
		ji = 2 * jr - 1;
		jl = jr;
	    }
	}
    }
}

/*
 * ford1 - in-place reordering subroutine
 */
void
ford1(int m, float *b)
{
    int     j, k, kl, n;
    float   t;

    k = 4;
    kl = 2;
    n = 1 << m;
    for (j = 4; j <= n; j += 2) {
	if (k > j) {
	    t = b[j - 1];
	    b[j - 1] = b[k - 1];
	    b[k - 1] = t;
	}
	k -= 2;
	if (k <= kl) {
	    k = 2 * j;
	    kl = j;
	}
    }
}

/*
 *-----------------------------------------------------------------------
 * subroutine:  ford2
 * in-place reordering subroutine
 *-----------------------------------------------------------------------
 */
void
ford2(int m, float *b)
{
    int     ji, ij, n;
    float   t;

    n = 1 << m;
    for (ij = 0; ij <= (n - 2); ij += 2) {
	ji = bitrev(ij >> 1, m) << 1;
	if (ij < ji) {
	    t = b[ij];
	    b[ij] = b[ji];
	    b[ji] = t;
	    t = b[ij + 1];
	    b[ij + 1] = b[ji + 1];
	    b[ji + 1] = t;
	}
    }
}

