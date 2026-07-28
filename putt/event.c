/* event.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <menu.h>
#include <dsp.h>
#include "screen.h"
#include "color.h"
#include "version.h"
#include "putt.h"

extern int xpos_r, ypos_r;

float   refresh_time = 0;
static int32_t timing = 0;

/* display the verison number and copyright info */
int
display_version()
{
    dsp_init();
    decide(0, 6, VERSION, PGM_NAME, COPYRIGHT, BTNRH, dspapi(), dspdev());
    return (0);
}

/* check the refresh timing */
int
check_timing()
{
    if(lookahead() == 0) {
	if(refresh_time > 0) {
	    if(timing < clock()) {
                return (1);
	    }
	}
    }
    return (0);
}

void
reset_timing()
{
    timing = clock() + (int32_t) (refresh_time * CLOCKS_PER_SEC);
}

/* capture events */
void
proc_event(int c)
{
    static int d = 1;

    if (c == 23) {
	putbackevent(0);
	prn_screen();
    }
    if(d) {
	if (c == (FN | 25) || c == (FN | 67)) {
	    putbackevent(0);
	    d = 0;
	    decide(0, 2, "Program paused",
		"Press any key or mouse to continue");
	} else if (c == (FN | 59)) {
	    putbackevent(0);
	    d = 0;
	    display_version();
	}
	d = 1;
    }
}

int     dfgc, xpix, ypix;

void
init_graphics()
{
    int k;
    extern int mono_screen;

    set_menu_colors();
    set_text_par();
    init_gr();
    gr_scrsiz(&xpix, &ypix);		/* get screen size */
    if(mono_screen) {
	for (k = 1; k < 8; k++) gr_remap_palette(k, 0);
	for (k = 8; k < 16; k++) gr_remap_palette(k, 7);
        txtpar.mono = 1;
    }
    set_capture_event(proc_event);
}

void
wait_for_key(double s)
{
    int32_t t;

    t = clock() + (int32_t) (s * CLOCKS_PER_SEC);
    while (clock() < t && !check_event())
        continue;
}

