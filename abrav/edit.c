#include <math.h>
/* edit.c */

#include <menu.h>
#include <util.h>
#include "savage.h"
#include "color.h"
#include "screen.h"

#define SQRT2   1.414213562

void    init_sh(void);
void	points_msg(void);
void	qm_info(void);
void    reconstruct_monitor(int);
void    show_A_B_labels(void);

extern float def_xrange, def_yrange;
extern float sh_range, sh_shift;
extern float sh_of[2][2];
extern int sh_12, sh_ab, sh_ul, sh_mk, sh_A, sh_B, sh_om, sh_sd;
extern int xrange, xbegin, xmrgn;
extern struct AUX_INF ai;
extern TOGGLETYPE monitor, few;

void
edit_control(int c, int e, int *es, int *up, int *cp, int *cu,
    int *go, int *pu, int *changed)
{
    double  vs = 0;
    int     i, j, k, mx, my, ms;
    static int gp = 0;

    ms = nint(0.001 * rate);
    switch (c) {
    case ' ':                           /* Space */
        *up = 1;
	break;
    case (FN | 75):	                /* Left-Arrow */
	*cp -= 1;
	*cu = 1;
	break;
    case (FN | 115):	                /* Ctrl-Left-Arrow */
	*cp -= 10;
	*cu = 1;
	break;
    case (FN | 77):	                /* Right-Arrow */
	*cp += 1;
	*cu = 1;
	break;
    case (FN | 116):	                /* Ctrl-Right-Arrow */
	*cp += 10;
	*cu = 1;
	break;
    case (FN | 72):	                /* Up-Arrow */
	vs = 0.01;
	break;
    case (FN | 80):	                /* Down-Arrow */
	vs = -0.01;
	break;
    case (FN | 141):	                /* Ctrl-Up-Arrow */
	vs = 0.1;
	break;
    case (FN | 145):	                /* Ctrl-Down-Arrow */
	vs = -0.1;
	break;
    case (FN | 73):	                /* Page-Up */
	if (sh_range > MINDISY)
	    sh_range /= (float) SQRT2;
	*up = 1;
	break;
    case (FN | 81):	                /* Page-Down */
	if (sh_range < MAXDISY)
	    sh_range *= (float) SQRT2;
	*up = 1;
	break;
    case (FN | 132):	                /* Ctrl-Page-Up */
	if (sh_range > MINDISY)
	    sh_range /= 2;
	*up = 1;
	break;
    case (FN | 118):	                /* Ctrl-Page-Down */
	if (sh_range < MAXDISY)
	    sh_range *= 2;
	*up = 1;
	break;
    case (FN | 83):                     /* Delete */
	sh_shift = 0.0;
	sh_of[0][0] = sh_of[0][1] = sh_of[1][0] = sh_of[1][1] = 0;
	*up = 1;
	break;
    case (FN | 71):	                /* Home */
	*cp = 0;
	*cu = 1;
	break;
    case (FN | 119):	                /* Ctrl-Home */
	*cp = -xbegin;
	*cu = 1;
	break;
    case (FN | 79):	                /* End */
	*cp = xrange - 1;
	*cu = 1;
	break;
    case (FN | 117):	                /* Ctrl-End */
	*cp = buflen - xbegin - 1;
	*cu = 1;
	break;
    case 9:                             /* Tab */
        *cp -= ms;
        xbegin += ms;
	if (xbegin > (buflen - xrange)) {
	    *cp += xbegin - (buflen - xrange);
	    xbegin = buflen - xrange;
	}
	if (*cp < 0)
	    *cp = 0;
	*cu = 1;
	*up = 1;
	break;
    case (FN | 15):                     /* Shift-Tab */
	if (xbegin < ms) {
	    *cp += xbegin;
	    xbegin = 0;
	} else {
	    *cp += ms;
	    xbegin -= ms;
	}
	if (*cp > (xrange - 1))
	    *cp = xrange - 1;
	*cu = 1;
	*up = 1;
	break;
    case '-':                           /* - */
    case (FN | 74):
	xrange += ms;
	if (xrange > buflen)
	    xrange = buflen;
	if (xbegin > (buflen - xrange)) {
	    *cp += xbegin - (buflen - xrange);
	    xbegin = buflen - xrange;
	}
	if (*cp < (xrange / 2 - ms / 2)
	    && xbegin > ms) {
	    *cp += ms;
	    xbegin -= ms;
	}
	*up = 1;
	break;
    case '+':                           /* + */
    case (FN | 78):
	xrange -= ms;
	if (xrange < ms)
	    xrange = ms;
	if (*cp > (xrange - 1)) {
	    if (*cp < ms) {
	        *cp = xrange - 1;
	    } else {
	        *cp -= ms;
     		xbegin += ms;
     	    }
	}
	if (*cp > (xrange / 2 + ms / 2)
	    && xbegin <= (buflen - xrange - ms)) {
	    *cp -= ms;
	    xbegin += ms;
	}
        *cu = 1;
	*up = 1;
	break;
    case '#':	                        /* # */
	few.at = (few.at + 1) % 4;
	points_msg();
	*up = 1;
	break;
    case '0':                           /* 0 */
	if (*go) {
            if (abr_pk[0][0] > 0) {
	        *cp = abr_pk[0][0] - xbegin;
	        *cu = 1;
	    }
	} else {
	    abr_pk[0][0] = *cp + xbegin;
	    *pu = 1;
	    *up = 1;
            *changed = 1;
	}
	break;
    case '1':                           /* 1 */
	if (*go) {
            if (abr_pk[sh_ab][1] > 0) {
                *cp = abr_pk[sh_ab][1] - xbegin;
                *cu = 1;
	    }
	} else {
	    abr_pk[sh_ab][1] = *cp + xbegin;
	    *pu = 1;
	    *up = 1;
            *changed = 1;
	}
	break;
    case '2':                           /* 2 */
	if (*go) {
            if (abr_pk[sh_ab][2] > 0) {
                *cp = abr_pk[sh_ab][2] - xbegin;
                *cu = 1;
	    }
	} else {
	    abr_pk[sh_ab][2] = *cp + xbegin;
	    *pu = 1;
	    *up = 1;
            *changed = 1;
	}
	break;
    case '3':                           /* 3 */
	if (*go) {
            if (abr_pk[sh_ab][3] > 0) {
                *cp = abr_pk[sh_ab][3] - xbegin;
                *cu = 1;
	    }
	} else {
	    abr_pk[sh_ab][3] = *cp + xbegin;
	    *pu = 1;
	    *up = 1;
            *changed = 1;
	}
	break;
    case '4':                           /* 4 */
	if (*go) {
            if (abr_pk[sh_ab][4] > 0) {
                *cp = abr_pk[sh_ab][4] - xbegin;
                *cu = 1;
	    }
	} else {
	    abr_pk[sh_ab][4] = *cp + xbegin;
	    *pu = 1;
	    *up = 1;
            *changed = 1;
	}
	break;
    case '5':                           /* 5 */
	if (*go) {
            if (abr_pk[sh_ab][5] > 0) {
                *cp = abr_pk[sh_ab][5] - xbegin;
                *cu = 1;
	    }
	} else {
	    abr_pk[sh_ab][5] = *cp + xbegin;
	    *pu = 1;
	    *up = 1;
            *changed = 1;
	}
	break;
    case '6':                           /* 6 */
	if (*go) {
            if (abr_pk[sh_ab][6] > 0) {
                *cp = abr_pk[sh_ab][6] - xbegin;
                *cu = 1;
	    }
	} else {
	    abr_pk[sh_ab][6] = *cp + xbegin;
	    *pu = 1;
	    *up = 1;
            *changed = 1;
	}
	break;

    case 'a':                           /* a */
	sh_ab = 0;
	if (sh_ul)
	    show_A_B_labels();
	*pu = 1;
	break;
    case 'b':                           /* b */
	sh_ab = 1;
	if (sh_ul)
	    show_A_B_labels();
	*pu = 1;
	break;
    case 'g':                           /* g */
	*go = 1;
	break;
    case 'm':                           /* m */
	sh_mk = !sh_mk;
	*up = 1;
	break;
    case 'n':                           /* n */
	if (sh_sd != 2) {
	    sh_12 = !sh_12;
	    show_A_B_labels();
	} else {
	    sh_12 = 0;
	}
	break;
    case 'r':                           /* 'r' */
        *cp += xbegin;
	xbegin = 0;
        xrange = (int) limit(0, 0.001 * def_xrange * rate + 1, buflen);
        init_sh();
        *up = 1;
	break;
    case 's':                           /* 's' */
	if (*go) {
            if (abr_pk[1][0] > 0) {
                *cp = abr_pk[1][0] - xbegin;
                *cu = 1;
	    }
	} else {
	    abr_pk[1][0] = *cp + xbegin;
	    *pu = 1;
	    *up = 1;
            *changed = 1;
	}
	break;
    case 'u':                           /* u */
	sh_ul = !sh_ul;
	show_A_B_labels();
	break;
    case 'A':                           /* A */
	sh_A = !sh_A;
	*up = 1;
	break;
    case 'B':                           /* B */
	sh_B = !sh_B;
	*up = 1;
	break;
    case (FN | 60):	                /* F2 */
	sh_om = (sh_om + 1) % 3;
	*up = 1;
	break;
    case (FN | 61):	                /* F3 */
	monitor.at = (monitor.at + 1) % 4;
	reconstruct_monitor(e);
	qm_info();
	break;
    case (FN | 65):	                /* F7 */
	sh_sd = (sh_sd + 1) % 3;
	if (sh_sd == 2)
	    sh_12 = 0;
	*up = 1;
	break;
    case 27:                            /* Esc */
        *es = 1;
        sh_ul = 0;
	break;
    case SINGLE_CLICK:                  /* Left-Click */
        mouse_position(&mx, &my);
	if (mx > w_msg.xtop && mx < w_msg.xbot &&
	    my > w_msg.ytop && my < w_msg.ybot) {
	    i = (my - w_msg.ytop) / txtpar.font_height;
	    if (i == 0)
		*up = 1;
	} else if (mx > w_sig.xtop && mx < w_sig.xbot &&
	    my > w_sig.ytop && my < w_sig.ybot) {
	    *cp = nint(xrange * ((float) mx - w_sig.xtop - xmrgn) 
	        / (w_sig.xbot - w_sig.xtop - xmrgn));
	    *cu = 1;
	} else if (my < w_info.ytop) {
            *es = 1;
	    sh_ul = 0;
	}
	break;
    }
    if (*cu) {
        if (*cp < 0 || *cp >= xrange) {
            if ((*cp + xbegin) < 0) {
                *cp = 0;
               	xbegin = 0;
            } else if ((*cp + xbegin + xrange / 2) >= buflen) {
               	*cp += xbegin + xrange - buflen;
               	xbegin = buflen - xrange;
            } else {
                xbegin += *cp - xrange / 2;
                *cp = xrange / 2;
                if (xbegin < 0) {
                    *cp += xbegin;
                   	xbegin = 0;
                }
            }
            if ((*cp + xbegin) >= buflen)
                *cp = buflen - xbegin - 1;
            *up = 1;
        }
    }
    if (*go && (c != 'g') && (c != 'a') && (c != 'b')) {
        *go = 0;
    }
    if (*go != gp) {
        text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
	j = w_info.xtop + 4;
        k = w_info.ytop + txtpar.font_height * 4;
        (void) gprintf(j, k, "%s", *go ? "Go To:" : "      ");
        gp = *go;
    }
    if (vs != 0) {
        if (sh_ul)
            sh_of[sh_ab][sh_12] += (float) (vs / 2);
        else
            sh_shift += (float) vs;
        vs = 0;
        *up = 1;
    }
}

