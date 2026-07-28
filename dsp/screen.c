/* screen.c */

#include <stdio.h>

#include <menu.h>
#include "screen.h"

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
    txtpar.gfg = YELLOW;	/* string input fg & bg colors in gr_gets() */
    txtpar.gbg = BLUE;
    txtpar.dfg = MAGENTA;	/* dialog fg & bg colors in decide() */
    txtpar.dbg = WHITE;
    txtpar.gcc = LIGHTRED;	/* when inputing string, cursor color */
    txtpar.cbg = BLUE;		/* screen background color */
    txtpar.mono = 0;		/* mono flag */
}

void
set_screen_element_colors()
{
    scrn_c[C_INFF] = LIGHTBLUE;	    /* C_INFF - info panel foreground */
    scrn_c[C_INFB] = LIGHTGRAY;	    /* C_INFB - info panel background */
    scrn_c[C_INFH] = LIGHTRED;	    /* C_INFH - info panel highlight */
    scrn_c[C_TXTF] = MAGENTA;	    /* C_TXTF - text panel foreground */
    scrn_c[C_TXTB] = WHITE;	    /* C_TXTB - text panel background */
    scrn_c[C_STMF] = YELLOW;	    /* C_STMF - simulus panel foreground */
    scrn_c[C_STMB] = CYAN;	    /* C_STMB - stimulus panel background */
    scrn_c[C_SFTF] = YELLOW;	    /* C_SFTF - stim. FFT panel foregound */
    scrn_c[C_SFTB] = CYAN;	    /* C_SFTB - stim. FFT panel background */
    scrn_c[C_RSPF] = WHITE;	    /* C_RSPF - response panel foreground */
    scrn_c[C_RSPB] = MAGENTA;	    /* C_RSPB - response panel background */
    scrn_c[C_INDF] = YELLOW;	    /* C_INDF - indicator foreground */
    scrn_c[C_INDB] = BROWN;	    /* C_INDB - indicator background */
    scrn_c[C_INDN] = LIGHTGREEN;    /* C_INDN - indicator noise bar */
    scrn_c[C_INDL] = LIGHTRED;	    /* C_INDL - indicator limit line */
    scrn_c[C_MSGF] = WHITE;	    /* C_MSGF - message panel foreground */
    scrn_c[C_MSGB] = BLUE;	    /* C_MSGB - message panel background */
    scrn_c[C_MSGH] = YELLOW;	    /* C_MSGH - message panel highlight */
    scrn_c[C_RFTF] = YELLOW;	    /* C_RFTF - resp. FFT panel foreground */
    scrn_c[C_RFTB] = GREEN;	    /* C_RFTB - resp. FFT panel background */
    scrn_c[C_WAVA] = WHITE;	    /* C_WAVA - waveform A */
    scrn_c[C_WAVB] = YELLOW;	    /* C_WAVB - waveform B */
    scrn_c[C_CHNA] = LIGHTRED;	    /* C_CHNA - channel A */
    scrn_c[C_CHNB] = LIGHTBLUE;	    /* C_CHNB - channel B */
    scrn_c[C_WAVE] = WHITE;	    /* C_WAVE - transient waveform */
    scrn_c[C_DPL2] = LIGHTRED;	    /* C_DPL2 - DP L2 level */
    scrn_c[C_DPL1] = LIGHTBLUE;	    /* C_DPL1 - DP L1 level */
    scrn_c[C_DPLP] = YELLOW;	    /* C_DPLP - DP Ldp+ level */
    scrn_c[C_DPLN] = LIGHTGREEN;    /* C_DPLN - DP Ldp- level */
    scrn_c[C_APBS] = LIGHTCYAN;	    /* C_APBS - A+B spectrum */
    scrn_c[C_AMBS] = LIGHTMAGENTA;  /* C_AMBS - A-B spectrum */
}

void
set_screen_element_mono_colors()
{
    int i;

    for(i = 0; i < NUM_SCRN_COLORS; i++) scrn_c[C_AMBS] /= 8;
}
