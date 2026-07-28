/* tstdsp.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <menu.h>
#include <util.h>
#include "dsp.h"
#include "savage.h"
#include "screen.h"
#ifdef _MSC_VER
#include <io.h>
#else /*_MSC_VER */
#include <unistd.h>
#define _access access
#endif /*_MSC_VER */

#define TICKS		18.207

int     Tone_wind(), set_tone_wind();
int     Quit_wind();
int     open_file();
int     save_w();
int     getevent(void);
int     gprintf(int, int, char *,...);
int     pgm_done();
int	maxnpts, accnpts, base_addr, dspprm;
void    putbackevent(int);
void    init_wind();
void    init_graphics();
void    clr_scr();
void    wind_color(WIND *, int, int);
void    text_color(int, int);
void    pgm_exit();

extern void (*run_prn[2]) ();
extern TOGGLETYPE thresh_algo, thresh_task, sim_obs;
extern TOGGLETYPE thresh_rand, thresh_cnct, levref, cond_thl;
extern TOGGLETYPE thv_itr, thv_ext, thv_xtk, thv_dbg;

int     rclflg = 0;
int     sig_type = 0;
float   ramp_time = 20;
float   spl_ref = (float) 20e-6;     /* SPL reference (rms Pa) */

WIND   *w_dis;

struct _tone tone = {0};

ATTENUATION attenu = {80.0, 80.0, 10000.0, 1};

sensitivity Sen = {
    (float) 5, (float) 0, {(float) 0, (float) 0}, {(float) 5, (float) 5}, 
};

char    ilstim[80] = {"b4.ils"};
char    exedir[80] = {0};
char    tmpbatchfile[40] = "";
char    batchfile[40] = "";
char    dsp_code_file[80] = "ASIO";
char    thsfn[40] = "";
char    ite1fn[40] = "";
char    ite2fn[40] = "";

char *month_name[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

int     scope = 2048;
int     mono_screen = 0;
short   *outbuf, *stimulus;
float  *accbuf, *accbuf_a, *accbuf_b;

char    line[MAXLINE];
char    tokstr[40];

void
init_alloc_space()
{
    accbuf = (float *) calloc(4 * MAXNPTS, sizeof(float));
    outbuf = (short *) calloc(4 * MAXNPTS, sizeof(float));
    stimulus = (short *) calloc(4 * MAXNPTS, sizeof(short));
    if (accbuf == NULL || outbuf == NULL || stimulus == NULL) {
	fprintf(stderr, "\nNot enough memory to run the program!");
	fprintf(stderr, "\nprogram aborted");
	pgm_exit();
    }
    accbuf_a = accbuf;
    accbuf_b = accbuf + (2 * MAXNPTS);
}

/* set default parameter values */
void
defpar()
{
    // TONE
    tone.rate = 32000;
    tone.size = 2048;
    tone.ramp = 0;
    tone.skips = 1;
    tone.fft_xrange = 8;
    tone.fft_yrange = 80;
    tone.levref = 0;
    tone.freq = 1000; 
    tone.level = 65;
    tone.duration = 2;
    tone.ramp_ms = 40;
    tone.hpff[0] = 0;
    tone.hpff[1] = 0;
}

int
testexist(char *n)
{
#ifdef _MSC_VER
    char   *env;
    char    fname[14], p[128];
    int     c;
    struct PATH tmp;

    if (_access(n, 0) == 0)
	return (1);
    strcpy(p, n);
    _splitpath(p, tmp.drive, tmp.dir, tmp.name, tmp.ext);
    if (strlen(tmp.drive) || strlen(tmp.dir)) 
        return (0);
    strcpy(fname, tmp.name);
    strcat(fname, tmp.ext);
    strcpy(p, exedir);
    strcat(p, fname);
    if (_access(p, 0) == 0) {
	strcpy(n, p);
	return (1);
    }
    env = getenv("PATH");
    if (env == NULL)
	return (0);

    c = 0;
    do {
	if (*env == ';' || *env == 0) {
	    if (c && p[c - 1] != '\\' && p[c - 1] != '/')
		p[c++] = '\\';
	    p[c] = 0;
	    strcat(p, fname);
	    if (_access(p, 0) == 0) {
		strcpy(n, p);
		return (1);
	    }
	    c = 0;
	} else
	    p[c++] = *env;
    } while (*env++ != 0);
#endif /* _MSC_VER */
    if (_access(n, 0) == 0)
	return (1);
    return (0);
}

/* display short message at top left */
void
top_message(char *s)
{
    gr_settc(scrn_c[C_INFB], txtpar.cbg);
    gr_text(560, txtpar.font_height, s);
    check_event();
}

/* display short message at top right */
void
title(char *s)
{
    text_color(scrn_c[C_INFB], txtpar.cbg);
    txtpar.text_wind_len = xpix / 2;
    gprintf(10, txtpar.font_height, s);
    check_event();
}

int
Quit_wind(char *no_use)
{
    extern char tmpbatchfile[];

    if (tmpbatchfile[0]) 
        (void) remove(tmpbatchfile);
    end_gr();
    dsprst(TRUE);	/* Assert the DSP reset	 */
    pgm_exit();
    return (0);
}

void
default_init()
{
    int wi, wo;

    dsppar.code = 2;
    dsppar.nic = 1;
    dsppar.greg = dspgreg(4);	// 1=16kw, 2=65kw, 3=97kw, 4=129kw
    switch (dsppar.greg) {
    case 1 :			    /* 16 KW SMA */
        base_addr = 392 * 128;	    /* = 0x0C400 */
        dspprm = 510 * 128;	    /* = 0x0FF00 */
	break;
    case 2 :			    /* 65 KW SMA */
        base_addr = 0;		    /* = 0x00000 */
        dspprm = 512 * 128;	    /* = 0x10000 */
	break;
    case 3 :			    /* 97 KW SMA */
        base_addr = 0;		    /* = 0x00000 */
        dspprm = 768 * 128;	    /* = 0x18000 */
	break;
    case 4 :			    /* 129KW SMA */
        base_addr = 0;		    /* = 0x00000 */
        dspprm = 1023 * 128;	    /* = 0x20000 */
	break;
    }
    wi = (dsppar.code == 2) ? 2 : 1;
    wo = (dsppar.nic == 2) ? 2 : 1;
    maxnpts = (dspprm - base_addr) / (2 * (wi + wo));
    accnpts = maxnpts * wi;
    dspseta(dsppar.code);
    Sen.MP = 5;
    dsppar.greg = 1;
    Sen.AD = 0;
    Sen.DA[0] = 0;
    Sen.DA[1] = 0;
    Sen.LS[0] = 5;
    Sen.LS[1] = 5;
}

int
MAIN_PGM(int ac, char **av)
{
    defpar();
    set_title("TstDsp");
    init_alloc_space();
    default_init();
    init_graphics();
    mouse_init();

    dspsatt(0.0);

    while (!pgm_done()) {
	clr_scr();
	text_color(menucolor.mfg, menucolor.mbg);
	init_wind();
        Tone_wind();
    }
    Quit_wind(0);

    return (0);
}
