/* =========================== event.c ================================= */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <menu.h>
#include <dsp.h>
#include "color.h"
#include "screen.h"
#include "savage.h"
#include "version.h"
#include "fk.h"
#include "emav.h"		/* function prototyes */

#define MXND 20

float   chkfit_time = 0;
float   refresh_time = 0;
static int32_t next_chkfit_time = 0;
static int32_t next_refresh_time = 0;

extern char dsp_code_file[MAXNAME];

/* display the verison number and copyright info */
int
display_version()
{
    decide(0, 6, VERSION, PGM_NAME, COPYRIGHT, BTNRH, dspapi(), dspdev());
    return (0);
}

/* display the verison number and copyright info */
int
device_list()
{
    char sel, *cur, *nam, *dev[MXND];
    int i, nd = MXND;

    cur = dspdev();
    for (i = 0; i < nd; i++) {
	dev[i] = (char *) malloc(84);
        nam = dspname(i);
	if (nam) {
	    sel = strcmp(nam, cur) ? ' ' : '>';
	    sprintf(dev[i], "%c%2d: %s", sel, i + 1, nam);
	} else {
	    nd = i;
	    free(dev[i]);
	    break;
	}
    }
    sel = decide(0, nd + 1, "Device List:",
	dev[ 0], dev[ 1], dev[ 2], dev[ 3],
	dev[ 4], dev[ 5], dev[ 6], dev[ 7],
	dev[ 8], dev[ 9], dev[10], dev[11],
	dev[12], dev[13], dev[14], dev[15],
	dev[16], dev[17], dev[18], dev[19]
    );
    if ((sel >= '1') && (sel <='9')) {
	sprintf(dsp_code_file, "%c", sel);
    }
    for (i = 0; i < nd; i++) {
        free(dev[i]);
    }
    return (0);
}

/* check the chkfit time */
int
check_chkfit_time()
{
    if(lookahead() == 0) {
	if(chkfit_time > 0) {
	    if(next_chkfit_time < clock()) {
                return (1);
	    }
	}
    }
    return (0);
}

void
reset_chkfit_time()
{
    next_chkfit_time = clock() + (int32_t) (chkfit_time * CLOCKS_PER_SEC);
}

/* check the refresh time */
int
check_refresh_time()
{
    if(lookahead() == 0) {
	if(refresh_time > 0) {
	    if(next_refresh_time < clock()) {
                return (1);
	    }
	}
    }
    return (0);
}

void
reset_refresh_time()
{
    next_refresh_time = clock() + (int32_t) (refresh_time * CLOCKS_PER_SEC);
}

/* capture events */
void
proc_event(int c)
{
    static int d = 1;
    static int dsp_active = 0;

    if (c == 23) {
	putbackevent(0);
	prn_screen();
    }
    if(d) {
	if (c == (FN | 25) || c == (FK_F9)) {
	    dsp_active = dsprst(TRUE);
	    putbackevent(0);
	    d = 0;
	    decide(0, 2, "Program paused",
		"Press any key to continue");
	} else if (c == (FK_F1)) {
	    dsp_active = dsprst(TRUE);
	    if (!dsp_active) {
		putbackevent(27);
		dsp_init();
	    }
	    putbackevent(0);
	    d = 0;
	    display_version();
	} else if (c == (FK_F2)) {
	    dsp_active = dsprst(TRUE);
	    if (!dsp_active) {
		putbackevent(27);
		dsp_init();
	    }
	    putbackevent(0);
	    d = 0;
	    device_list();
	} else if (c == 1) {
	    mainmenu_exit();
    	}
	if (dsp_active) {
	    dsprst(FALSE);
	    dsp_active = 0;
	}
	d = 1;
    }
}

int     dfgc, xpix, ypix;

void
init_graphics()
{
    set_menu_colors();
    set_text_par();
    init_gr();
    gr_scrsiz(&xpix, &ypix);		/* get screen size */
    set_capture_event(proc_event);
}
