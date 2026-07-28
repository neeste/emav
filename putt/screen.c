/* screen.c */

#include <stdio.h>
#include <menu.h>
#include "screen.h"
#include "color.h"

int scrn_c[NUM_SCRN_COLORS] = {
    LIGHTBLUE,		/* C_INFF - info panel foreground */
    LIGHTGRAY,		/* C_INFB - info panel background */
    LIGHTRED,		/* C_INFH - info panel highlight */
    MAGENTA,		/* C_TXTF - text panel foreground */
    WHITE,		/* C_TXTB - text panel background */
    YELLOW,		/* C_STMF - simulus panel foreground */
    CYAN,		/* C_STMB - stimulus panel background */
    YELLOW,		/* C_SFTF - stim. FFT panel foregound */
    CYAN,		/* C_SFTB - stim. FFT panel background */
    WHITE,		/* C_RSPF - response panel foreground */
    LIGHTGRAY,		/* C_RSPB - response panel background */
    YELLOW,		/* C_INDF - indicator foreground */
    BROWN,		/* C_INDB - indicator background */
    LIGHTGREEN,		/* C_INDN - indicator noise bar */
    LIGHTRED,		/* C_INDL - indicator limit line */
    WHITE,		/* C_MSGF - message panel foreground */
    BLUE,		/* C_MSGB - message panel background */
    YELLOW,		/* C_MSGH - message panel highlight */
    YELLOW,		/* C_RFTF - resp. FFT panel foreground */
    GREEN,		/* C_RFTB - resp. FFT panel background */
    WHITE,		/* C_WAVA - waveform A */
    YELLOW,		/* C_WAVB - waveform B */
    LIGHTRED,		/* C_CHNA - channel A */
    LIGHTBLUE,		/* C_CHNB - channel B */
    WHITE,		/* C_WAVE - transient waveform */
    LIGHTRED,		/* C_DPL2 - DP L2 level */
    LIGHTBLUE,		/* C_DPL1 - DP L1 level */
    YELLOW,		/* C_DPLP - DP Ldp+ level */
    LIGHTGREEN,		/* C_DPLN - DP Ldp- level */
    LIGHTCYAN,	        /* C_APBS - A+B spectrum */
    LIGHTMAGENTA,	/* C_AMBS - A-B spectrum */
};

void
set_menu_colors()
{
    menucolor.mfg = BLACK;	/* non active menu fg and bg colors */
    menucolor.mbg = WHITE;
    menucolor.afg = WHITE;	/* active menu fg and bg colors */
    menucolor.abg = GREEN;
    menucolor.dfg = LIGHTGRAY;	/* disable menu fg and bg colors */
    menucolor.dbg = WHITE;
    menucolor.boxc = DARKGRAY;	/* menu frame color */
}

void
set_text_par()
{
    txtpar.menu_height = 13;	/* each menu item height in pixels */
    txtpar.font_height = 10;	/* the font height in pixels */
    txtpar.font_width = 8;	/* the font width in pixels */
    txtpar.text_wind_len = 34;	/* text window length in characters */
    txtpar.gfg = YELLOW;	/* string input fg and bg colors in gr_gets */
    txtpar.gbg = BLUE;
    txtpar.gcc = LIGHTRED;	/* when inputing string, cursor color */
    txtpar.cbg = BLUE;		/* screen background color */
    txtpar.mono = 0;		/* mono flag */
}
