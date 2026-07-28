#include <stdio.h>
#include "fftlib.h"


/*
#-----------------------------------------------------------------------
# main program: fastmain  -  fast fourier transforms
# authors:      g. d. bergland and m. t. dolan
#               bell laboratories, murray hill, new jersey 07974
#
# input:        the program calls on a random number
#               generator for input and checks dft and
#               idft with a 32-point sequence
#-----------------------------------------------------------------------
*/
int
main()
{
    int     i, j, m, n;
    float   x[32], b[34];
/*
# generate random numbers and store array in b so
# the same sequence can be used in all tests.
# note that b is dimensioned to size n+2.
*/
    for (i = 0; i < 32; i++) {;
	x[i] = (float) uni();
	b[i] = x[i];
    }
    m = 5;
    n = 1 << m;
/*
# test fast-fsst then ffa-ffs
*/
    printf("\n\nTest fast and fsst\n");
    printf("\nreal input sequence\n");
    for (i = 0; i < n; i = i + 4) {
	for (j = i; j < i + 4; j++)
	    printf("%17.8f", b[j]);
	printf("\n");
    }
    fast(b, n);
    printf("\nreal components of transform\n");
    for (i = 0; i < n + 2; i = i + 8) {
	for (j = i; j < i + 8 && j < n + 2; j = j + 2)
	    printf("%17.8f", b[j]);
	printf("\n");
    }
    printf("\nimag components of transform\n");
    for (i = 1; i < n + 2; i = i + 8) {
	for (j = i; j < i + 8 && j < n + 2; j = j + 2)
	    printf("%17.8f", b[j]);
	printf("\n");
    }
    fsst(b, n);
    printf("\nreal inverse transform\n");
    for (i = 0; i < n; i = i + 4) {
	for (j = i; j < i + 4; j++)
	    printf("%17.8f", b[j]);
	printf("\n");
    }
	return (0);
}
