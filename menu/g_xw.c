#include <string.h>
/* g_grx.c - graphics using DJC and GRX */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include "menu.h"
#include "fk.h"

#define MXNPL 8
#define icon_width 40
#define icon_height 40

extern int mxpos, mypos;

int     bx, by, bw, bh, xpix, ypix, ytop, ybot, xmax, ymax;

static char pgm_title[80];
static char icon_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, 0x00,
   0x00, 0xc6, 0x01, 0x00, 0x00, 0x00, 0x83, 0x03, 0x38, 0x00, 0x80, 0x01,
   0x03, 0x78, 0x00, 0x80, 0x38, 0x02, 0xf0, 0x00, 0xc0, 0x6c, 0x02, 0xe0,
   0x01, 0x40, 0x04, 0xe4, 0xff, 0x03, 0x40, 0x0c, 0xe4, 0xff, 0x07, 0xc0,
   0x18, 0xe6, 0xff, 0x03, 0x80, 0x30, 0x02, 0xe0, 0x01, 0x80, 0x61, 0x06,
   0xf0, 0x00, 0x00, 0x41, 0x0c, 0x78, 0x00, 0x00, 0x07, 0x18, 0x38, 0x00,
   0x00, 0x1c, 0x30, 0x00, 0x00, 0x00, 0x30, 0x20, 0x00, 0x00, 0x00, 0x70,
   0x30, 0x00, 0x00, 0x00, 0x60, 0x18, 0x00, 0x00, 0x00, 0xc0, 0x1f, 0x00,
   0x00, 0x00, 0x80, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static struct {
    short int r, g, b;
} ct[16] = {
    {0, 0, 0},			/* black      */
    {0, 0, 170},		/* blue       */
    {0, 170, 0},		/* green      */
    {0, 170, 170},		/* cyan       */
    {170, 0, 0},		/* red        */
    {170, 0, 170},		/* magenta    */
    {170, 85, 0},		/* brown      */
    {170, 170, 170},		/* lt grey    */
    {85, 85, 85},		/* dk grey    */
    {85, 85, 255},		/* lt blue    */
    {85, 255, 85},		/* lt green   */
    {85, 255, 255},		/* lt cyan    */
    {255, 85, 85},		/* lt red     */
    {255, 85, 255},		/* lt magenta */
    {255, 255, 85},		/* yellow     */
    {255, 255, 255},		/* white      */
};

static int count;
static int keyval = 0; 
static int txtbgc = 0; 
static int txtfgc = 0;
static int xw_dpth;
static int xw_scrn;
static int xw_x = 8;		/* x top left corner of window */
static int xw_y;		/* y top left corner of window */
static unsigned int xw_height = 480;	/* height of the window */
static unsigned int xw_width = 640;	/* width of the window */
static unsigned long xw_bp;
static unsigned long xw_wmsk;
static unsigned long xw_wp;

static Atom xw_kill;
static Colormap xw_cmap;
static Display *xw_mdsp;
static GC      xw_mgc;
static Pixmap graphdata;
static Window  xw_mwin;
static XComposeStatus compose; 
static XFontStruct *xw_xfs;
static XGCValues xw_gcv;
static XImage *graphimage = NULL;
static XSetWindowAttributes xw_swa;
static XSizeHints xw_theSizeHints;
static XWindowAttributes xw_wa; 

void
set_title(char *s)
{
    strcpy(pgm_title, s);
}

static void
set_fg_color(int c)
{
    XColor  hw;
    static int xw_last_color = -1;

    if (c == xw_last_color)
	return;
    hw.red = ct[c].r << 8;
    hw.green = ct[c].g << 8;
    hw.blue = ct[c].b << 8;
    hw.pixel = 0x20;
    hw.flags = -1;
    XAllocColor(xw_mdsp, xw_cmap, &hw);
    XSetForeground(xw_mdsp, xw_mgc, hw.pixel);
    xw_last_color = c;
}

static int
get_color(int p)
{
    int i, r, g, b;

    r = (p >> 16) & 0xFF;
    g = (p >> 8) & 0xFF;
    b = (p) & 0xFF;
    for (i = 0; i < 16; i++) {
	if (r == ct[i].r
	    && g == ct[i].g
	    && b == ct[i].b)
	    return (i);
    }

    return (0);
}

void
init_gr()
{
    Pixmap icon;
    
    xw_mdsp = XOpenDisplay("");
    if (xw_mdsp == NULL) {
	fprintf(stderr, "No X window display.\n");
	exit(1);
    }
    xw_wmsk = CWBackPixel | CWBorderPixel | CWOverrideRedirect | CWBackingStore | CWEventMask; 
    xw_scrn = DefaultScreen(xw_mdsp); 
    xw_dpth = DefaultDepth(xw_mdsp, xw_scrn); 
    xw_cmap = DefaultColormap(xw_mdsp, xw_scrn); 
    if (XDisplayWidth(xw_mdsp, xw_scrn) > 1000) {
	xw_width = 800;
    	xw_height = 600;
    };
    xw_x = (XDisplayWidth(xw_mdsp, xw_scrn) - xw_width) / 2;
    xw_y = 50;
    xw_swa.border_pixel = BlackPixel(xw_mdsp, xw_scrn); 
    xw_swa.background_pixel = WhitePixel(xw_mdsp, xw_scrn); 
    xw_swa.override_redirect = False;
    xw_swa.backing_store = Always;
    xw_mwin = XCreateWindow(xw_mdsp,DefaultRootWindow(xw_mdsp),
            xw_x, xw_y, xw_width, xw_height, 3, xw_dpth, InputOutput, 
            CopyFromParent, xw_wmsk, &xw_swa);
    
    xw_wp = WhitePixel(xw_mdsp, xw_scrn);
    xw_bp = BlackPixel(xw_mdsp, xw_scrn);

    icon = XCreateBitmapFromData(xw_mdsp, xw_mwin, icon_bits, 
            icon_height, icon_height); 

    xw_theSizeHints.flags = PPosition | PSize | PMinSize | PMaxSize;	/* set mask for the hints */
    xw_theSizeHints.x = xw_x;		/* x position */
    xw_theSizeHints.y = xw_y;		/* y position */
    xw_theSizeHints.width = xw_width;	/* width of the window */
    xw_theSizeHints.height = xw_height;	/* height of the window */
    // these set it so the window can't be resized
    xw_theSizeHints.min_width = xw_width;
    xw_theSizeHints.min_height = xw_height;
    xw_theSizeHints.max_width = xw_width;
    xw_theSizeHints.max_height = xw_height;

    XSetStandardProperties(xw_mdsp, xw_mwin, pgm_title, "",
           icon, 0, 0, &xw_theSizeHints);

    // tell what events that we'll be using, keyboard input
    XSelectInput(xw_mdsp, xw_mwin, ExposureMask | KeyPressMask | ButtonPressMask
	    | ClientMessage | StructureNotifyMask);
    
    xw_mgc = XCreateGC(xw_mdsp, xw_mwin, 0, &xw_gcv);

    if (xw_mgc == 0) {		
        XDestroyWindow(xw_mdsp, xw_scrn);
	fprintf(stderr, "error... cannot create graphics context\n");
        exit(1);
    }

    /* set forground and background defaults */
    XSetForeground(xw_mdsp, xw_mgc, xw_bp);
    XSetBackground(xw_mdsp, xw_mgc, xw_wp);

    XMapRaised(xw_mdsp, xw_mwin);
    XGetWindowAttributes(xw_mdsp, xw_mwin, &xw_wa);
    // this will set up our pixmap as the backup window to redraw it
    graphdata = XCreatePixmap(xw_mdsp, RootWindow(xw_mdsp, xw_scrn), 
            xw_width, xw_height, DefaultDepth(xw_mdsp, xw_scrn));
    XFlush(xw_mdsp);

    xw_kill = XInternAtom(xw_mdsp, "WM_DELETE_WINDOW", True);
    XSetWMProtocols(xw_mdsp, xw_mwin, &xw_kill, 1);
    
    xw_xfs = XQueryFont(xw_mdsp, XGContextFromGC(xw_mgc));
    txtpar.font_width = xw_xfs->max_bounds.width;
    txtpar.font_height = xw_xfs->ascent + xw_xfs->descent + 4;
    txtpar.menu_height = txtpar.font_height + 4;

    txtbgc = 15;	/* background color */
    txtfgc = 0;		/* foreground color */
    xpix = xw_wa.width;
    ypix = xw_wa.height;
    xmax = xpix - 1;
    ymax = ypix - 1;

    signal(SIGINT, SIG_IGN);
}

void
end_gr()
{
    XDestroyWindow(xw_mdsp, xw_mwin);
    XCloseDisplay(xw_mdsp);
}

void
gr_recto(int x1, int y1, int x2, int y2, int c)
{
    set_fg_color(c);
    XDrawLine(xw_mdsp, xw_mwin, xw_mgc, x1, y1, x2, y1);
    XDrawLine(xw_mdsp, xw_mwin, xw_mgc, x2, y1, x2, y2);
    XDrawLine(xw_mdsp, xw_mwin, xw_mgc, x2, y2, x1, y2);
    XDrawLine(xw_mdsp, xw_mwin, xw_mgc, x1, y2, x1, y1);
    XDrawLine(xw_mdsp, graphdata, xw_mgc, x1, y1, x2, y1);
    XDrawLine(xw_mdsp, graphdata, xw_mgc, x2, y1, x2, y2);
    XDrawLine(xw_mdsp, graphdata, xw_mgc, x2, y2, x1, y2);
    XDrawLine(xw_mdsp, graphdata, xw_mgc, x1, y2, x1, y1);
    XFlush(xw_mdsp);
}

void
gr_rectf(int x1, int y1, int x2, int y2, int c)
{
    int     x, y, w, h;

    x = (x1 < x2) ? x1 : x2;
    y = (y1 < y2) ? y1 : y2;
    w = (x2 > x1) ? (x2 - x1 + 1) : (x1 - x2 + 1);
    h = (y2 > y1) ? (y2 - y1 + 1) : (y1 - y2 + 1);
    set_fg_color(c);
    XFillRectangle(xw_mdsp, xw_mwin, xw_mgc, x, y, w, h);
    XFillRectangle(xw_mdsp, graphdata, xw_mgc, x, y, w, h);
    XFlush(xw_mdsp);
}

void
gr_text(int x, int y, char *s)
{
    gr_rectf(x, y, x + strlen(s) * txtpar.font_width + 4, y - txtpar.font_height + 1, txtbgc);
    x += 1;
    y -= 3;
    set_fg_color(txtfgc);
    XDrawString(xw_mdsp, xw_mwin, xw_mgc, x, y, s, strlen(s));
    XDrawString(xw_mdsp, graphdata, xw_mgc, x, y, s, strlen(s));
    XFlush(xw_mdsp);
}

void
gr_scrsiz(int *x, int *y)
{
    *x = xpix;
    *y = ypix;
}

void
gr_savpix()
{
    // save screen image for getpix
    graphimage = XGetImage(xw_mdsp, graphdata, 0, 0, xw_width, xw_height, xw_wp, XYPixmap);
}

void
gr_line(int x1, int y1, int x2, int y2, int c)
{
    set_fg_color(c);
    XDrawLine(xw_mdsp, xw_mwin, xw_mgc, x1, y1, x2, y2);
    XDrawLine(xw_mdsp, graphdata, xw_mgc, x1, y1, x2, y2);
    XFlush(xw_mdsp);
}

void
gr_clear(int c)
{
    gr_rectf(0, 0, xw_width, xw_height, c);
}

void
gr_setpix(int x, int y, int c)
{
}

/************************************************************************
 * missing functions Feb-2003, STN
 */

#define MAXNSS 32

static int nss = 0;
static XImage *scrnbuf[MAXNSS] = {NULL};

void
g_savscr(int x1, int y1, int x2, int y2)
{
    long    w, h;

    if (nss < MAXNSS) {
        w = (x2 > x1) ? (x2 - x1 + 1) : (x1 - x2 + 1);
        h = (y2 > y1) ? (y2 - y1 + 1) : (y1 - y2 + 1);
        scrnbuf[nss] = XGetImage(xw_mdsp, graphdata, x1, y1, w, h, xw_wp, XYPixmap);
        if (scrnbuf[nss])
            nss++;
    }
}

void
g_rstscr(int x1, int y1)
{
    int w, h;

    if (nss > 0) {
        nss--;
	w = scrnbuf[nss]->width;
	h = scrnbuf[nss]->height;
        XPutImage(xw_mdsp, xw_mwin, xw_mgc, scrnbuf[nss], 0, 0, x1, y1, w, h);
        XPutImage(xw_mdsp, graphdata, xw_mgc, scrnbuf[nss], 0, 0, x1, y1, w, h);
        XFlush(xw_mdsp);
	// free image data and struct
	free(scrnbuf[nss]->data);
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
    return (1);
}

int
gr_getpix(int x, int y)
{
    return (get_color(XGetPixel(graphimage, x, y)));
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

/************************************************************************/

int
xw_kbhit(void)
{
    char xw_buf[10];
    int buflen = 1;
    int keyer;
    KeySym keysym;
    XEvent report;   // holds type of event

    // only check KeyPress events when keyval is empty
    if(!keyval) {
    	// check key presses
        if(XCheckTypedEvent(xw_mdsp, KeyPress, &report)) {
            count = XLookupString(&report.xkey, xw_buf, buflen, &keysym,
                    &compose);
            keyer = xw_buf[0];
            if (keyer > 0) {
                keyval = keyer;
		if (keyval == 3)
		    pgm_exit();
	    } else if(keysym == XK_End) {
                keyval = FK_End;
	    } else if(keysym == XK_Home) {
                keyval = FK_Home;
	    } else if(keysym == XK_Left) {
                keyval = FK_Left_Arrow;
	    } else if(keysym == XK_Right) {
                keyval = FK_Right_Arrow;
	    } else if(keysym == XK_Up) {
                keyval = FK_Up_Arrow;
	    } else if(keysym == XK_Down) {
                keyval = FK_Down_Arrow;
	    } else if(keysym == XK_Delete) {
                keyval = FK_Del;
	    } else if(keysym == XK_F1) {
                keyval = FK_F1;
	    } else if(keysym == XK_F2) {
                keyval = FK_F2;
	    } else if(keysym == XK_F3) {
                keyval = FK_F3;
	    } else if(keysym == XK_F4) {
                keyval = FK_F4;
	    } else if(keysym == XK_F5) {
                keyval = FK_F5;
	    } else if(keysym == XK_F6) {
                keyval = FK_F6;
	    } else if(keysym == XK_F7) {
                keyval = FK_F7;
	    } else if(keysym == XK_F8) {
                keyval = FK_F8;
	    } else if(keysym == XK_F9) {
                keyval = FK_F9;
	    }
        }
    } 
    // check mouse for clicks
    if(XCheckTypedEvent(xw_mdsp, ButtonPress, &report)) {
	mxpos = report.xbutton.x;
	mypos = report.xbutton.y;
        keyval = (report.xbutton.button == 3) ? RIGHT_CLICK : LEFT_CLICK;
    }
    // always check ClientMessage & Expose events
    if (XCheckTypedEvent(xw_mdsp, ClientMessage, &report)) {
	pgm_exit();
    }
    if (XCheckTypedEvent(xw_mdsp, Expose, &report)) {
        XCopyArea(xw_mdsp, graphdata, xw_mwin, xw_mgc, 0,0, xw_width,
                xw_height, 0,0);
    }

    return (keyval);
}

int
xw_getch(void)
{
    int c;

    while (!xw_kbhit())	// loop until keyval is not empty
	msleep(50);	// wait 50 msec
    c = keyval;		// save keyval in c
    keyval = 0;		// make keyval empty

    return (c);
}

