/* dcremo.c */

#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <util.h>

int32_t
rescale(float *longbuf, short *intbuf, int n, int maxfs)
{
    double  scale, val, maxval;
    int     i;

    maxval = 0;
    for (i = 0; i < n; i++) {
	val = longbuf[i];
	if (maxval < val)
	    maxval = val;
	if (maxval < -val)
	    maxval = -val;
    }
    if (maxval <= maxfs || maxval <= 0) {
	for (i = 0; i < n; i++)
	    intbuf[i] = (int) longbuf[i];
    } else {
	scale = maxfs / maxval;
	for (i = 0; i < n; i++)
	    intbuf[i] = nint(longbuf[i] * scale);
    }
    return ((int32_t)maxval);
}
