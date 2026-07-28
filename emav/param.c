/* ================================= param.c ============================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <menu.h>
#include <util.h>
#include "savage.h"
#include "color.h"
#include "emav.h"		/* function prototyes */
#include "screen.h"

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#define _access access
#endif /* WIN32 */

#define BREAK	1
#define LOG2	0.69314718

extern TOGGLETYPE xtype, dpftype, datafmt, Sbin, dp_octave, stim_unit;
extern char dpoae_file[MAXNAME], *dpsigtyp[], *dpfrqtyp[];
extern int nnsb, sig_type;
extern struct datlnk *dtlnk, *datptr;
extern int disconnect;
extern char *absci[];

float   L1b = 60, L2b = 50, F1b = 6689, F2b = 8000, ATb = 32;
float   L1e = 30, L2e = 25, F1e =  391, F2e =  500, ATe = 32;
float   F1s = 0, F2s = 0;
float   noise = -30, snr = 60, ratio = 1.2F;

static int ptsperoct = 3;
static int num_cond_req = 3;
static int num_cond_gen = 3;
static int repeat = 1;

int
TOG_axis(char **s)
{
    return(FN|113);
}

/* check against negative numbers in menu items */
int
chk_neg(float **f)
{
    if (**f <= 0)
	**f = 1.0;
    return (0);
}

MENUITEM paramx[] =
{
    {"&Noise (dB) :", (char *) &noise, FLOAT, 10, 1, 1, NULL},
    {"&S/N Ratio (dB) :", (char *) &snr, FLOAT, 10, 1, 1, NULL},
    {"&Repeat :", (char *) &repeat, INT, 6, 0, 1, chk_int},
    {"NNS&B :", (char *) &nnsb, INT, 6, 0, 1, NULL},
    {"Data &Format Type =", (char *) &datafmt, TOGGLE, 6, 0, 1, NULL},
    {"Save Response &Binary =", (char *) &Sbin, TOGGLE, 6, 0, 1, NULL},
    {"&DP Octave =", (char *) &dp_octave, TOGGLE, 6, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

MENUITEM param_f2[] =
{
    {"D&isplay Type =", (char *) &xtype, TOGGLE, 0, 0, 1, TOG_axis},
    {"F2/F1 R&atio :", (char *) &ratio, FLOAT, 6, 3, 1, chk_neg},
    {"F2 Frequency &Begin (Hz) :", (char *) &F2b, FLOAT, 6, 0, 1, chk_neg},
    {"F2 Frequency &End (Hz) :", (char *) &F2e, FLOAT, 6, 0, 1, chk_neg},
    {"&Points per Octave :", (char *) &ptsperoct, INT, 3, 0, 1, chk_neg},
    {"L&1 Level (dB) :", (char *) &L1b, FLOAT, 5, 1, 1, NULL},
    {"L&2 Level (dB) :", (char *) &L2b, FLOAT, 5, 1, 1, NULL},
    {"Level &Unit =", (char *) &stim_unit, TOGGLE, 6, 0, 1, NULL},
    {"Average &Time (sec) :", (char *) &ATb, FLOAT, 6, 0, 1, chk_neg},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

MENUITEM param_f1[] =
{
    {"D&isplay Type =", (char *) &xtype, TOGGLE, 0, 0, 1, TOG_axis},
    {"F1 Frequency &Begin (Hz) :", (char *) &F1b, FLOAT, 6, 0, 1, chk_neg},
    {"F1 Frequency &End (Hz) :", (char *) &F1e, FLOAT, 6, 0, 1, chk_neg},
    {"F1 Frequency &Step (Hz) :", (char *) &F1s, FLOAT, 6, 0, 1, chk_neg},
    {"&F2 Frequency (Hz) :", (char *) &F2b, FLOAT, 6, 0, 1, chk_neg},
    {"L&1 Level (dB) :", (char *) &L1b, FLOAT, 5, 1, 1, NULL},
    {"L&2 Level (dB) :", (char *) &L2b, FLOAT, 5, 1, 1, NULL},
    {"Level &Unit =", (char *) &stim_unit, TOGGLE, 6, 0, 1, NULL},
    {"Average &Time (sec) :", (char *) &ATb, FLOAT, 6, 0, 1, chk_neg},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

MENUITEM param_lev[] =
{
    {"D&isplay Type =", (char *) &xtype, TOGGLE, 0, 0, 1, TOG_axis},
    {"&F2 Frequency (Hz) :", (char *) &F2b, FLOAT, 6, 0, 1, chk_neg},
    {"F2/F1 R&atio :", (char *) &ratio, FLOAT, 6, 3, 1, chk_neg},
    {"L1 Level &Begin (dB) :", (char *) &L1b, FLOAT, 5, 1, 1, NULL},
    {"L1 Level &End (dB) :", (char *) &L1e, FLOAT, 5, 1, 1, NULL},
    {"L2 Level Be&gin (dB) :", (char *) &L2b, FLOAT, 5, 1, 1, NULL},
    {"L2 Level En&d (dB) :", (char *) &L2e, FLOAT, 5, 1, 1, NULL},
    {"Level &Unit =", (char *) &stim_unit, TOGGLE, 6, 0, 1, NULL},
    {"Number of &Conditions :", (char *) &num_cond_req, INT, 6, 0, 1, chk_int},
    {"Average &Time (sec) :", (char *) &ATb, FLOAT, 6, 0, 1, chk_neg},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

MENUITEM param_tr[] =
{
    {"D&isplay Type =", (char *) &xtype, TOGGLE, 0, 0, 1, TOG_axis},
    {"F2/F1 R&atio :", (char *) &ratio, FLOAT, 6, 0, 1, chk_neg},
    {"F2 Fre&quency Begin (Hz) :", (char *) &F2b, FLOAT, 6, 0, 1, chk_neg},
    {"F2 Freq&uency End (Hz) :", (char *) &F2e, FLOAT, 6, 0, 1, chk_neg},
    {"L1 Level &Begin (dB) :", (char *) &L1b, FLOAT, 6, 0, 1, NULL},
    {"L1 Level &End (dB) :", (char *) &L1e, FLOAT, 6, 0, 1, NULL},
    {"L2 Level Be&gin (dB) :", (char *) &L2b, FLOAT, 6, 0, 1, NULL},
    {"L2 Level En&d (dB) :", (char *) &L2e, FLOAT, 6, 0, 1, NULL},
    {"Level &Unit =", (char *) &stim_unit, TOGGLE, 6, 0, 1, NULL},
    {"Number of &Conditions :", (char *) &num_cond_req, INT, 6, 0, 1, chk_int},
    {"Average &Time Begin (sec) :", (char *) &ATb, FLOAT, 6, 0, 1, chk_neg},
    {"Average Ti&me End (sec) :", (char *) &ATe, FLOAT, 6, 0, 1, chk_neg},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

float   fdp_f2_start = (float) 0.9, fdp_f2_end = (float) 0.5;
static int f1_ptsperoct = 16;

MENUITEM param_fdp[] =
{
    {"D&isplay Type =", (char *) &xtype, TOGGLE, 0, 0, 1, TOG_axis},
    {"F2 Frequency &Begin (Hz):", (char *) &F2b, FLOAT, 6, 0, 1, chk_neg},
    {"F2 Frequency &End (Hz) :", (char *) &F2e, FLOAT, 6, 0, 1, chk_neg},
    {"F2 &Points per Octave :", (char *) &ptsperoct, INT, 6, 0, 1, chk_neg},
    {"F1 Points per &Octave :", (char *) &f1_ptsperoct, INT, 3, 0, 1, chk_neg},
    {"F&d/F2 Start Ratio :", (char *) &fdp_f2_start, FLOAT, 5, 3, 1, chk_neg},
    {"&Fd/F2 Stop Ratio :", (char *) &fdp_f2_end, FLOAT, 5, 3, 1, chk_neg},
    {"L&1 Level (dB) :", (char *) &L1b, FLOAT, 5, 1, 1, NULL},
    {"L&2 Level (dB) :", (char *) &L2b, FLOAT, 5, 1, 1, NULL},
    {"Level &Unit =", (char *) &stim_unit, TOGGLE, 6, 0, 1, NULL},
    {"Average &Time (sec) :", (char *) &ATb, FLOAT, 6, 0, 1, chk_neg},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

/* write the list into a file */
int
savethelist()
{
    WIND    w;
    int     i, j, saved, accept, k;
    FILE   *fp;

    w.xtop = menu_step(2);
    w.ytop = txtpar.menu_height * 6;
    w.xbot = w.xtop + 40 * 8;
    w.ybot = w.ytop + txtpar.menu_height;
    wind_color(&w, scrn_c[C_TXTF], scrn_c[C_TXTB]);

    saved = save_w(&w);
    i = draw_w(&w);
    j = w.ytop + txtpar.font_height + 1;
    k = gprintf(i, j, "File Name : ");
    i += 8 * k;
    txtpar.text_wind_len -= k;
    k = txtpar.text_wind_len;
    accept = 0;
    do {
	txtpar.text_wind_len = k;
	if (get_str(i, j, MAXNAME-1, 0, STRING, dpoae_file) == -1) {
	    if (saved)
		close_w(&w);
	    return (0);
	}
	if (!strlen(dpoae_file)) {
	    decide(0, 2, "You must input a valid file name",
		"Press and key to try again");
	} else if (_access(dpoae_file, 0) == 0) {
	    if (decide(1, 3, dpoae_file, "already exist.",
		    "Do you want to overwrite it ?")) {
		remove(dpoae_file);
		accept = 1;
	    }
	} else
	    accept = 1;
	if (accept) {
	    fp = fopen(dpoae_file, "wt");
	    if (fp == NULL) {
		decide(0, 2, "Sorry, cannot open the file",
		    "Please try a different name");
		accept = 0;
	    }
	}
    } while (!accept);

    if (saved)
	close_w(&w);

    fprintf(fp, ";Items=%d\n", num_cond_gen);
    fprintf(fp, ";Repeat=%d\n", repeat);
    fprintf(fp, ";Octave=%ld\n", dp_octave.at);
    fprintf(fp, ";Abscissa=%s\n", absci[xtype.at]);
    fprintf(fp, ";DP_freq=%s\n", dpfrqtyp[dpftype.at]);
    fprintf(fp, ";NNSB=%d\n", nnsb);
    fprintf(fp, ";Signal=%s\n", dpsigtyp[sig_type]);
    fprintf(fp, ";Datafmt=%s\n", datafmt.strs[datafmt.at]);
    fprintf(fp, ";SaveBin=%s\n", Sbin.strs[Sbin.at]);
    fprintf(fp, ";Level_Unit=%s\n", stim_unit.strs[stim_unit.at]);
    fprintf(fp, ";  F2      F1      L2      L1       T    Noise    SNR\n");

    datptr = dtlnk;
    while (datptr != NULL) {
	if(datptr->ctrl == BREAK) fprintf(fp, ";\n");
	fprintf(fp, "%7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f\n",
	    datptr->f2, datptr->f1, datptr->d2, datptr->d1, datptr->tm,
	    datptr->noise, datptr->snr);
	datptr = datptr->next;
    }
    fclose(fp);
    return (1);
}

/* compute the DP freq */
static double
fdp_compute(double f1, double f2)
{
    return (dpfreq(f1, f2));    
}

/* compute F1 with known F2 */
static double
f1_compute(double fdp, double f2)
{
    double c1, c2, f1 = 0;
    
    c1 = dpfreq(1.0,0.0);
    c2 = dpfreq(0.0,1.0);
    if (c1 != 0)
	f1 = (fdp - c2 * f2) / c1;
    return (f1); 
}

/* # of conditions */
void
cal_conds()
{
    int i;

    if (ptsperoct < 1)
	ptsperoct = 1;
    i = nint(ptsperoct * log(F2e / F2b) / LOG2);
    F2e = F2b * (float) pow(2.0, (double) i / ptsperoct);
    num_cond_gen = abs(i) + 1;
}

/* display list file info */
static void
list_info()
{
    int i, j, k;

    i = draw_w(&w_info);
    j = w_info.ytop + 1;
    k = txtpar.font_height;
    gprintf(i, j + 1 * k, " Items=%d", num_cond_gen);
    gprintf(i, j + 2 * k, " Repeat=%d", repeat);
    gprintf(i, j + 3 * k, " Abscissa=%s", absci[xtype.at]);
    gprintf(i, j + 4 * k, " DP_freq=%s", dpfrqtyp[dpftype.at]);
    gprintf(i, j + 5 * k, " NNSB=%d", nnsb);
    gprintf(i, j + 6 * k, " Datafmt=%s", datafmt.strs[datafmt.at]);
    gprintf(i, j + 7 * k, " SaveBin=%s", Sbin.strs[Sbin.at]);
}

/* generate a list of parameters */
int
gen_param_list()
{
    float   L1i, L2i, F1i, F2i, ATi;
    int     i, j, k = 0, cnt = 0;

    cnt = 0;
    i = menu_step(2);
    j = 8 * txtpar.menu_height;
    do {
	switch (xtype.at) {
	case 0:
	    k = simple_submenu(i, j, param_f2);
	    L1e = L1b;
	    L2e = L2b;
	    F1b = F2b / ratio;
	    F1e = F2e / ratio;
	    F1s = 0;
	    ATe = ATb;
	    cal_conds();
	    break;
	case 1:
	    F1s = (float) adjustf(limit(rate / buflen, fabs(F1s), rate / 2));
	    k = simple_submenu(i, j, param_f1);
	    L1e = L1b;
	    L2e = L2b;
	    F2e = F2b;
	    ATe = ATb;
	    F1b = (float) adjustf(F1b);
	    F1e = (float) adjustf(F1e);
	    F1s = (float) adjustf(limit(rate / buflen, fabs(F1s), rate / 2));
	    if (F1e < F1b)
	    	F1s = -F1s;
            num_cond_gen = (int) (1 + (F1e - F1b) / F1s);
	    break;
 	case 2:
	    k = simple_submenu(i, j, param_fdp);
	    F1s = 0;
	    L1e = L1b;
	    L2e = L2b;
	    ATe = ATb;
	    cal_conds();
	    break;
	case 3:
	case 4:
	case 5:
	    k = simple_submenu(i, j, param_lev);
	    F2e = F2b;
	    F1b = F2b / ratio;
	    F1e = F1b;
	    F1s = 0;
	    ATe = ATb;
            num_cond_gen = num_cond_req;
	    break;
	case 6:
	    k = simple_submenu(i, j, param_tr);
	    F1b = F2b / ratio;
	    F1e = F2e / ratio;
	    F1s = 0;
            num_cond_gen = num_cond_req;
	    break;
	}
    } while(k);
    k = simple_submenu(i, j, paramx);
    if (xtype.at == 2) {
	disconnect = 0;
	if (fdp_f2_end >= 1.0)
	    return (0);
	if (num_cond_gen <= 1)
	    F2i = 1;
	else
	    F2i = (float) pow(F2e / F2b, 1.0 / (double) (num_cond_gen - 1));
	for (i = 0; i < num_cond_gen; i++) {
	    disconnect = BREAK;
	    F2e = F2b * (float) pow(F2i, (double) i);
	    F1e = (float) f1_compute(fdp_f2_end * F2e, F2e);
	    ATi = (float) rate / (float) buflen;
	    j = (int) (F1e / ATi + 0.5);
	    F1e = (float) j * ATi;
	    j = (int) (F2e / ATi + 0.5) - 1;
	    F1b = (float) j * ATi;
	    if (fdp_f2_start < fdp_compute(F1b, F2e) / F2e) {
	        F1b = (float) f1_compute(fdp_f2_start * F2e, F2e);
	    }
	    if (f1_ptsperoct <= 1) {
		f1_ptsperoct = 1;
		if(add_data_lnk(0)) cnt++;
	    } else {
		F1i = (float) pow(F1e / F1b, 1.0 / (double) (f1_ptsperoct - 1));
		for (j = 0; j < f1_ptsperoct; j++) {
		    F1e = F1b * (float) pow(F1i, (double) j);
		    if(add_data_lnk(0)) cnt++;
		}
	    }
	}
	disconnect = 0;
    } else {
	if (num_cond_gen <= 1) {
	    F1e = F1b;
	    F2e = F2b;
	    L1e = L1b;
	    L2e = L2b;
	    ATe = ATb;
	    if(add_data_lnk(0)) cnt++;
	} else if (F1s != 0) {
	    for (i = 0; i < num_cond_gen; i++) {
		F1e = F1b + i * F1s;
		if(add_data_lnk(0)) cnt++;
	    }
	} else {
	    i = num_cond_gen - 1;
	    L1i = (L1e - L1b) / i;
	    L2i = (L2e - L2b) / i;
	    F1i = (float) pow(F1e / F1b, 1.0 / i);
	    F2i = (float) pow(F2e / F2b, 1.0 / i);
	    ATi = (float) pow(ATe / ATb, 1.0 / i);
	    for (i = 0; i < num_cond_gen; i++) {
		L1e = L1b + i * L1i;
		L2e = L2b + i * L2i;
		F1e = F1b * (float) pow(F1i, (double) i);
		F2e = F2b * (float) pow(F2i, (double) i);
		ATe = ATb * (float) pow(ATi, (double) i);
		if(add_data_lnk(0)) cnt++;
	    }
	}
    }
    num_cond_gen = cnt;
    list_info();
    while (!pgm_done()) {
	if (decide(1, 1, "Do you want to save the parameter list ?")) {
	    if (savethelist())
		break;
	} else {
	    dpoae_file[0] = 0;
	    break;
	}
    }
    return (cnt);
}
