/*  ============================= menu.c =================================
    This is the library for the simple menu system
    ====================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "menu.h"

#define Ctrl_C	        3
#define STACK_SIZE	32
#define MAX_LEN		256
#define MAX_STR		(MAX_LEN+1)

int     pgm_done();
void    g_savscr(int, int, int, int);
void    g_rstscr(int, int);

MENUCOLOR menucolor = {
    BLACK,			/* non-active text foreground */
    WHITE,			/* non-active text background */
    WHITE,			/* active text foreground */
    GREEN,			/* active text background */
    LIGHTGRAY,			/* disabled foreground */
    WHITE,			/* disabled background */
    DARKGRAY			/* box boundary color */
};

TEXTPAR txtpar = {
    13,				/* menu height */
    10,				/* font height */
    8,				/* font width */
    34,				/* text window length */
    YELLOW,			/* edit foreground color */
    BLUE,			/* edit background color */
    MAGENTA,			/* dialog foreground color */
    WHITE,			/* dialog background color */
    LIGHTRED,			/* cursor color */
    BLUE,			/* screen background color */
    0				/* monochrome flag */
};

static int tfgc = WHITE, tbgc = BLUE, cw = 8;
static int resume = 27;
static int ysub[STACK_SIZE], isub = 0;
static MENUITEM *submenu;
static WIND sub;

static struct {
    WIND   *h_wptr[STACK_SIZE];
    int     ptr;
} STACK = {{NULL}, 0};

static struct {
    void    (*exe)(void);
    int     flag;
} trailer = {NULL, 0};

void
set_resume(int c)
{
    resume = c;
}

int
get_resume()
{
    return (resume);
}

int
is_mono()
{
    return (txtpar.mono);
}

void
set_trailer(void (*func)(void))
{
    trailer.exe = func;
    trailer.flag = 1;
}

void
text_color(int fgc, int bgc)
{
    tfgc = fgc;
    tbgc = bgc;
}

void
rd_text_color(int *fg, int *bg)
{
    *fg = tfgc;
    *bg = tbgc;
}

void
wind_color(WIND * w, int fgc, int bgc)
{
    w->wfgc = fgc;
    w->wbgc = bgc;
}

int
num_items(MENUITEM * menu, int *len)
{
    int     num, i, j, maxlen;

    maxlen = 0;
    for (num = 0; num < MAX_LEN; num++) {
        if (menu[num].menustr == NULL)
	    break;
	i = menu[num].inputlen;
	if (menu[num].ptr2val) {
	    j = strlen(menu[num].ptr2val);
	 } else {
	    j = 0;
	 }
	if (j > i)
	    i = j;
	if (i > 24)
	    i = 24;
	i += strlen(menu[num].menustr);
	if (maxlen < i)
	    maxlen = i;
    }
    *len = maxlen;
    return (num);
}

static char
draw_sub(int i, int active, WIND * w)
{
    int     j, k, m, n;
    char   *s, *t, c;

    sub.xtop = w->xtop;
    sub.ytop = w->ytop + i * txtpar.menu_height + 1;
    sub.xbot = w->xbot;
    sub.ybot = sub.ytop + txtpar.menu_height;
    if (!submenu[i].status)
        wind_color(&sub, menucolor.dfg, menucolor.dbg);
    else if (active)
        wind_color(&sub, menucolor.afg, menucolor.abg);
    else
        wind_color(&sub, menucolor.mfg, menucolor.mbg);
    j = draw_w(&sub);
    k = sub.ytop + txtpar.font_height;
    m = (int) submenu[i].valtype;
    s = strchr(submenu[i].menustr, '&');
    if (s) {
        n = s++ - submenu[i].menustr;
    } else {
        s = submenu[i].menustr;
        n = 0;
    }
    c = toupper(*s);
    if (submenu[i].ptr2val == NULL)
        m = (int) NONE;

    switch (m) {
    case NONE:
        gprintf(j, k, "%.*s%s", n, submenu[i].menustr, s);
        break;
    case STRING:
        gprintf(j, k, "%.*s%s %s", n, submenu[i].menustr, s,
            submenu[i].ptr2val);
        break;
    case SHORT:
        gprintf(j, k, "%.*s%s %-*d", n, submenu[i].menustr, s,
            submenu[i].inputlen, *((short *) submenu[i].ptr2val));
        break;
    case INT:
        gprintf(j, k, "%.*s%s %-*d", n, submenu[i].menustr, s,
            submenu[i].inputlen, *((int *) submenu[i].ptr2val));
        break;
    case CHAR:
        gprintf(j, k, "%.*s%s %c", n, submenu[i].menustr, s,
            *submenu[i].ptr2val);
        break;
    case LONG:
        gprintf(j, k, "%.*s%s %-*ld", n, submenu[i].menustr, s,
            submenu[i].inputlen, *((long *) submenu[i].ptr2val));
        break;
    case FLOAT:
        gprintf(j, k, "%.*s%s %-*.*f", n, submenu[i].menustr, s,
            submenu[i].inputlen, submenu[i].decpts,
            *((float *) submenu[i].ptr2val));
        break;
    case DOUBLE:
        gprintf(j, k, "%.*s%s %-*.*lf", n, submenu[i].menustr, s,
            submenu[i].inputlen, submenu[i].decpts,
            *((double *) submenu[i].ptr2val));
        break;
    case TOGGLE:
        m = ((TOGGLETYPE *) (submenu[i].ptr2val))->at;
        if (active == 2)
            m++;
        t = ((TOGGLETYPE *) submenu[i].ptr2val)->strs[m];
        if (t == NULL)
            m = 0;
        ((TOGGLETYPE *) (submenu[i].ptr2val))->at = m;
        t = ((TOGGLETYPE *) submenu[i].ptr2val)->strs[m];
        gprintf(j, k, "%.*s%s %s", n, submenu[i].menustr, s, t);
    }
    j += cw * n;
    txtpar.text_wind_len -= (int) strlen(submenu[i].menustr);
    underline(j, k);
    return (c);
}

int
menu_y()
{
    return (isub ? ysub[isub] : txtpar.menu_height);
}

static int
submenu_driver(WIND * w)
{
    int     act = 0, index = 0, i = 0, ch, saved = 0, c;
    int     num_item, mx, my, cancel = 0;
    char    keys[20];

    saved = save_w(w);
    num_item = 0;
    while (submenu[num_item].menustr != NULL)
	num_item++;
    for (ch = 0; ch < num_item; ch++)
	if (ch != index)
	    keys[ch] = draw_sub(ch, 0, w);
    keys[index] = draw_sub(index, 1, w);
    i = index;
    while (!cancel) {
	if (i != index) {
	    (void) draw_sub(index, 0, w);
	    (void) draw_sub(i, 1, w);
	    index = i;
	}
	if (act && submenu[index].status) {
	    if (submenu[index].valtype != TOGGLE) {
		act = get_str(sub.xtop + 4 + (int) strlen(submenu[index].menustr)
		    * cw, sub.ytop + txtpar.font_height, submenu[index].inputlen,
		    submenu[index].decpts, submenu[index].valtype,
		    submenu[index].ptr2val);
		(void) draw_sub(index, 1, w);
	    } else {
		(void) draw_sub(index, act, w);
	    }
	    if (submenu[index].exe != NULL && act >= 0) {
		putbackevent(submenu[index].exe(&submenu[index].ptr2val));
		for (ch = 0; ch < num_item; ch++)
		    if (ch != index)
			(void) draw_sub(ch, 0, w);
		(void) draw_sub(index, 1, w);
	    }
	}
	ch = getevent();
	if (ch == Ctrl_C)
	    return (0);
	c = (ch & FN) ? ch : toupper(ch);
	if (c == resume || ch == (FN | 113)) {
	    ch = (ch == (FN | 113));
	    break;
	}
	for (act = 0; act < num_item; act++) {
	    if (c == keys[act])
		break;
	}
	if (act < num_item && submenu[act].status) {
	    i = act;
	    ch = '\r';
	} else {
	    i = index;
	}
	act = 1;
	switch (ch) {
	case LEFT_CLICK:
	    mouse_position(&mx, &my);
	    if ((mx > w->xbot) || (my > w->ybot) ||
		(mx < w->xtop) || (my < w->ytop)) {
		cancel = 1;
                ch = 0;
		break;
	    }
	    i = (my - w->ytop) / txtpar.menu_height;
	    if (i >= num_item || !submenu[i].status) {
		i = index;
		act = 0;
	    } else if (i == index && submenu[i].valtype == TOGGLE) {
		act++;
	    }
	    break;
	case '\r':		/* Return */
	    if (submenu[i].valtype == TOGGLE)
		act++;
	    break;
	case (FN | 72):	/* Up-Arrow */
	case (FN | 15):	/* Shift-Tab */
	    do {
		i--;
		if (i < 0)
		    i = num_item - 1;
	    } while (!submenu[i].status);
	    act = 0;
	    break;
	case (FN | 80):	/* Down-Arrow */
	case 9:		/* Tab */
	    do {
		i++;
		if (i >= num_item)
		    i = 0;
	    } while (!submenu[i].status);
	    act = 0;
	    break;
	case (FN | 75):	/* Left-Arrow */
	case (FN | 77):	/* Right-Arrow */
            cancel = 1;
	    break;
	default:
	    act = 0;
	    if (submenu[num_item].exe != NULL)
		putbackevent(submenu[num_item].exe(ch));
	}
    }
    if (saved)
        close_w(w);
    return (ch);
}

int
simple_submenu(int x, int y, MENUITEM *m)
{
    int     retval, len, xpix, ypix;
    MENUITEM *prvmenu;
    WIND    w;

    if (pgm_done())
        return (Ctrl_C);
    cw = txtpar.font_width;
    retval = num_items(m, &len);
    w.xtop = x;
    w.xbot = w.xtop + len * cw + cw;
    w.ytop = y;
    w.ybot = w.ytop + retval * txtpar.menu_height + 1;
    gr_scrsiz(&xpix, &ypix);
    if (w.xbot > xpix - 1)
	w.xbot = xpix - 1;
    prvmenu = submenu;
    submenu = m;
    if (isub < STACK_SIZE)
        isub++;
    ysub[isub] = w.ybot;
    retval = submenu_driver(&w);
    if (isub > 0)
        isub--;
    if (trailer.flag) {
        trailer.flag = 0;
        if (trailer.exe != NULL)
            trailer.exe();
    }
    submenu = prvmenu;
    return (retval);
}

void
underline(int x, int y)
{
    y++;
    mouse_exclude(x, y, x + cw, y);
    gr_line(x, y, x + cw, y, tfgc);
    mouse_show(1);
}

void
close_w(WIND * w)
{
    if (STACK.ptr > 0) {
	STACK.ptr--;
        w = STACK.h_wptr[STACK.ptr];
        mouse_exclude(w->xtop, w->ytop, w->xbot, w->ybot);
	g_rstscr(w->xtop, w->ytop);
	mouse_show(1);
    }
}

int
draw_w(WIND * w)
{
    int     i;

    text_color(w->wfgc, w->wbgc);
    mouse_exclude(w->xtop, w->ytop, w->xbot, w->ybot);
    gr_rectf(w->xtop, w->ytop, w->xbot, w->ybot, tbgc);
    gr_recto(w->xtop, w->ytop, w->xbot, w->ybot, menucolor.boxc);
    mouse_show(1);
    i = w->xbot - w->xtop - 3;
    txtpar.text_wind_len = (i - 4) / cw;
    return (w->xtop + 4);
}

int
save_w(WIND * w)
{
    if (STACK.ptr < STACK_SIZE) {
	mouse_exclude(w->xtop, w->ytop, w->xbot, w->ybot);
	g_savscr(w->xtop, w->ytop, w->xbot, w->ybot);
	mouse_show(1);
	STACK.h_wptr[STACK.ptr] = w;
	STACK.ptr++;
	return (1);
    }
    return (0);
}

void
clr_scr()
{
    mouse_show(0);
    gr_clear(txtpar.cbg);
    mouse_show(1);
}

static void
underdot(int x, int y)
{
    gr_setpix(x + 2, y, tfgc);
    gr_setpix(x + 4, y, tfgc);
    gr_setpix(x + 6, y, tfgc);
}

int
gprintf(int x, int y, char *fmt, ...)
{
    char    s[MAX_STR];
    va_list arg_ptr;
    int     i, w;

    va_start(arg_ptr, fmt);
    (void) vsprintf(s, fmt, arg_ptr);
    va_end(arg_ptr);

    i = strlen(s);
    w = txtpar.text_wind_len;
    cw = txtpar.font_width;
    mouse_exclude(x, y - txtpar.font_height, x + w * cw, y);
    if (i > w) {
	i = w;
	s[i] = 0;
	underdot(x + (w - 1) * cw, y);  /* draw underdot */
    } else if (i == w) {                /* erase underdot */
        gr_rectf(x + (w - 1) * cw, y, x + w * cw - 1, y, tbgc);
    }
    gr_settc(tfgc, tbgc);
    gr_text(x, y, s);
    mouse_show(1);
    return (i);
}

static void
char_box(int x, int y, int c)
{
    gr_recto(x, y-txtpar.font_height+2, x+8, y, c);
}

static void
char_fill(int x, int y, int c)
{
    gr_rectf(x, y-txtpar.font_height+2, x+8, y, c);
}

static void
strcopy(char *d, char *s)
{
    while(*s)
        *d++ = *s++;
    *d = '\0';
}

int
get_str(int x, int y, int len, int dec, enum TYPE type, char *s)
{
    char    str[MAX_STR] = {0};
    int     i = 0, n = 0, c, L, j;
    int     bbf, bbb, mx, my;
    union __u {
	double  dn;
	float   fn;
	long    ln;
	short   sn;
	int     in;
	char    cn;
    }      *p;

    if (len <= 0 || s == NULL || type == NONE)
	return (0);
    p = (union __u *) s;
    bbf = tfgc;
    bbb = tbgc;
    tfgc = txtpar.gfg;
    tbgc = txtpar.gbg;

    switch (type) {
    case STRING:
	strcopy(str, s);
	break;
    case SHORT:
	sprintf(str, "%-*d", len, *((short *) s));
	break;
    case INT:
	sprintf(str, "%-*d", len, *((int *) s));
	break;
    case CHAR:
	*str = *s;
	break;
    case LONG:
	sprintf(str, "%-*ld", len, *((long *) s));
	break;
    case FLOAT:
	sprintf(str, "%-*.*f", len, dec, *((float *) s));
	break;
    case DOUBLE:
	sprintf(str, "%-*.*f", len, dec, *((double *) s));
	break;
    case NONE:
    case TOGGLE:
	return (0);
    }
    L = strlen(str);
    for (;;) {
 	mouse_exclude(x, y - txtpar.font_height, x + txtpar.text_wind_len * cw, y);
	for (c = 0; c < txtpar.text_wind_len; c++)
	    char_fill(x + cw * c, y, tbgc);
	(void) gprintf(x, y, &str[i - n]);
	if (i > n)
	    underdot(x, y);
	c = strlen(&str[i - n]);
	if (i == 1 && len == 1) {
	    *s = c;
	    tfgc = bbf;
	    tbgc = bbb;
	    mouse_show(1);
	    return (c);
	};
	if (n < txtpar.text_wind_len)
	    char_box(x + n * cw, y, txtpar.gcc);
	mouse_show(1);
	c = getevent();
	if (c == Ctrl_C)
	    return (0);
	mouse_exclude(x, y - txtpar.font_height, x + txtpar.text_wind_len * cw, y);
	char_fill(x + n * cw, y, tbgc);
	mouse_show(1);
	switch (c) {
	case LEFT_CLICK:
	    mouse_position(&mx, &my);
	    if (mx > sub.xbot || mx < sub.xtop || my < sub.ytop ||
		my > sub.ybot) {
		tfgc = bbf;
		tbgc = bbb;
		return (-1);
	    } else {
		i -= n;
		c = strlen(&str[i]);
		n = (mx - x) / cw;
		if (n >= c)
		    n = c - 1;
		if (n < 0) {
		    i -= txtpar.text_wind_len / 2;
		    if (i < 0)
			i = 0;
		    n = 0;
		} else
		    i += n;
	    }
	    break;
	case 1:		/* Ctrl-A */
	case (FN | 71):	/* Home */
	    i = 0;
	    n = 0;
	    break;
	case 5:		/* Ctrl-E */
	case (FN | 79):	/* End */
	    i = L;
	    n = L < txtpar.text_wind_len ? L : txtpar.text_wind_len / 2;
	    break;
	case (FN | 83):	/* Delete */
	case 4:		/* Ctrl-D */
	case 8:         /* Ctrl-H */
	case 2:		/* Ctrl-B */
	case (FN | 75):	/* Left-Arrow */
	case (FN | 115):/* Ctrl-Left-Arrow */
	    if (c == (FN | 83))
                c = 4;
	    if (c == 4) {
	        if (i < L) {
		    i++;
		    n++;
		} else {
	    	    break;
	    	}
	    }
	    if (n == 0) {
		if (i < txtpar.text_wind_len)
		    n = i;
		else
		    n = txtpar.text_wind_len - 1;
	    }
	    if (i > 0) {
	        if (c == 4 || c == 8) {
		    for (j = i; j < L; j++)
		        str[j - 1] = str[j];
		    L--;
		    str[L] = 0;
	        }
	        i--;
	        n--;
	    }
	    break;
	case 6:		/* Ctrl-F */
	case (FN | 77):	/* Right-Arrow */
	case (FN | 116):/* Ctrl-Right-Arrow */
	    if (i < L) {
		n++;
		if (n >= txtpar.text_wind_len)
		    n = txtpar.text_wind_len / 2 - 1;
		i++;
	    }
	    break;
	case 11:	/* Ctrl-K */
	    L = i;
	    str[L] = 0;
	    break;
	case 21:	/* Ctrl-U */
	    for (j = i; j < L; j++)
	        str[j - i] = str[j];
	    L -= i;
	    str[L] = 0;
	    i = n = 0;
	    break;
	case '\r':      /* Return */
	    str[len] = 0;
	    switch (type) {
	    case SHORT:
		(*p).sn = atoi(str);
		break;
	    case INT:
		(*p).in = atoi(str);
		break;
	    case CHAR:
		(*p).cn = *str;
		break;
	    case LONG:
		(*p).ln = atol(str);
		break;
	    case FLOAT:
		(*p).fn = (float) atof(str);
		break;
	    case DOUBLE:
		(*p).dn = (double) atof(str);
		break;
	    case STRING:
	    default:
		strcopy(s, str);
		break;
	    }
	    return (len);
	case Ctrl_C:    /* Ctrl-C */
	case 27:        /* Esc */
	    tfgc = bbf;
	    tbgc = bbb;
	    return (-1);
	default:
	    if ((c & FN) || (c < ' '))
	        break;
	    if (L < MAX_LEN) {
		for (j = L; j > i; j--)
		    str[j] = str[j - 1];
		str[i] = c;
		L++;
		str[L] = 0;
	        if (i < L) {
		    i++;
		    n++;
		    if (n >= txtpar.text_wind_len)
		        n = txtpar.text_wind_len / 2 - 1;
	        }
	    }
	}
    }
}

