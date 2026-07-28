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

#define FK_Shift_Tab    (15|FN)
#define FK_Alt_M        (50|FN)
#define FK_F1           (59|FN)
#define FK_F2           (60|FN)
#define FK_F3           (61|FN)
#define FK_F4           (62|FN)
#define FK_F5           (63|FN)
#define FK_F6           (64|FN)
#define FK_F7           (65|FN)
#define FK_F8           (66|FN)
#define FK_F9           (67|FN)
#define FK_F10          (68|FN)
#define FK_Home         (71|FN)
#define FK_Up_Arrow     (72|FN)
#define FK_PgUp         (73|FN)
#define FK_Left_Arrow   (75|FN)
#define FK_Right_Arrow  (77|FN)
#define FK_End          (79|FN)
#define FK_Down_Arrow   (80|FN)
#define FK_PgDn         (81|FN)
#define FK_Ins          (82|FN)
#define FK_Del          (83|FN)
#define FK_Ctrl_Left    (115|FN)
#define FK_Ctrl_Right   (116|FN)
#define FK_Ctrl_End     (117|FN)
#define FK_Ctrl_PgDn    (118|FN)
#define FK_Ctrl_Home    (119|FN)
#define FK_Ctrl_PgUp    (132|FN)

extern int colours[16];
