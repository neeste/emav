/* =========================== naverage.c ================================ */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <dsp.h>
#include <menu.h>
#include <token.h>
#include <util.h>
#include "savage.h"
#include "color.h"
#include "screen.h"
#include "emav.h"		/* function prototyes */

#define TRIGGER_LEVEL	3277

extern char *tmp_dat_file, *tmp_tok_file, *not_saved;
extern float F1_cutoff, F2_cutoff;
extern float limit_mv, range_mv;
extern int fileflag, xbegin, xrange;
extern int iramp, teoae_counter;
extern float teoae_refresh;
extern struct _teoae teoae;
extern struct _dpoae dpoae;
extern struct _probe probe;
extern TOKENFILE *t;
extern TOGGLETYPE Stim_mode;

float   resid_vpp = 0;


INDICATOR t_ind;

void
pre_dis_ind(char *s, char *m)
{
    int     i, c, tfgc_s, tbgc_s;

    rd_text_color(&tfgc_s, &tbgc_s);
    w_ind.ybot = ypix - 10 - txtpar.menu_height;
    w_ind.ytop = w_ind.ybot - txtpar.menu_height;
    i = draw_w(&w_ind);
    gprintf(i, w_ind.ytop + txtpar.font_height, s);
    w_ind.ytop = w_ind.ybot;
    w_ind.ybot = ypix - 1;
    i = draw_w(&w_ind);
    c = w_ind.ytop + txtpar.font_height;
    gprintf(i, c, m);
    i = w_ind.ybot - 10;
    mouse_exclude(w_ind.xtop, i, w_ind.xbot, i);
    gr_line(w_ind.xtop, i, w_ind.xbot, i, menucolor.boxc);
    mouse_show(1);
    t_ind.top = (short)(++i);
    t_ind.bot = (short)(w_ind.ybot - 1);
    t_ind.old = (short)(t_ind.old1 = w_ind.xtop + 1);
    text_color(tfgc_s, tbgc_s);
}

void
dis_artifact()
{
    double  ppc, ind_mv, max_mv, x;
    int     i, j, k, tfgc_s, tbgc_s;


    rd_text_color(&tfgc_s, &tbgc_s);
    text_color(w_ind.wfgc, w_ind.wbgc);
    j = w_ind.xtop + 54;
    k = w_ind.ytop + txtpar.font_height;
    txtpar.text_wind_len = 12;
    i = (limit_mv < 100) ? 1 : 0;
    gprintf(j, k, "%4.*f mPa", i, limit_mv);

    ppc = (Sen.AD && Sen.MP) ? (Sen.AD * Sen.MP) : 1;
    ind_mv = t_ind.new * 1000 / ppc;
    max_mv = (ind_mv > limit_mv) ? ind_mv : limit_mv;
    range_mv = (float) ceil(max_mv * 1.1 / 5) * 5;
    j = (w_ind.xbot - w_ind.xtop);
    k = (short)((int) (w_ind.xtop + limit_mv / range_mv * j));
    mouse_exclude(w_ind.xtop, w_ind.ytop, w_ind.xbot, w_ind.ybot);
    gr_rectf(w_ind.xtop + 1, t_ind.top + 4, t_ind.old1, t_ind.bot, scrn_c[C_INDB]);
    gr_rectf(t_ind.old, t_ind.top, t_ind.old, t_ind.bot, scrn_c[C_INDB]);
    t_ind.old = (short)(k);
    x = w_ind.xtop + ind_mv / range_mv * j;
    if (x > w_ind.xbot)
	x = w_ind.xbot;
    k = (int) x;
    t_ind.old1 = (short)(k);
    gr_rectf(w_ind.xtop + 1, t_ind.top + 4, k, t_ind.bot, scrn_c[C_INDN]);
    gr_rectf(t_ind.old, t_ind.top, t_ind.old, t_ind.bot, scrn_c[C_INDL]);
    mouse_show(1);
    text_color(tfgc_s, tbgc_s);
}

void
compute_resid(int32_t *p)
{
    int     i, start, end;
    float   v;
    int32_t    tm, mn, mx;

    v = (float)(1 / (swp1set * Sen.AD));
    mn = mx = 0;
    start = (int) (0.001 * rate + 0.5);	/* start = 1 ms */
    end = (int) (0.004 * rate + 0.5);	/* end = 4 ms */
    for (i = start; i < end; i++) {
	tm = p[i];
	if (mx < tm)
	    mx = tm;
	if (mx < -tm)
	    mx = -tm;
	if (mn > tm)
	    mn = tm;
	if (mn > -tm)
	    mn = -tm;
    }
    resid_vpp = (float)((mx - mn) * v);
}

int
check_rej(int32_t *p)
{
    int     i, start, end;
    int32_t    tm, m, U;

    if (dpoae_flag) {
	start = 0;
	end = buflen;
    } else {
	start = itime1;
	end = itime2;
    }
    U = (int32_t) (limit_mv * 0.001 * Sen.AD * Sen.MP) * swp1set;
    m = 0;
    for (i = start; i < end; i++) {
	tm = p[i];
	if (tm < 0)
	    tm = -tm;
	if (tm > m)
	    m = tm;
    }
    t_ind.new = (float) m / swp1set;
    dis_artifact();
    if (m > U)
	return (1);
    return (0);
}

int
init_aver()
{
    int     i;

    xrange = buflen;
    for (i = 0; i < buflen; i++) {
	accbuf_a[i] = 0;
	accbuf_b[i] = 0;
    }
    i = spm[Stim_mode.at].nset * buflen;
    return (i);
}

static float t_stab;

void
show_A_B(int c, int m, int nss, int chan)
{
    double   sa = 0, sb = 0, saa = 0, sbb = 0, sab = 0;
    double  sx = 0, sy = 0, sxx = 0, syy = 0;
    double  A, B, eaa, ebb, eab, rep;
    double  X, Y, exx, eyy, ech, noi, scl;
    float   rb[5];
    int     i, j, k, n, yb, ym, yt;

    if (c <= 0)
        return;

    display_reset = 1;
    dfgc = scrn_c[C_WAVE];
    if (nss > 0)
        dis_stim(0, swpbuf, 1, swp1set, nss);
    t_stab = (float) stability(1, swpbuf, savbuf, itime1);
    dis_cali();

    R_clear();

    i = w_sig.ytop + (w_sig.ybot - w_sig.ytop - txtpar.font_height) / 2;
    text_color(scrn_c[C_WAVA], w_sig.wbgc);
    gprintf(w_sig.xtop + 4, i, "A");
    text_color(scrn_c[C_WAVB], w_sig.wbgc);
    gprintf(w_sig.xtop + 4, i + txtpar.font_height, "B");

    w_dis = &w_sig;
    w_sig.xtop += 20;

    show_xlabel(xbegin * 1000.0 / rate, xrange * 1000.0 / rate, 1, "ms");
    w_sig.ybot -= txtpar.font_height;
    gr_recto(w_sig.xtop, w_sig.ytop, w_sig.xbot, w_sig.ybot, menucolor.boxc);

    yb = w_sig.ybot;
    ym = (w_sig.ytop + w_sig.ybot) / 2;
    yt = w_sig.ytop;
    w_sig.ybot = m ? ym : yb;
    dfgc = scrn_c[C_WAVA];
    disply(c * swp1set, &accbuf_a[xbegin], xrange, 1);
    display_reset = 0;
    w_sig.ybot = yb;
    w_sig.ytop = m ? ym : yt;
    dfgc = scrn_c[C_WAVB];
    disply(c * swp1set, &accbuf_b[xbegin], xrange, 0);
    w_sig.ytop = yt;
    w_sig.xtop -= 20;
    w_sig.ybot += txtpar.font_height;

    for (i = itime1; i < itime2; i++) {
	A = accbuf_a[i];
	B = accbuf_b[i];
	sa += A;
	sb += B;
	saa += A * A;
	sbb += B * B;
	sab += A * B;
	X = accbuf_a[i] + accbuf_b[i];
	Y = accbuf_a[i] - accbuf_b[i];
	sx += X;
	sy += Y;
	sxx += X * X;
	syy += Y * Y;
    }
    n = itime2 - itime1;
    if (n == 0 || saa == 0 || sbb == 0 || sxx == 0 || syy == 0)
	return;
    eaa = (saa - sa * sa / n) / n;
    ebb = (sbb - sb * sb / n) / n;
    eab = (sab - sa * sb / n) / n;
    exx = (sxx - sx * sx / n) / n;
    eyy = (syy - sy * sy / n) / n;
    scl = (float)(2.5e+4 / (c * swp1set * Sen.AD * Sen.MP));
    ech = dbv(sqrt(exx) * scl);
    noi = dbv(sqrt(eyy) * scl);
    rep = eab / sqrt(eaa * ebb);
    if (rep < 0) 
        rep = 0;
    text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
    j = w_info.xtop + 4;
    k = w_info.ybot - txtpar.font_height - 1;
    txtpar.text_wind_len = 100;
    j += 8 * gprintf(j, k, "Echo: %5.1lf dB SPL  ", ech);
    gprintf(j, k, "Noise: %5.1lf dB SPL", noi);
    k += txtpar.font_height;
    j = w_info.xtop + 4;
    repro(accbuf_a, accbuf_b, buflen, rb);
    i = gprintf(j, k, "Repro: %4.1f%%  (%4.1f | %4.1f %4.1f %4.1f %4.1f)",
	100 * rep, rb[0], rb[1], rb[2], rb[3], rb[4]);

    draw_w(&w_spec);
    fft_A_B(accbuf_a, accbuf_b, 1, c, 1, 0);
    show_fft(teoae.fft_xrange, teoae.fft_yrange, teoae.levref, scrn_c[C_APBS], 1);
    fft_A_B(accbuf_a, accbuf_b, 0, c, 1, 0);
    show_fft(teoae.fft_xrange, teoae.fft_yrange, teoae.levref, scrn_c[C_AMBS], -1);
    dfgc = scrn_c[C_WAVE];
}

int
message_out(int *x, int *y)
{
    int     j, k, m;

    j = draw_w(&w_msg);
    k = w_msg.ytop + txtpar.font_height;
    gprintf(j, k, "Space refresh");
    k += txtpar.font_height;
    gprintf(j, k, "-> or <- keys");
    k += txtpar.font_height;
    gprintf(j, k, "adjust reject");
    k += txtpar.font_height;
    gprintf(j, k, "Esc to stop");
    k += txtpar.font_height + 8;
    gprintf(j, k, "Sweep Sets");
    k += txtpar.font_height;
    gprintf(j, k, "Remaining");
    k += txtpar.font_height;
    gprintf(j, k, "%5d (%2d)", acc_sets, swp1set);
    m = k + txtpar.font_height;
    gprintf(j, m, "Accepted");
    m += txtpar.font_height;
    *x = j;
    *y = k;
    return (m);
}

void
filter_it(int32_t *lptr, int n)
{
    if (F1_cutoff > 0 || F2_cutoff > 0) {
	long2float(lptr, n);
	bwfilt((float *) lptr, n, F1_cutoff / rate, 2, 1);
	bwfilt((float *) lptr, n, F2_cutoff / rate, 6, 0);
	float2long(lptr, n);
    }
}

int
fold_it()
{
    int i, j, ssw;
    int32_t val, stm, res;
    short ns, *sw, *rw, rd;

    ns = spm[Stim_mode.at].nset;
    sw = spm[Stim_mode.at].fold_stm;
    rw = spm[Stim_mode.at].fold_res;
    rd = spm[Stim_mode.at].res_div;
    for (i = 0; i < buflen; i++) {
        stm = res = 0;
        for (j = 0; j < ns ; j++) {
            val = swpbuf[i + j * buflen];
            stm += val * sw[j];
            res += val * rw[j];
        }
        swpbuf[i] = stm;
        swpbuf[i + buflen] = res / rd;
    }
    ssw = 0;
    for (j = 0; j < ns ; j++)
        ssw += sw[j];
    return (ssw);
}

void
take_the_aver(int len, int32_t *p)	// detrend
{
    double  sum, avg;
    int     i;

    sum = 0;
    for (i = 0; i < len; i++) {
	sum += p[i];
    }
    avg = sum / len;
    for (i = 0; i < len; i++) {
	p[i] = nint(p[i] - avg);
    }
}

void
ramp_it(int32_t *buf)
{
    double  r;
    int     i, j1, j2;

    r = (iramp > 0) ? 1.0 / iramp : 1;
    j1 = itime1 - iramp;
    j2 = itime2 + iramp;
    for (i = 0; i < j1; i++)
        buf[i] = 0;
    for (i = j1; i < itime1; i++)
        buf[i] = (int32_t) (buf[i] * r * (i - j1));
    for (i = itime2; i < j2; i++)
        buf[i] = (int32_t) (buf[i] * r * (j2 - i));
    for (i = j2; i < buflen; i++)
        buf[i] = 0;
}

/* display the stimulus, average a/d into  accum */
void
teoae_aver()
{
    char    outmsg[3][16];
    int     saved, mx, my;
    int     np, i, sweep, j, k, m, cnt = 0, sweeps = 0;
    int     alt = 0, escflg = 0, nss = 0;
    int32_t    run_time, update, tr, *fldbuf;
    short   data;

    if (acc_sets <= 0 || buflen <= 0)
	return;

    np = init_aver();
    if (!np)
	return;

    saved = save_w(&w_msg);
    m = message_out(&j, &k);

    pre_dis_ind("Artifact", "Limit");
    dis_artifact();

    tr = (int32_t) (teoae_refresh * CLOCKS_PER_SEC);
    run_time = (int32_t)(clock());
    update = run_time + tr;
    fldbuf= swpbuf + buflen;	/* swpbuf offset */
    sweep = acc_sets;
    while (sweep && (!escflg || alt)) {
	if(update < clock()) {
	    show_A_B(sweeps, 0, nss, 0);
	    update = clock() + tr;
	}
	if (dspdget()) {
	    dspdread(&data);
	    if (data <= 0) {
		dspuarr(dsppar.acc, np * 2, (short *) swpbuf);
		dsprst(TRUE);
		dsprst(FALSE);
		take_the_aver(np, swpbuf);
		nss = fold_it();
		cnt++;
		filter_it(fldbuf, buflen);
		compute_resid(fldbuf);
		ramp_it(fldbuf);
                text_color(scrn_c[C_MSGH], scrn_c[C_MSGB]);
		if (!check_rej(fldbuf)) {
		    if (alt) {
			sweep--;
			gprintf(j, k, "%5d", sweep);
			for (i = 0; i < buflen; i++)
			    accbuf_b[i] += fldbuf[i];
			alt = 0;
		    } else {
			for (i = 0; i < buflen; i++)
			    accbuf_a[i] += fldbuf[i];
			alt = 1;
		    }
		    sweeps = acc_sets - sweep;
		    if (sweeps == 1 && !alt) {
	                update = (int32_t)(clock());
			memcpy(savbuf, swpbuf, (size_t)(buflen) * sizeof(int32_t));
			tok_store(nss, swpbuf);
                        memcpy(swpbuf, savbuf, (size_t)(buflen) * sizeof(int32_t));
		    }
		}
		gprintf(j, m, "%d / %d", 2 * sweeps + alt, cnt);
	    }
	}
	while (check_event()) {
	    i = getevent();
	    if (i == 27 || i == 3) {
		escflg = 1;
	    } else if (i == ' ' && sweeps) {
	        update = (int32_t)(clock());
	    } else if (i == 'd' && sweeps) {
		R_clear();
		disply(swp1set, fldbuf, np, 1);
	    } else if(i == SINGLE_CLICK) {
	    	mouse_position(&mx, &my);
	    	if(in_wind(mx, my, w_msg)) {
	    	    i = (my - w_msg.ytop) / txtpar.font_height;
	    	    if(i == 0 && sweeps) 
	    	        show_A_B(sweeps, 0, nss, 0);
	    	    else if(i == 3) 
	    	        escflg = 1;
	    	}
	    } else if(i == 9) {
		i = (int) (0.001 * (float) rate);
		xbegin += i;
		if (xbegin + xrange >= buflen) xbegin = 0;
	    } else if(i == (FN|15) && xbegin > 0) {
		i = (int) (0.001 * (float) rate);
		xbegin -= i;
		if (xbegin < 0) xbegin = 0;
	    } else if(i == '+') {
		xrange += (int) (0.001 * (float) rate);
		if (xbegin + xrange > buflen) xrange = buflen - xbegin;
	    } else if(i == '-') {
		xrange -= (int) (0.001 * (float) rate);
		if (xrange < 16) xrange = 16;
	    } else {
		proc_art(i);
	    }
	}
    }

    gr_beep();
    if (saved)
	close_w(&w_msg);
    sweeps = acc_sets - sweep;
    if (!sweeps)
	return;
    xbegin = 0;
    xrange = buflen;
    show_A_B(sweeps, 0, nss, 0);

    run_time -= clock();
    j = w_sig.xtop + 1;
    k = (w_sig.ytop + w_sig.ybot) / 2;

    tok_store(sweeps, accbuf_a);
    tok_store(sweeps, accbuf_b);

    genfn("T", "TOK", file_name, &teoae_counter);
    snprintf(outmsg[0], sizeof(outmsg[0]), "%2d/%c%c/%c%c", (int) (file_name[2] - 'A' + 1),
	file_name[3], file_name[4], file_name[0], file_name[1]);
    i = (int) ((float) -run_time / CLOCKS_PER_SEC);
    snprintf(outmsg[2], sizeof(outmsg[2]), "%dx%d sweeps", sweeps, swp1set);
    sweeps *= 2;
    snprintf(outmsg[1], sizeof(outmsg[1]), "%.0f%% (%d/%d)", (float) sweeps * 100.0 / cnt,
	sweeps, cnt);
    t->hitch.time = (int16_t)(i);
    t->hitch.stab = t_stab;
    t->hitch.rejlmt = limit_mv;
    t->hitch.resid_vpp = resid_vpp;
    memcpy(t->hitch.outmsg, outmsg, 3 * 16);
    twrhitch(t);
    tclose(t);
    t = NULL;

    if (decide(1, 2, "Save the file", file_name)) {
	rename(tmp_tok_file, file_name);
	teoae_counter++;
	write_counter_file();
    } else {
    	strcpy(file_name, not_saved);
    }

    fileflag = 0;
    out_msg(i, outmsg);
}
