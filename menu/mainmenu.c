/* mainmenu.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "menu.h"
#include "fk.h"

#define MAX_ITEMS  32

int     pgm_done();
void    prn_screen();

static int step = 0;	/* previously gloabal */

static int active_menu = 1, action = 0, items = 0;
static int _exit_flg_ = 0, xsub = -1, xdef = 320;
static MAIN_MENU *mm;

void
mainmenu_init(MAIN_MENU * m)
{
    mm = m;
    active_menu = 0;
    for (items = 0; (items < MAX_ITEMS) && (mm[items].menustr != NULL); items++)
	continue;
}

void
mainmenu_exit()
{
    _exit_flg_ = 1;
}

void
mainmenu_clear()
{
    gr_rectf(0, 0, items * step, txtpar.menu_height, txtpar.cbg);
}

int
menu_x()
{
    return ((xsub < 0) ? xdef : xsub);
}

int
menu_step(int n)
{
    return (n * step);
}

static char
draw_menu(int i, int active)
{
    WIND    w;
    char   *s, c;
    int     loc, x, y;

    w.xtop = i * step;
    w.xbot = w.xtop + step - 1;
    w.ytop = 0;
    w.ybot = txtpar.menu_height;
    if (active)
	wind_color(&w, menucolor.afg, menucolor.abg);
    else
	wind_color(&w, menucolor.mfg, menucolor.mbg);
    x = draw_w(&w);
    y = txtpar.font_height;
    s = strchr(mm[i].menustr, '&');
    if (s == NULL) {
        (void) gprintf(x, y, "%s", mm[i].menustr);
        c = 0;
    } else {
        loc = s++ - mm[i].menustr;
        (void) gprintf(x, y, "%.*s%s", loc, mm[i].menustr, s);
        underline(x + 8 * loc, y);
        c = *s;
    }
    return (c);
}

int
mainmenu_show(char *key)
{
    int     i, j, m, n, k, xpix, ypix;

    m = 0;
    for (i = 0; i < items; i++) {
	n = strlen(mm[i].menustr);
	if (n > m)
	    m = n;
    }
    j = (m + 1) * 8;
    gr_scrsiz(&xpix, &ypix);
    step = xpix / items;
    if (step > j)
	step = j;
    for (i = 0; i < items; i++) {
	k = draw_menu(i, (i == active_menu));
	if (key != NULL)
	    key[i] = k;
    }
    return (items);
}

void
mainmenu_driver()
{
    int     i, j, n, c, mx, my;
    char    keys[MAX_ITEMS];

    (void) mainmenu_show(keys);
    i = active_menu = 0;
    _exit_flg_ = action = 0;
    while (!pgm_done()) {
	while (i < 0)
	    i += items;
	while (i >= items)
	    i -= items;
	if (i != active_menu) {
	    (void) draw_menu(active_menu, 0);
	    (void) draw_menu(i, 1);
	    active_menu = i;
	}
	if (action) {
            xsub = step * active_menu;
	    c = mm[active_menu].exe();
	    putbackevent(c);
	    for (n = 0; n < items; n++)
		(void) draw_menu(n, (active_menu == n));
	    action = (c == FK_Right_Arrow || c == FK_Left_Arrow);
	}
	if (_exit_flg_)
	    break;
    	n = getevent();
	if (_exit_flg_)
	    break;
	c = (n & FN) ? n : toupper(n);
	for (j = 0; j < items; j++)
	    if (c == keys[j])
		break;
	if (j < items) {
	    i = j;
	    action = 1;
	} else {
	    switch (n) {
	    case Ctrl_C:
		return;
	    case Ctrl_P:
	    case Ctrl_W:
		prn_screen();
		break;
	    case LEFT_CLICK:
		mouse_position(&mx, &my);
		n = (mx + 1) / step;
		if (my <= txtpar.menu_height && n < items) {
		    i = n;
		    action = 1;
		}
		break;
	    case '\r':
	    case FK_Down_Arrow:
		action = 1;
		break;
	    case FK_Right_Arrow:
		i = active_menu + 1;
		break;
	    case FK_Left_Arrow:
		i = active_menu - 1;
		break;
	    }
	}
    }
}
