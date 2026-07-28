/* g_grx.c - graphics using DJC and GRX */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <grx20.h>
#include <unistd.h>
#include "menu.h"

#define MXNPL 8

static int txtfgc = 0;
static int txtbgc = 0;

void
init_gr()
{
    int     ncolors = 16;

    GrSetMode(GR_width_height_color_graphics, 640, 480, ncolors);
    txtpar.font_height = 16;
    txtpar.menu_height = txtpar.font_height + 3;
    txtpar.font_width = 8;
    signal(SIGINT, SIG_IGN);
}

void
end_gr()
{
    GrSetMode(GR_default_text);
}

void
gr_recto(int x1, int y1, int x2, int y2, int c)
{
    GrBox(x1, y1, x2, y2, c);
}

void
gr_rectf(int x1, int y1, int x2, int y2, int c)
{
    GrFilledBox(x1, y1, x2, y2, c);
}

void
gr_text(int x, int y, char *s)
{
    GrTextXY(x, y - 14, s, txtfgc, txtbgc);
}

void
gr_scrsiz(int *x, int *y)
{
    *x = GrSizeX();
    *y = GrSizeY();
}

void
gr_line(int x1, int y1, int x2, int y2, int c)
{
    GrLine(x1, y1, x2, y2, c);
}

void
gr_clear(int c)
{
    GrClearScreen(c);
}

long
gr_imagesize(int x1, int y1, int x2, int y2)
{
    long s;
    int   w, h, o, p;

    w = (x2 > x1) ? (x2 - x1 + 1) : (x1 - x2 + 1);
    h = (y2 > y1) ? (y2 - y1 + 1) : (y1 - y2 + 1);
    o = sizeof(GrContext);
    p = GrPlaneSize(w, h);
    s = o + GrNumPlanes() * p;

    return (s);
}

void
gr_getimage(int x1, int y1, int x2, int y2, unsigned char *i)
{
    char *m[MXNPL];
    int   k, w, h, o, p;
    GrContext *c, *s;

    w = (x2 > x1) ? (x2 - x1 + 1) : (x1 - x2 + 1);
    h = (y2 > y1) ? (y2 - y1 + 1) : (y1 - y2 + 1);
    o = sizeof(GrContext);
    p = GrPlaneSize(w, h);
    m[0] = i + o;
    for (k = 1; k < GrNumPlanes(); k++)
        m[k] = m[k - 1] + p;
    GrCreateContext(w, h, m, (GrContext *) i);
    c = (GrContext *) i;
    s = (GrContext *) NULL;
    GrBitBlt(c, 0, 0, s, x1, y1, x2, y2, GrWRITE);
}

void
gr_putimage(int x, int y, unsigned char *i, int m)
{
    int   xx, yy;
    GrContext *c, *s;

    c = (GrContext *) i;
    s = (GrContext *) NULL;
    xx = c->gc_xmax;
    yy = c->gc_ymax;
    GrBitBlt(s, x, y, c, 0, 0, xx, yy, GrWRITE);
}

void
gr_setpix(int x, int y, int c)
{
    GrLine(x, y, x, y, c);
}

/************************************************************************
 * missing functions Feb-2003, STN
 */

#define MAXNSS 32

static int nss = 0;
static unsigned char *scrnbuf[MAXNSS];

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
gr_settc(int fgc, int bgc)
{
    txtfgc = fgc;
    txtbgc = bgc;
}

void
gr_beep()
{
}

int
gr_getmsb(int x, int y)
{
    return (GrPixel(x, y) == GrWhite());
}

int
gr_getpix(int x, int y)
{
    return (GrPixel(x, y));
}

void
gr_remap_palette(int n, int c)
{
}

void
gr_setfillmask(unsigned char *m)
{
}

void
gr_dotty(int t)
{
}

void
toggle_mono()
{
}

void
msleep(int msec)
{
    usleep(1000 * msec);
}

