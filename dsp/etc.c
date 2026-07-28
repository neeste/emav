/* etc.c */

#include "menu.h"
#include "savage.h"

void
zero_all()
{
    int     i;

    for (i = 0; i < buflen; i++) {
	accbuf_a[i] = 0;
	accbuf_b[i] = 0;
	outbuf[i] = 0;
    }
}

