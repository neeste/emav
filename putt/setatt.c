/* setatt.c */

#include <stdio.h>
#include <math.h>
#include <menu.h>
#include <util.h>
#include "savage.h"

/* set_extern_att - sets external attenuator as close as possible
 to the requested value. (Dummy version always returns 0.) */
 
static int
set_extern_att(int channel, int att_db)
{
    att_db = channel ? 0 : 0;
    return (att_db);
}

static void
set_intern_att(float att)
{
    int     i = 0;

    for (i = 0; i < buflen; i++)
	outbuf[i] = (int) ((float) stimulus[i] / att);
}

int
recalstim(int sm)
{
    float ext, att;

    att = limit(0, attenu.att1, 100);
    ext = (float) set_extern_att(0, nint(att));
    attenu.intern_att = (float) pow(10.0, (att - ext) / 20);
    set_intern_att(attenu.intern_att);
    attenu.att1 = att;
    attenu.att2 = 0;
    return(0);
}

