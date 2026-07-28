/* wind.c - window functions */

#include <stdio.h>
#include <menu.h>
#include "savage.h"
#include "color.h"
#include "emav.h"		/* function prototyes */
#include "screen.h"

WIND    w_stim;
WIND    w_sig;
WIND    w_ind;
WIND    w_info;
WIND    w_text;
WIND    w_msg;
WIND    w_spec;
WIND    w_stim_fft;

/* initialize the window borders and colors */
void
init_wind(void)
{
    int hstm, hmsg;

    hstm = (ypix > 35 * txtpar.font_height) ? ypix / 5 : 7 * txtpar.font_height;
    hmsg = (ypix > 36 * txtpar.font_height) ? ypix / 4 : 9 * txtpar.font_height;

    w_text.xtop = 100;
    w_text.xbot = xpix - 100;
    w_text.ybot = ypix - txtpar.menu_height;
    w_text.ytop = w_text.ybot - hstm;
    wind_color(&w_text, scrn_c[C_TXTF], scrn_c[C_TXTB]);

    w_stim.xtop = 0;
    w_stim.xbot = xpix / 4;
    w_stim.ytop = txtpar.menu_height * 2 + 1;
    w_stim.ybot = w_stim.ytop + hstm + 1;
    wind_color(&w_stim, scrn_c[C_STMF], scrn_c[C_STMB]);
    draw_w(&w_stim);

    w_stim_fft.xtop = w_stim.xbot + 1;
    w_stim_fft.xbot = (xpix * 7) / 16;
    w_stim_fft.ytop = w_stim.ytop;
    w_stim_fft.ybot = w_stim.ybot;
    wind_color(&w_stim_fft, scrn_c[C_STMF], scrn_c[C_STMB]);
    draw_w(&w_stim_fft);

    w_info.xtop = w_stim_fft.xbot + 1;
    w_info.ytop = txtpar.menu_height + 1;
    w_info.xbot = xpix - 1;
    w_info.ybot = w_stim.ybot;
    wind_color(&w_info, scrn_c[C_INFF], scrn_c[C_INFB]);
    draw_w(&w_info);

    w_sig.xtop = 0;
    w_sig.ytop = w_stim.ybot + 1;
    w_sig.xbot = (xpix * 13) / 16 - 1;
    w_sig.ybot = ypix - 1;
    wind_color(&w_sig, scrn_c[C_RSPF], scrn_c[C_RSPB]);
    draw_w(&w_sig);

    w_ind.xtop = w_sig.xbot + 1;
    w_ind.xbot = xpix - 1;
    wind_color(&w_ind, scrn_c[C_INDF], scrn_c[C_INDB]);
    pre_dis_ind("Artifact", "Limit");
    w_dis = &w_sig;

    w_msg.xtop = w_ind.xtop;
    w_msg.xbot = w_ind.xbot;
    w_msg.ytop = w_info.ybot + 1;
    w_msg.ybot = w_msg.ytop + hmsg + 8;
    wind_color(&w_msg, scrn_c[C_MSGF], scrn_c[C_MSGB]);
    draw_w(&w_msg);

    w_spec.xtop = w_ind.xtop;
    w_spec.xbot = w_ind.xbot;
    w_spec.ytop = w_msg.ybot + 1;
    w_spec.ybot = w_ind.ytop - txtpar.menu_height - 1;
    wind_color(&w_spec, scrn_c[C_RFTF], scrn_c[C_RFTB]);
    draw_w(&w_spec);
}
