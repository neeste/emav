/* decide.c */

#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include "menu.h"

static int decide_xoff = 0, decide_yoff = 0;

/* general decision making box
   input : set	- when is one, a yes or no is needed
		  when is zero, any key is O.K.
	   item	- number of lines of messages
	   s	- the first line of message
   output: 1	- if yes
	   0	- if no
*/

int
decide(int set, int item, char *s, ...)
{
    int     c, cw, j, k, n, saved, x1, y1, mxpos, mypos;
    int     xpix, ypix, tfgc_s, tbgc_s, twl_s;
    WIND    YorN;
    void    (*old) ();
    char  *addr;
    va_list ap;

    if (pgm_done())
        return (0);
    gr_scrsiz(&xpix, &ypix);
    x1 = xpix / 2 + decide_xoff;
    y1 = ypix / 8 + decide_yoff;
    cw = txtpar.font_width;
    twl_s = txtpar.text_wind_len;
    rd_text_color(&tfgc_s, &tbgc_s);
    k = 0;
    addr = s;
    va_start(ap, s);
    for (c = 0; c < item; c++) {
	j = strlen(addr);
	if (j > k)
	    k = j;
        addr = va_arg(ap, char *);
    }
    va_end(ap);
    if (k < 8)
	k = 8;
    if (k > 80)
	k = 80;
    k += 2;
    k *= cw;
    if (k >= xpix)
	k = xpix - 1;
    txtpar.text_wind_len = k / cw;
    c = k / 2;
    if (x1 < c)
        x1 = c;
    n = (item + (set>0)) * txtpar.menu_height;
    YorN.xtop = x1 - c;
    YorN.xbot = YorN.xtop + k;
    YorN.ytop = y1;
    YorN.ybot = YorN.ytop + n;
    old = get_bg_exe();
    set_bg_exe((void (*) ()) 0);
    saved = save_w(&YorN);
    wind_color(&YorN, txtpar.dfg, txtpar.dbg);
    j = draw_w(&YorN);
    k = YorN.ytop + txtpar.font_height;
    addr = s;
    va_start(ap, s);
    for (n = 0; n < item; n++) {
	(void) gprintf(x1 - (cw * strlen(addr)) / 2, k, addr);
	k += txtpar.menu_height;
        addr = va_arg(ap, char *);
    }
    va_end(ap);
    if (set > 0) {
	n = YorN.ybot - txtpar.menu_height;
	mouse_exclude(YorN.xtop, n, YorN.xbot, n);
	gr_line(YorN.xtop, n, YorN.xbot, n, menucolor.boxc);
	mouse_show(1);
	(void) gprintf(j, k, "Yes");
	j += c;
	c += YorN.xtop;
	mouse_exclude(c, n, c, YorN.ybot);
	gr_line(c, n, c, YorN.ybot, menucolor.boxc);
	mouse_show(1);
	(void) gprintf(j, k, "No");
	for (;;) {
	    j = getevent();
	    if (toupper(j) == 'Y') {
		n = 1;
		break;
	    } else if (toupper(j) == 'N' || j == 3) {
		n = 0;
		break;
	    } else if (j == SINGLE_CLICK) {
	    	mouse_position(&mxpos, &mypos);
		if (mypos <= YorN.ybot && mypos >= n &&
		    mxpos >= YorN.xtop && mxpos <= YorN.xbot) {
		    n = 1 - mxpos / c;
		    break;
		}
	    }
	}
    } else if (set < 0) {
	n = wait_event(-set);	// wait "set" ms
    } else {
	n = getevent();
    }
    if (saved)
	close_w(&YorN);
    text_color(tfgc_s, tbgc_s);
    txtpar.text_wind_len = twl_s;
    set_bg_exe(old);
    return (n);
}

void
decide_offset(int x, int y)
{
    decide_xoff = x;
    decide_yoff = y;
}

int
wait_event(int sec)
{
    int ev = 0, et = 0;
    long  t0;

    t0 = clock();
    et = 0;
    while (et < sec) {
	if (check_event()) {
	    ev = getevent();
	    break;
	}
	et = ((clock() - t0) * 1000) / CLOCKS_PER_SEC;
    }
    return (ev);
}

