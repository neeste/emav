/* showdp.c */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <menu.h>
#include <util.h>
#include "savage.h"
#include "screen.h"
#include "emav.h"

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#define _access access
#endif /* WIN32 */

FILE *	check_dpoae_file(char *fn, int flag);

extern char line[];
extern int dp_items, dp_repeat, sig_type, num_oct, nnsb;
extern struct _dpoae dpoae;
extern TOGGLETYPE xtype, dpftype, datafmt, Sbin, Suppr, dpsptype;
extern WIND w_dp;

static int dp_count = 0;
static int dp_yrange = 0;
static int dp_ymax = 0;
static FILE *fbin = NULL;

void
initial_dp()
{
    w_dp.xtop = w_sig.xtop + 30;
    w_dp.ytop = w_sig.ytop;
    w_dp.xbot = w_sig.xbot;
    w_dp.ybot = w_sig.ybot - txtpar.font_height;
    dp_count = 0;
    del_data_lnk();
    swp1set = dpoae.swp1set;
    acc_sets = dpoae.sets;
}

static double
scaled(double x, double r)
{
    return (log10(x / r));
}

static double
resize(double a)
{
    a = (float) dp_ymax - a;
    if (a < 0)
	a = 0;
    else if (a > dp_yrange)
	a = dp_yrange;
    return (a);
}

static void
yanno(int y, int num)
{
    int     i, cw, ch;

    cw = txtpar.font_width;
    ch = txtpar.font_height;
    gr_line(w_dp.xtop, y, w_dp.xtop - cw / 2, y, menucolor.boxc);
    gr_line(w_dp.xbot, y, w_dp.xbot - cw / 2, y, menucolor.boxc);
    text_color(scrn_c[C_RSPF], scrn_c[C_RSPB]);
    gprintf(w_dp.xtop - 28, y + ch / 2, "%3d", num);
    if (num % 20 == 0) {
	for (i = w_dp.xtop + 2; i < w_dp.xbot - 4; i += 4)
	    gr_setpix(i, y, menucolor.boxc);
    }
}

static void
xtick(int x, int s)
{
    gr_line(x, w_dp.ybot, x, w_dp.ybot - s, menucolor.boxc);
    gr_line(x, w_dp.ytop, x, w_dp.ytop + s, menucolor.boxc);
}

static void
fanno(int x, double f, int xin)
{
    char    s[10];
    int     nc, y, cw, ch;

    xtick(x, 4);
    if (f == 125) {
        strcpy(s, "0.125");
    } else if (f == 250) {
        strcpy(s, "0.25");
    } else if (f == 500) {
        strcpy(s, "0.5");
    } else {
        sprintf(s, "%d", nint(f / 1000));
    }
    cw = txtpar.font_width;
    ch = txtpar.font_height;
    nc = (int)(strlen(s));
    if (nc * 8 < xin) {
        text_color(scrn_c[C_RSPF], scrn_c[C_RSPB]);
        y = w_dp.ybot + ch - 1;
        gprintf(x - (nc * cw) / 2, y, s);
    }
}

static void
xanno(int x, int num, int xin)
{
    char    s[10];
    int     nc, y, cw, ch;

    xtick(x, 4);
    sprintf(s, "%d", num);
    cw = txtpar.font_width;
    ch = txtpar.font_height;
    nc = (int)(strlen(s));
    if (nc * 8 < xin) {
        text_color(scrn_c[C_RSPF], scrn_c[C_RSPB]);
        y = w_dp.ybot + ch - 1;
        gprintf(x - (nc * cw) / 2, y, s);
    }
}

static void
xlabel(char *s)
{
    int x, y, cw, ch;
    
    cw = txtpar.font_width;
    ch = txtpar.font_height;
    text_color(scrn_c[C_RSPF], scrn_c[C_RSPB]);
    x = w_dp.xbot - strlen(s) * cw - cw / 2;
    y = w_dp.ybot + ch - 1;
    gprintf(x, y, s);
}

/* draw the marker m at (x, y) with color */
static void
marker(int x, int y, int m, int color, int select)
{
    int     i, y1, y2;
    static int xx[4][12] = {
	{-2, 2, -1, 0, 1, -1, 1, -1, 0, 1, -2, 2},
	{0, -1, 0, 1, -2, -1, 1, 2, -1, 0, 1, 0},
	{0, 0, -1, 1, -1, 0, 1, -2, -1, 0, 1, 2},
	{-2, -1, 0, 1, 2, -1, 0, 1, -1, 1, 0, 0},
    };
    static int yy[4][12] = {
    	{2, 2, 1, 1, 1, 0, 0, -1, -1, -1, -2, -2},
    	{2, 1, 1, 1, 0, 0, 0, 0, -1, -1, -1, -2},
    	{2, 1, 0, 0, -1, -1, -1, -2, -2, -2, -2, -2},
    	{2, 2, 2, 2, 2, 1, 1, 1, 0, 0, -1, -2},
    };

    mouse_exclude(x - 1, y - 1, x + 1, y + 1);
    if (select) {
	y1 = limit(w_dp.ytop, y - 8, w_dp.ybot);
	y2 = limit(w_dp.ytop, y + 8, w_dp.ybot);
        gr_line(x, y1, x, y2, scrn_c[C_APBS]);
    }
    for (i = 0; i < 12; i++)
	gr_setpix(x + xx[m][i], y + yy[m][i], color);
    gr_setpix(x, y, color);
    mouse_show(1);
}

/* print symbol key at bottom of plot */
static void
print_key(int x, int y, int *cm, int st, int sf)
{
    char **k;
    int cw, ch;
    static char *dpk[4]= {"L1", "L2", "Ld+", "Ld-"};
    static char *sfk[4]= {"L1", "L3", "Lr+", "Lr-"};
    static char *msf[4]= {"L1", "L2", "Ls+", "Ls-"};

    cw = txtpar.font_width;
    ch = txtpar.font_height;
    txtpar.text_wind_len = (w_dp.xbot - w_dp.xtop - cw) / cw;
    if (x < w_dp.xbot) {
	if (sf == 2)
	    k = msf;
	else if (sf == 1)
	    k = sfk;
	else
	    k = dpk;
        y -= ch / 2 + 1;
        marker(x, y, 1, cm[1], 0);
	x += cw;
        gprintf(x, w_dp.ybot - 4, k[0]);
        x += cw * 4;
        marker(x, y, 0, cm[0], 0);
        x += cw;
        gprintf(x, w_dp.ybot - 4, k[1]);
        x += cw * 4;
        marker(x, y, 2, cm[2], 0);
        x += cw;
        gprintf(x, w_dp.ybot - 4, k[2]);
        x += cw * 5;
        marker(x, y, 3, cm[3], 0);
        x += cw;
        gprintf(x, w_dp.ybot - 4, k[3]);
        x += cw * 5;
    }
}

/* graphical display of the DP product result */
void
show_dp(double f2, float *lev, int count, int conct, int select, int sf, int py)
{
    double  xgn = 0, ygn = 0, xmk = 0, omn, omx;
    double  f, fmn = 0, fmx = 0, ldb;
    int     xm[12], ym[12], cm[12];
    int     i, xrn, yrn, x, y, db, step, nmk = 0;
    int     a, cw, amn = 0, amx = 0, ain = 0, xin = 0, flag, itm, oct; 
    static int xp[12] = {0}, yp[12] = {0};
    static int levinc = 10;

    if (!py) {
	R_clear();
	gr_recto(w_dp.xtop, w_dp.ytop, w_dp.xbot, w_dp.ybot, menucolor.boxc);
    }
    xrn = w_dp.xbot - w_dp.xtop;
    yrn = w_dp.ybot - w_dp.ytop;
    dp_yrange = dpoae.ord_max - dpoae.ord_min;
    dp_ymax = dpoae.ord_max;
    ygn = (double) yrn / dp_yrange;
    switch (xtype.at) {
    case 0:
    case 1:
    case 2:
        omn = limit(-10, dpoae.oct_min, 9);
        omx = limit(omn + 1, dpoae.oct_max, 10);
        fmn = 1000 * pow(2.0, omn);
        fmx = 1000 * pow(2.0, omx);
	xgn = xrn / scaled(fmx, fmn);
	xmk = scaled(f2, fmn) * xgn + w_dp.xtop;
	xin = nint(xrn / (omx - omn));
        nmk = 4 * (num_oct + 1);
	break;
    case 3:
    case 4:
    case 5:
	ldb = (xtype.at == 3) ? lev[0] : (xtype.at == 4) ? lev[1] : lev[12];
        amn = dpoae.abs_min;
        amx = dpoae.abs_max;
	ain = levinc;
	xgn = (double) xrn / (amx - amn);
	xmk = (ldb - amn) * xgn + w_dp.xtop;
	xin = nint(xrn * (double) ain / (amx - amn));
        nmk = 4;
	break;
    case 6:
	amn = 0;
	amx = dp_repeat * dp_items + 1;
	if (amx < 5)
	    amx = 5;
	if (amx <= 7) {
	    ain = 1;
	} else if (amx <= 14) {
	    ain = 2;
	} else if (amx <= 30) {
	    ain = 5;
	} else if (amx <= 70) {
	    ain = 10;
	} else if (amx <= 140) {
	    ain = 20;
	} else if (amx <= 300) {
	    ain = 50;
	} else {
	    ain = 100;
	}
        amx = ((amx + ain - 1) / ain) * ain;
	xgn = (double) xrn / amx;
	xmk = ++dp_count * xgn + w_dp.xtop;
	xin = nint(xrn * (double) ain / (amx - amn));
        nmk = 4;
	break;
    }
    xm[0] = xm[1] = xm[2] = xm[3] = nint(limit(w_dp.xtop, xmk, w_dp.xbot));
    if (nmk >= 8) {
	xmk = scaled(2 * f2, fmn) * xgn + w_dp.xtop;
        xm[4] = xm[5] = xm[6] = xm[7] = nint(limit(w_dp.xtop, xmk, w_dp.xbot));
    }
    if (nmk >= 12) {
	xmk = scaled(4 * f2, fmn) * xgn + w_dp.xtop;
        xm[8] = xm[9] = xm[10] = xm[11] = nint(limit(w_dp.xtop, xmk, w_dp.xbot));
    }
    for (i = 0; i < nmk; i++) {
        ym[i] = nint(resize(lev[i]) * ygn + w_dp.ytop);
    }
    if (sf == 1)
        ym[0] = nint(resize(lev[12]) * ygn + w_dp.ytop);
    cm[0] = scrn_c[C_CHNA];
    cm[1] = scrn_c[C_CHNB];
    cm[2] = scrn_c[C_DPLP];
    cm[3] = scrn_c[C_DPLN];
    mouse_exclude(w_dp.xtop, w_dp.ytop, w_dp.ytop, w_dp.ybot);
    if (count == 0) {
	txtpar.text_wind_len = 25;
	step = (yrn < dp_yrange) ? 20 : 10;
	db = dp_ymax % step;
	if (db == 0)
	    db = step;
	while (db < dp_yrange) {
	    y = nint(db * ygn + w_dp.ytop);
	    yanno(y, dp_ymax - db);
	    db += step;
	}
	if (xtype.at == 0 || xtype.at == 1 || xtype.at == 2) {
	    for (f = fmn; f < fmx; f *= 2) {
		x = nint(scaled(f, fmn) * xgn + w_dp.xtop);
                fanno(x, f, xin);
		if (xrn >= 320) {
	            x = nint(scaled(f * sqrt(2), fmn) * xgn + w_dp.xtop);
	            xtick(x, 2);
		}
	    }
	    if (xrn >= 320)
                xlabel("kHz");
	} else {
	    for (a = amn; a < amx; a += ain) {
		x = nint(xgn * (a - amn) + w_dp.xtop);
		xanno(x, a, xin);
		if (xrn >= 320) {
		    for (i = 1; i < ain; i++) {
			x = nint(xgn * (a + i - amn) + w_dp.xtop);
			xtick(x, 2);
		    }
		}
	    }
	    if (xrn >= 320)
                if (xtype.at == 3 || xtype.at == 4 || xtype.at == 5)
                    xlabel("dB");
	}
    } else if (conct) {
        for (i = 0; i < nmk; i++) {
            gr_line(xp[i], yp[i], xm[i], ym[i], cm[i % 4]);
        }
    }
    for (i = 0; i < nmk; i++) {
    	oct = select % (num_oct + 1);
    	itm = select / (num_oct + 1);
    	flag = (count == itm) && ((i / 4) == oct);
        marker(xm[i], ym[i], i % 4, cm[i % 4], flag);
        xp[i] = xm[i];
        yp[i] = ym[i];
    }
    if (count == 0) {
        cw = txtpar.font_width;
	x = w_dp.xbot - cw * strlen(xtype.strs[xtype.at]) - cw / 2;
	y = w_dp.ybot - 4;
	gprintf(x, y, xtype.strs[xtype.at]);
        x = w_dp.xtop + 12;
	print_key(x, y, cm, sig_type, sf);
    }
    mouse_show(1);
}

double
show_dp_fft(float *fst, int ns, float *fdp, int nd, float *pst, float *nst, 
    float *ast, float *pdp, float *ndp, float *adp, int chan)
{
    double  fdif, rep;
    float   ar, ai, br, bi, am, bm, cm;
    int     dp_fft_xrange, k;

    if (dpsptype.at == 1)
        dp_fft_xrange = (int) (fst[1] * 0.0012 + 1);
    else
        dp_fft_xrange = dpoae.fft_xrange;
    fdif = fst[1] - fst[0];

    fft_A_B(accbuf_a, accbuf_b, 1, acc_sets, 1, chan);
    for (k = 0; k < nd; k++) {
        get_level(fdp[k], nnsb, sig_type, fdif, &pdp[k], &ndp[k], &adp[k]);
    }
    for (k = 0; k < ns; k++) {
        get_level(fst[k], nnsb, sig_type, fdif, &pst[k], &nst[k], &ast[k]);
    }
    show_fft(dp_fft_xrange, dpoae.fft_yrange, dpoae.levref, scrn_c[C_APBS], 1);

    fft_A_B(accbuf_a, accbuf_b, 0, acc_sets, 1, chan);
    show_fft(dp_fft_xrange, dpoae.fft_yrange, dpoae.levref, scrn_c[C_AMBS], 0);

    /* compute reproducibility */
    pick_A_B(fdp[0], &ar, &ai, &br, &bi);
    am = ar * ar + ai * ai;
    bm = br * br + bi * bi;
    cm = ar * br + ai * bi;
    rep = 100 * fabs(cm) / sqrt(am * bm);

    return (rep);
}

static void
dis_cond(int itm, int oct, struct BINwav w)
{
    int     i, j, cw, ch;

    cw = txtpar.font_width;
    ch = txtpar.font_height;
    text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
    i = w_info.xtop + 4;
    j = w_info.ytop + ch;
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / cw - 2;
    gprintf(i, j, "condition %d/%d:  ", itm + 1, dp_items * dp_repeat);
    if (num_oct > 0)
        gprintf(i + 20 * 8, j, "octave=%d/%d", oct, num_oct);
    j += txtpar.font_height;
    if (w.f1 != w.f2) {
	gprintf(i, j, "F2/F1 = %.0f/%.0f = %.3f  ", w.f2, w.f1, w.f2 / w.f1);
        j += txtpar.font_height;
    } else {
	gprintf(i, j, "F2 = F1 = %.0f            ", w.f1);
        j += txtpar.font_height;
	gprintf(i, j, ": F3 =%5.0f ", w.f3);
        j += txtpar.font_height;
	gprintf(i, j, ": L3 =%5.0f ", w.L3);
        j += txtpar.font_height;
    }
    if (w.T > 0) {
        gprintf(i, j, "L1-L2 = %.0f-%.0f = %.0f, time = %d sec  ",
            w.L1, w.L2, w.L1 - w.L2, w.T);
        j += txtpar.font_height;
        gprintf(i + 240, j, ": F3 =%5.0f ", w.f3);
        j += txtpar.font_height;
        gprintf(i + 240, j, ": L3 =%5.0f ", w.L3);
        j += txtpar.font_height;
    }
}

static int
show_bin_file(char *fn, int itm, int oct, double tm, float *lev, int chan)
{
    char   *bfn;
    float   pst[2], nst[2], ast[2], fst[2];
    float   pdp[1], ndp[1], adp[1], fdp[1];
    int     nd, ns, accnpts;
    int32_t wavsiz;
    struct BINhdr bhdr = {0};
    struct BINwav wavhdr = {0};

    bfn = line;
    strcpy(bfn, fn);
    newext(bfn, "BIN");
    if(_access(bfn, 0) != 0) {
        fbin = NULL;
    } else {
	fbin = fopen(bfn, "rb");
	fread(&bhdr, sizeof(struct BINhdr), (size_t)(1), fbin);
	dp_items = bhdr.total;
	dp_repeat = 1;
	buflen = bhdr.wav_len;
	rate = (int32_t) bhdr.samp_rate;
	Sen.AD = 1 / bhdr.int2volt;
	Sen.MP = 1 / bhdr.volt2pas;
	if (dp_items <= 0 || buflen <= 0 || rate <= 0) {
	    fclose(fbin);
	    fbin = NULL;
	}
    }
    if (fbin == NULL)
        return (0);
    change_nic(bhdr.nic);
    accnpts = (bhdr.nic < 2) ? buflen : buflen * 2;
    wavsiz = sizeof(struct BINwav) + 2 * 4 * accnpts;
    fseek(fbin, sizeof(struct BINhdr) + (int32_t) itm * wavsiz, 0);
    fread(&wavhdr, sizeof(struct BINwav), (size_t)(1), fbin);
    fread(accbuf_a, 4, (size_t)(accnpts), fbin);
    fread(accbuf_b, 4, (size_t)(accnpts), fbin);

    if (bhdr.swp1set > 0 &&  (wavhdr.N % bhdr.swp1set) == 0) {
	acc_sets = wavhdr.N / bhdr.swp1set;
	swp1set = bhdr.swp1set;
    } else if (swp1set > 0 && (wavhdr.N % swp1set) == 0) {
	acc_sets = wavhdr.N / swp1set;
    } else {
	acc_sets = wavhdr.N;
	swp1set = 1;
    }
    fst[0] = wavhdr.f1;
    fst[1] = wavhdr.f2;
    ns = 2;
    fdp[0] = (float) dpfreq(fst[0], fst[1]);
    nd = 1;
    show_dp_fft(fst, ns, fdp, nd, pst, nst, ast, pdp, ndp, adp, chan);
    dis_cond(itm, oct, wavhdr);
    dis_result(pst, nst, pdp[0], ndp[0], fdp[0], fst[0] == fst[1]);
    fclose(fbin);

    return (1);
}

int 
show_dp_file(char *fn, int select, int chan)
{
    float   f, f1, f2, f3, f4, pst[4], nst[4], tm;
    float   fdp, pdp[NEX], ndp[NEX], lev[14];
    int     i, j, count, conct, itm, oct, nd, sf;
    int     dpfold, datafmtold, plot_yet, idp;
    struct BINwav wavhdr = {0};
    FILE   *fpt;

    fpt = check_dpoae_file(fn, 1);
    if (fpt == NULL)
	return (0);
    fpt = fopen(fn, "rt");
    strcpy(file_name, fn);
    o_file_name[0] = o_file_name[40] = o_file_name[80] = 0;
    dpfold = (int)(dpftype.at);
    datafmtold = (int)(datafmt.at);
    nst[0] = nst[1] = 0;
    num_oct = 0;
    count = conct = plot_yet = 0;
    //swp = 0;
    initial_dp();
    while (fgets(line, MAXLINE, fpt) != NULL) {
	if (line[0] == ';') {
	    line[0] = ' ';
	    conct = 0;
	    proc_hdr();
	} else if (strlen(line) > 12) {
	    if (datafmt.at == 0) {
	        sscanf(line, "%f %f %f %f %f %f %f", 
		    &f2, &f1, &pst[1], &pst[0], &tm, &pdp[0], &ndp[0]);
		lev[12] = lev[13] = -99;
	    } else {
                if (datafmt.at == 3)
		    nd = 5 + 4;
		else
		    nd = 5 + 3 * num_oct;
	        sscanf(line, "%f %f %f %f %f", &f2, &f1, &pst[1], &pst[0], &tm);
	        for (i = 0; i < nd; i++) {
	            j = skip_num(line, 5 + i * 3);
	            sscanf(line + j, "%f %f %f", &pdp[i], &ndp[i], &tm);
	        }
                j = skip_num(line, 5 + nd * 3);
	        sscanf(line + j, "%f %f %f %f", &nst[0], &tm, &nst[1], &tm);
                j = skip_num(line, 5 + nd * 3 + 4);
		if (Suppr.at) {
		    sscanf(line + j, "%f %f", &f3, &pst[2]);
		    j = skip_num(line, 5 + nd * 3 + 8);
		    sscanf(line + j, "%f %f", &f4, &pst[3]);
		    j = skip_num(line, 5 + nd * 3 + 12);
		    sscanf(line + j, "%f", &pst[3]);
                }
	    }
            fdp = (float) dpfreq(f1, f2);
	    if (xtype.at == 1) {
		f = f1;
	    } else if (xtype.at == 2) {
	    	f = fdp;
	    } else {
		f = f2;
	    }
	    idp = dpindx();
            lev[0] = pst[1];
            lev[1] = pst[0];
            lev[2] = pdp[idp];
            lev[3] = ndp[idp];
            if (num_oct >= 1) {
                lev[4] = pdp[5];
                lev[5] = pdp[6];
                lev[6] = pdp[7];
                lev[7] = ndp[7];
            }
            if (num_oct >= 2) {
                lev[8] = pdp[8];
                lev[9] = pdp[9];
                lev[10] = pdp[10];
                lev[11] = ndp[10];
            }
            lev[12] = pst[2];
            lev[13] = pst[3];
	    if (datafmt.at == 4)
	        sf = 2;
	    else if (f1 == f2)
	        sf = 1;
	    else
	        sf = 0;
	    show_dp(f, lev, count, conct, select, sf, plot_yet++);
            oct = select % (num_oct + 1);
            itm = select / (num_oct + 1);
	    if (count == itm) {
                if (!show_bin_file(fn, itm, oct, tm, lev, chan)) {
                    wavhdr.f1 = f1;
                    wavhdr.f2 = f2;
                    wavhdr.f3 = f3;
                    wavhdr.L3 = pst[2];
                    wavhdr.T = 0;
                    dis_cond(itm, oct, wavhdr);
                    if (oct == 0)
                        dis_result(pst, nst, pdp[0], ndp[0], fdp, sf);
                    else if (oct == 1)
                        dis_result(pdp + 5, ndp + 5, pdp[7], ndp[7], fdp, sf);
                    else if (oct == 2)
                        dis_result(pdp + 8, ndp + 8, pdp[10], ndp[10], fdp, sf);
                    draw_w(&w_spec);
                }
            }
	    count++;
	    conct++;
	}
    }
    fclose(fpt);

    dpftype.at = dpfold;
    datafmt.at = datafmtold;

    return (count);
}
