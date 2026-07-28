/* view.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <menu.h>
#include "savage.h"
#include "color.h"
#include "emav.h"
#include "screen.h"

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif /* WIN32 */

extern char *tmp_dat_file, *tmp_tok_file, *not_saved;
extern int fileflag, dp_repeat, dp_items, num_oct;

int xbegin = 0, xrange = 0;

static void
swap_wind()
{
    WIND    w;

    w = w_sig;
    w_sig = w_spec;
    w_spec = w;
    wind_color(&w_sig, scrn_c[C_RSPF], scrn_c[C_RSPB]);
    wind_color(&w_spec, scrn_c[C_RFTF], scrn_c[C_RFTB]);
}

static void
view_teoae()
{
    int     i, mx, my, sweeps, quit = 0, show = 0, chan = 0;
    static int mode = 0;

    sweeps = rd_teoae_file();
    if (sweeps <= 0)
	return;

    while (!quit) {
	i = getevent();
	switch (i) {
	case 3:                             /* Ctrl-C */
	case ESC:                           /* Esc */
	    quit = 1;
	    show = 0;
	    break;
	case (FN | 77):                     /* Right Arrow */
	case 9:                             /* Tab */
	    i = (int) (0.001 * (float) rate);
	    xbegin += i;
	    if (xbegin + xrange >= buflen)
		xbegin = buflen - xrange;
	    show = 1;
	    break;
	case (FN | 75):                     /* Left Arrow */
	case (FN | 15):                     /* Shift-Tab */
	    i = (int) (0.001 * (float) rate);
	    xbegin -= i;
	    if (xbegin < 0)
		xbegin = 0;
	    show = 1;
	    break;
	case '-':                           /* - */
	    i = (int) (0.001 * (float) rate);
	    xrange += i;
	    if (xrange > buflen)
		xrange = buflen;
	    if (xbegin + xrange > buflen)
		xbegin = buflen - xrange;
	    show = 1;
	    break;
	case '+':                           /* + */
	    xrange -= (int) (0.001 * (float) rate);
	    if (xrange < 16)
		xrange = 16;
	    show = 1;
	    break;
	case (FN | 64):                     /* F6 */
	    mode = !mode;
	    show = 1;
	    break;
	case (FN | 65):                     /* F7 */
            swap_wind();
            show = 1;
	    break;
	case SINGLE_CLICK:
	    mouse_position(&mx, &my);
	    if (my < txtpar.font_height) {
                quit = 1;
	    } else if (in_wind(mx, my, w_spec)) {
                swap_wind();
                show = 1;
	    }
	    break;
	}
        if (show) {
            show_A_B(sweeps, mode, 1, chan);
            show = 0;
        }
    }
}

static void
view_dpoae()
{
    char dat_file[80];
    int     i, mx, my, maxsel;
    int     quit = 0, select = 0, show = 0, incr = 0, chan = 0;

    strcpy(dat_file, o_file_name);
    draw_w(&w_info);
    maxsel = show_dp_file(dat_file, select, chan) - 1;
    while (!quit) {
	i = getevent();
    	switch (i) {
	case 3:                             /* Ctrl-C */
	    quit = 1;
	    show = 0;
	    break;
	case ESC:                           /* Esc */
	    quit = 1;
	    show = 1;
	    break;
	case (FN | 65):                     /* F7 */
            swap_wind();
	    show = 1;
	    break;
	case (FN | 66):                     /* F8 */
	    if (dspnic < 2) {
		decide(0,1,"Only one A/D channel recorded.");
	    } else {
		chan = !chan;
		show = 1;
	    }
	    break;
	case '+':                           /* + */
	case 9:                             /* Tab */
    	    incr = 1;
	    break;
	case '-':                           /* - */
	case (FN | 15):                     /* Shift-Tab */
	    incr = -1;
	    break;
	case SINGLE_CLICK:
	    mouse_position(&mx, &my);
	    if (my < txtpar.font_height) {
                quit = 1;
	    } else if (in_wind(mx, my, w_spec)) {
                swap_wind();
                show = 1;
	    } else if (in_wind(mx, my, w_sig)) {
                incr = 1;
	    }
	    break;
	case RIGHT_CLICK:
	    mouse_position(&mx, &my);
	    if (my < txtpar.font_height) {
                quit = 1;
	    } else if (in_wind(mx, my, w_sig)) {
                incr = -1;
	    }
	    break;
        }
        if (incr) {
	    select = limit(0, select + incr, maxsel);
	    incr = 0;
	    show = 1;
	}
        if (show && !quit) {
            strcpy(o_file_name, dat_file);
            show_dp_file(dat_file, select, chan);
            show = 0;
        }
    }
    if (show) {
        strcpy(o_file_name, dat_file);
        rd_dpoae_file();
    }
}

static void
view_dpcal()
{
    char    cfn[MAXNAME];
    int     i, mx, my, icav = 0;
    int     quit = 0, show = 0, sil = 0;
    WIND    wf;

    strcpy(cfn, o_file_name);
    wf = w_spec;	// save w_spec
    w_spec.ybot = ypix - 1;
    if (cal_read(cfn, icav)) {
 	cal_disp(cfn, 1);
	if (numcav > 1) {
	    dis_cal_cav(icav);
	} else {
	    thev_load_show(cfn, sil);
	}
    }
    while (!quit) {
	i = getevent();
    	switch (i) {
	case 3:                             /* Ctrl-C */
	    quit = 1;
	    show = 0;
	    break;
	case 'i':                           /* i */
	    sil = !sil;
	    show = 1;
	    break;
	case ESC:                           /* Esc */
	    quit = 1;
	    show = 1;
	    break;
	case (FN | 65):                     /* F7 */
            swap_wind();
	    show = 1;
	    break;
	case SINGLE_CLICK:
	    mouse_position(&mx, &my);
	    if (my < txtpar.font_height) {
                quit = 1;
	    } else if (in_wind(mx, my, w_spec)) {
                swap_wind();
                show = 1;
	    }
	    break;
	case RIGHT_CLICK:
	    mouse_position(&mx, &my);
	    if (my < txtpar.font_height) {
                quit = 1;
	    }
	    break;
	case (FN | 77):                     /* Right Arrow */
	case 9:                             /* Tab */
	    icav++;
	    show = 1;
	    break;
	case (FN | 75):                     /* Left Arrow */
	case (FN | 15):                     /* Shift-Tab */
	    icav--;
	    show = 1;
	    break;
        }
        if (show) {
	    icav = (icav + numcav) % numcav;
	    if (cal_read(cfn, icav)) {
 		cal_disp(cfn, 1);
		if (numcav > 1) {
		    dis_cal_cav(icav);
		} else {
		    thev_load_show(cfn, sil);
		}
	    }
            show = 0;
        }
    }
    w_spec = wf;	// restore w_spec
}

void
view_file()
{
    int tmp;
    WIND    w0, w1;

    w0 = w_sig;
    w1 = w_spec;
    tmp = !strcmp(file_name, not_saved);
    if (fileflag == 0) {
        strcpy(o_file_name, tmp ? tmp_tok_file : file_name);
        view_teoae();
    } else if (fileflag == 1) {
        strcpy(o_file_name, tmp ? tmp_dat_file : file_name);
        view_dpoae();
    } else if (fileflag == 2) {
        view_dpcal();
    }
    if (tmp) {
	strcpy(file_name, not_saved);
	strcpy(o_file_name, not_saved);
    }
    w_sig = w0;
    w_spec = w1;
}
