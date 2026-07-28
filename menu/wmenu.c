/* wmenu.c */

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <windows.h>
#include <windowsx.h>

#define CTRL_C      3
#define EASZ        2000
#define PATHSZ      200

void test();
void set_hwnd(HWND);
void set_mesg(UINT, WPARAM, LPARAM);
void paint_scr(HWND, PAINTSTRUCT);
void pgm_main();
void pgm_start();
void pgm_title();

static int  xpix = 640, ypix = 480;
static int  xwin = 640, ywin = 480;

#ifndef MGW
static char ea[EASZ], path[PATHSZ];
static int  ep = 0;
#endif

LRESULT CALLBACK 
WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps ;

    switch (iMsg) {

    case WM_CREATE:
        return 0 ;

    case WM_SIZE:
        SetWindowPos(hwnd, HWND_TOP, 0, 0, xwin, ywin, SWP_NOMOVE);
        return 0 ;

    case WM_PAINT :
        BeginPaint(hwnd, (LPPAINTSTRUCT) &ps);
        paint_scr(hwnd, ps);
        EndPaint(hwnd, (LPPAINTSTRUCT) &ps);
        return 0 ;

    case WM_KEYDOWN:
    case WM_CHAR:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MOUSEMOVE:
        set_mesg(iMsg, wParam, lParam);
        return 0 ;

    case WM_DESTROY :
        set_mesg(iMsg, wParam, lParam);
        PostQuitMessage (0) ;
        return 0 ;
    }
    return DefWindowProc (hwnd, iMsg, wParam, lParam) ;
}

static void
check_args(int ac, char **av)
{
    int r;

    while (ac > 1) {
	if (av[1][0] == '-') {
	    if (av[1][1] == 'r') {
		r = atoi(av[1] + 2);
		if (r >= 640) {
		    xpix = r;
		    ypix = (3 * r) / 4;
		    ywin -= (ywin % 4);
		}
	    }
	}
	ac--;
	av++;
    }
}

int WINAPI 
WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, 
    char * szCmdLine, int iCmdShow)
{
    HWND    hwnd;
    WNDCLASS wndclass;
    static RECT lpsz;
    static char szAppName[] = "wmenu";

    GetClientRect(GetDesktopWindow(), &lpsz);
    if (lpsz.right > 1024) {
	xpix = 1024;
	ypix = 768;
    } else if (lpsz.right > 800) {
	xpix = 800;
	ypix = 600;
    }
    check_args(__argc, __argv);

    wndclass.style         = 0 ;
    wndclass.lpfnWndProc   = WndProc ;
    wndclass.cbClsExtra    = 0 ;
    wndclass.cbWndExtra    = 0 ;
    wndclass.hInstance     = hInstance ;
    wndclass.hIcon         = LoadIcon (hInstance, "MENU") ;
    wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
    wndclass.hbrBackground = NULL;
    wndclass.lpszMenuName  = NULL ;
    wndclass.lpszClassName = szAppName ;
    RegisterClass (&wndclass) ;
    xwin = xpix;		    /* adjust window size ... */
    ywin = ypix;		    /* to get desired client size */
    hwnd = CreateWindow (szAppName, /* window class name */
        szAppName,                  /* window caption */
        WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, /* window style */
        CW_USEDEFAULT,              /* initial x position */
        CW_USEDEFAULT,              /* initial y position */
        xwin,                       /* initial x size */
        ywin,                       /* initial y size */
        NULL,                       /* parent window handle */
        NULL,                       /* window menu handle */
        hInstance,                  /* program instance handle */
        NULL) ;	                    /* creation parameters */

    GetClientRect(hwnd, &lpsz);
    xwin = xpix + (xpix - (lpsz.right - lpsz.left));
    ywin = ypix + (ypix - (lpsz.bottom - lpsz.top));
    SetWindowPos(hwnd, HWND_TOP, 0, 0, xwin, ywin, SWP_NOMOVE);

    ShowWindow (hwnd, iCmdShow);
    UpdateWindow (hwnd);

    set_hwnd(hwnd);
    pgm_start();
    pgm_main(__argc, __argv);
    DestroyWindow(hwnd);
    return (0);
}  

void
set_title(char *s)
{
    pgm_title(s);
}
