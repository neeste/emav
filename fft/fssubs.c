/* fssubs.c */

#include <math.h>

int bitrev(int, int);

static double pii = 3.1415926535898;
static double p7two = 1.414213562731;

/*
 * fr4syn - radix 4 synthesis
 */
void
fr4syn(int jnt, int nn, float *b0, float *b1, float *b2, float *b3,
    float *b4, float *b5, float *b6, float *b7)
{
    double  arg, tpiovn;
    float   c1, c2, c3, s1, s2, s3;
    float   t0, t1, t2, t3, t4, t5, t6, t7;
    int     ii, j, j0, ji, jr, jl, jlast, jnt4, k, k0, kl, m, n, ni;

    tpiovn = 2 * pii / nn;
    ji = 3;
    jl = 2;
    jr = 2;
    n = nn / 4;
    for (m = 1; (1 << m) < n; m++)
	continue;
    ni = (n + 1) / 2;
    for (ii = 0; ii < ni; ii++) {
	if (ii == 0) {
	    for (k = 0; k < jnt; k++) {
		t0 = b0[k] + b1[k];
		t1 = b0[k] - b1[k];
		t2 = b2[k] * 2;
		t3 = b3[k] * 2;
		b0[k] = t0 + t2;
		b2[k] = t0 - t2;
		b1[k] = t1 + t3;
		b3[k] = t1 - t3;
	    }
	    if (nn > 4) {
		k0 = jnt * 4;
		kl = k0 + jnt;
		for (k = k0; k < kl; k++) {
		    t2 = b0[k] - b2[k];
		    t3 = b1[k] + b3[k];
		    b0[k] = (b0[k] + b2[k]) * 2;
		    b2[k] = (b3[k] - b1[k]) * 2;
		    b1[k] = (float) ((t2 + t3) * p7two);
		    b3[k] = (float) ((t3 - t2) * p7two);
		}
	    }
	} else {
	    arg = tpiovn * bitrev(ii, m);
	    c1 = (float) cos(arg);
	    s1 = (float) -sin(arg);
	    c2 = c1 * c1 - s1 * s1;
	    s2 = c1 * s1 + c1 * s1;
	    c3 = c1 * c2 - s1 * s2;
	    s3 = c2 * s1 + s2 * c1;
	    jnt4 = jnt * 4;
	    j0 = jr * jnt4;
	    k0 = ji * jnt4;
	    jlast = j0 + jnt;
	    for (j = j0; j < jlast; j++) {
		k = k0 + j - j0;
		t0 = b0[j] + b6[k];
		t1 = b7[k] - b1[j];
		t2 = b0[j] - b6[k];
		t3 = b7[k] + b1[j];
		t4 = b2[j] + b4[k];
		t5 = b5[k] - b3[j];
		t6 = b5[k] + b3[j];
		t7 = b4[k] - b2[j];
		b0[j] = t0 + t4;
		b4[k] = t1 + t5;
		b1[j] = (t2 + t6) * c1 - (t3 + t7) * s1;
		b5[k] = (t2 + t6) * s1 + (t3 + t7) * c1;
		b2[j] = (t0 - t4) * c2 - (t1 - t5) * s2;
		b6[k] = (t0 - t4) * s2 + (t1 - t5) * c2;
		b3[j] = (t2 - t6) * c3 - (t3 - t7) * s3;
		b7[k] = (t2 - t6) * s3 + (t3 - t7) * c3;
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

