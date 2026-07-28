/* screen.h */

#define C_INFF	0
#define C_INFB	1
#define C_INFH	2
#define C_TXTF	3
#define C_TXTB	4
#define C_STMF	5
#define C_STMB	6
#define C_SFTF	7
#define C_SFTB	8
#define C_RSPF	9
#define C_RSPB	10
#define C_INDF	11
#define C_INDB	12
#define C_INDN	13
#define C_INDL	14
#define C_MSGF	15
#define C_MSGB	16
#define C_MSGH	17
#define C_RFTF	18
#define C_RFTB	19
#define C_WAVA	20
#define C_WAVB	21
#define C_CHNA	22
#define C_CHNB	23
#define C_WAVE	24
#define C_DPL2	25
#define C_DPL1	26
#define C_DPLP	27
#define C_DPLN	28
#define C_APBS	29
#define C_AMBS	30
#define NUM_SCRN_COLORS	31

extern int scrn_c[];    /* The default values are set in screen.c */

void set_menu_colors(void);
void set_text_par(void);
void set_screen_element_colors(void);
void set_screen_element_mono_colors(void);
