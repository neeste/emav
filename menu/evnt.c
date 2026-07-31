#include <stdlib.h>
/* ========================== evnt.c ==================================== */

#include <stdio.h>
#include <bios.h>
#include <dos.h>
#ifndef _MSC_VER
#include <grx20.h>
#define _REGS       REGS
#define _int86      int86
#endif /* _MSC_VER */
#include "menu.h"

int is_mono();

int mouse_on = 0, mxpos = 0, mypos = 0;

static int alt_c = 0;
static int event_q = 0;

static void (*execute_it)() = NULL;
static void (*capture_events)(int) = NULL;
static int pgm_terminate = 0;

/************ lower-level mouse functions ********************************/

#define	MINT	0x33
#define	Ctrl_C	3

/* returns 1 if mouse is present and resets mouse, otherwise returns 0 */
static int
g_minit()
{
    union _REGS regs;

    regs.x.ax = 0;		/* function 0, reset and status */
    (void) _int86(MINT, &regs, &regs);
    return ((regs.x.ax == 0xFFFF) ? 1 : 0);
}

/* use nonzero flag to turn cursor on,  flag = 0 to turn cursor off */
static void
g_mshow(int flag)
{
    union _REGS regs;

    regs.x.ax = (flag == 0) ? 2 : 1;	/* function 1: show cursor, funct 2:
					   hide */
    (void) _int86(MINT, &regs, &regs);
}

/* get current status of buttons and location, store in m */
static void
g_mstatus(int *but1, int *but2, int *x, int *y)
{
    union _REGS regs;

    regs.x.ax = 3;		/* function 3: get button status and mouse
				   position */
    (void) _int86(MINT, &regs, &regs);
    *but1 = (regs.x.bx & 1) == 1;	/* check bit 0 */
    *but2 = (regs.x.bx & 2) == 2;	/* check bit 1 */
    *x = regs.x.cx & 0xFFFF;
    *y = regs.x.dx & 0xFFFF;
}

/* turn cursor off only if within specified rectangle */
static void
g_mexclu(int xtop, int ytop, int xbot, int ybot)
{
    union _REGS r;

    r.x.ax = 0x10;
    r.x.cx = xtop;
    r.x.dx = ytop;
    r.x.si = xbot;
    r.x.di = ybot;
    (void) (void) _int86(MINT, &r, &r);
}

/************ higher-level mouse functions ********************************/

void
mouse_init()
{
    mouse_on = g_minit();
    if (mouse_on)
        g_mshow(1);
}

void
mouse_show(int on)
{
    if (mouse_on)
        g_mshow(on);
}

void
mouse_exclude(int x1, int y1, int x2, int y2)
{
    if (mouse_on)
        g_mexclu(x1, y1, x2, y2);
}

void
mouse_position(int *x, int *y)
{
    *x = mxpos;
    *y = mypos;
}

/****************************************************************/

void
set_bg_exe(void (*e)())
{
    execute_it = e;
}

void
(*get_bg_exe())()
{
    return (execute_it);
}

void
putbackevent(int c)
{
    event_q = c;
}

int
lookahead()
{
    return (event_q);
}

/* get an event from event queue, while waiting for the event to come,
   background process is executed
*/
int
getevent()
{
    int     c;

    while (check_event() == 0)
        if (pgm_terminate)
            event_q = Ctrl_C;
    	else if (execute_it != NULL)
	    (*execute_it) ();

    c = event_q;
    event_q = 0;
    return (c);
}

static void
remapping(int cflg)
{
    int     k;

#ifdef _MSC_VER
    if (!is_mono()) {
	if (cflg) {
	    for (k = 1; k < 8; k++)
		gr_remap_palette(k, k);
	    gr_remap_palette(6, 20);
	    for (k = 8; k < 16; k++)
		gr_remap_palette(k, k + 48);
	} else {
	    for (k = 1; k < 8; k++)
		gr_remap_palette(k, 0);
	    for (k = 8; k < 16; k++)
		gr_remap_palette(k, 7);
	}
    }
#else /* _MSC_VER */
    static struct {
        short int r, g, b;
    } ct[16] = {
        {0, 0, 0},		/* black      */
        {0, 0, 170},		/* blue       */
        {0, 170, 0},		/* green      */
        {0, 170, 170},		/* cyan       */
        {170, 0, 0},		/* red        */
        {170, 0, 170},		/* magenta    */
        {170, 85, 0},		/* brown      */
        {170, 170, 170},	/* lt grey    */
        {85, 85, 85},		/* dk grey    */
        {85, 85, 255},		/* lt blue    */
        {85, 255, 85},		/* lt green   */
        {85, 255, 255},		/* lt cyan    */
        {255, 85, 85},		/* lt red     */
        {255, 85, 255},		/* lt magenta */
        {255, 255, 85},		/* yellow     */
        {255, 255, 255},	/* white      */
    };

    if (!is_mono()) {
	if (cflg) {
	    for (k = 1; k < 16; k++)
		GrSetColor(k, ct[k].r, ct[k].g, ct[k].b);
	} else {
	    for (k = 1; k < 8; k++)
		GrSetColor(k, 0, 0, 0);
	    for (k = 8; k < 16; k++)
		GrSetColor(k, 170, 170, 170);
	}
    }
#endif /* _MSC_VER */
}

void
set_capture_event(void (*g)(int))
{
    capture_events = g;
}

/**********************************************************************
*  CHECK_EVENT()
*
*  Check if any event is waiting to be processed, specific events will
*  be captured at this point
***********************************************************************
*  Created:
*    09-20-94 : JDS : Ported from Boys Town National Research Hospital.
*                     Was part of EMAV package vers. 2.07 date 08-15-94.
*  Modified:
*    03-01-95 : JDS : Now handles keyboard without jumping to dos
*                     function calls.  Also does right mouse click.
***********************************************************************/
int 
check_event()
{
    unsigned c;
    int     ls, rs;
    static int lls = 0, lrs = 0;

    if (event_q == 0) {
	c = 0;
	if (_bios_keybrd(_NKEYBRD_READY)) {
	    c = _bios_keybrd(_NKEYBRD_READ);
	    if ((c & 0xFF) == 0x00 ||    /* old scan code */
	    	(c & 0xFF) == 0xE0) {   /* new scan code */
		c = FN | ((c >> 8) & 0xFF);
		if (c == (FN | 46)) {
		    remapping(alt_c);
		    alt_c = !alt_c;
		    c = 0;
		}
	    } else if ((c & 0xFF)) {
		c &= 0xFF;
	        if (c == Ctrl_C) {
	            pgm_terminate = 1;
	            event_q = c;
	        }
	    }
	}
	if (mouse_on) {
	    g_mstatus(&ls, &rs, &mxpos, &mypos);
       	    if (!ls && lls) {       /* check left button release */
		c = LEFT_CLICK;
	    } else if (!rs && lrs) { /* check right button release */
		c = RIGHT_CLICK;
	    }
	    lls = ls;
	    lrs = rs;
	}
	event_q = c;
    }
    if (!pgm_terminate && capture_events != NULL)
	(*capture_events)(event_q);
    return (event_q);
}


/****************************************************************/

void
pgm_start()
{
    pgm_terminate = 0;
}

void
pgm_exit()
{
    pgm_terminate = 1;
}

int
pgm_done()
{
    return (pgm_terminate);
}

void
set_title(char *s)
{
}
