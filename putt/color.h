/* color.h */

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

#define BLINK		128

#define CLICK_WAIT	5
#define MBG		WHITE
#define MFG		BLACK
#define ABG		GREEN
#define AFG		WHITE
#define FN		0x0800
#define MFN		0x0400
#define CLEAR_WIN	0x10FF
#define SINGLE_CLICK	(MFN | 1)
#define DOUBLE_CLICK	(MFN | 4)
#define RIGHT_CLICK	(MFN | 2)

extern int colours[16];
