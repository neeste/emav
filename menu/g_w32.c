/* g_w32.c - graphics using WIN32 */

#include <windows.h>
#include <windowsx.h>
#include "fk.h"

#define MAXNSS          32
#define MAXNCO          16
#define MAXNPNT		256

typedef struct {
    int menu_height;	/* each menu item height in pixels */
    int font_height;	/* the font height in pixels */
    int font_width;	/* the font width in pixels */
    int text_wind_len;	/* text window length in characters */
    int gfg, gbg;	/* string input fg and bg colors used in get_str */
    int gcc;		/* when inputing string, cursor color */
    int cbg;		/* screen background color */
    int mono;		/* mono flag */
} TEXTPAR;

extern TEXTPAR txtpar;

static int ch_wd, ch_ht, ln_sp; /* character width, height, and line spacing */
static int xpix, ypix;
static int dotty = 0;
static int nss = 0;
static int xpos = 0, ypos = 0;
static int mxpos = 0, mypos = 0;
static int mono = 0;
static int pgm_terminate = 0;
static int xpnt[MAXNPNT] = {0}, ypnt[MAXNPNT] = {0};
static int line_color = 0, npnt = 0;

static COLORREF palette_color[MAXNCO];
static HBITMAP hOBM, hNBM;
static HBRUSH  hbr[MAXNCO];
static HDC hDCMem = (HDC) 0;
static HFONT hfnt, hOldFont;      
static HWND hwnd ;
static HPEN hpen;
static RECT lpSize;
static struct {
    HDC hdc;
    HBITMAP hbm, obm;
    int w, h;
} scrn[MAXNSS];

static long
rgb_color(int c)
{
    static struct {
        short int r, g, b;
    } ct[MAXNCO] = {
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

    if (mono) {
	c = (c < 8) ? 15 : 0;
    }

    return (RGB(ct[c].r, ct[c].g, ct[c].b));
}


void
g_invalidate(int x, int y, int w, int h)
{
    RECT r;
    
    SetRect(&r, x, y, x + w, y + h);
    InvalidateRect(hwnd, &r, FALSE);
}

static void
flush_line()
{
    int i, xmn, ymn, xmx, ymx;
    static int last_color = 0;

    if (npnt > 1) {
	if (line_color != last_color) {
            SetROP2(hDCMem, dotty ? R2_XORPEN : R2_COPYPEN);
	    hpen = CreatePen(PS_SOLID, 1, rgb_color(line_color));
	    DeleteObject(SelectObject(hDCMem, hpen));
	    last_color = line_color;
	}
	MoveToEx(hDCMem, xpnt[0], ypnt[0], NULL);
	xmn = xmx = xpnt[0];
	ymn = ymx = ypnt[0];
	for (i = 0; i < npnt; i++) {
	    LineTo(hDCMem, xpnt[i], ypnt[i]);
	    if (xmn > xpnt[i])
		xmn = xpnt[i];
	    if (ymn > ypnt[i])
		ymn = ypnt[i];
	    if (xmx < xpnt[i])
		xmx = xpnt[i];
	    if (ymx < ypnt[i])
		ymx = ypnt[i];
	}
	g_invalidate(xmn, ymn, xmx - xmn + 1, ymx - ymn + 1);
    }
    npnt = 0;
}

/****************************************************************/

void
gr_line(int x1, int y1, int x2, int y2, int c)
{
    if (c != line_color || x1 != xpnt[npnt - 1] || y1 != ypnt[npnt - 1])
	flush_line();
    if (npnt == 0) {
        line_color = c;
	xpnt[0] = x1;
	ypnt[0] = y1;
	npnt = 1;
    }
    xpnt[npnt] = x2;
    ypnt[npnt] = y2;
    npnt++;	   
    if (npnt >= MAXNPNT) {
	flush_line();
	xpnt[0] = x2;
	ypnt[0] = y2;
	npnt = 1;
    }
}

void
gr_recto(int x1, int y1, int x2, int y2, int c)
{
    gr_line(x1, y1, x2, y1, c);
    gr_line(x2, y1, x2, y2, c);
    gr_line(x2, y2, x1, y2, c);
    gr_line(x1, y2, x1, y1, c);
    flush_line();
}

void
gr_rectf(int x1, int y1, int x2, int y2, int c)
{
    RECT    r;
    HDC     hdc;

    r.left = (x1 < x2) ? x1 : x2;
    r.right = (x1 < x2) ? x2 : x1;
    r.top = (y1 < y2) ? y1 : y2;
    r.bottom = (y1 < y2) ? y2 : y1;
    r.bottom++;
    r.right++;
    hdc = GetDC(hwnd);
    FillRect(hdc, &r, hbr[c]);
    FillRect(hDCMem, &r, hbr[c]);
    ReleaseDC(hwnd, hdc);
}

void
gr_settc(int fgc, int bgc)
{
    (void) SetBkMode(hDCMem, OPAQUE);
    (void) SetBkColor(hDCMem, rgb_color(bgc));
    (void) SetTextColor(hDCMem, rgb_color(fgc));
}

void
gr_text(int x, int y, char *s)
{
    int n, h;
    
    n = strlen(s);
    h = ch_ht + 2;
    y++;
    TextOut(hDCMem, x, y - h, s, n);
    g_invalidate(x, y - h, ch_wd * n, ln_sp);
    xpos = x + ch_wd * n;
    ypos = y;
}

void
gr_scrsiz(int *x, int *y)
{
    *x = xpix;
    *y = ypix;
}

void
gr_clear(int c)
{
    gr_rectf(0, 0, xpix, ypix, c);
}

void
g_savscr(int x1, int y1, int x2, int y2)
{
    int w, h;

    if (nss >= MAXNSS)
	return;

    w = x2 - x1 + 1;
    h = y2 - y1 + 1;
    scrn[nss].w = w;
    scrn[nss].h = h;
    scrn[nss].hdc = CreateCompatibleDC(hDCMem);
    scrn[nss].hbm = CreateCompatibleBitmap(hDCMem,w,h); 
    scrn[nss].obm = SelectObject(scrn[nss].hdc, scrn[nss].hbm);
    BitBlt(scrn[nss].hdc, 0, 0, w, h, hDCMem, x1, y1, SRCCOPY); 
    nss++;
}

void
g_rstscr(int x1, int y1)
{
    int w, h;

    if (nss <= 0)
	return;
    nss--;
    w = scrn[nss].w;
    h = scrn[nss].h;
    BitBlt(hDCMem, x1, y1, w, h, scrn[nss].hdc, 0, 0, SRCCOPY); 
    g_invalidate(x1, y1, w, h);
    DeleteObject(SelectObject(scrn[nss].hdc, scrn[nss].obm));
    DeleteDC(scrn[nss].hdc);
}

void
gr_setpix(int x, int y, int c)
{
    SetPixel(hDCMem, x, y, rgb_color(c));
}

int
gr_getpix(int x, int y)
{
    int i, b = 0;
    COLORREF r;

    r = GetPixel(hDCMem, x, y);
    for (i = 0; i < MAXNCO; i++) {
        if (r == palette_color[i]) {
            b = i;
	    break;
        }
    }
    return (b);
}

void
gr_beep()
{
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
    flush_line();
    dotty = t;
}

void
init_gr()
{
    int     i;
    HDC     hdc;
    TEXTMETRIC tm;

    if (hDCMem)
	return;
    hdc = GetDC(hwnd);
    GetClientRect(hwnd, (LPRECT) &lpSize);
    xpix = lpSize.right - lpSize.left;
    ypix = lpSize.bottom - lpSize.top;

    hDCMem = CreateCompatibleDC(hdc);
    hNBM = CreateCompatibleBitmap(hdc,xpix,ypix); 
    hOBM = SelectObject(hDCMem, hNBM);

    hpen = CreatePen(PS_SOLID, 1, rgb_color(0));
    (void) SelectPen(hDCMem, hpen);
    hfnt = GetStockObject(ANSI_FIXED_FONT); 
    hOldFont = SelectObject(hdc, hfnt);
    if (hOldFont)
        (void) SelectObject(hDCMem, hfnt);     
    ReleaseDC(hwnd, hdc);

    for (i = 0; i < MAXNCO; i++)
        hbr[i] = CreateSolidBrush(rgb_color(i));

    GetTextMetrics(hDCMem, &tm);
    ch_wd = tm.tmAveCharWidth;
    ch_ht = tm.tmAscent + 2;
    ln_sp = tm.tmHeight + 3;

    txtpar.font_width = ch_wd;
    txtpar.font_height = ln_sp;
    txtpar.menu_height = txtpar.font_height + 3;

    for (i = 0; i < MAXNCO; i++) {
        SetPixel(hDCMem, 0, 0, rgb_color(i));
	palette_color[i] = GetPixel(hDCMem, 0, 0);
    }

    gr_rectf(0, 0, xpix, ypix, 0);
}

void
end_gr()
{
    int     i;

    flush_line();
    for (i = 0; i < MAXNCO; i++)
        DeleteObject(hbr[i]);
    DeleteObject(hpen);
    DeleteObject(hNBM);
    DeleteDC(hDCMem);
    hDCMem = (HDC) 0;
}

/************************** event functions *******************************/

int event_q = 0;

static void (*execute_it)() = NULL;
static void (*capture_events)() = NULL;

void set_hwnd(HWND hw)
{
    hwnd = hw;
}

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

void
set_capture_event(void (*g) ())
{
    capture_events = g;
}

void
get_msg()
{
    MSG         msg;

    if (GetMessage (&msg, NULL, 0, 0)) {
        TranslateMessage (&msg);
        DispatchMessage (&msg);
    } else {
        event_q = Ctrl_C;
    }
    if (!pgm_terminate && capture_events != NULL)
	(*capture_events)(event_q);
}

void 
set_mesg(UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    static int ctrl = 0, shft = 0;
    
    switch (iMsg) {

    case WM_CHAR:
        event_q = wParam;
        if (event_q == Ctrl_C) {
            pgm_terminate = 1;
        }
        if (event_q == Tab)
            event_q = shft ? FK_Shift_Tab : Tab;
        break;

    case WM_KEYDOWN:
        ctrl = (GetKeyState(VK_CONTROL) < 0);
        shft = (GetKeyState(VK_SHIFT) < 0);
        if (wParam == VK_F1)
            event_q = FK_F1;
        else if (wParam == VK_F2)
            event_q = FK_F2;
        else if (wParam == VK_F3)
            event_q = FK_F3;
        else if (wParam == VK_F4)
            event_q = FK_F4;
        else if (wParam == VK_F5)
            event_q = FK_F5;
        else if (wParam == VK_F6)
            event_q = FK_F6;
        else if (wParam == VK_F7)
            event_q = FK_F7;
        else if (wParam == VK_F8)
            event_q = FK_F8;
        else if (wParam == VK_F9)
            event_q = FK_F9;
        else if (wParam == VK_F10)
            event_q = FK_F10;
        else if (wParam == VK_RIGHT)
            event_q = ctrl ? FK_Ctrl_Right : shft ? '6' : FK_Right_Arrow;
        else if (wParam == VK_LEFT)
            event_q = ctrl ? FK_Ctrl_Left : shft ? '4' : FK_Left_Arrow;
        else if (wParam == VK_UP)
            event_q = FK_Up_Arrow;
        else if (wParam == VK_DOWN)
            event_q = FK_Down_Arrow;
        else if (wParam == VK_INSERT)
            event_q = FK_Ins;
        else if (wParam == VK_DELETE)
            event_q = FK_Del;
        else if (wParam == VK_HOME)
            event_q = ctrl ? FK_Ctrl_Home : FK_Home;
        else if (wParam == VK_END)
            event_q = ctrl ? FK_Ctrl_End : FK_End;
        else if (wParam == VK_PRIOR)
            event_q = FK_PgUp;
        else if (wParam == VK_NEXT)
            event_q = FK_PgDn;
        break;

    case WM_LBUTTONDOWN:
        event_q = LEFT_CLICK;
        break;
    
    case WM_RBUTTONDOWN:
        event_q = RIGHT_CLICK;
        break;
    
    case WM_MOUSEMOVE:
        mxpos = LOWORD(lParam);  // horizontal position of cursor 
        mypos = HIWORD(lParam);  // vertical position of cursor 
        break;
    
    case WM_DESTROY:
        event_q = Ctrl_C;
        pgm_terminate = 1;
        break;
    
    }
}

/* get an event from event queue, while waiting for the event to come,
   background process is executed
*/

int
getevent()
{
    int         e;
    MSG         msg;

    flush_line();
    UpdateWindow(hwnd);
    while (event_q == 0)
        if (pgm_terminate)
            event_q = Ctrl_C;
        else if (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
            get_msg();
        else if (execute_it != NULL)
            (*execute_it) ();

    e = event_q;
    event_q = 0;
    
    return (e);
}

int
check_event()
{
    MSG         msg;

    flush_line();
    UpdateWindow(hwnd);
    if (event_q == 0) {
        if (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE)) {
            get_msg();
	}
    }

    return (event_q);
}

void
paint_scr(HWND hw, PAINTSTRUCT ps)
{
    HDC   hdc;
    INT  x = 0, y = 0, w = xpix, h = ypix;

    hdc = GetDC(hw);
    x = ps.rcPaint.left;
    y = ps.rcPaint.top;
    w = ps.rcPaint.right - ps.rcPaint.left;
    h = ps.rcPaint.bottom - ps.rcPaint.top;
    BitBlt(hdc, x, y, w, h, hDCMem, x, y, SRCCOPY); 
    ReleaseDC(hw, hdc);
}


/************ higher-level mouse functions ********************************/

void
mouse_init()
{
}

void
mouse_show(int on)
{
    flush_line();
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
pgm_title(char *s)
{
    if (hwnd)
        SetWindowText(hwnd, s);
}

void
gr_get_cwd(char *s, int n)
{
    GetCurrentDirectory(n, s);
}

void
gr_set_cwd(char *s)
{
    SetCurrentDirectory(s);
}

void
gr_set_mono(int m)
{
    mono = m;
}

void
toggle_mono()
{
    end_gr();
    mono = !mono;
    init_gr();
}

void
msleep(int msec)
{       
    Sleep(msec);
}
