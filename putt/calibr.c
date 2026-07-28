/* calibr.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
#include "putt.h"

#define BREAK		1

float   spl2hl(float, float);
float   normalize(float);
int     cal_shut(char *, int);
int     check_list(char *, char **);
int     check_rej(float *);
int     draw_w(WIND *);
int     E_cancel(char *);
int     gen_param_list(void);
int     recalstim(int);
void    cal_gain(SAV_FFT *);
void    fft_A_B(float *, float *, int, int);
void    take_the_aver(int, int32_t *);
void    cal_creat(char *, char *, int);
void    cal_store(int, float *);
void    bwfilt(float *, int, double, int, int);
void    set_channel(int);
void    thev_load(char *);
void    dis_cali();

extern int prob_cal_cnt, hear_cal_cnt;
extern int fileflag, sig_type;
extern char tokstr[], line[], o_file_name[];
extern char *month_name[];
extern char *hear_cal, *probe_cal;
extern MENUITEM file_opt[];

extern TOGGLETYPE xtype;
extern TOGGLETYPE levref;
extern struct {
    int     top, bot, old, old1;
    float   new, lim;
}       t_ind;

extern int prb_flg, prb_cav, g_ncav;

SAV_FFT A_fft, B_fft;
int     repeat = 1;
int     noreject = 0;
int     items = 0;
int32_t    run_time = 0;

static int cal_ramp = 0;

void
zero_all()
{
    int     i;

    for (i = 0; i < buflen; i++) {
	accbuf_a[i] = 0;
	accbuf_b[i] = 0;
	outbuf[i] = 0;
    }
}

extern void hifilter(int32_t *p, int n, double fco);

int
dp_msg(int *x, int *y)
{
    int     j, k, m;

    j = draw_w(&w_msg);
    k = w_msg.ytop + txtpar.font_height;
    gprintf(j, k, "Sweep Sets");
    k += txtpar.font_height;
    gprintf(j, k, "Remaining = %2d (%2d)", nsets, swp1set);
    m = k + txtpar.font_height;
    gprintf(j, m, "Accepted =  ");
    *x = j + 12 * txtpar.font_width;
    *y = k;
    return (m);
}

static int bk_flg = 0;

void
proc_art(int i)
{
    if (i & FN) {
	switch (i) {
	case FN | 77:		/* arrow right */
	    upperv += 2;
	    break;
	case FN | 116:		/* control right */
	    upperv += 20;
	    break;
	case FN | 75:		/* arrow left */
	    upperv -= 2;
	    break;
	case FN | 115:		/* contrl left */
	    upperv -= 20;
	    break;
	case FN | 71:
	    upperv = (int) (0.8 * t_ind.new);
	    break;
	case FN | 79:		/* control end */
	    upperv = (int) (t_ind.new + 0.5);
	    break;
	}
	if (upperv < 0)
	    upperv = 0;
        t_ind.lim = (float) (upperv * 1000.0 / (Sen.MP * Sen.AD));
	dis_artifact();
    } 
}

int
proc_accu()
{
    float  *lp;
    int     i, j, k, m, mx, my;
    int     pair = 0, cnt = 0, total = 0;
    short   data;

    m = dp_msg(&j, &k);
    while (cnt < nsets) {
	if (dspdget()) {
	    dspdread(&data);
	    if (data <= 0) {
		data = buflen * 2;
		if (pair) {
		    dspuarr(dsppar.acc, data, &outbuf[data]);
		    dsprst(TRUE);
		    dsprst(FALSE);
                    long2float((int32_t *) &outbuf[data], buflen);
		    lp = fltbuf + data;
		    for (i = 0; i < buflen; i++)
			lp[i] = fltbuf[i] - fltbuf[i + buflen];
		    pair = !check_rej(lp);
		    if (pair || noreject) {
			for (i = 0; i < buflen; i++) {
			    accbuf_a[i] += fltbuf[i];
			    accbuf_b[i] += fltbuf[i + buflen];
			}
			cnt++;
			if (!noreject) {
			    fft_A_B(accbuf_a, accbuf_b, 1, 2 * cnt);
			    show_fft(probe.fft_xrange, probe.fft_yrange,
				probe.levref, scrn_c[C_APBS], 1);
			    fft_A_B(accbuf_a, accbuf_b, 0, 2 * cnt);
			    show_fft(probe.fft_xrange, probe.fft_yrange,
				probe.levref, scrn_c[C_AMBS], 0);
                            text_color(scrn_c[C_MSGF], scrn_c[C_MSGB]);
			}
		    }
		    pair = 0;
		    gprintf(j, k, "%2d", nsets - cnt);
		} else {
		    dspuarr(dsppar.acc, data, outbuf);
		    dsprst(TRUE);
		    dsprst(FALSE);
                    long2float((int32_t *) outbuf, buflen);
		    pair = 1;
		}
		total++;
		gprintf(j, m, "%d / %d", 2 * cnt, total);
	    }
	}
	while (check_event()) {
	    i = getevent();
	    if (bk_flg > 0) {
		if (i == 'n' || i == 'N')
		    nsets = cnt;
		else if (i == (FN | 60)) {
		    bk_flg = -2;
		    nsets = cnt;
		} else if (i == 'b' || i == 'B') {
		    bk_flg = -1;
		    nsets = cnt;
		}
	    }
	    if (i == 27) {
		nsets = cnt;
		if (total >= 0)
		    total = -(total + 1);
	    } else if (i == SINGLE_CLICK) {
	    	mouse_position(&mx, &my);
		if (mx > w_msg.xtop && mx < w_msg.xbot &&
		    my > w_msg.ytop && my < w_msg.ybot) {
		    i = (my - w_msg.ytop) / txtpar.font_height;
		    if (i == 1) {
			nsets = cnt;
		    } else if (i == 2) {
			nsets = cnt;
			if (total >= 0)
			    total = -(total + 1);
		    }
		}
	    } else {
		proc_art(i);
            }
	}
    }
    dsprst(TRUE);
    return (total);
}

int
sumup(int color, int reset)
{
    int     data;
    extern int stim_flag, scope;

    dfgc = color;
    display_reset = reset;
    for (data = 0; data < buflen; data++)
	accbuf_a[data] += accbuf_b[data];

    text_color(scrn_c[C_RSPF], scrn_c[C_RSPB]);
    if (bk_flg == -2) {
	w_dis = &w_stim;
	stim_flag = 1;
	if (scope < buflen)
	    data = scope;
	else
	    data = buflen;
	if (reset)
	    draw_w(&w_stim);
    }
    dis_cali();
    disply(swp1set * nsets * 2, accbuf_a, data, reset, 0);
    data = fft_stim(accbuf_a, nsets * 2, 1);
    show_fft(probe.fft_xrange, probe.fft_yrange, probe.levref, color, reset);
    if (bk_flg == -2) {
	w_dis = &w_sig;
	stim_flag = 0;
    }
    return (data);
}

/* calibrate single channel */
int
cali_x(int c)
{
    SAV_FFT *x_fft;
    int     color, data;

    if (c == 0) {
        x_fft = &A_fft;
        color = C_CHNA;
    } else {
        x_fft = &B_fft;
        color = C_CHNB;
    }
    set_channel(c);
    if (proc_accu() <= 0) {
	return (0);
    }
    data = sumup(scrn_c[color], 1);
    x_fft->rate = rate;
    if (x_fft->rp == NULL || x_fft->npts != data) {
        if (x_fft->rp != NULL)
            free(x_fft->rp);
	x_fft->rp = (float *) malloc(data * sizeof(float));
	x_fft->npts = data;
    }
    memcpy(x_fft->rp, outbuf, data * sizeof(float));
    memcpy(sav_stim, accbuf, buflen * sizeof(int32_t));
    cal_gain(x_fft);

    return (1);
}

/* setup single channel for calibration */

void
set_channel(int c)
{
    zero_all();
    dsprst(TRUE);	/* Assert the DSP reset	 */
    downldpar(2, buflen, probe.cal_skps, cal_ramp);
    if (c == 0) {
        attenu.att1 = probe.cal_atten;
        dspdarr(dsppar.dac_b, buflen, outbuf);	/* Download stimulus */
        recalstim(0);
        dspdarr(dsppar.dac_a, buflen, outbuf);	/* Download stimulus */
    } else {
        attenu.att2 = probe.cal_atten;
        dspdarr(dsppar.dac_a, buflen, outbuf);	/* Download stimulus */
        recalstim(0);
        dspdarr(dsppar.dac_b, buflen, outbuf);	/* Download stimulus */
    }
    dsprst(FALSE);	/* Assert the DSP reset	 */
    text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
    gprintf(w_info.xtop + 4, w_info.ybot - 4, (c == 0) ?
        "Calibrate channel A ..." : "Calibrate channel B ...");
}

int
test_dsp()
{
    int timeout = 1000;
    short data;

    swp1set = 1;
    zero_all();
    dsprst(TRUE);	/* Assert the DSP reset	 */
    downldpar(0, 8, 0, 0);
    dspdarr(dsppar.dac_b, buflen, outbuf);	/* Download stimulus */
    dspdarr(dsppar.dac_a, buflen, outbuf);	/* Download stimulus */
    dsprst(FALSE);	/* Assert the DSP reset	 */
    while (timeout-- > 0) {
        draw_w(&w_info);
        text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
        gprintf(w_info.xtop + 4, w_info.ybot - 4, "Test DSP ...");
	if (dspdget()) {
	    dspdread(&data);
	    if (data <= 0) {
	        break;
	    }
	}
    }
    dsprst(TRUE);
    return (timeout > 0);
}

/* save the calibration responses into a token file */
void
save_cali_tokens()
{
    int     mo, da, yr, rts, close_it = 0;
    static char outmsg[3][16] = {{0}};

    if (!prb_flg) {
        remove(tmp_cal_fn);
        cal_creat(tmp_cal_fn, hear_cal, 1);
    } else if (prb_cav == 0) {
        remove(tmp_cal_fn);
        cal_creat(tmp_cal_fn, probe_cal, g_ncav);
        prb_cav = 0;
    }
    cal_store(nsets * 2, accbuf_a);
    if (!prb_flg) {
        close_it = 1;
    } else if (++prb_cav >= g_ncav) {
        close_it = 1;
    }
    if (close_it) {
        get_date(&mo, &da, &yr);
        sprintf(outmsg[0], "%02d-%s-%02d", da, month_name[mo], yr);
        rts = nint((clock() - run_time) / (double) CLOCKS_PER_SEC);
        cal_shut((char *) outmsg, rts);
    }
}

int
calibrate_it(char *let)
{
    int accept;

    if (cali_file[0]) {
        if (!prb_flg)
            return (1);
    } else if (*let == 'H') {
	genfn(let, "CAL", cali_file, &hear_cal_cnt);
    } else if (*let == 'P') {
	genfn(let, "CAL", cali_file, &prob_cal_cnt);
    } else {
        return (0);
    }

    nsets = probe.cal_sets;
    swp1set = probe.cal_swps;
    pre_dis_ind("Artifact", "Limit");
    dis_artifact();

    draw_w(&w_info);

    if (!cali_x(0)) {
	decide(0, 2, "Calibration has been aborted",
	    "Press any key or mouse, back to Checkfit");
	cali_file[0] = 0;
	return (0);
    }
    txtpar.text_wind_len = 40;
    text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
    gprintf(w_info.xtop + 4, w_info.ybot - 4, 
        "Finished recording calibration data.");
    thev_load(cali_file);
    dfgc = scrn_c[C_WAVE];
    draw_w(&w_msg);
    accept = decide(1, 3, 
        "Accept the Check Fit and Calibration?",
	"Yes: continue with test         ", 
	"No:  redo CheckFit and Calibrate");
    if (!accept) {
	cali_file[0] = 0;
    } else {
	save_cali_tokens();
    }
    return (accept);
}

void
out_msg(int seconds, char outmsg[][16], int osec)
{
    int     j, k;
    struct PATH path;

    file_opt[4].status = 1;
    j = draw_w(&w_msg);
    k = w_msg.ytop + txtpar.font_height;
    split_path(file_name, path.drive, path.dir, path.name, path.ext);
    gprintf(j, k, "Filename: %s%s", path.name, path.ext);
    k += txtpar.font_height;
    gprintf(j, k, "Test Date: %s", outmsg[0]);
    k += txtpar.font_height;
    gprintf(j, k, "Test Time: %d sec", seconds);
    k += txtpar.font_height;
    if (osec > 0) {
        gprintf(j, k, "           %d sec", osec);
        k += txtpar.font_height;
    }
}
