/* hear_w.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <dsp.h>
#include <menu.h>
#include <util.h>
#include "savage.h"
#include "color.h"
#include "screen.h"
#include "version.h"
#include "putt.h"

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif /* WIN32 */

#define BREAK           1
#define WAIT_COUNT      4
#define BOXSIZ          75
#define MAXREV		9
#define MAXF		16000
#define MINF		250
#define MAXNCND		64
#define TWOPI		(8 * atan(1.0))

char   *atline();
double  unirand();
double  get_first_lev();
double  get_next_lev();
double  pick_ml_lev();
double  ml_fit(int, double, float *, char *, float *, float *, float *);
double  cmp_sd_lev();
int     dis_patient(void);
int     hear_it(double);
int     draw_w(WIND *);
int     check_list(char *, char **);
int     dis_stim(int load, void *p, enum TYPE type);
int     adjust_f(float **);
int     chk_neg(float **);
int     button_down(void);
int     get_th_src_fn(char *);
int     stop_check(char *, char *, int);
void    open_stim_w(char *);
void    bwfilt(float *, int, double, int, int);
void    show_hearing_file(char *);
void    show_cond(char *, int);
void    grab_probe(void);
void    comp_tpr(double, int);
void    comp_cpr(void);
void    corner(char *);
void    init_cpr(void);
void    init_tpr(void);
void    set_seed(int32_t);
void    set_algo(int, int);
void    set_levlim(double, double);
void    set_obser(double, double, double, double);

extern char *month_name[];
extern char tokstr[], line[], o_file_name[];
extern char thsfn[];
extern int sig_type;
extern int hear_cal_cnt, hear_tth_cnt;
extern float z1_freq, notch_freq;
extern float L1b, L2b, F1b, F2b;
extern float L1e, L2e, F1e, F2e, ATb, ATe, noise, snr;
extern int32_t run_time;
extern struct {
    int     top, bot, old, old1;
    float   new, lim;
}       t_ind;
extern TOGGLETYPE levref, sim_obs, f5type, distype, cond_thl;
extern TOGGLETYPE thresh_algo, thresh_task;
extern TOGGLETYPE thresh_rand, thresh_cnct;
extern MENUITEM file_opt[];
extern MENUITEM data_type[];

static int datfmt = 2;
static int nstd = 5;
static int nnot = 7;

#ifdef MAP
/* Ref. Eq. Th. Zwislocki (MAP) Killion, 1978 */
 * from Table AI in Wilbur, Kruger, and Killion (1988, JASA 83, 669-676)
 */
float   retspl[9][2] = {
    {125.0, 30.0},
    {250.0, 19.0},
    {500.0, 12.0},
    {1000.0, 9.0},
    {2000.0, 15.0},
    {3000.0, 15.5},
    {4000.0, 13.0},
    {6000.0, 13.0},
    {8000.0, 14.0}
};
#else /* MAP */
/* RETSPL for Occluded Ear Simulator
 * from ANSI S3.6-1997
 */
float   retspl[11][2] = {
    {125.0, 28.0},
    {250.0, 17.5},
    {500.0, 9.5},
    {750.0, 6.0},
    {1000.0, 5.5},
    {1500.0, 9.5},
    {2000.0, 11.5},
    {3000.0, 13.0},
    {4000.0, 15.0},
    {6000.0, 16.0},
    {8000.0, 15.5}
};
#endif /* MAP */

TOGGLETYPE freq_set_1 = {
    0,
    { "Std+Notch", "Standard", "Notch", "Single", "Wide", "Optional", NULL },
};

TOGGLETYPE freq_set_2 = {
    3,
    { "F2", "F1", "Fd", "F2,F1,Fd", NULL },
};

static char *rsp_hist;
static char *max_hist;
static char *tth_tmp_file = "out.tth";
static double fset[MAXNCND] = {0};
static double so_a = 0, so_b = 1, so_k = 0.2;
static double dsp_scale[2] = {1, 1};
static float *lev_hist;
static float freq_ratio = 1.2F;
static float F2_freq = 1000;
static int cal_flg = 0;
static int ncnds = 0;
static FILE *ofp = NULL;
static WIND w_dp;

void
out_message()
{
    int     j, k;

    j = draw_w(&w_info);
    k = w_info.ytop + txtpar.font_height;
    gprintf(j, k, "  Hearing Threshold Test (%s,%s)", 
            thresh_task.strs[thresh_task.at], 
            thresh_algo.strs[thresh_algo.at]);
    k += 2 * txtpar.font_height;
    if (thresh_task.at) {
	gprintf(j, k, "Select interval '1' or '2'.");
	k += txtpar.font_height;
    } else {
	gprintf(j, k, "Respond with 'y' or 'n'.");
	k += txtpar.font_height;
    }
}

float   reversals[MAXREV];
float   stddev, avg;
int     nrev;

double
ref_eq_thr(double f)
{
    double  rl = 0;
    int     i;

    i = 0;
    if (f < retspl[0][0])
	rl = retspl[0][1];
    else if (f >= retspl[8][0])
	rl = retspl[8][1];
    else {
	while (i++ < 8) {
	    if (f >= retspl[i][0] && f < retspl[i + 1][0]) {
		rl = f - retspl[i][0];
		rl /= (retspl[i + 1][0] - retspl[i][0]);
		rl *= (retspl[i + 1][1] - retspl[i][1]);
		rl += retspl[i][1];
		break;
	    }
	}
    }
    return (rl);
}

void
fill_full()
{
    int     i;

    for (i = strlen(line); i < 42; i++)
	line[i] = ' ';
    line[i] = 0;
}

double
volt_out(double f, double db, int c)
{
    int     i;
    double  mag, osen, ref, uv_rms;
    SAV_FFT *p;

    p = (c == 0) ? &A_fft : &B_fft;
    if (p->npts == 0) {
        return (0);
    }
    i = nint(f * (float) p->npts * 2.0 / p->rate);
    osen = Sen.MP * Sen.AD / p->rp[i];	            // cnt/Pa
    ref = sqrt(2.0) * spl_ref * osen;	            // pk @ 0 dB SPL
    mag = pow(10.0, db / 20.0) * ref;		    // max. integer value
    uv_rms = 1e6 * mag / Sen.AD / sqrt(2.0);	    // uV rms
    return (20 * log10(uv_rms));
}

static double
gen_stim(double f, double db, int c, int *clip)
{
    int     i;
    double  mag, osen, ref, dp, amp, lev;
    SAV_FFT *p;

    p = (c == 0) ? &A_fft : &B_fft;
    if (p->npts == 0) {
        return (-90);
    }
    i = nint(f * (float) p->npts * 2.0 / p->rate);
    osen = Sen.MP * Sen.AD / p->rp[i];	            // cnt/Pa
    ref = sqrt(2.0) * spl_ref * osen;	            // pk @ 0 dB SPL
    mag = pow(10.0, db / 20.0) * ref;
    dsp_scale[c] = dspsets(mag / MAXPOSINT);
    amp = limit(0.7, mag / dsp_scale[c], MAXPOSINT);
    dp = TWOPI * f / rate;
    for (i = 0; i < buflen; i++)
	outbuf[i] = nint(amp * sin(dp * i));
    lev = 20 * log10(amp * dsp_scale[c] / ref);
    *clip = (lev < db);
    return (lev);
}

void
sig_buf(int N, float *p, int np)
{
    float   v, sum, sumsq;
    float   min, max, val;
    int     i;

    if ((np <= 0) || (N <= 0))
	return;

    sum = sumsq = 0;
    min = max = p[0];
    for (i = 0; i < np; i++) {
	val = p[i];
	v = (float) val;
	sum += (float) val;
	if (val < min)
	    min = val;
	if (val > max)
	    max = val;
	sumsq += v * v;
    }
    v = (1 / Sen.AD) / N;
    val = (sumsq - sum * sum / np) / np;
    sig_v.rms = (float) ((val > 0) ? sqrt((sumsq - sum * sum / np) / np) * v : 0);
    sig_v.dco = (sum / np) * v;
    sig_v.ppk = max * v;
    sig_v.npk = min * v;
    sig_v.vpp = sig_v.ppk - sig_v.npk;
    sig_v.mvdc2 = sig_v.dco * 1000;
}

void
dis_tone2(double f)
{
    double  fft_lev;
    int     j, k;

    fft_stim((float *) outbuf, 1, 0);
    sig_buf(swp1set, (float *) outbuf, buflen);

    text_color(scrn_c[C_MSGF], scrn_c[C_MSGB]);
    txtpar.text_wind_len = (w_msg.xbot - w_msg.xtop) / 8;
    j = w_msg.xtop + 4;
    k = w_msg.ytop + txtpar.font_height;
    fft_lev = pickat(f);
    gprintf(j, k, "FFT Level = %-4.1f dBSPL", fft_lev);
    k += txtpar.font_height;
    gprintf(j, k, "RMS Level = %-4.1lf dBSPL", vtodb(sig_v.rms));
    k += txtpar.font_height;

    text_color(scrn_c[C_RSPF], scrn_c[C_RSPB]);
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop) / 8;
}

/**************************************************************************/

// 2AFC hearing test

int
do_hear2_test()
{
    char *cs;
    double  cur = 0, f, et, spl, con, sil, fpl, tml, dbv, max_lev;
    float   pfk, pfa, pfb;
    int     j, i, k, x, y, g, evnt = 0, yn = 0, chan = 0, clip = 0;
    int32_t    ttime, old, etime;
    short   data;
    unsigned int cnt, c, trial;

    i = w_sig.xtop + 4 + BOXSIZ / 2;
    j = w_sig.ytop + 3 * txtpar.font_height;
    gr_recto(i, j, i + BOXSIZ, j + BOXSIZ, scrn_c[C_WAVB]);
    i += BOXSIZ * 3 / 2;
    gr_recto(i, j, i + BOXSIZ, j + BOXSIZ, scrn_c[C_WAVB]);

    etime = clock();
    max_lev = hear.maxlev;
    for (k = 0; k < ncnds; k++) {
	f = adjustf(fset[k]);
        if (sim_obs.at)
            set_obser(ref_eq_thr(f), so_a, so_b, so_k);
	for (trial = 0; trial < (unsigned) hear.max_trial; trial++) {
	    if (trial == 0) {
                avg = 0.0;
                stddev = 1000.0;
                nrev = 0;
                cur = get_first_lev(hear.start, hear.step);
	    } else if (stop_check(rsp_hist, max_hist, trial)) {
		break;
	    } else if (stddev < hear.sd || nrev >= MAXREV) {
                break;
	    } else {
                cur = get_next_lev(cur, yn);
	    }
	    if (!sim_obs.at) {
   		cur = gen_stim(f, cur, chan, &clip);
            }
            cs = clip ? "(clip)" : (cur >= max_lev) ? "(max) " : "      ";
	    c = (unirand() > 0.5);
	    x = w_info.xtop + 4,
	    y = w_info.ybot - 2 * txtpar.font_height;
	    sprintf(line, "Frequency=%.0f Hz, Level=%.0f dB %s", f, cur, cs);
	    fill_full();
	    gprintf(x, y, line);
	    i = w_sig.xtop + 4;
	    j = w_sig.ytop + 2 * txtpar.font_height;
            i += BOXSIZ / 2;
	    if (sim_obs.at) {
	        g = hear_it(cur) ? c : (unirand() > 0);
	        evnt = g ? '1' : '2';
	        gprintf(i, j, "Simulated Listener ... %c", evnt);
		if (check_event() && (getevent() == 27))
		    evnt = 27;
	    } else {
    	        line[0] = 0;
	        fill_full();
	        gprintf(i, j, line);
	        old = clock() + WAIT_COUNT;
	        while (clock() && clock() < old)
		    continue;

	        dsprst(TRUE);
	        if (c)
	            dspdarr(dsppar.dac_a, buflen, outbuf);
	        else
	            dspdarr(dsppar.dac_a, buflen, (short *) accbuf);
                dspsarr(dsp_scale[chan], chan);
	        dsprst(FALSE);

	        gprintf(i, j, "Interval #1");
	        j += txtpar.font_height;
	        gr_rectf(i, j, i + BOXSIZ, j + BOXSIZ, scrn_c[C_WAVA]);
	        cnt = 0;
	        while (cnt < (unsigned) nsets) {
		    if (dspdget()) {
		        dspdread(&data);
		        if (data <= 0) {
                            if (c) {
                                dspuarr(dsppar.acc, buflen * 2, outbuf);
                                long2float((int32_t *) outbuf, buflen);
                            }
			    cnt++;
			}
		        check_event();
		    }
	        }
	        dsprst(TRUE);
	        gr_rectf(i, j, i + BOXSIZ, j + BOXSIZ, scrn_c[C_RSPB]);
	        gr_recto(i, j, i + BOXSIZ, j + BOXSIZ, scrn_c[C_WAVB]);

	        if (c)
		    dspdarr(dsppar.dac_a, buflen, (short *) accbuf);
	        else
		    dspdarr(dsppar.dac_a, buflen, outbuf);
	        dsprst(FALSE);
	        j -= txtpar.font_height;
	        i += BOXSIZ * 3 / 2;
	        gprintf(i, j, "Interval #2");
	        j += txtpar.font_height;
	        gr_rectf(i, j, i + BOXSIZ, j + BOXSIZ, scrn_c[C_WAVA]);
	        cnt = 0;
	        while (cnt < (unsigned) nsets) {
		    if (dspdget()) {
		        dspdread(&data);
		        if (data <= 0) {
                            if (!c) {
                                dspuarr(dsppar.acc, buflen * 2, outbuf);
                                long2float((int32_t *) outbuf, buflen);
                            }
			    cnt++;
			}
		        check_event();
		     }
	         }
	        dsprst(TRUE);
                dis_tone2(f);
	        gr_rectf(i, j, i + BOXSIZ, j + BOXSIZ, scrn_c[C_RSPB]);
	        gr_recto(i, j, i + BOXSIZ, j + BOXSIZ, scrn_c[C_WAVB]);
	        i -= BOXSIZ + 50;
	        j += BOXSIZ + 30;
	        gprintf(i, j,
		    "In which interval did you hear the tone (1/2)?");
	        evnt = getevent();
	        gprintf(i, j, 
	            "                                              ");
	    }
	    if (evnt == 3) {
                exit(0);
	    } else if (evnt == 27 || evnt == 'B') {
		break;
	    } else if (evnt == 'R') {
		trial = 0;
	    } else {
		if (evnt == '1') {
		    yn = c;
		} else if (evnt == '2') {
		    yn = !c;
		} else {
		    trial = 0;
		    gprintf(i, j, " Error ");
		    continue;
		}
                gprintf(i, j, yn ? "Correct" : " Wrong ");
		lev_hist[trial] = (float) cur;
		rsp_hist[trial] = yn;
		max_hist[trial] = clip || (cur >= max_lev);
		avg = (float) pick_ml_lev();
		stddev = (float) cmp_sd_lev();	/* get stddev ml prob */
		x = w_info.xtop + 4,
		y = w_info.ybot - 2 * txtpar.font_height;
		sprintf(line, "Frequency=%.0f Hz, Level=%.0f dB  ", f, cur);
		fill_full();
		gprintf(x, y, line);
		y += txtpar.font_height;
		sprintf(line, "ml=%.0f dB, sd=%.1f, trial=%d/%d",
		    avg, stddev, trial + 1, hear.max_trial);
		fill_full();
		gprintf(x, y, line);
	    }			/* end of else { */
	}			/* end of for(;;) */
        if (thresh_algo.at == 3) {      // ANSI tracking?
            spl = lev_hist[trial - 1];
        } else {
            spl = ml_fit(trial, avg, lev_hist, rsp_hist, &pfk, &pfa, &pfb);
        }
	con = sim_obs.at ? (-10 * log10(rho_c)) : thev_cond(f);
	dbv = volt_out(f, spl, chan);
	if (datfmt == 2) {
	    thev_adjust(f, spl, &sil, &fpl, &tml);
	}
	ttime = clock();
	et = (ttime - etime) / (double) CLOCKS_PER_SEC;
	etime = ttime;
        if (ofp) {
            if ((hear.protocol == 1) && (freq_set_1.at == 0)) {
                if (thresh_rand.at == 0 && k == nstd)
                    fprintf(ofp, ";\n");
		if (datfmt == 2) {
		    fprintf(ofp, "%7.1f %5.1f %5.1f %5.1f %5.1f %5.1f %6.1f %4.1f ",
			f, spl, sil, fpl, dbv, tml, ref_eq_thr(f), stddev);
		} else {
		    fprintf(ofp, "%7.1f %5.1f %6.1f %5.1f %4.1f %4.2f %4.2f %4.2f ",
			f, spl, con, ref_eq_thr(f), stddev, pfk, pfa, pfb);
		}
            }
	    fprintf(ofp, "%5.1f %4d", et, trial);
	    for (i = 0; i < (int) trial; i++)
	        fprintf(ofp, " %3.0f%c", lev_hist[i], rsp_hist[i] ? 'c' : 'w');
	    fprintf(ofp, "\n");
	}
	if (evnt == 3) {
	    exit(0);
	} else if (evnt == 27) {
	    break;
	} else if (evnt == 'B') {
	    k -= 2;
	    if (k < 0)
		k = -1;
	}
    }

    return (k);
}

/************************************************************************/

// Y/N hearing test

int
do_hear1_test()
{
    char *cs;
    double  cur = 0, f, et, spl, con, sil, fpl, tml, dbv, int_btw_s, mn_g, mx_g, max_lev;
    float   pfk, pfa, pfb;
    int     j, i, k, x, y, yn = 0, chan = 0, clip = 0;
    int     rsp_tim_t, evnt = 0;
    int32_t    ttime, etime, yn_timeout, tone_gap;
    short   data;
    unsigned int cnt, trial;

    i = w_sig.xtop + 4 + BOXSIZ / 2;
    j = w_sig.ytop + 3 * txtpar.font_height;
    gr_recto(i, j, i + BOXSIZ, j + BOXSIZ, scrn_c[C_WAVB]);

    rsp_tim_t = (int) (hear.rsp_tim_s * CLOCKS_PER_SEC);
    mn_g = hear.min_gap_s;
    mx_g = hear.max_gap_s;
    max_lev = hear.maxlev;
    etime = clock();
    for (k = 0; k < ncnds; k++) {
        tone_gap = clock() + (int) (mn_g * CLOCKS_PER_SEC);
       	f = fset[k];
        if (sim_obs.at)
            set_obser(ref_eq_thr(f), so_a, so_b, so_k);
	for (trial = 0; trial < (unsigned) hear.max_trial; trial++) {
	    if (trial == 0) {
                avg = 0.0;
                stddev = 1000.0;
                cur = get_first_lev(hear.start, hear.step);
	    } else if (stop_check(rsp_hist, max_hist, trial)) {
                break;
	    } else if (stddev < hear.sd) {
                break;
	    } else {
                cur = get_next_lev(cur, yn);
            }
	    if (!sim_obs.at) {
   		cur = gen_stim(f, cur, chan, &clip);
            }
            cs = clip ? "(clip)" : (cur >= max_lev) ? "(max) " : "      ";
	    x = w_info.xtop + 4,
	    y = w_info.ybot - 2 * txtpar.font_height;
	    sprintf(line, "Frequency=%.0f Hz, Level=%.0f dB %s", f, cur, cs);
	    fill_full();
	    gprintf(x, y, line);
	    i = w_sig.xtop + 4;
	    j = w_sig.ytop + 2 * txtpar.font_height;
	    i += BOXSIZ / 2;
	    if (sim_obs.at) {
	        evnt = hear_it(cur) ? 'Y' : 'N';
 	        gprintf(i, j, "Simulated Listener ... %c", evnt);
		if (check_event() && (getevent() == 27))
		    evnt = 27;
	    } else {
	        while (check_event())	/* flush events */
		    getevent();
	        while (clock() < tone_gap)	/* wait between tones */
		    if (check_event())
		        if (getevent() == 27)
			    break;
                while (button_down()) {     /* wait until button release */
                    gprintf(i, j, "Button Down");
		    if (check_event())
		        if (getevent() == 27)
			    break;
	        }
	        dsprst(TRUE);
	        dspdarr(dsppar.dac_a, buflen, outbuf);
                dspsarr(dsp_scale[chan], chan);
	        dsprst(FALSE);

	        gprintf(i, j, "Signal On   ");
	        j += txtpar.font_height;
	        gr_rectf(i, j, i + BOXSIZ, j + BOXSIZ, scrn_c[C_WAVA]);
	        cnt = 0;
	        while (cnt < (unsigned) nsets) {
		    if (dspdget()) {
		        dspdread(&data);
		        if (data <= 0) {
		            dspuarr(dsppar.acc, buflen * 2, outbuf);
                            long2float((int32_t *) outbuf, buflen);
                            dis_tone2(f);
			    cnt++;
			}
		    } else if (check_event()) {
		        evnt = getevent();
		        if (evnt == 3) {
			    exit(0);
		        } else if (evnt == 27) {
			    break;
		        } else if (evnt == SINGLE_CLICK) {
		            putbackevent('Y');
		        } else if (evnt == 'Y' || evnt == 'y') {
		            putbackevent('Y');
		        }
		    } else if (button_down()) {
		        putbackevent('Y');
		    }
	        }
	        dsprst(TRUE);
	        j -= txtpar.font_height;
	        gprintf(i, j, "Signal Off  ");
	        j += txtpar.font_height;
	        gr_rectf(i, j, i + BOXSIZ, j + BOXSIZ, scrn_c[C_RSPB]);
	        gr_recto(i, j, i + BOXSIZ, j + BOXSIZ, scrn_c[C_WAVB]);
	        j += BOXSIZ + 30;
	        gprintf(i, j, "Did you hear the tone (Y/N)?");
	        yn_timeout = clock() + rsp_tim_t;
                int_btw_s = mn_g + (mx_g - mn_g) * unirand();
	        for (;;) {
		    if (check_event()) {
		        evnt = getevent();
		        if (evnt == 3) {
                            exit(0);
		        } else if (evnt & 0x7F || evnt == 27) {
			    break;
		        } else if (evnt == SINGLE_CLICK) {
		            putbackevent('Y');
		        }
		    } else if (button_down()) {
		        putbackevent('Y');
		    } else if (clock() >= yn_timeout) {
		        putbackevent('N');
	                int_btw_s = 0;
		    }
	        }
                gprintf(i, j, "                                ");
    	        tone_gap = clock() + (int) (int_btw_s * CLOCKS_PER_SEC);
	    }
	    if (evnt == 3) {
                exit(0);
	    } else if (evnt == 27 || evnt == 'B') {
		break;
	    } else if (evnt == 'R') {
		trial = 0;
	    } else {
		yn = (evnt == SINGLE_CLICK || evnt == 'y' || evnt == 'Y');
		lev_hist[trial] = (float) cur;
		rsp_hist[trial] = yn;
		max_hist[trial] = clip || (cur >= max_lev);
		avg = (float) pick_ml_lev();
		stddev = (float) cmp_sd_lev();	/* get stddev from ml prob. */
		x = w_info.xtop + 4,
		y = w_info.ybot - 2 * txtpar.font_height;
		sprintf(line, "Frequency=%.0f Hz, Level=%.0f dB  ", f, cur);
		fill_full();
		gprintf(x, y, line);
		y += txtpar.font_height;
		sprintf(line, "ml=%.0f dB, sd=%.1f, trial=%d/%d",
		    avg, stddev, trial + 1, hear.max_trial);
		fill_full();
		gprintf(x, y, line);
	    }
	}			/* end of for(;;) */
        if (thresh_algo.at == 3) {      // ANSI tracking?
            spl = lev_hist[trial - 1];
        } else {
            spl = ml_fit(trial, avg, lev_hist, rsp_hist, &pfk, &pfa, &pfb);
        }
	con = sim_obs.at ? (-10 * log10(rho_c)) : thev_cond(f);
	dbv = volt_out(f, spl, chan);
	if (datfmt == 2) {
	    thev_adjust(f, spl, &sil, &fpl, &tml);
	}
	ttime = clock();
	et = (ttime - etime) / (double) CLOCKS_PER_SEC;
	etime = ttime;
        if (ofp) {
            if ((hear.protocol == 1) && (freq_set_1.at == 0)) {
                if (thresh_rand.at == 0 && k == nstd)
                    fprintf(ofp, ";\n");
		if (datfmt == 2) {
		    fprintf(ofp, "%7.1f %5.1f %5.1f %5.1f %5.1f %5.1f %5.1f %4.1f ",
			f, spl, sil, fpl, dbv, tml, ref_eq_thr(f), stddev);
		} else {
		    fprintf(ofp, "%7.1f %5.1f %6.1f %5.1f %4.1f %4.2f %4.2f %4.2f ",
			f, spl, con, ref_eq_thr(f), stddev, pfk, pfa, pfb);
		}
            }
	    fprintf(ofp, "%5.1f %4d", et, trial);
	    for (i = 0; i < (int) trial; i++)
	        fprintf(ofp, " %3.0f%c", lev_hist[i], rsp_hist[i] ? 'y' : 'n');
	    fprintf(ofp, "\n");
	}
	if (evnt == 3) {
	    exit(0);
	} if (evnt == 27) {
	    break;
	} else if (evnt == 'B') {
	    k -= 2;
	    if (k < 0)
		k = -1;
	}
    }

    return (k);
}

/***************************************************************************/

static int
hear_open()
{
    if (sim_obs.at) {
        strcpy(patient.Firstname, "Simulated");
        strcpy(patient.Lastname, "Listener");
        sprintf(patient.Comment, "a=%4.2f b=%4.2f k=%4.2f", so_a, so_b, so_k);
    }
    ofp = fopen(tth_tmp_file, "wt");
    if (ofp == NULL) {
	decide(0, 4, "Cannot open file for writing", tth_tmp_file,
	    "HEARING test aborted!", "Press any key or mouse to continue");
	return (0);
    }
    genfn("H", "TTH", file_name, &hear_tth_cnt);
    run_time = clock();
    set_seed(run_time & 0xFFFF);
    fprintf(ofp, ";HEARING\n");
    fprintf(ofp, ";%s\n", VERSION);
    fprintf(ofp, ";LastName:  %s\n", patient.Lastname);
    fprintf(ofp, ";FirstName: %s\n", patient.Firstname);
    fprintf(ofp, ";Ear: %5s\n", patient.Ear == 'R' ? "Right" : (patient.Ear == 'L' ? "Left" : "?"));
    fprintf(ofp, ";ID: %s\n", patient.ID);
    fprintf(ofp, ";Dob: %12s\n", patient.Dob);
    fprintf(ofp, ";Comment: %s\n", patient.Comment);
    fprintf(ofp, ";DATfmt=%d\n", datfmt);
    if (get_th_src_fn(thsfn))
	fprintf(ofp, ";Thev_source=%s\n", thsfn);
    fprintf(ofp, ";Date: %c%c-%s-%c%c\n", file_name[3], file_name[4],
	month_name[file_name[2] - 'A'], file_name[0], file_name[1]);
    fprintf(ofp, ";Level_Ref = %s\n", levref.strs[levref.at]);

    return (1);
}

int
hear_cali()
{
    if (sim_obs.at) {
        notch_freq = 6000;
        return (1);
    }
    for (;;) {
        open_stim_w(probe.calibrate);
        if (check_fit() == 0) {
            return (0);
        }
        display_reset = 1;
        R_clear();
        open_stim_w(probe.calibrate);
        w_dis = &w_sig;
        if (calibrate_it("H"))
            break;
    }
    if (rename(tmp_cal_fn, cali_file)) {
        decide(0, 3, "Can't rename file!", tmp_cal_fn, cali_file);
        return (0);
    }
    text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
    gprintf(w_info.xtop + 4, w_info.ybot - 4, 
        "Wrote calibration data to %s.", cali_file);
    hear_cal_cnt++;
    write_counter_file();

    display_reset = 1;
    dfgc = scrn_c[C_CHNA];
    dis_stim(0, sav_stim, FLOAT);
    display_reset = 0;

    if (ofp) {
        fprintf(ofp, "\n");
        fprintf(ofp, ";Calibrate=%s\n", cali_file);
        fprintf(ofp, ";Stimulus=%s\n", probe.calibrate);
        fprintf(ofp, ";Fz1__=%5.0f Hz\n", z1_freq);
    }
    if (notch_freq <= 0)
        notch_freq = z1_freq;
 
    return (1);
}

void
hear_close()
{
    if (ofp == NULL)
        return;
    fprintf(ofp, "\n");
    fprintf(ofp, ";FileName: %s\n", file_name);
    fprintf(ofp, ";Elapsed_Time = %3.1f (sec)\n", (clock() - run_time) / (double) CLOCKS_PER_SEC);
    fclose(ofp);
    ofp = NULL;
}

int
hear_test()
{
    int nr = 0, nswps, nskps, ramp;
    int32_t stime;
  
    rsp_hist = (char *) calloc(hear.max_trial, sizeof(char));
    max_hist = (char *) calloc(hear.max_trial, sizeof(char));
    lev_hist = (float *) calloc(hear.max_trial, sizeof(float));
    if (lev_hist == NULL) {
	decide(0, 3, "Can't allocate required memory",
	    "HEAR test aborted!", "Press any key or mouse to continue");
	return (0);
    }
    stime = clock();
    if (ofp) {  
        if (hear.protocol == 1) {
            fprintf(ofp, ";Notch=%5.0f Hz\n", notch_freq);
        } else if (hear.protocol == 2) {
            fprintf(ofp, ";F2=%5.0f Hz\n", F2_freq);
        }
        fprintf(ofp, "\n");
        fprintf(ofp, "; Hearing Threshold Test (%s,%s)\n", 
            thresh_task.strs[thresh_task.at], 
            thresh_algo.strs[thresh_algo.at]);
	if (datfmt == 2) {
	    fprintf(ofp, ";Freq    SPL   SIL   FPL   DBuV  TML   RET    sd");
	} else {
	    fprintf(ofp, ";Freq    SPL    G10   RET   sd  pfk  pfa  pfb   ");
	}
        fprintf(ofp, "    ET    nT  Level History (SPL) ...\n;\n");
    }

    nswps = nint((hear.dur * rate) / (double) buflen);
    swp1set = limit(1, nswps - tone.skips, 255);
    nskps = nswps - swp1set;
    nsets = 1;
    if (!sim_obs.at) {
        ramp = nint(hear.ramp_ms * rate / 1000);
        downldpar(2, buflen, nskps, ramp);
        zero_all();
        dspdarr(dsppar.dac_b, buflen, outbuf);
    }
    draw_w(&w_sig);
    out_message();

    set_algo(thresh_algo.at, thresh_task.at);
    set_levlim(hear.minlev, hear.maxlev);
    switch (thresh_task.at) {
    case 0:
        nr = do_hear1_test();      // Y/N task
        break;
    case 1:
        nr = do_hear2_test();      // 2AFC task
        break;
    }
    gr_beep();
    free(rsp_hist);
    free(max_hist);
    free(lev_hist);
    if (ofp) {
        fprintf(ofp, ";\n");
        fprintf(ofp, ";Test_Time = %3.1f (sec)\n", (clock() - stime) / (double) CLOCKS_PER_SEC);
    }

    return (nr);
}

void
build_fset()
{
    double fi, fs, fe, obn, qtroct;
    double F1, F2, Fd, f;
    int k, k1, k2, otemp, o[MAXNCND];
    static int fwide[] = {
        125, 250, 500, 750, 1000, 1500, 2000, 3000, 4000, 6000, 8000, 9000, 10000, 11200
    };
    static int nwide = sizeof(fwide) / sizeof(fwide[0]);

    nstd = hear.hfthr ? 7 : 5;
    if (hear.protocol == 1) {
        qtroct = pow(2.0, 0.25);
        obn = notch_freq / 2;
        while (obn > hear.obn_max)
            obn /= qtroct;
        if (freq_set_1.at == 0) {
            ncnds = nstd + nnot;
            fi = qtroct;
            fs = obn;
        } else if (freq_set_1.at == 1) {
            ncnds = nstd;
            fi = 2;
            fs = 500;
        } else if (freq_set_1.at == 2) {
            ncnds = nnot;
            fi = qtroct;
            fs = obn;
        } else if (freq_set_1.at == 3) {
            ncnds = 1;
            fi = 1;
            fs = notch_freq;
        } else if (freq_set_1.at == 4) {
            ncnds = nwide;
            fi = sqrt(2);
            fs = 125;
        } else {
            fs = hear.f_start;
            fe = hear.f_end;
            ncnds = 1 + nint(fabs(log(fe / fs) / log(2.0)) * hear.pts_oct);
            if (ncnds < 2) {
                fi = 1;
            } else {
                fi = pow(fe / fs, 1.0 / (ncnds - 1));
	        if (hear.hfthr) {
                    ncnds += 2;
                }
            }
        }
        for (k = 0; k < ncnds; k++) {
            o[k] = k;
        }
        if (thresh_rand.at) {
            for (k = 0; k < ncnds; k++) {
                k1 = (int) (unirand() * ncnds);
                k2 = (int) (unirand() * ncnds);
                otemp = o[k1];
                o[k1] = o[k2];
                o[k2] = otemp;
            }
        }
        if (freq_set_1.at == 0) {
            for (k = 0; k < 5; k++)
                fset[o[k]] = adjustf(500 * pow(2.0, (double) k));
	    if (nstd == 7) {
                fset[o[5]] = adjustf(9000);
                fset[o[6]] = adjustf(10000);
	    }
            for (k = nstd; k < ncnds; k++)
                fset[o[k]] = adjustf(fs * pow(fi, (double) (k - nstd)));
        } else if (freq_set_1.at == 4) {    // wide set
            for (k = 0; k < ncnds; k++) {
                fset[o[k]] = adjustf(fwide[k]);
            }
        } else {
            for (k = 0; k < ncnds; k++) {
                if (hear.hfthr && k >= (ncnds - 2)) {
                    if (k == (ncnds - 2)) {
                        f = 9000;
                    } else {
                        f = 10000;
                    }
                } else {
                    f = fs * pow(fi, (double) k);
                }
                fset[o[k]] = adjustf(f);
            }
        }
    } else if (hear.protocol == 2) {
        F2 = adjustf(F2_freq);
        F1 = adjustf(F2_freq / freq_ratio);
        Fd = 2 * F1 - F2;
        if (freq_set_2.at == 0) {
            ncnds = 1;
            fset[0] = F2;
        } else if (freq_set_2.at == 1) {
            ncnds = 1;
            fset[0] = F1;
        } else if (freq_set_2.at == 2) {
            ncnds = 1;
            fset[0] = Fd;
        } else if (freq_set_2.at == 3) {
            ncnds = 3;
            fset[0] = F2;
            fset[1] = F1;
            fset[2] = Fd;
        }
    }
}

int
start_test(char *no_use)
{
    build_fset();
    return (27);
}

int
recalibr(char *no_use)
{
    cal_flg = 1;
    return (27);
}

int
change_ear(char *no_use)
{
    patient.Ear = (patient.Ear == 'L') ? 'R' : 'L';
    fprintf(ofp, ";Ear: %5s\n", (patient.Ear == 'L') ? "Left" : "Right");
    cal_flg = 1;
    return (27);
}

int
done_with_hearing_test(char *no_use)
{
    return (27);
}

MENUITEM hear_menu_1[] = {
    {"&Start Test", NULL, NONE, 0, 0, 1, start_test},
    {"&Frequency Set =", (char *) &freq_set_1, TOGGLE, 8, 0, 1, NULL},
    {"&Notch Frequency (Hz) :", (char *) &notch_freq, FLOAT, 8, 0, 1, NULL},
    {"&Calibrate", NULL, NONE, 0, 0, 1, recalibr},
    {"&Other Ear", NULL, NONE, 0, 0, 1, change_ear},
    {"&Done", NULL, NONE, 0, 0, 1, done_with_hearing_test},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

MENUITEM hear_menu_2[] = {
    {"&Test", NULL, NONE, 8, 0, 1, start_test},
    {"&F2 (Hz) :", (char *) &F2_freq, FLOAT, 7, 0, 1, NULL},
    {"&Ratio :", (char *) &freq_ratio, FLOAT, 6, 3, 1, NULL},
    {"&Set =", (char *) &freq_set_2, TOGGLE, 8, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

void
initial()
{
    w_dp.xtop = w_sig.xtop;
    w_dp.ytop = w_sig.ytop;
    w_dp.xbot = w_sig.xbot;
    w_dp.ybot = ypix - 1;
    w_dp.wbgc = w_sig.wbgc;
    w_dis = &w_dp;
    R_clear();
    w_dp.xtop += 30;
    w_dp.ybot -= txtpar.font_height;
    gr_recto(w_dp.xtop, w_dp.ytop, w_dp.xbot, w_dp.ybot, menucolor.boxc);
}

void
show_hearing_file(char *fn)
{
    FILE   *fpt;
    int     cflg, k, itecnt, datfmt;
    float   f, spl, sil, fpl, cond;
    static char *kw[] = {
	"sweeps", "stimulus", "calibrate", "list",
	"LastName", "FirstName", "ID", "Ear", "DOB", "Comment",
	"FileName", "Date", "Elapsed_Time", "Accepted", 
	"Thev_Source", "Other_Ear", "DATfmt", ""
    };

    if (!check_hearing_file(fn, 1)) {
        return;
    }
    fpt = fopen(fn, "rt");
    initial();
    dis_patient();
    itecnt = 0;
    cflg = 0;
    datfmt = 1;
    while (fgets(line, MAXLINE, fpt) != NULL) {
	if (line[0] == ';') {
	    cflg = 0;
	    line[0] = ' ';
	    get_token(1);
	    k = check_list(tokstr, kw);
	    get_token(0);
	    if (k == 3) {
		show_cond(tokstr, itecnt++);
	    } else if (k == 19) {
		datfmt = atoi(tokstr);
            }
	} else if (strlen(line) > 12) {
	    if (datfmt == 2) {	    // new data format
                sscanf(line, "%f %f %f %f", &f, &spl, &sil, &fpl);
	    } else {		    // old data format
		sscanf(line, "%f %f %f", &f, &spl, &cond);
		if (cond_thl.at)
            	    cond = (float) thev_cond(f);
		sil = (float) (spl + cond + 10 * log10(rho_c));
		fpl = -400;
	    }
	    show_hearing_result(f, spl, sil, fpl, cflg);
	    cflg = thresh_cnct.at;
	}
    }
    fclose(fpt);
}

double
scaled(double x)
{
    return (log10(x / 250.0));
}

void
ytick(int y, int num)
{
    int     i;

    gr_line(w_dp.xtop, y, w_dp.xtop - 4, y, menucolor.boxc);
    gr_line(w_dp.xbot, y, w_dp.xbot - 4, y, menucolor.boxc);
    text_color(scrn_c[C_RSPF], scrn_c[C_RSPB]);
    gprintf(w_dp.xtop - 26, y + txtpar.font_height / 2, "%2d", num);
    if (num % 20 == 0) {
	for (i = w_dp.xtop + 2; i < w_dp.xbot - 4; i += 3)
	    gr_setpix(i, y, menucolor.boxc);
    }
}

void
xtick(int x, int num)
{
    char    s[10];

    gr_line(x, w_dp.ybot, x, w_dp.ybot - 4, menucolor.boxc);
    gr_line(x, w_dp.ytop, x, w_dp.ytop + 4, menucolor.boxc);
    sprintf(s, "%d", num);
    if (num > 8000) {
	num /= 1000;
	sprintf(s, "%d", num);
	num = strlen(s);
	s[num++] = 'K';
	s[num] = 0;
	x -= strlen(s) * 8;
    } else
	x -= strlen(s) * 4;
    text_color(scrn_c[C_RSPF], scrn_c[C_RSPB]);
    gprintf(x, w_dp.ybot + txtpar.font_height - 1, s);
}

void
xtic(int x)
{
    gr_line(x, w_dp.ybot, x, w_dp.ybot - 2, menucolor.boxc);
    gr_line(x, w_dp.ytop, x, w_dp.ytop + 2, menucolor.boxc);
}

/* draw the marker m at (x, y) with color */
void
marker(int x, int y, int m, int color)
{
    static int xx[4][4] = {
	{1, 1, -1, -1},
	{0, 1, 0, -1},
	{0, 1, 0, -1},
	{-1, 0, 1, 0}
    };
    static int yy[4][4] = {
	{-1, 1, 1, -1},
	{-1, 0, 1, 0},
	{-1, 1, 1, 1},
	{-1, -1, -1, 1}
    };
    int     i;

    mouse_exclude(x - 1, y - 1, x + 1, y + 1);
    for (i = 0; i < 4; i++)
	gr_setpix(x + xx[m][i], y + yy[m][i], color);
    gr_setpix(x, y, color);
    mouse_show(1);
}

void
show_hearing_result(float f, float spl, float sil, float fpl, int cflg)
{
    char   *ylab;
    double  ygain, xgain, m_lev;
    double  lev = 0, dbfs = 0, dbof = 0;
    int     xrn, yrn, x, y, i, j, k, m, cl, cm;
    static int FF, MM_lev;

    switch (levref.at) {
    case 0:
	lev = spl;
	dbfs = 100;
	dbof = -10;
	break;
    case 1:
	lev = spl - ref_eq_thr(f);
	dbfs = -100;
	dbof = 90;
	break;
    case 2:
	lev = sil;
	dbfs = 100;
	dbof = -10;
	break;
    case 3:
	lev = fpl;
	dbfs = 100;
	dbof = -10;
	break;
    }
    lev = limit(-10, lev, 90);
    ylab = levref.strs[levref.at];

    xrn = w_dp.xbot - w_dp.xtop;
    xgain = (float) xrn / scaled((float) MAXF);
    ygain = (float) (w_dp.ybot - w_dp.ytop) / dbfs;
    yrn = 10;

    f = (float) (scaled(f) * xgain + w_dp.xtop);
    m_lev = (dbof - lev) * ygain + w_dp.ybot;

    if (!cflg) {
	text_color(scrn_c[C_RSPF], scrn_c[C_RSPB]);
	gprintf(w_dp.xtop - 26, w_dp.ytop + txtpar.font_height, ylab);
	txtpar.text_wind_len = 25;
	for (i = 1; i <= 9; i++) {
	    x = (int) (dbof + i * dbfs * 0.1 + 0.5);
	    y = (int) ((dbof - x) * ygain + w_dp.ybot);
	    ytick(y, x);
	}
	yrn = MINF;
	xtick(w_dp.xtop, yrn);
	for (y = MINF * 2; y <= MAXF; y += y) {
	    x = (int) (scaled((float) y) * xgain) + w_dp.xtop;
	    xtick(x, y);
	    xrn = yrn / 10;
	    while ((yrn += xrn) < y) {
		x = (int) (scaled((float) yrn) * xgain) + w_dp.xtop;
		xtic(x);
	    }
	}
	FF = w_dp.xtop;
	MM_lev = w_dp.ytop;

	i = w_dp.xtop + 12;
	j = w_dp.ybot - 4;
	k = j - txtpar.font_height / 2 + 1;
	marker(i, k, 0, scrn_c[C_CHNA]);
	i += 8;
	gprintf(i, j, "L");
	i += 8 * 2;
	marker(i, k, 1, scrn_c[C_CHNB]);
	i += 8;
	gprintf(i, j, "R");
    }
    if (patient.Ear == 'L') {
	m = 0;
	cl = cm = scrn_c[C_CHNA];
    } else if (patient.Ear == 'R') {
	m = 1;
	cl = cm = scrn_c[C_CHNB];
    } else {
	m = 2;
	cl = cm = scrn_c[C_WAVE];
    }
    mouse_exclude(FF - 1, MM_lev - 1, (int) f + 1, (int) m_lev + 1);
    if (cflg) {	    //  connect symbols
	gr_line(FF, MM_lev, (int) f, (int) m_lev, cl);
	marker(FF, MM_lev, m, cm);
    }
    marker((int) f, (int) m_lev, m, cm);
    mouse_show(1);
    FF = (int) f;
    MM_lev = (int) m_lev;
}

void
Hear_wind()
{
    int     nr = 0, done = 0;
    int     mx, my, c;

    if (!sim_obs.at && (dsp_init() > 0)) {
        if (!(sim_obs.at = decide(1, 1, "Simulate listener?")))
            return;
    }
    top_message("Hearing ");
    probe.rate = adjust_rate(probe.rate);
    grab_probe();
    dis_units(Sen.AD, Sen.MP);
    upperv = (int) (hear.rejlim * 0.001 * Sen.MP * Sen.AD);
    notch_freq = 0;
    file_opt[4].status = 0;
    mx = menu_step(2);
    my = 3 * txtpar.menu_height;
    draw_w(&w_msg);
    if (!hear_open())
        return;
    if (!hear_cali()) {
        hear_close();
        return;
    }
    while(!done) {
        ncnds = 0;
        if (hear.protocol == 1) {
            simple_submenu(mx, my, hear_menu_1);
        } else if (hear.protocol == 2) {
            simple_submenu(mx, my, hear_menu_2);
        }
        if (cal_flg) {
            hear_cali();
            cal_flg = 0;
        } else if (ncnds > 0) {
            nr += hear_test();
        } else {
            if (decide(1, 1, "Terminate hearing test")) {
                hear_close();
                done = 1;
            }
        }
	c = check_event();
	if (c == 3 || c == 27) {
	    done = 1;
	}
    }

    if (sim_obs.at || nr <= 0 || !decide(1, 2, "Save the file", file_name)) {
        strcpy(file_name, tth_tmp_file);
    } else {
        rename(tth_tmp_file, file_name);
        hear_tth_cnt++;
        write_counter_file();
    }
    if (nr > 0) {
	show_hearing_file(file_name);
    }
    dfgc = scrn_c[C_WAVE];
    txtpar.text_wind_len = 34;
    top_message("        ");
}

