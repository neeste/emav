#include <stdlib.h>

double
r1unif(int icseed, int itseed, int ibyte[4], int ifcn)
{
/*
#
#  r1unif - output, the uniform random number if ifcn .ne. 0
#  icseed - input, the new congruential seed if ifcn = 0
#  itseed - input, the new tausworthe seed if ifcn = 0
#  ibyte  - output, the bits of r1unif, 8 per word, if ifcn = 2
#  ifcn   - input, = 0 for initialization
#                  = 1 if only the value of r1unif is of interest
#                  = 2 if both r1unif and ibyte are of interest
#
#  this is a portable fortran implementation of uni, a
#  uniform random number generator on (0.0, 1.0) devised
#  by marsaglia, et. al., and included in their package
#  called "super-duper".
#
#  two independent 32 bit generators are maintained internally and
#  updated for each call.
#
#  the first of these is a congruential generator with
#  multiplier 69069 (=16*64**2 + 55*64 + 13).
#
#  the second is a tausworthe or shift-register generator.
#  this generator takes the seed, shifts it right 15 bits, exclusive
#  ors it with itself, shifts the result 17 bits to the left, and
#  exclusive ors the shifted result with itself (not with the
#  original seed).  the output of the procedure is the tausworthe
#  random number and is used as the seed for the next call.
#
#  finally, the output from the two generators is
#  exclusively or-ed together.
#
#  the following program should work on any 16+ bit computer.
#
*/
    double  unif;
    int     iscr[5];
    int     i, i1, i2, j1, j2, it1, it2, icarry, k;
    static int xor[30] = {
	1, 2, 3, 3, 2, 1, 4, 5, 6, 7, 5, 4, 7, 6, 1, 6, 7, 4, 5, 2, 3, 7, 6, 5, 4, 3, 2, 1, 0};
    static int first = 1, jcseed = 12345, jtseed = 1073;
/*
#
#  initialize cseed and tseed for portability
#
*/
    static int cseed[6] = {6 * 0};
    static int tseed[32] = {32 * 0};

    unif = 0.0;
    if (first || ifcn <= 0) {
	if (ifcn <= 0) {
/*
#
#     take user values as seeds
#
*/
	    jcseed = abs(icseed);
	    jtseed = abs(itseed);
	}
	first = 0;
/*
#
#.....decode seeds
#
*/
	cseed[0] = jcseed;
	for (i = 0; i < 5; i++) {
	    cseed[i + 1] = cseed[i] / 64;
	    cseed[i] = cseed[i] - cseed[i + 1] * 64;
	}
	cseed[5] = cseed[5] % 4;
/*
#
#     ensure odd unless zero
#
*/
	if (jcseed != 0 && (cseed[0] % 2) == 0)
	    cseed[0] = cseed[0] + 1;
	tseed[0] = jtseed;
	for (i = 0; i < 11; i++) {
	    tseed[i + 1] = tseed[i] / 2;
	    tseed[i] = tseed[i] - tseed[i + 1] * 2;
	}
/*
#
#     only use initial value mod 2048
#
*/
	for (i = 11; i < 32; i++)
	    tseed[i] = 0;
/*
#
#     ensure odd unless zero
#
*/
	if (jtseed != 0)
	    tseed[0] = 1;
/*
#
#     end of initialization
#
*/
	if (ifcn == 0)
	    return (0.0);
    }
/*
#
#.....tausworthe generator -- shift right 15, then left 17
#
*/
    for (i = 0; i < 17; i++)
	tseed[i] = abs(tseed[i] - tseed[i + 15]);
    for (i = 17; i < 32; i++)
	tseed[i] = abs(tseed[i] - tseed[i - 17]);
/*
#
#.....congruential generator -- multiplication in base 64
#
#     multiply base 64
#
*/
    cseed[5] = 13 * cseed[5] + 55 * cseed[4] + 16 * cseed[3];
    cseed[4] = 13 * cseed[4] + 55 * cseed[3] + 16 * cseed[2];
    cseed[3] = 13 * cseed[3] + 55 * cseed[2] + 16 * cseed[1];
    cseed[2] = 13 * cseed[2] + 55 * cseed[1] + 16 * cseed[0];
    cseed[1] = 13 * cseed[1] + 55 * cseed[0];
    cseed[0] = 13 * cseed[0];
    k = -6;
    icarry = 0;
    for (i = 0; i < 5; i++) {
	k = k + 6;
	cseed[i] = cseed[i] + icarry;
	icarry = cseed[i] / 64;
	cseed[i] = cseed[i] - 64 * icarry;
	i2 = cseed[i] / 8;
	i1 = cseed[i] - 8 * i2;
	j1 = 4 * tseed[k + 2] + tseed[k + 1] + tseed[k + 1] + tseed[k];
	j2 = 4 * tseed[k + 5] + tseed[k + 4] + tseed[k + 4] + tseed[k + 3];
	it1 = 28;
	if (i1 > j1)
	    it1 = (i1 * i1 - i1) / 2 + j1;
	if (i1 < j1)
	    it1 = (j1 * j1 - j1) / 2 + i1;
	it2 = 28;
	if (i2 > j2)
	    it2 = (i2 * i2 - i2) / 2 + j2;
	if (i2 < j2)
	    it2 = (j2 * j2 - j2) / 2 + i2;
	iscr[i] = 8 * xor[it2] + xor[it1];
	unif = (unif + iscr[i]) / 64.0;
    }
    cseed[5] = (cseed[5] + icarry) % 4;
    j1 = tseed[30] + tseed[31] + tseed[31];
    it1 = abs(cseed[5] - j1);
    if (it1 == 1 && (cseed[5] + j1) == 3)
	it1 = 3;
    unif = (unif + it1) / 4.0;
    if (ifcn != 1) {
	ibyte[3] = iscr[0] + (iscr[1] % 4) * 64;
	ibyte[2] = iscr[1] / 4 + (iscr[2] % 16) * 16;
	ibyte[1] = iscr[2] / 16 + iscr[3] * 4;
	ibyte[0] = iscr[4] + it1 * 64;
    }
    return (unif);
}

/*
#
#-----------------------------------------------------------------------
#  function:  uni
#  author:    alan m. gross
#             bell laboratories, murray hill, new jersey 07974
#  portable random number generator
#-----------------------------------------------------------------------
#
*/
double
uni()
{
    int     ibyte[4];
    static int icseed = 0, itseed = 0, ifcn = 1;

/*
#
#  uni is returned as a single real random variate
#  from the uniform distribution 0.0 .le. uni .lt. 1.0 .
#
#  ifcn = 1 implies that icseed, itseed, ibyte, and k are ignored.
#
*/
    return (r1unif(icseed, itseed, ibyte, ifcn));
}

