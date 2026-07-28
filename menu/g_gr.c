/* g_gr.c - graphics using MSC and GRLIB */

#include <stdio.h>
#include <stdlib.h>
#include <grlib.h>
#include "menu.h"

#define MAXNSS 32

static int old_vid_mode = 0;
static int pc_video_adapt = 0;
static int nss = 0;
static unsigned char *scrnbuf[MAXNSS];

void
init_gr()
{
    int     ncolors = 2;
    int     new_vid_mode = 0, mono = 0;

    if (old_vid_mode)
    	return;
    old_vid_mode = gr_gtmod();	/* save orignal graphics mode */

    if (pc_video_adapt == GR_UNKNOWN)
	pc_video_adapt = gr_adapt();

    switch (pc_video_adapt) {
    case GR_CGA:
	new_vid_mode = 6;
	ncolors = 2;
	break;
    case GR_HGC:
	new_vid_mode = 8;
	ncolors = 2;
	break;
    case GR_EGA_MONO:
	new_vid_mode = 15;
	ncolors = 2;
	break;
    case GR_EGA_COLOR:
    case GR_PEGA:
    case GR_EGA_HIGH:
	new_vid_mode = mono ? 15 : 16;
	ncolors = mono ? 2 : 16;
	break;
    case GR_VGA_MONO:
	new_vid_mode = 17;
	ncolors = 2;
	break;
    case GR_PVGA:
    case GR_VGA_COLOR:
	new_vid_mode = mono ? 17 : 18;
	ncolors = mono ? 2 : 16;
	break;
    default:
	fprintf(stderr, "\nUnable to proceed.  No graphics.\n");
	exit(1);
    }

    gr_init(new_vid_mode, WHITE, txtpar.cbg);	/* initialize graphics mode */

    if (new_vid_mode == 17 || new_vid_mode == 18) {
	if (getenv("FONTBUG") == NULL) {
	    txtpar.font_height = gr_selectfont(1) + 2;
	} else {
	    txtpar.font_height = 16;
	}
	txtpar.menu_height = txtpar.font_height + 3;
    }
    txtpar.font_width = 8;
}

void
end_gr()
{
    if (old_vid_mode == 0)
	return;
    gr_init(old_vid_mode, 0, 0);
}

void
g_savscr(x1, y1, x2, y2)
{
    long    nb;

    if (nss < MAXNSS) {
        nb = gr_imagesize(x1, y1, x2, y2);
        if (nb >= 65000)
    	    return;
        scrnbuf[nss] = (unsigned char *) malloc((size_t) nb);
        if (scrnbuf[nss] == NULL)
            return;
        gr_getimage(x1, y1, x2, y2, scrnbuf[nss]);
        nss++;
    }
}

void
g_rstscr(x1, y1)
{
    if (nss > 0) {
        nss--;
        gr_putimage(x1, y1, scrnbuf[nss], 0);
        free(scrnbuf[nss]);
    }
}

void
gr_get_cwd(char *s, int n)
{
    getcwd(s, n);
}

void
gr_set_cwd(char *s)
{
    chdir(s);
}

