/* ============================== menu.h =====================================
Minimenu is a library of C callable simple menu routines on IBM-PC's video
graphics mode.

written by Zhiqiang Liu.
revised by Stephen Neely.
Boystown National Research Hospital, 1992-1999
============================================================================ */

#define in_wind(x,y,w)  ((x>w.xtop)&&(x<w.xbot)&&(y>w.ytop)&&(y<w.ybot))

#define BLACK		0
#define BLUE		1
#define GREEN		2
#define CYAN		3
#define RED		4
#define MAGENTA 	5
#define BROWN		6
#define LIGHTGRAY	7
#define DARKGRAY	8
#define LIGHTBLUE	9
#define LIGHTGREEN	10
#define LIGHTCYAN	11
#define LIGHTRED	12
#define LIGHTMAGENTA	13
#define YELLOW  	14
#define WHITE	        15

#define C_BOXC  0
#define C_GETF  1
#define C_GETB  2
#define C_GETC  3
#define C_MNUF  4
#define C_MNUB  5
#define C_AMNF  6
#define C_AMNB  7
#define C_DMNF  8
#define C_SCRB  9
#define N_MC	10

#define CLICK_WAIT	5
#define MBG		WHITE
#define MFG		BLACK
#define ABG		GREEN
#define AFG		WHITE
#define FN		0x0800
#define MFN		0x0400
#define CLEAR_WIN	0x10FF
#define SINGLE_CLICK	(MFN | 1)
#define LEFT_CLICK	(MFN | 1)
#define RIGHT_CLICK	(MFN | 2)

#define gr_gets     get_str

enum TYPE { NONE, STRING, SHORT, INT, CHAR, LONG, FLOAT, DOUBLE, TOGGLE };

typedef struct {
    long at;		/* current selection */
    char *strs[12];	/* name of each toggled type */
} TOGGLETYPE;

typedef struct {
    int xtop, ytop;	/* left-top corner */
    int xbot, ybot;	/* right-bottom corner */
    int wfgc, wbgc;	/* text fg and bg colors */
} WIND;

typedef struct {
    int mfg, mbg;	/* non active menu fg and bg colors */
    int afg, abg;	/* active menu fg and bg colors */
    int dfg, dbg;	/* disable menu fg and bg colors */
    int boxc;		/* menu frame color */
} MENUCOLOR;

typedef struct {
    int menu_height;	/* each menu item height in pixels */
    int font_height;	/* the font height in pixels */
    int font_width;	/* the font width in pixels */
    int text_wind_len;	/* text window length in characters */
    int gfg, gbg;	/* edit fg and bg colors used in get_str() */
    int dfg, dbg;	/* dialog fg and bg colors used in decide() */
    int gcc;		/* when inputing string, cursor color */
    int cbg;		/* screen background color */
    int mono;		/* mono flag */
} TEXTPAR;

typedef struct {
    char *menustr;	/* each menu item description string */
    char *ptr2val;	/* generic pointer to the target value */
    enum TYPE valtype;	/* target value type */
    short inputlen;	/* input length in characters */
    char decpts;	/* how many decimal points if applicable */
    char status;	/* 1 for enabled, 0 for disabled */
    int (*exe)(void);       /* execute this each time get_str is invoked */
} MENUITEM;

/* =======================================================================
   The following are the global variables used in the mini-menu library
   ======================================================================= */

extern int mouse_on;		/* 1 if mouse is on, 0 if mouse is off */
extern int mxpos, mypos;	/* current mouse position */

extern MENUCOLOR menucolor;
//    The default values are
//    BLACK,	/* non-active text foreground */
//    WHITE,	/* non-active text background */
//    WHITE,	/* active text foreground */
//    GREEN,	/* active text background */
//    LIGHTGRAY,/* disabled foreground */
//    WHITE,	/* disabled background */
//    DARKGRAY	/* box boundary color */

extern TEXTPAR txtpar;
//    The default values are
//    13,	/* menu height */
//    10,	/* font height */
//     8,	/* font width */
//    34,	/* text window length */
//    YELLOW,	/* foreground color */
//    BLUE,	/* background color */
//    LIGHTRED,	/* cursor color */
//    BLUE,	/* screen background color */
//    0		/* monochrome flag */

/* =========================================================================
   The following are the function prototypes in the mini-menu library
   ========================================================================= */

void	set_resume(int c);	// set the menu close key
int	get_resume(void);	// return the menu close key
void	toggle_mono(void);	// switch between B&W and color mode 
void	init_gr(void);		// initialize the graphics video mode 
void	end_gr(void);		// switch off the graphics mode 
void	mouse_init(void);	// initialisze mouse
void	mouse_show(int on);	// turn on/off mouse cursor
void	msleep(int msec);	// ms delay

/* setup mouse exclusion area */
void	mouse_exclude(int xtop, int ytop, int xbot, int ybot);

/* fetch mouse cursor location */
void	mouse_position(int *x, int *y);

/* set the tailer function when close a menu */
void	set_trailer(void (*func )(void));

/* set the text foreground and back ground colors */
void	text_color(int fgc, int bgc);

/* returns the current text foreground and background colors in fg and bg */
void	rd_text_color(int *fg, int *bg);

/* set the foreground and background colors for a window defined in w */
void	wind_color(WIND *w, int fgc, int bgc);

/* returns the number of menu items and the maximum length of text in len.
Counts the number of items in a MENUITEM array terminated by a NULL pointer.
Returns the number of items and the maximum text length in *len;
*/
int	num_items(MENUITEM * menu, int *len);

/* does a simple menu at (x, y), the size of the window is auto-determined
Invokes a simple submenu at location (x, y) in pixels, where m points to the
MENUITEM data structure terminated by a NULL pointer.   It returns the last
action when exiting the menu window.
*/
int	simple_submenu(int x, int y, MENUITEM * m);

/* draw a short underline at (x, y) */
void	underline(int x, int y);

/* give back the area previously saved and remove the top of STACK
Gives back the area specified in the WIND data structure pointed by w.  Pops
the saved information out of the stack.
*/
void	close_w(WIND * w);

/* draw the area specified in w
Draw a frame specified in the WIND data structure pointed by w.  It returns
The x location where the first text char should be.
*/
int	draw_w(WIND * w);

/* save the area specified in w to a buffer in STACK */
int	save_w(WIND * w);

/* clear screen with color defined in txtpar.cbg */
void	clr_scr(void);

/* output formatted text at (x, y)
Prints formatted text string at location (x, y).  It returns the length of the
string.
*/
int	gprintf(int x, int y, char *fmt,...);

/* get inputs from keyboard at (x, y). the length is len and the decimal
   part lenght is dec, data type is type with generic storage location s

Gets an input string from console, and converts it into the right type specified
by type.  On screen the string will be displayed at (x, y).  The length of the
string is specified by len, of which there are dec decimal points applicable for
FLOAT and DOUBLE types.
The seven types are
	NONE	get_str will do nothing and return 0
 	STRING	string
	SHORT	short
	INT	int
	CHAR	char
	LONG	long int
	FLOAT	float
	DOUBLE	double

It returns the length of the string and the converted string or value at a 
location pointed by a generic pointer s;
*/
int	get_str(int x, int y, int len, int dec, enum TYPE type, char *s);


/* ===================== event related functions ====================== */

/* check if any event is waiting to be processed, specific events will 
   be captured at this point
*/
int	check_event(void);

/* get an event from event queue, 
   background process is executed while waiting for the event to come
*/
int     getevent(void);

/* get an event from event queue or return in "wt" ms,
   background process is executed while waiting for the event to come
*/
int     wait_event(int wt);

/* put an event into event queue, overwrites any uprocessed event
*/
void     putbackevent(int);

/* look ahead into the event queue to see if there is an event.
   if there is an event, it is return, else return 0
*/
int     lookahead(void);

/* sets the capture event function, to capture some events before they
   put into the event queue.
*/
void	set_capture_event(void (*)(int));

/* sets the background process function */
void	set_bg_exe(void (*)(void));

/* returns the current background process function */
void    (*get_bg_exe(void))(void);

/* ----------- mainmenu --------------- */

typedef struct {
	char *menustr;
	int (*exe)(void);
} MAIN_MENU;

int     menu_step(int);
int     menu_x(void);
int     menu_y(void);
int     mainmenu_show(char *);
void    mainmenu_clear(void);
void    mainmenu_exit(void);
void    mainmenu_init(MAIN_MENU *);
void    mainmenu_driver(void);

/* ----------- pgm functions --------------- */

int     pgm_done(void);
void    pgm_exit(void);
#ifdef WIN32
#define MAIN_PGM(ac,av)    pgm_main(ac,av)
int     pgm_main(int, char **);
#else   /* WIN32 */
#define MAIN_PGM(ac,av)    main(ac,av)
#endif  /* WIN32 */
void    set_title(char *);

/* ----------- dialog functions --------------- */

int     decide(int, int, char *,...);
void    decide_offset(int, int);

/* ----------- gr functions --------------- */

int     gr_getmsb(int, int);
int     gr_getpix(int, int);
void    gr_beep(void);
void    gr_clear(int);
void    gr_dotty(int);
void    gr_get_cwd(char *s, int n);
void    gr_line(int, int, int, int, int);
void    gr_rectf(int, int, int, int, int);
void    gr_recto(int, int, int, int, int);
void    gr_remap_palette(int, int);
void    gr_scrsiz(int *, int *);
void    gr_set_cwd(char *s);
void    gr_setfillmask(unsigned char *);
void    gr_setpix(int, int, int);
void    gr_settc(int, int);
void    gr_text(int, int, char *);
#ifdef linux
void    gr_savpix();
#endif // linux

