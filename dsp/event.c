/* event.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <menu.h>

int set_menu_colors();
int set_text_par();

float   refresh_time = 5;   /* sec */
static int32_t timing = 0;

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
    }
    if(d) {
	if (c == (FN | 25) || c == (FN | 67)) {
	    putbackevent(0);
	    d = 0;
	    (void) decide(0, 2, "Program paused",
		"Press any key or mouse to continue");
	} else if (c == (FN | 59)) {
	    putbackevent(0);
	    d = 0;
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
}
