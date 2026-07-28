/* chkfit.c */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include <menu.h>
#include "savage.h"
#include "screen.h"

WIND    w_stim;
WIND    w_sig;
WIND    w_ind;
WIND    w_info;
WIND    w_text;
WIND    w_msg;
WIND    w_spec;
WIND    w_stim_fft;

extern float resid_vpp;
extern int swp1set;
extern int xpix, ypix;
extern int scope;

double  vtodb();
void    reset_timing();
void    wind_color(WIND *, int, int);

extern int g_ncav;
extern int limits;
extern struct hear hear;
extern TOGGLETYPE distype, thv_chk;

int     fft_flag = 0, stim_flag = 0;
int     display_reset = 0;

void
init_wind()
{
    w_text.xtop = 100;
    w_text.xbot = xpix - 100;
    w_text.ybot = ypix - txtpar.menu_height;
    w_text.ytop = w_text.ybot - 7 * txtpar.font_height;
    wind_color(&w_text, scrn_c[C_TXTF], scrn_c[C_TXTB]);

    w_stim.xtop = 0;
    w_stim.xbot = xpix / 4;
    w_stim.ytop = txtpar.menu_height * 2 + 1;
    w_stim.ybot = w_stim.ytop + 7 * txtpar.font_height + 1;
    wind_color(&w_stim, scrn_c[C_STMF], scrn_c[C_STMB]);

    w_stim_fft.xtop = w_stim.xbot + 1;
    w_stim_fft.xbot = (xpix * 7) / 16;
    w_stim_fft.ytop = w_stim.ytop;
    w_stim_fft.ybot = w_stim.ybot;
    wind_color(&w_stim_fft, scrn_c[C_STMF], scrn_c[C_STMB]);

    w_info.xtop = w_stim_fft.xbot + 1;
    w_info.ytop = txtpar.menu_height + 1;
    w_info.xbot = xpix - 1;
    w_info.ybot = w_stim.ybot;
    wind_color(&w_info, scrn_c[C_INFF], scrn_c[C_INFB]);

    w_sig.xtop = 0;
    w_sig.xbot = (xpix * 2) / 3 - 1;
    w_sig.ytop = w_stim.ybot + 1;
    w_sig.ybot = ypix - 1;
    wind_color(&w_sig, scrn_c[C_RSPF], scrn_c[C_RSPB]);
    (void) draw_w(&w_sig);

    w_ind.xtop = w_sig.xbot + 1;
    w_ind.xbot = xpix - 1;
    wind_color(&w_ind, scrn_c[C_INDF], scrn_c[C_INDB]);
    w_dis = &w_sig;

    w_spec.xtop = w_sig.xbot + 1;
    w_spec.xbot = xpix - 1;
    w_spec.ytop = w_stim.ybot + 1;
    w_spec.ybot = (w_sig.ytop + ypix) / 2 - 1;
    wind_color(&w_spec, scrn_c[C_RFTF], scrn_c[C_RFTB]);
    (void) draw_w(&w_spec);
    (void) gprintf(w_spec.xtop + 4, w_spec.ytop + txtpar.font_height, "Spec:");
    w_spec.ytop += txtpar.menu_height;

    w_msg.xtop = w_spec.xtop;
    w_msg.xbot = w_spec.xbot;
    w_msg.ytop = w_spec.ybot + 1;
    w_msg.ybot = w_ind.ytop - 1;
    wind_color(&w_msg, scrn_c[C_MSGF], scrn_c[C_MSGB]);
}

double
units_conv(double in)       /* in = V */
{
    if (distype.at)
	in /= Sen.MP;       /* Sen.MP = V/Pa */

    return (in * 1000);     /* return = mV or mPa */
}


/* clears the current display box */
int
R_clear()
{
    mouse_exclude(w_dis->xtop, w_dis->ytop, w_dis->xbot, w_dis->ybot);
    gr_rectf(w_dis->xtop + 1, w_dis->ytop + 1, w_dis->xbot - 1, w_dis->ybot - 1,
	w_dis->wbgc);
    mouse_show(1);
    reset_timing();
    display_reset = 1;
    return (FN | 72);
}

static double
dbv(double v)
{
    if (v < 1e-39)
	return (-800.0);
    return (20 * log10(v));
}

double
vtodb(double v)
{
    return (dbv(v / (spl_ref * Sen.MP)));
}
