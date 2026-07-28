/* ========================== evnt.c ==================================== */

#include <stdio.h>
#include "menu.h"
#include "fk.h"

int is_mono();
int xw_getch(void);
int xw_kbhit(void);

int mouse_on = 0, mxpos = 0, mypos = 0;

static int event_q = 0;

static void (*execute_it)() = NULL;
static void (*capture_events)(int) = NULL;
static int pgm_terminate = 0;

/************ higher-level mouse functions ********************************/

void
mouse_init()
{
}

void
mouse_show(int on)
{
}

void
mouse_exclude(int x1, int y1, int x2, int y2)
{
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

    if (event_q == 0) {
	c = 0;
	if (xw_kbhit()) {
	    c = xw_getch();
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

