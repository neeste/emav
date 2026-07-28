/* ============================ dpoae_w.c ================================ */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>
#include <dsp.h>
#include <menu.h>
#include <util.h>
#include "savage.h"
#include "color.h"
#include "emav.h"		/* function prototyes */
#include "screen.h"
#include "version.h"
#include "fk.h"

#define TWOPI		(M_PI * 2)
#define PIO2		(M_PI / 2)
#define BREAK		1
#define NTL             8
#define LONG_STIM

#ifndef WIN32
#include <unistd.h>
#define _access access
#endif /* WIN32 */

int  _access(const char *, int);
int  thev_adjust(char *, char *, SAV_FFT *, int);
void cal_gain(SAV_FFT *, int);
void file_flush(FILE *);

extern char tokstr[], line[], o_file_name[], swp_file[];
extern char *absci[], *dpsigtyp[], *dpfrqtyp[];
extern char *not_saved;
extern float L1b, L2b, F1b, F2b, limit_mv, red_thr, chkfit_time;
extern float L1e, L2e, F1e, F2e, ATb, ATe, noise, snr;
extern int dpoae_counter, probe_counter, nab, maxnpts, calphase;
extern int fileflag, sig_type, rd_cal_flag, stim_flag, scope, contav;
extern int rand_seed;
extern struct _teoae teoae;
extern struct _dpoae dpoae;
extern struct _probe probe;
extern MENUITEM file_opt[];
extern TOGGLETYPE f5type, dpsptype, xtype, F1_adj, r_mode, dpnr_mode;
extern TOGGLETYPE dpftype, datafmt, Sbin, Suppr, dp_octave, stim_unit;

char    cali_save[MAXNAME] = "";
char    dpoae_file[MAXNAME] = "";
char   *tmp_dat_file = "out.dat";
char   *tmp_bin_file = "out.bin";
float   cal_attn = 20;
float   chk_attn = 20;
float   F5_cutoff = 200.0;
int     cal_sets = 2;
int     cal_swps = 4;
int     chk_swps = 16;
int     nnsb = 0;
int     dpoae_flag = 0;
int     probe_flag = 0;
int     cavit_test = 0;
int     dp_repeat = 1;
int     noreject = 0;
int     disconnect = 0;
int     num_oct = 0;
int     dp_items = 0;
#ifdef LONG_STIM
int     stmtyp = 1;
#else
int     stmtyp = 0;
#endif
struct  datlnk *dtlnk = NULL;
struct  datlnk *datptr = NULL;
WIND    w_dp;

static char   *tmp_swp_file = "out.swp";
static double db_adj = 0, ph_adj = 0;
static double dsp_scale[2] = {1, 1};
static double f1adj_db = 0, f1adj_ph = 0;
static double fexcl[2] = {0};
static double m0 = 0;
static double m1 = 0;
static double m2 = 0;
static double thrsh_freq[NTL] = {0};
static double thrsh_level[NTL] = {0};
static double outdbv[4];
static float pas_per_vlt = 0;
static float vlt_per_cnt = 0;
static int bk_flg = 0;
static int debug = 0;
static int dp_fft_xrange = 8;
static int f1adj = 0;
static int calmode = 0;
static int nadj = 0;
static int32_t swpsiz = 0;
static short modcyc = 0;
static short modper = 0;
static FILE *fswp;
static SAV_FFT AB_fft[2];

/* make the dpoae paramters current */

void
grab_dpoae()
{
    mp_transfer(Sys.MPfn);
    Sen = Sys;
    dis_units(Sen.AD, Sen.MP);

    if (dpoae.size > maxnpts) {
	dpoae.size = (short)(maxnpts);
    }
    buflen = dpoae.size;
    rate = dpoae.rate;
    acc_sets = dpoae.sets;
    numcav = 1;
    numsrc = 2;
    sig_type = (int)(dpnr_mode.at);
    nnsb = dpoae.nnsb;
    num_oct = (int)(dp_octave.at);
    limit_mv = dpoae.mvmax;
    chk_ramp_dpoae();
    pas_per_vlt = Sen.MP;
    vlt_per_cnt = Sen.AD;
    fft_xrange = dpoae.fft_xrange;
    fft_yrange = dpoae.fft_yrange;
    fft_levref = dpoae.levref;
    cal_attn = dpoae.cal_atten;
    cal_swps = dpoae.cal_swps;
    chk_attn = dpoae.chk_atten;
    chk_swps = dpoae.chk_swps;
    stim_unit.at = dpoae.level_unit;
    modcyc = dpoae.modcyc;
    modper = dpoae.modper;
    contav = dpoae.contav;
    change_nic(dpoae.nic);
    input_filter(hpf_type, hpf_ord, dpoae.hpff, dpoae.rate);
    rand_seed = dpoae.seed;
}

/* zero accbuff */
void
zero_acc()
{
    int     i, accnpts;

    accnpts = (dspnic < 2) ? buflen : buflen * 2;
    for (i = 0; i < accnpts; i++) {
	accbuf_a[i] = 0;
	accbuf_b[i] = 0;
    }
}

/* zero outbuf */
void
zero_out()
{
    int     i;

    for (i = 0; i < buflen; i++) {
	outbuf[i] = 0;
    }
}

/* zero savbuf */
void
zero_sav()
{
    int     i;

    for (i = 0; i < buflen; i++) {
	savbuf[i] = 0;
    }
}

/* display the DP message */
static int
dp_msg(int *x, int *y)
{
    int     j, k, m;

    j = draw_w(&w_msg);
    k = w_msg.ytop + txtpar.font_height;
    gprintf(j, k, "-> or <- keys");
    k += txtpar.font_height;
    gprintf(j, k, "(N)ext, (B)ack");
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

static double
thresh_level(double f)
{
    int     i, j, k;
    char    str[20] = { 20*0 };
    double  thlv = 0;

    for (i = 0; i < NTL; i++) {
        if (f == thrsh_freq[i]) {
            thlv = thrsh_level[i];
            break;
        }
    }
    if (i >= NTL) {
        k = open_text_w(&j);
        i = gprintf(j, k, "Threshold at %.0f Hz (dB SPL) = ", f);
        txtpar.text_wind_len -= i;
        get_str(j + i * 8, k, 20, 0, STRING, str);
        txtpar.text_wind_len += i;
        thlv = atof(str);
        close_text_w();
        for (i = NTL - 1; i > 0; i--) {
            thrsh_freq[i] = thrsh_freq[i - 1];
            thrsh_level[i] = thrsh_level[i - 1];
        }
        thrsh_freq[0] = f;
        thrsh_level[0] = thlv;
    }
    return (thlv);
}

/* compute frequencies to be excluded */
static void
freq_excl(double f1, double f2)
{
    fexcl[0] = 2 * f1 - f2;
    fexcl[1] = 2 * f2 - f1;
}

/* compute calibrated tone amplitdue */
static double
tone_amp(double f, double db, int c, double *ph)
{
    double ref, mag;
    int i;
    SAV_FFT *p;

    if (stim_unit.at == 1)			    // SL
        db += thresh_level(f);
    p = &AB_fft[c];
    i = nint(f * p->npts * 2.0 / p->rate);
    ref = sqrt(2.0) / p->rp[i];
    mag = pow(10.0, db / 20.0) * ref / (CALFIX);   // 3.22
    if (ph)
	*ph = (f > 0 && calphase) ? (PIO2 - p->ph[i]) : 0; 

    return (mag);
}

/* generate the stimulus as specified according to the calibration */
static void
gen_stim(double f, double db, double ph, int c, int a, short mc, short mp)
{
    int     i, j, k, nc, x1, x2, mf;
    double  mag, pha, dph, a0, a1, a2, dp, w, out, ma, mr, mxam;
    float  *rp;
#ifdef LONG_STIM
    int32_t *sbuf = savbuf;    // store stimulus in this buffer
#else
    short *sbuf = outbuf;   // store stimulus in this buffer
#endif
    static double e = 1e-6;
    static double d0 = 0, d1 = 0, d2 = 0;
    static double p0 = 0, p1 = 0, p2 = 0;
    static int n1 = 0, n2 = 0;

    if ((f <= 0) || (f >= (rate / 2))) {
	return;
    }
#ifdef LONG_STIM
    mxam = pow(2,31) - 1;   // maximum stimulus amplitude
#else
    mxam = MAXPOSINT;	    // maximum stimulus amplitude
#endif
    if (stim_unit.at == 2) {			    // VdB
        mag = db * Sen.DA[c];
	pha = 0;
    } else {
        mag = tone_amp(f, db, c, &pha);
    }
    mag *= (mxam / MAXPOSINT);
    dph = TWOPI * adjustf(f) / rate;
    if (dph == 0)
	pha = 0;
    if ((mc > 0) && (mp > 0)) {
	ma = mp / 200.0;
	mr = mc * TWOPI / buflen;
	mf = 1;
    } else {
	ma = mr = mf = 0;
    }
    if (a == 0) {
        m0 = mag;
	p0 = pha + ph * (TWOPI / 360);    // ph only affects first-tone phase
        d0 = dph;
        dsp_scale[c] = dspsets(m0 / mxam);
        a0 = limit(e, m0 / dsp_scale[c], mxam);
        for (i = 0; i < buflen; i++) {
            out = a0 * sin(d0 * i + p0);
	    if (mf)
		out *= (1 - ma * (cos(mr * i) + 1));
            sbuf[i] = nint(out);
	}
    } else if (a == 1) {
        m1 = mag;
	p1 = pha;
        d1 = dph;
        dsp_scale[c] = dspsets((m0 + m1) / mxam);
        a0 = limit(e, m0 / dsp_scale[c], mxam);
        a1 = limit(e, m1 / dsp_scale[c], mxam - a0);
        for (i = 0; i < buflen; i++) {
            out = a0 * sin(d0 * i + p0) 
		+ a1 * sin(d1 * i + p1);
            sbuf[i] = nint(out);
	}
    } else if (a == 2) {
        m2 = mag;
	p2 = pha;
        d2 = dph;
        dsp_scale[c] = dspsets((m0 + m1 + m2) / mxam);
        a0 = limit(e, m0 / dsp_scale[c], mxam);
        a1 = limit(e, m1 / dsp_scale[c], mxam - a0);
        a2 = limit(e, m2 / dsp_scale[c], mxam - a0 - a1);
        for (i = 0; i < buflen; i++) {
            out = a0 * sin(d0 * i + p0) 
		+ a1 * sin(d1 * i + p1)
                + a2 * sin(d2 * i + p2);
            sbuf[i] = nint(out);
	}
    } else if (a == 11) {
        m1 = mag;
	p1 = pha;
        n1 = nint((f * buflen) / rate);
    } else if (a == 12) {
        m2 = mag;
	p2 = pha;
        d2 = dph;
        n2 = nint((f * buflen) / rate);
        if (Suppr.at == 3) {
            x1 = nint((fexcl[0] * buflen) / rate);
            x2 = nint((fexcl[1] * buflen) / rate);
        } else {
            x1 = x2 =  0;
        }
        nc = n2 - n1 + 1;
        if (nc > 0) {
            w = sqrt((double) nc);
            dsp_scale[c] = dspsets((m0 + m1 + m2) / mxam);
            a0 = limit(e, m0 / dsp_scale[c], mxam);
            a1 = limit(e, m1 / dsp_scale[c], mxam - a0) / w;
            a2 = limit(e, m2 / dsp_scale[c], mxam - a0 - a1) / w;
            dp = TWOPI / buflen;
	    m1 = log(a1);
            m2 = (nc > 1) ? (log(a2) - log(a1)) / (nc - 1) : 0;
            rp = (float *) accbuf_a;
            for (j = 0; j < nc; j++)
                rp[j] = (float) (TWOPI * rand() / RAND_MAX);
            for (i = 0; i < buflen; i++) {
                w = a0 * sin(d0 * i);
                for (j = 0; j < nc; j++) {
                    k = j + n1;
                    if (k != x1 && k != x2)
                        w += exp(m1 + j * m2) * sin(k * dp * i + rp[j]);
                }
                sbuf[i] = nint(w);
                if (w < -mxam || mxam < w) {
                    decide(0, 2, "Stimulus overflow!", 
                        "Reducing WBN level!");
                    m1 += log(0.1) / 10;
                    i = 0;
                }
            }
        }
    }
}

static void
pre_adj_F1()
{
    int     i, c, tfgc_s, tbgc_s;

    rd_text_color(&tfgc_s, &tbgc_s);
    w_ind.ybot = ypix - 10 - txtpar.menu_height;
    w_ind.ytop = w_ind.ybot - txtpar.menu_height;
    i = draw_w(&w_ind);
    gprintf(i, w_ind.ytop + txtpar.font_height, "Adjust F1");
    w_ind.ytop = w_ind.ybot;
    w_ind.ybot = ypix - 1;
    i = draw_w(&w_ind);
    c = w_ind.ytop + txtpar.font_height;
    gprintf(i, c, "%.f < %.0f", f1adj_db, f1adj_ph);
    text_color(tfgc_s, tbgc_s);
}

static void
reload_F1()
{
    if (f1adj_db == db_adj && f1adj_ph == ph_adj)
        return;
    gen_stim(datptr->f1, datptr->d1 + f1adj_db, datptr->p1 + f1adj_ph, 1, 0, 0, 0);
    dwnld_dac(1, 1, dsp_scale[1], stmtyp);
    dsprst(TRUE);
    dsprst(FALSE);
    zero_acc();
    zero_out();
    db_adj = f1adj_db;
    ph_adj = f1adj_ph;
}

static void
adjust_F1(int i)
{
    if (i == ' ') {
        f1adj = 0;
        pre_dis_ind("Artifact", "Limit");
        dis_artifact();
        return;
    } else if (i == FK_Up_Arrow) {
        f1adj_db += 1;
    } else if (i == FK_Down_Arrow) {
        f1adj_db -= 1;
    } else if (i == FK_Right_Arrow) {
        f1adj_ph += 1;
    } else if (i == FK_Left_Arrow) {
        f1adj_ph -= 1;
    } else if (i == FK_Ctrl_Up) {
        f1adj_db += 10;
    } else if (i == FK_Ctrl_Down) {
        f1adj_db -= 10;
    } else if (i == FK_Ctrl_Right) {
        f1adj_ph += 10;
    } else if (i == FK_Ctrl_Left) {
        f1adj_ph -= 10;
    }
    if (f1adj_db > 100)
    	f1adj_db = 100;
    if (f1adj_db < -100)
    	f1adj_db = -100;
    while (f1adj_ph > 180)
        f1adj_ph -= 360;
    while (f1adj_ph < -180)
        f1adj_ph += 360;
    pre_adj_F1();
    reload_F1();
}

/* ======================================================================= */

static void
dsp_start()
{
    if (r_mode.at == 2) {
	if (fswp) {
	    fread(&buflen, sizeof(short), (size_t)(1), fswp);
	    fread(&swp1set, sizeof(short), (size_t)(1), fswp);
	}
    } else {
        dsprst(TRUE);
        downldpar(2, buflen, dpoae.skips, dpoae.ramp, swp1set);
    }
    if (r_mode.at == 1) {
	if (fswp) {
	    fwrite(&buflen, sizeof(short), (size_t)(1), fswp);
	    fwrite(&swp1set, sizeof(short), (size_t)(1), fswp);
	}
    }
}

static void
dsp_stim()
{
    if (r_mode.at == 2) {
        zero_acc();
	zero_out();
	outdbv[0] = dbv(tone_amp(datptr->f1, datptr->d1, 1, NULL) / Sen.DA[1]);
	outdbv[1] = dbv(tone_amp(datptr->f2, datptr->d2, 0, NULL) / Sen.DA[0]);
        if (Suppr.at) {
	    outdbv[2] = dbv(tone_amp(datptr->f3, datptr->d3, 0, NULL) / Sen.DA[0]);
	    outdbv[3] = dbv(tone_amp(datptr->f4, datptr->d4, 0, NULL) / Sen.DA[0]);
	}
    } else {
	gen_stim(datptr->f2, datptr->d2, 0.0, 0, 0, 0, 0);
	if (num_oct == 0) {
	    if (Suppr.at == 1) {		    // Tone
		gen_stim(datptr->f3, datptr->d3, 0.0, 0, 1, 0, 0);
		gen_stim(datptr->f4, datptr->d4, 0.0, 0, 2, 0, 0);
	    } else if (Suppr.at == 2 || Suppr.at == 3) {
		freq_excl(datptr->f1, datptr->f2);  // BPN or XBPN
		gen_stim(datptr->f3, datptr->d3, 0.0, 0, 11, 0, 0);
		gen_stim(datptr->f4, datptr->d4, 0.0, 0, 12, 0, 0);
	    }
	}
	if (num_oct >= 1)
	    gen_stim(datptr->f2 * 2, datptr->d2, 0.0, 0, 1, 0, 0);
	if (num_oct >= 2)
	    gen_stim(datptr->f2 * 4, datptr->d2, 0.0, 0, 2, 0, 0);
	dwnld_dac(0, 1, dsp_scale[0], stmtyp);
	outdbv[1] = dbv(m0 / Sen.DA[0]);
        if (Suppr.at) {
	    outdbv[2] = dbv(m1 / Sen.DA[0]);
	    outdbv[3] = dbv(m2 / Sen.DA[0]);
	}
	gen_stim(datptr->f1, datptr->d1, datptr->p1, 1, 0, modcyc, modper);
	if (num_oct >= 1)
	    gen_stim(datptr->f1 * 2, datptr->d1, 0.0, 1, 1, 0, 0);
	if (num_oct >= 2)
	    gen_stim(datptr->f1 * 4, datptr->d1, 0.0, 1, 2, 0, 0);
	dwnld_dac(1, 1, dsp_scale[1], stmtyp);
	outdbv[0] = dbv(m0 / Sen.DA[1]);
        zero_acc();
	zero_out();
	dsprst(TRUE);
	dsprst(FALSE);
    }
}

static void
dsp_count(int n)
{
    if (r_mode.at != 2) {
        if (contav && !calmode) {
	    dspcnt(n * swp1set);
	}
    }
}

static int
dsp_ready()
{
    short data;

    if (r_mode.at == 2) {
	return (1);
    } else {
	if (dspdget()) {
	    dspdread(&data);
	    if ((data % swp1set) == 0) {
		return (1);
	    }
	}
    }
    return (0);
}

static void
ramp_down()
{
    dspend();                       // tell dsp to ramp down
    msleep(nint(dpoae.ramp_ms));    // wait for ramp down
    dspdget();                      // check dsp status
}

static void
dsp_fetch(int cnt, int pair)
{
    int    npts;
    short *obuf;

    npts = (dspnic < 2) ? buflen * 2 : buflen * 4;
    obuf = &outbuf[npts * pair];
    if (r_mode.at == 2) {
	if (fswp && !calmode) {
	    fread(obuf, sizeof(short), (size_t)(npts), fswp);
	}
    } else  {
        dspuarr(dsppar.acc, npts, obuf);
	if (contav && !calmode) {
	    dspclr();
	} else {
            ramp_down();
            dsprst(TRUE);
	    dsprst(FALSE);
	}
    }
    if (r_mode.at == 1) {
	if (fswp && !calmode) {
	    fwrite(obuf, sizeof(short), (size_t)(npts), fswp);
	    fflush(fswp);
	    swpsiz = sizeof(short) * npts;
	}
    }
}

static void
dsp_sync(int32_t nsw)
{
    int32_t    nrd = 0;

    if (r_mode.at == 2) {
	if (fswp && !calmode) {
	    fread(&nrd, sizeof(int32_t), (size_t)(1), fswp);
	}
	if (nrd != nsw)
	    decide(0, 1, "Sync error in SWP file.");
    } else {
        if (contav && !calmode) {
	    dspcnt(0);
	}
        dsprst(TRUE);                   // reset dsp
    }
    if (r_mode.at == 1) {
	if (fswp && !calmode) {
	    fwrite(&nsw, sizeof(int32_t), (size_t)(1), fswp);
	    fflush(fswp);
	}
    }
}

static int
dsp_chk_sync(int32_t nsw)
{
    int32_t    nb, nrd = 0;

    if (r_mode.at == 2) {
	if (fswp && !calmode) {
	    nb = (int32_t) sizeof(int32_t);
	    fread(&nrd, nb, (size_t)(1), fswp);
	    fseek(fswp, -nb, SEEK_CUR);
	    nrd &= 0xFFFF;
	}
	if (nrd == nsw)
	    return (1);
    }
    return (0);
}

static void
dsp_reject()
{
    if (r_mode.at == 1) {
	if (fswp && !calmode) {
	    fseek(fswp, -2 * swpsiz, SEEK_CUR);
	}
    }
}

/* ======================================================================= */

static int
proc_accu()
{
    double  fdp = 0, fdif = 0;
    float   dpp, dpa, dpn;
    int     i, j, k, m, n, mx, my, reject, accnpts;
    int     pair = 0, cnt = 0, total = 0;
    int32_t   *chkbuf, *swpbuf_a, *swpbuf_b;

    m = dp_msg(&j, &k);
    n = m + txtpar.font_height;
    accnpts = (dspnic < 2) ? buflen : buflen * 2;
    swpbuf_a = swpbuf;
    swpbuf_b = swpbuf + accnpts;
    chkbuf = swpbuf + accnpts * 2;
    if (!calmode) {
	if (datafmt.at == 4) {
	    fdif = modcyc * rate / (double) buflen;
    	    fdp = datptr->f2 - fdif;
	} else {
	    fdif = datptr->f2 - datptr->f1;
    	    fdp = dpfreq(datptr->f1, datptr->f2);
	}
        dsp_count(acc_sets * 10);
    }
    noreject = (r_mode.at == 2) || (calmode && !cavit_test);
    while (cnt < acc_sets) {
	if (dsp_ready()) {
	    dsp_fetch(cnt, pair);
	    if (pair) {
                take_the_aver(buflen, swpbuf_a);
                take_the_aver(buflen, swpbuf_b);
	        for (i = 0; i < buflen; i++) {
		    chkbuf[i] = swpbuf_a[i] - swpbuf_b[i];
                }
                if (f1adj) {
                    reject = 1;
                } else if (noreject) {
                    reject = 0;
                } else {
                    if (F5_cutoff > 0) {
		        long2float(chkbuf, buflen);
		        bwfilt((float *) chkbuf, buflen, F5_cutoff / rate, 6, 1);
		        float2long(chkbuf, buflen);
                    }
                    reject = check_rej(chkbuf);
		}
		if (!calmode) {
		    fft_A_B(swpbuf_a, swpbuf_b, 1, 1, 1, 0);
		    if (dpoae.nfsb) {
			if (check_noise(fdp, fdif, dpoae.nfdb, dpoae.nfsb)) {
			    if( r_mode.at != 2) {
    				reject = 1;
			    }
			}
		    }
		}
		if (reject) {
		    dsp_reject();
		} else {
		    for (i = 0; i < accnpts; i++) {
		        accbuf_a[i] += swpbuf_a[i];
		        accbuf_b[i] += swpbuf_b[i];
		    }
		    cnt++;
		    if (!calmode) {
    			if (debug) {
			    take_the_aver(buflen, swpbuf_a);
			    take_the_aver(buflen, swpbuf_b);
			    draw_w(w_dis);
			    dfgc = scrn_c[C_CHNA];
			    disply(swp1set, swpbuf_a, buflen, 1);
			    dfgc = scrn_c[C_CHNB];
			    disply(swp1set, swpbuf_b, buflen, 0);
			}
			dis_artifact();
			fft_A_B(accbuf_a, accbuf_b, 1, cnt, 1, 0);
			show_fft(dp_fft_xrange, dpoae.fft_yrange,
			dpoae.levref, scrn_c[C_APBS], 1);
			fft_A_B(accbuf_a, accbuf_b, 0, cnt, 1, 0);
			show_fft(dp_fft_xrange, dpoae.fft_yrange,
			dpoae.levref, scrn_c[C_AMBS], 0);
			get_level(fdp, nnsb, sig_type, fdif, &dpp, &dpn, &dpa);
			// measurement base stopping rules
			if (dpn < datptr->noise
			    || (dpp - dpn) > datptr->snr) {
			    // [disabled during record or playback]
			    if (r_mode.at == 0) {
				acc_sets = cnt;
			    }
			}
			text_color(txtpar.gfg, txtpar.gbg);
			txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / 8;
			gprintf(j, n, "S/N=%.0f/%.0f  ", dpp, dpn);
			if (debug) {
			    if (debug > 1)
    				if (getevent() == 27)
				    cnt = acc_sets;
			}
		    }
		}
		gprintf(j, k, "%5d", acc_sets - cnt);
	    }
	    pair = 1 - pair;
	    total++;
	    gprintf(j, m, "%d / %d", 2 * cnt, total);
	}
	while (check_event()) {
	    i = getevent();
	    if (bk_flg > 0 && r_mode.at == 0) {
		if (i == 'n' || i == 'N')
		    acc_sets = cnt;
		else if (i == (FK_F2)) {
		    bk_flg = -2;
		    acc_sets = cnt;
		} else if (i == 'b' || i == 'B') {
		    bk_flg = -1;
		    acc_sets = cnt;
		}
	    }
	    if (i == 27 || i == 3) {
		acc_sets = cnt;
		if (total >= 0)
		    total = -(total + 1);
	    } else if (i == SINGLE_CLICK) {
	    	mouse_position(&mx, &my);
		if (in_wind(mx, my, w_msg)) {
		    i = (my - w_msg.ytop) / txtpar.font_height;
		    if (i == 1) {
			acc_sets = cnt;
		    } else if (i == 2) {
			acc_sets = cnt;
			if (total >= 0)
			    total = -(total + 1);
		    }
		}
	    } else if (!noreject) {
		if (f1adj) {
      		    adjust_F1(i);
		} else {
		    proc_art(i);
		}
	        dis_artifact();
            }
	}
        if (cnt < acc_sets) {
            if (dsp_chk_sync(cnt)) {
		acc_sets = cnt;
	    }
	}
    }
    dsp_sync(acc_sets);
    return (total);
}

static int
show_accbuf(int color, int reset, int32_t *buf)
{
    double  tms;
    int     n;

    dfgc = color;
    display_reset = reset;
    text_color(scrn_c[C_RSPF], scrn_c[C_RSPB]);
    n = buflen;
    if (bk_flg == -2) {
	w_dis = &w_stim;
	stim_flag = 1;
	if (scope < buflen)
	    n = scope;
	if (reset)
	    draw_w(&w_stim);
    }
    if (!probe_flag && !dpoae_flag)
        dis_cali();
    w_sig.ybot -= txtpar.font_height;
    disply(acc_sets * swp1set * 2, buf, n, reset);
    w_sig.ybot += txtpar.font_height;
    tms = 1000 * (double) n / rate;
    show_xlabel(0.0, tms, 0, "ms");

    n = fft_stim(buf, 2 * acc_sets, 1);
    show_fft(teoae.fft_xrange, teoae.fft_yrange, teoae.levref, color, reset);
    if (bk_flg == -2) {
	w_dis = &w_sig;
	stim_flag = 0;
    }
    return (n);
}

/* setup single channel calibration (0=A, 1=B) */
static void
set_channel(int c)
{
    zero_acc();
    zero_out();
    downldpar(2, buflen, 1, 0, swp1set);// 1 skip needed for calibration
    dwnld_dac(!c, 1, 1, 0);
    recalstim(0);
    dwnld_dac(c, 1, 1, 0);
    dsprst(TRUE);			/* Stop I/O */
    dsprst(FALSE);			/* Start I/O */
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / txtpar.font_width - 1;
    text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
    gprintf(w_info.xtop + 4, w_info.ybot - 4, "Calibrating source %d ...", c + 1);
}

/* compute single channel calibration gain (0=A, 1=B) */
static void
compute_calgain(int c)
{
    int     n, color, reset;
    int32_t   *buf;
    SAV_FFT *pft;

    color = c ? scrn_c[C_CHNB] : scrn_c[C_CHNA];
    reset = c ? 0 : 1;
    buf = c ? accbuf_b : accbuf_a;
    pft = &AB_fft[c];

    n = show_accbuf(color, reset, buf);
    cal_gain(pft, n);
}

/* calibrate one channel (0=A, 1=B) */
static int
cali_chan(int c)
{
    int i;
    static int32_t *tmpbuf;

    set_channel(c);
    dp_fft_xrange = dpoae.fft_xrange;
    calmode = 1;
    if (proc_accu() <= 0) {
	acc_sets = dpoae.sets;
	return 0;
    }
    stability(1, accbuf_a, accbuf_b, buflen);
    for (i = 0; i < buflen; i++)
	accbuf_a[i] += accbuf_b[i];
    if (c == 0) {
	tmpbuf = ((int32_t *) calloc((size_t)(buflen), sizeof(int32_t)));
        memcpy(tmpbuf, accbuf_a, (size_t)(buflen) * sizeof(int32_t));
    } else {
        memcpy(accbuf_b, accbuf_a, (size_t)(buflen) * sizeof(int32_t));
        memcpy(accbuf_a, tmpbuf, (size_t)(buflen) * sizeof(int32_t));
	free(tmpbuf);
    }
    compute_calgain(c);

    return (1);
}

/* read calibration from file and compute gain */
static int
rd_cal()
{
    int ok = 0;

    //x = (w_sig.xbot + w_sig.xtop - 40 * 8) / 2;
    //y = (w_sig.ybot + w_sig.ytop - txtpar.font_height) / 2;
    strcpy(o_file_name, "*.CAL");
    open_file(NULL);
    strcpy(cali_file, o_file_name);
    if (_access(cali_file, 0) == 0) {
        decide(-2000, 2, "Reading calibration file", cali_file);
        if (cal_read(cali_file, 0))
	    cal_disp(cali_file, 0);
        compute_calgain(0);
        compute_calgain(1);
        ok = 1;
    } else {
        decide(0, 2, "Can't open calibration file", cali_file);
        ok = 0;
    }
        
    return (ok);
}

static void
dis_cav(int icav)
{
    int     k, i;

    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / txtpar.font_width - 1;
    i = w_info.xtop + 4;
    k = w_info.ytop + nint(txtpar.font_height * 1.5);
    text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
    if (probe_flag) {
        gprintf(i, k, "Cavity #%d of %d", icav + 1, numcav);
    } else {
        gprintf(i, k, "DPOAE Calibration");
    }
    k += nint(txtpar.font_height * 1.5);
    text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
    gprintf(i, k, "Stimulus: %-13s Attenu: %4.1f dB",
	stmfn, attenu.att1);
    k += txtpar.font_height;
    if (cali_file[0]) {
        gprintf(i, k, "Calibration file: %-13s ", cali_file);
        k += txtpar.font_height;
    }
    text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
    gprintf(w_info.xtop + 4, w_info.ybot - 4, "Checking Microphone Fit ..............");
}

/* calibration */
static int
calibrate_it(int icav)
{
    int isrc, ok = 0;

    if (probe_flag) {
        if (rd_cal_flag) {	    // select cal file from list
	    return (rd_cal());
	}
    } else {
        if (rd_cal_flag) {	    // select cal file from list
	    return (rd_cal());
        } else if (cali_file[0]) {  // use previous cal file
    	    return (1);
	} else {		    // create new cal file
	    genfn("D", "DAT", cali_file, &dpoae_counter);
	    newext(cali_file, "CAL");
	    dis_cav(icav);
	}
    }

    swp1set = cal_swps;
    dec_att((float **) NULL);
    pre_dis_ind("Artifact", "Limit");
    dis_artifact();

    for (isrc = 0; isrc < numsrc; isrc++) {
        if (!cali_chan(isrc)) {
	    decide(0, 2, "Calibration has been aborted",
		"Press key or click mouse to return to Checkfit");
	    cali_file[0] = 0;
	    return (-1);
	}
    }
    if (isrc < numsrc) {
	return (0);
    }
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / txtpar.font_width - 1;
    text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
    gprintf(w_info.xtop + 4, w_info.ybot - 4, "Finished both channels.");
    dfgc = scrn_c[C_WAVE];
    if (chkfit_time > 0) {
	ok = 1;
    } else {
        ok = decide(1, 3, "Accept the Calibration ?",
	    "Yes: Continue with Test         ", 
	    "No:  Redo CheckFit and Calibrate");
    }
    draw_w(&w_msg);
    draw_w(&w_info);
    if (ok) {
	cal_save(cali_file, icav);
	if (dpoae_flag) {
	    strcpy(cali_save, cali_file);
	}
    }
    return (ok);
}

/* compute total number of sweeps */
static double
calnswp(double tm)
{
    tm *= rate;
    tm /= buflen;
    return (tm);
}

/* put the message */
static void
dis_processing()
{
    int     i, j;

    text_color(menucolor.afg, scrn_c[C_INFB]);
    i = w_info.xtop + 4;
    j = w_info.ybot - 2;
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / 8;
    gprintf(i, j, "-> F1,F2=%.0f,%.0f  L1,L2=%.0f,%.0f   ",
	datptr->f1, datptr->f2, datptr->d1, datptr->d2);
}

static void
dis_prod(int cond, int pass)
{
    double  tt;
    int     i, j;

    if (bk_flg == 2) {
	draw_w(&w_info);
	bk_flg = 1;
    }
    text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
    i = w_info.xtop + 4;
    j = w_info.ytop + txtpar.font_height;
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / 8 - 2;
    tt = 2.0 * buflen * acc_sets * swp1set / rate;
    gprintf(i, j, "completed condition %d/%d, pass %d/%d:   ",
	cond, dp_items, pass, dp_repeat);
    j += txtpar.font_height;
    gprintf(i, j, "F2/F1 = %.0f/%.0f = %.3f     ",
	datptr->f2, datptr->f1, datptr->f2 / datptr->f1);
    j += txtpar.font_height;
    gprintf(i, j, "L1-L2 = %.0f-%.0f = %.0f, time = %.1f/%.1f sec   ",
	datptr->d1, datptr->d2, datptr->d1 - datptr->d2, tt, datptr->tm);
    j += txtpar.font_height;
    if (datafmt.at == 4) {
	gprintf(i, j, "Fsb = F2-%.0f Hz    ", 
	    modcyc * rate / (double) buflen);
        j -= txtpar.font_height;
    }
    if (Suppr.at) {
        gprintf(i + 240, j, ": F3 =%5.0f ", datptr->f3);
        j += txtpar.font_height;
        gprintf(i + 240, j, ": L3 =%5.0f ", datptr->d3);
        j += txtpar.font_height;
        gprintf(i + 240, j, ": F4 =%5.0f ", datptr->f4);
        j += txtpar.font_height;
        gprintf(i + 240, j, ": L4 =%5.0f ", datptr->d4);
    }
}

void
dis_result(float *pst, float *nst, double rtp, double rtn, double fdp, int sf)
{
    int     i, j;

    text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
    i = w_info.xtop + 4;
    j = w_info.ytop + 4 * txtpar.font_height;
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / 8 - 2;
    if (datafmt.at == 4) {
        j += txtpar.font_height;
        gprintf(i, j, "        1     2    sb   ");
        j += txtpar.font_height;
    } else if (sf) {
        j += txtpar.font_height;
        gprintf(i, j, "        1     2    res  ");
        j += txtpar.font_height;
    } else {
	gprintf(i, j, "Fd = %s = %.0f Hz    ", 
	    dpfrqtyp[dpftype.at], fdp);
        j += txtpar.font_height;
        gprintf(i, j, "        1     2     dp  ");
        j += txtpar.font_height;
    }
    gprintf(i, j, "A+B: %5.1f %5.1f %5.1f dB SPL ", pst[0], pst[1], rtp);
    j += txtpar.font_height;
    gprintf(i, j, "A-B: %5.1f %5.1f %5.1f dB SPL ", nst[0], nst[1], rtn);
}

/* deallocate the data linked list */
void
del_data_lnk()
{
    struct datlnk *prev;

    if (dtlnk != NULL) {
        prev = dtlnk;
        datptr = prev->next;
        while (prev != NULL) {
            free(prev);
            prev = datptr;
            if (datptr != NULL)
                datptr = datptr->next;
        }
        dtlnk = NULL;
    }
}

/* add elements to the data linked list */
int
add_data_lnk(int set)
{
    struct datlnk *p;

    p = (struct datlnk *) calloc((size_t)(1), sizeof(struct datlnk));
    if (p == NULL)
	return (0);
    p->next = NULL;
    p->ctrl = (short)(disconnect);
    p->f3 = 0;
    p->d3 = 0;
    p->f4 = 0;
    p->d4 = 0;
    p->p1 = 0;
    p->at = 0;
    disconnect = 0;
    if (set) {
        p->noise = -99;
        p->snr = 99;
	sscanf(line, "%f %f %f %f %f %f %f %f %f %f %f %f %f",
	    &p->f2, &p->f1, &p->d2, &p->d1, &p->tm, &p->noise, &p->snr,
	    &p->f3, &p->d3, &p->f4, &p->d4, &p->p1, &p->at);
    } else {
	p->f2 = F2e;
	p->f1 = F1e;
	p->d2 = L2e;
	p->d1 = L1e;
	p->tm = ATe;
	p->noise = noise;
	p->snr = snr;
    }
    if (p->f2 <= 0 || p->f1 <= 0 || p->tm <= 0) {
        free(p);
        return (0);
    }
    p->f1 = (float) adjustf(p->f1);
    p->f2 = (float) adjustf(p->f2);
    p->f3 = (float) adjustf(p->f3);
    p->f4 = (float) adjustf(p->f4);
    if (dtlnk == NULL) {
        dtlnk = p;
        datptr = p;
    } else if (set == 0
        && datptr->f1 == p->f1 && datptr->d1 == p->d1
        && datptr->f2 == p->f2 && datptr->d2 == p->d2
        && datptr->f3 == p->f3 && datptr->d3 == p->d3
        && datptr->f4 == p->f4 && datptr->d4 == p->d4
        && datptr->p1 == p->p1 && datptr->at == p->at
        && datptr->noise == p->noise && datptr->snr == p->snr
        && datptr->tm == p->tm) {
        free(p);
        return (0);
    } else {
        datptr->next = p;
        datptr = p;
    }
    return (1);
}

int
create_list()
{
    strcpy(dpoae_file, dpoae.file);
    grab_dpoae();
    del_data_lnk();
    dp_items = gen_param_list();
    del_data_lnk();
    return (0);
}

void
out_msg(int seconds, char outmsg[][16])
{
    int     j, k;
    struct PATH path;

    split_path(file_name, path.drive, path.dir, path.name, path.ext);
    FILE_SAVE.status = 1;
    j = draw_w(&w_msg);
    k = w_msg.ytop + txtpar.font_height;
    gprintf(j, k, "Filename:");
    k += txtpar.font_height;
    gprintf(j, k, " %s%s", path.name, path.ext);
    k += txtpar.font_height;
    gprintf(j, k, "Test Date:");
    k += txtpar.font_height;
    gprintf(j, k, " %s", outmsg[0]);
    k += txtpar.font_height;
    gprintf(j, k, "Test Time:");
    k += txtpar.font_height;
    gprintf(j, k, " %d sec", seconds);
    k += txtpar.font_height;
    gprintf(j, k, "Accepted:");
    k += txtpar.font_height;
    gprintf(j, k, " %s", outmsg[1]);
    k += txtpar.font_height;
    gprintf(j, k, " %s", outmsg[2]);
}

/* write header to DAT file */
static void
wr_dat_hdr(FILE * fout, char *date)
{
    int i, n;
    static char *fmtlab[NEX] = {
    	"2*F1-F2  ", "3*F1-2*F2", "4*F1-3*F2", "2*F2-F1  ", "F2-F1    ",
    	"2*F2     ", "2*F1     ", "4*F2-2*F1", "4*F2     ", "4*F1     ",
    	"8*F2-4*F1",
    };
    static char *mullab[4] = {
    	"2*F1-F3  ", "2*F3-F1  ", "2*F1-F4  ", "2*F4-F1  ",
    };
    static char *modlab[4] = {
    	"F2-Fm    ", "F2+Fm    ", "F1-Fm    ", "F1+Fm    ",
    };
    static char *SNA = "Ldp     Ndp     <dp    ";

    fprintf(fout, ";DPOAE\n");
    fprintf(fout, ";LastName:  %s\n;FirstName: %s\n",
	patient.Lastname, patient.Firstname);
    fprintf(fout, ";Ear: %5s\n;ID: %s\n", patient.Ear == 'R' ?
	"Right" : (patient.Ear == 'L' ? "Left" : "?"), patient.ID);
    fprintf(fout, ";Dob: %12s\n", patient.Dob);
    fprintf(fout, ";Threshold: %s\n", patient.Threshold);
    fprintf(fout, ";Comment: %s\n", patient.Comment);
    fprintf(fout, ";Date: %s\n", date);
    fprintf(fout, ";Sweeps=%d\n;List=%s\n", swp1set, dpoae_file);
    fprintf(fout, ";Stimulus=%s\n", dpoae.calibrate);
    fprintf(fout, ";Calibrate=%s\n", cali_file);
    fprintf(fout, ";Version=%s\n", VERSION);
    fprintf(fout, ";Items=%d\n", dp_items);
    fprintf(fout, ";Repeat=%d\n", dp_repeat);
    fprintf(fout, ";Octave=%d\n", num_oct);
    fprintf(fout, ";Abscissa=%s\n", absci[xtype.at]);
    fprintf(fout, ";NNSB=%d (A%sB)\n", nnsb, (nnsb > 0) ? "+" : "-");
    fprintf(fout, ";Signal=%s\n", dpsigtyp[sig_type]);
    fprintf(fout, ";RedThr=%.2f\n", red_thr);
    fprintf(fout, ";Datafmt=%s\n", datafmt.strs[datafmt.at]);
    fprintf(fout, ";SaveBin=%s\n", Sbin.strs[Sbin.at]);
    fprintf(fout, ";CalibratePhase=%s\n", calphase ? "Yes" : "No");
    fprintf(fout, ";Suppressor=%s\n", Suppr.strs[Suppr.at]);
    fprintf(fout, ";F1_adjust=%s\n", F1_adj.strs[F1_adj.at]);
    fprintf(fout, ";MP_sensitivity=%.4g\n", Sen.MP);
    fprintf(fout, ";MP_transfer=%s\n", Sen.MPfn);
    fprintf(fout, ";Level_Unit=%s\n", stim_unit.strs[stim_unit.at]);
    if (stim_unit.at == 3) {
	fprintf(fout, ";THS_File=%s\n", ths_file);
    }
    if (datafmt.at == 4) {
	fprintf(fout, ";ModCyc=%d (%.0f Hz)\n", modcyc, 
	    modcyc * rate / (double) buflen);
	fprintf(fout, ";ModPer=%d\n", modper);
    } else {
        fprintf(fout, ";DP_freq=%s\n", dpfrqtyp[dpftype.at]);
    }
    if (datafmt.at < 1) {
	fprintf(fout, ";F2     F1      L2    L1     T    Ldp    Ndp    Rep    <dp    AvT\n");
    } else if (datafmt.at < 4) {
	// 1st header line
        fprintf(fout, "; . . . . .                          ");
        if (datafmt.at == 3) {
            n = 5 + 4;
            for (i = 0; i < 5; i++)
                fprintf(fout, "    %s . .      ", fmtlab[i]);
            for (i = 0; i < 4; i++)
                fprintf(fout, "    %s . .      ", mullab[i]);
        } else {
            if (datafmt.at == 1)
                fmtlab[3] = "5*F1-4*F2";
            n = 5 + 3 * num_oct;
            for (i = 0; i < n; i++)
                fprintf(fout, "    %s . .      ", fmtlab[i]);
         }
	fprintf(fout, "    Stimulus_Noise  .  .  .     ");
        if (Suppr.at) {
            fprintf(fout, "  .       .       .       .     ");
            fprintf(fout, "  .       .       .       .     ");
        }
	fprintf(fout, " Set  .    ");
        if (Suppr.at)
	    fprintf(fout, ".    .    ");
	fprintf(fout, " VdB     .      ");
        if (Suppr.at)
	    fprintf(fout, " .       .      ");
	if (nadj) {
	    fprintf(fout, " FPL     .      ");
	    if (Suppr.at)
		fprintf(fout, " .       .      ");
	    fprintf(fout, " SIL     .      ");
	    if (Suppr.at)
		fprintf(fout, " .       .      ");
	    fprintf(fout, " RFL     .      ");
	    if (Suppr.at)
		fprintf(fout, " .       .      ");
	}
	fprintf(fout, "\n");
	// 2nd header line
	fprintf(fout, ";   F2      F1      L2     L1      T     ");
        for (i = 0; i < n; i++)
            fprintf(fout, "%s", SNA);
	fprintf(fout, "N1      <1     N2      <2    ");
        if (Suppr.at) {
            fprintf(fout, "  F3      L3      N3      <3    ");
            fprintf(fout, "  F4      L4      N4      <4    ");
        }
	fprintf(fout, "L1   L2   ");
        if (Suppr.at)
	    fprintf(fout, "L3   L4   ");
	fprintf(fout, " V1      V2     ");
        if (Suppr.at)
	    fprintf(fout, " V3      V4     ");
	if (nadj) {
	    fprintf(fout, " L1      L2     ");
	    if (Suppr.at)
		fprintf(fout, " L3      L4     ");
	    fprintf(fout, " L1      L2     ");
	    if (Suppr.at)
		fprintf(fout, " L3      L4     ");
	    fprintf(fout, " R1      R2     ");
	    if (Suppr.at)
		fprintf(fout, " R3      R4     ");
	}
	fprintf(fout, "\n");
    } else {					// SFOAE
        n = 4;
        fprintf(fout, "; . . . . .                          ");
        for (i = 0; i < n; i++)
            fprintf(fout, "    %s . .      ", modlab[i]);
	fprintf(fout, "\n");
	fprintf(fout, ";   F2      F1      L2     L1      T     ");
        for (i = 0; i < n; i++)
            fprintf(fout, "%s", SNA);
	fprintf(fout, "N1      <1     N2      <2    ");
        if (Suppr.at) {
            fprintf(fout, "  F3      L3      N3      <3    ");
            fprintf(fout, "  F4      L4      N4      <4    ");
        }
	fprintf(fout, "L1   L2   ");
        if (Suppr.at)
	    fprintf(fout, "L3   L4   ");
	fprintf(fout, " V1      V2     ");
        if (Suppr.at)
	    fprintf(fout, " V3      V4     ");
	fprintf(fout, "\n");
    }
}

/* write data line to DAT file */
static void
wr_dat_rec(FILE *fout, int nd, double tel, double rep,
    float *fdp, float *pdp, float *adp, float *ndp, 
    float *fst, float *pst, float *ast, float *nst)
{
    double  tav;
    float   fpl[4], sil[4], rfl[4];
    int     k;

    if (datafmt.at == 0) {
        tav = 2.0 * buflen * acc_sets * swp1set / rate;
	fprintf(fout, "%6.1f %6.1f %5.1f %5.1f %5.1f %6.1f %6.1f %6.1f %6.1f %5.1f\n",
	    fst[1], fst[0], pst[1], pst[0], tav,
	    pdp[0], ndp[0], rep, adp[0], tel);
    } else if (datafmt.at < 4) {
	fprintf(fout, "%8.2f %8.2f %6.2f %6.2f %5.1f ",
	    fst[1], fst[0], pst[1], pst[0], tel);
        for (k = 0; k < nd; k++)
	    fprintf(fout, "%7.2f %7.2f %6.1f ",
		pdp[k], ndp[k], adp[k]);
 	fprintf(fout, "%7.2f %6.1f ", nst[0], ast[0]);
	fprintf(fout, "%7.2f %6.1f ", nst[1], ast[1]);
	if (Suppr.at) {
	    fprintf(fout, "%8.2f %7.2f %7.2f %6.1f ",
	        fst[2], pst[2], nst[2], ast[2]);
	    fprintf(fout, "%8.2f %7.2f %7.2f %6.1f ",
	        fst[3], pst[3], nst[3], ast[3]);
        }
        fprintf(fout, "%4.0f %4.0f ", datptr->d1, datptr->d2);
	if (Suppr.at)
            fprintf(fout, "%4.0f %4.0f ", datptr->d3, datptr->d4);
        fprintf(fout, "%7.2f %7.2f ", outdbv[0], outdbv[1]);
	if (Suppr.at)
            fprintf(fout, "%7.2f %7.2f ", outdbv[2], outdbv[3]);
	if (nadj) {
	    thev_adj_cal(fst, pst, fpl, sil, rfl);
	    fprintf(fout, "%7.2f %7.2f ", fpl[0], fpl[1]);
	    if (Suppr.at)
		fprintf(fout, "%7.2f %7.2f ", fpl[2], fpl[3]);
	    fprintf(fout, "%7.2f %7.2f ", sil[0], sil[1]);
	    if (Suppr.at)
		fprintf(fout, "%7.2f %7.2f ", sil[2], sil[3]);
	    fprintf(fout, "%7.2f %7.2f ", rfl[0], rfl[1]);
	    if (Suppr.at)
		fprintf(fout, "%7.2f %7.2f ", rfl[2], rfl[3]);
	}
        fprintf(fout, "\n");
    } else {				    // SFOAE
	fprintf(fout, "%8.2f %8.2f %6.2f %6.2f %5.1f ",
	    fst[1], fst[0], pst[1], pst[0], tel);
        for (k = 0; k < nd; k++)
	    fprintf(fout, "%7.2f %7.2f %6.1f ",
		pdp[k], ndp[k], adp[k]);
 	fprintf(fout, "%7.2f %6.1f ", nst[0], ast[0]);
	fprintf(fout, "%7.2f %6.1f ", nst[1], ast[1]);
	if (Suppr.at) {
	    fprintf(fout, "%8.2f %7.2f %7.2f %6.1f ",
	        fst[2], pst[2], nst[2], ast[2]);
	    fprintf(fout, "%8.2f %7.2f %7.2f %6.1f ",
	        fst[3], pst[3], nst[3], ast[3]);
        }
        fprintf(fout, "%4.0f %4.0f ", datptr->d1, datptr->d2);
	if (Suppr.at)
            fprintf(fout, "%4.0f %4.0f ", datptr->d3, datptr->d4);
        fprintf(fout, "%7.2f %7.2f ", outdbv[0], outdbv[1]);
	if (Suppr.at)
            fprintf(fout, "%7.2f %7.2f ", outdbv[2], outdbv[3]);
        fprintf(fout, "\n");
    }
    file_flush(fout);
}

/* process header in DAT file */
void
proc_hdr()
{
    int     i, j, k;
    static char *ky[] = {
	"Limit", "Abscissa", "Repeat", "Items", "DP_freq", "dpftype.at",
	"Signal", "NNSB", "Datafmt", "SaveBin", "Suppressor",
	"Octave", "F1_adjust", "Sweeps",
	"MinLevAbs", "MaxLevAbs", "MinLevOrd", 
	"MaxLevOrd", "MinFrqOct", "MaxFrqOct",
	"RedThr", "NIC", "modcyc", "modper", 
	"Units", "Level_Unit", ""
    };

    get_token(1);
    k = check_list(tokstr, ky);
    get_token(0);
    i = atoi(tokstr);
    switch (k) {
    case 1:			/* limit */
	limit_mv = (float) atof(tokstr);
	break;
    case 2:			/* abscissa */
        j = check_list(tokstr, absci);
	if (j > 0 && j <= NAB)
	    xtype.at = j - 1;
	break;
    case 3:			/* repeat */
	if (i > 0)
	    dp_repeat = i;
	break;
    case 4:			/* items */
	if (i > 0)
	    dp_items = i;
	break;
    case 5:			/* DP_freq */
    case 6:			/* dpftype.at (for backward compatibility) */
	i = check_list(tokstr, dpfrqtyp);
	if (i > 0)
	    dpftype.at = i - 1;
	break;
    case 7:                     /* Signal */
	i = check_list(tokstr, dpsigtyp);
	if (i > 0)
	    sig_type = i - 1;
	break;
    case 8:                     /* NNSB */
	if (i >= -MAXNNSB && i <= MAXNNSB)
	    nnsb = i;
	break;
    case 9:                     /* DataFmt */
	i = toupper(tokstr[0]);
    	if(i == 'N')       
	    datafmt.at = 0;     /* Normal */
	else if(i == 'H')  
	    datafmt.at = 1;     /* Higher-order */
	else if(i == 'E') 
	    datafmt.at = 2;     /* Extended */
	else if(i == 'M') 
	    datafmt.at = 3;     /* Multi */
	else if(i == 'S') 
	    datafmt.at = 4;     /* SFOAE */
	break;
    case 10:                    /* SaveBin */
	Sbin.at = (toupper(tokstr[0]) == 'Y');
	break;
    case 11:                    /* Suppressor */
	if (toupper(tokstr[0]) == 'X') {         /* XBPN */
            Suppr.at = 3;
	} else if (toupper(tokstr[0]) == 'B') {  /* BPN */
            Suppr.at = 2;
	} else if (toupper(tokstr[0]) == 'T') {  /* Tone */
            Suppr.at = 1;
	} else if (toupper(tokstr[0]) == 'Y') {  /* Yes = Tone */
            Suppr.at = 1;
	} else if (toupper(tokstr[0]) == 'N') {  /* No = None */
            Suppr.at = 0;
        }
	break;
    case 12:                    /* Octave */
        num_oct = limit(0, i, 2);
	break;
    case 13:                    /* F1_adjust */
	F1_adj.at = (toupper(tokstr[0]) == 'Y');
	break;
    case 14:                    /* Sweeps */
	swp1set = i;
	break;
    case 15:                    /* MinLevAbs */
	dpoae.abs_min = (short)(i);
	break;
    case 16:                    /* MaxLevAbs */
	dpoae.abs_max = (short)(i);
	break;
    case 17:                    /* MinLevOrd */
	dpoae.ord_min = (short)(i);
	break;
    case 18:                    /* MaxLevOrd */
	dpoae.ord_max = (short)(i);
	break;
    case 19:                    /* MinFrqOct */
	dpoae.oct_min = (short)(i);
	break;
    case 20:                    /* MaxFrqOct */
	dpoae.oct_max = (short)(i);
	break;
    case 21:                    /* RedThr */
	red_thr = (float) atof(tokstr);
	break;
    case 22:                    /* NIC */
	if (i > 0) {
	    change_nic(i);
	}
	break;
    case 23:                    /* modcyc */
	if (i > 0) {
	    modcyc = (short)(i);
	}
	break;
    case 24:                    /* modper */
	if (i > 0) {
	    modper = (short)(i);
	}
	break;
    case 25:                    /* Units  (obselete) */
    case 26:                    /* Level_Unit        */
	stim_unit.at = set_level_unit(tokstr);
	break;
    }
}

/* read list file */
static int
rdlstfn()
{
    FILE   *fptr;
    int     j, k, cnt, gflg;

    disconnect = 0;
    dp_items = 0;
    del_data_lnk();
    strcpy(dpoae_file, dpoae.file);
    gflg = 0;
    if (r_mode.at == 2) {
	fread(dpoae_file, 1, (size_t)(MAXNAME), fswp);
    } else if (dpoae_file[0] == '\0') {
	gflg = 1;
    } else {
	if (strchr(dpoae_file, '*') || strchr(dpoae_file, '?')) {
	    j = draw_w(&w_info);
	    k = w_info.ytop + txtpar.font_height * 3;
	    text_color(scrn_c[C_INFF], scrn_c[C_INFB]);
	    gprintf(j, k, "Select list file and press enter.");
	    k += txtpar.font_height;
	    gprintf(j, k, "Or press escape to generate a new list.");
	    strcpy(o_file_name, dpoae_file);
	    if (open_file(NULL) == 27) {
		strcpy(dpoae_file, o_file_name);
	    } else {
		gflg = 1;
	    }
	    j = draw_w(&w_info);
	}
    }
    if (r_mode.at == 1) {
	fwrite(dpoae_file, 1, (size_t)(MAXNAME), fswp);
    }
    if (!gflg) {
	fptr = fopen(dpoae_file, "rt");
	if (fptr == NULL) {
            gflg = 1;
	} else {
	    cnt = 0;
	    while (fgets(line, MAXLINE, fptr) != NULL) {
		if (line[0] == ';') {
		    line[0] = ' ';
		    proc_hdr();
		    if (strlen(tokstr) == 0)
			disconnect = BREAK;
		} else if (strlen(line) > 16) {
		    if (add_data_lnk(1))
			cnt++;
		}
	    }
	    fclose(fptr);
	    dp_items = cnt;
	}
    }
    if (gflg) {
	if(decide(1, 3, "Parameter list file not found.", dpoae_file,
		"Generate a new parameter list ?")) {
	    dp_items = gen_param_list();
	} else {
	    dp_items = 0;
	}
    }
    return (dp_items);
}

/* check stimulus frequencies */

static int
chk_freqs(double f1, double f2, double f3, double f4)
{
    double fmx, fd;
    int    err = 0;

    fmx = rate / 2;
    if (f1 <= 0 || f1 >= fmx
        || f2 <= 0 || f2 >= fmx
        || f3 < 0 || f3 >= fmx
        || f4 < 0 || f4 >= fmx) {
        err++;			// invalid primary frequency
    } else if (!modcyc) {	// don't check fd for AM-SFOAE
        fd = dpfreq(f1, f2);
	if (fd <= 0 || fd >= fmx) {
            err++;		// invalid distortion frequency
	}
    }
        
    return (err);
}

static void
close_swp()
{
    if (r_mode.at && fswp) {
	fclose(fswp);
	fswp = NULL;
    }
}

static void
recalibrate()
{
    cali_file[0] = 0;
    open_stim_w(dpoae.calibrate);
    set_att(chk_attn);
    w_dis = &w_stim;
    stim_flag = 1;
    check_fit();
    stim_flag = 0;
    w_dis = &w_sig;
    open_stim_w(dpoae.calibrate);
    set_att(cal_attn);
    swp1set = dpoae.cal_swps;
    draw_w(&w_info);
    dec_att((float **) NULL);
    acc_sets = dpoae.sets;
    if (cali_chan(0) && cali_chan(1)) {
	genfn("D", "DAT", cali_file, &dpoae_counter);
	newext(cali_file, "CAL");
	cal_save(cali_file, 0);
    }
    swp1set = dpoae.swp1set;
}

/* find the previous pointer of the data linked list */

static struct datlnk *
previous_ptr()
{
    struct datlnk *ptr;

    ptr = dtlnk;
    while (ptr->next != NULL) {
	if (ptr->next == datptr)
	    break;
	ptr = ptr->next;
    }
    return (ptr);
}

/* Do the DPOAE task */

static int
do_dpoae_task()
{
    char    outmsg[3][16];
    double  pct, f, tel, rep;
    float   fm, lev[14];
    float   pst[NST], nst[NST], ast[NST], fst[NST];
    float   pdp[NEX], ndp[NEX], adp[NEX], fdp[NEX];
    int     i, j, k, ns, cnt, cnd, idp, accnpts, sf;
    int     nd = 0, flag = 0, plot_yet = 0;
    int32_t    run_time, each_time, acpt = 0, total = 0;
    struct  BINhdr *hdr;
    struct  BINwav wavhdr;
    FILE   *fout, *fbin;
    static char *month[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    static float cf1[] = {2, 3, 4, -1, -1, 0, 2, 4, 0, 4, 8};
    static float cf2[] = {-1, -2, -3, 2, 1, 2, 0, -2, 4, 0, -4};

    initial_dp();
    if (!rdlstfn()) {
	return (0);
    }
    if (!dp_items) {
	decide(0, 3, "No parameter list!",
	    "DPOAE test aborted!", 
	    "Press any key to continue");
	return (0);
    }
    if (pgm_done()) {
    	return (0);
    }
    if (stim_unit.at == 0) {		    // SPL
        nadj = 0;
    } else if (stim_unit.at == 1) {	    // SL
        for (i = 0; i < NTL; i++) {
            thrsh_level[i] = thrsh_freq[i] = 0;
	}
        nadj = 0;
    } else if (stim_unit.at == 2) {	    // Volt
        nadj = 0;
    } else if (stim_unit.at == 3) {	    // FPL
	nadj = thev_adjust(cali_file, dpoae.ths_file, AB_fft, 0);
    } else if (stim_unit.at == 4) {	    // SIL
	nadj = thev_adjust(cali_file, dpoae.ths_file, AB_fft, 1);
    }
    draw_w(&w_info);
    draw_w(&w_spec);
    proc_art(0);
    if (F1_adj.at && num_oct == 0) {
        f1adj = 1;
        f1adj_db = db_adj = 0;
        f1adj_ph = ph_adj = 0;
        pre_adj_F1();
    } else {
        f1adj = 0;
        pre_dis_ind("Artifact", "Limit");
        dis_artifact();
    }

    swp1set = dpoae.swp1set;		    // moved after thev_adj [Jun-2009]
    fout = fopen(tmp_dat_file, "wt");
    if (fout == NULL) {
	decide(0, 4, "Can't open .DAT for writing", tmp_dat_file,
	    "DPOAE test aborted!", "Press any key to continue");
	del_data_lnk();
	thev_adj_free();
	return (0);
    }
    if(Sbin.at == 0) {
        fbin = NULL;
    } else {
	fbin = fopen(tmp_bin_file, "wb");
	if(fbin == NULL && decide(1, 3, "Can't open .BIN for writing",
	    tmp_bin_file, "Abort DPOAE test ?")) {
		del_data_lnk();
		thev_adj_free();
		fclose(fout);
		close_swp();
		return(0);
	}
    }
    if(fbin != NULL) {
	hdr = (struct BINhdr *) outmsg[0];
	hdr->total = (unsigned) dp_items * dp_repeat;
	hdr->wav_len = (unsigned short)((unsigned) buflen);
	hdr->samp_rate = (float) rate;
	hdr->int2volt = 1 / vlt_per_cnt;
	hdr->volt2pas = 1 / pas_per_vlt;
	hdr->swp1set = (unsigned short)(swp1set);
	hdr->nic = (unsigned short)(dspnic);
	fseek(fbin, 0L, SEEK_SET);
	fwrite(hdr, sizeof(struct BINhdr), (size_t)(1), fbin);
    }
    accnpts = (dspnic < 2) ? buflen : buflen * 2;
    genfn("D", "DAT", file_name, &dpoae_counter);
    sprintf(outmsg[0], "%c%c-%s-%c%c", file_name[3], file_name[4],
	month[file_name[2] - 'A'], file_name[0], file_name[1]);
    wr_dat_hdr(fout, outmsg[0]);
    calmode = 0;
    dsp_start();
    run_time = (int32_t)(each_time = clock());
    j = 0;			/* use j as an Esc flag */
    bk_flg = 1;
    for (cnt = 0; cnt < dp_repeat; cnt++) {
	datptr = dtlnk;
	for (cnd = 0; datptr != NULL; cnd++) {
	    if (datptr->ctrl == BREAK && dp_items > 1) {
		flag = 0;
		fprintf(fout, ";\n");
	    }
	    if (chk_freqs(datptr->f1, datptr->f2, datptr->f3, datptr->f4)) {
		datptr = datptr->next;
		continue;
	    }
	    if (f5type.at == 1)
		F5_cutoff = (float) (datptr->f2 / 2.0);
	    if (dpsptype.at == 1)
                dp_fft_xrange = (int) (datptr->f2 * 0.0012 + 1);
            else
                dp_fft_xrange = dpoae.fft_xrange;
	    acc_sets = (int) (calnswp(datptr->tm / 2.0) / swp1set) + 1;
	    dsp_stim();
	    dis_processing();
	    i = proc_accu();
	    acpt += acc_sets * 2;
	    if (i < 0) {
		i = -(i + 1);
		j = 1;
	    }
	    total += i;
	    if (acc_sets > 0) {
		dis_prod(cnd + 1, cnt + 1);
		if(fbin != NULL) {
		    wavhdr.f1 = datptr->f1;
		    wavhdr.L1 = datptr->d1;
		    wavhdr.f2 = datptr->f2;
		    wavhdr.L2 = datptr->d2;
		    wavhdr.f3 = datptr->f3;
		    wavhdr.L3 = datptr->d3;
		    wavhdr.f4 = (unsigned short) nint(datptr->f4);
		    wavhdr.L4 = (short) nint(datptr->d4);
		    wavhdr.N = (unsigned short)(acc_sets * swp1set);
		    wavhdr.T = (unsigned short)(nint(datptr->tm));
		    fwrite(&wavhdr, sizeof(struct BINwav), (size_t)(1), fbin);
		    fwrite(accbuf_a, accnpts, (size_t)(4), fbin);
		    fwrite(accbuf_b, accnpts, (size_t)(4), fbin);
		    file_flush(fbin);
		}
		ns = 4;
                fst[0] = datptr->f1;
                fst[1] = datptr->f2;
                fst[2] = datptr->f3;
                fst[3] = datptr->f4;
                if (datafmt.at == 0) {		    // datafmt = Normal
		    dpftype.at = 0;
                    fdp[0] = (float) dpfreq(fst[0], fst[1]);
                    nd = 1;
		    idp = 0;
		} else if (datafmt.at < 4) {	    
		    if (datafmt.at == 1) {	    // datafmt = Higher-order
                        cf1[3] = 5;
                        cf2[3] = -4;
		    }
		    if (datafmt.at == 3) {	    // datafmt = Multi
			for (k = 0; k < 5; k++) {
			    fdp[k] = cf2[k] * fst[1] + cf1[k] * fst[0];
			}
			fdp[5] = 2 * fst[0] - fst[2];   /* 2*F1-F3 */
			fdp[6] = 2 * fst[2] - fst[0];   /* 2*F3-F1 */
			fdp[7] = 2 * fst[0] - fst[3];   /* 2*F1-F4 */
			fdp[8] = 2 * fst[3] - fst[0];   /* 2*F4-F1 */
			nd = 5 + 4;
		    } else {
			num_oct = limit(0, num_oct, 2);
			nd = 5 + 3 * num_oct;
			for (k = 0; k < nd; k++) {
			    fdp[k] = cf2[k] * fst[1] + cf1[k] * fst[0];
			}
		    }
		    idp = (int)(dpftype.at);
		} else {
		    fm = (float) (modcyc * rate / (double) buflen);
		    fdp[0] = fst[1] - fm;   /* F2-FM */
		    fdp[1] = fst[1] + fm;   /* F2+FM */
		    fdp[2] = fst[0] - fm;   /* F1-FM */
		    fdp[3] = fst[0] + fm;   /* F1+FM */
		    nd = 4;
		    idp = 0;
		}
                rep = show_dp_fft(fst, ns, fdp, nd, pst, nst, ast, pdp, ndp, adp, 0);
		dis_result(pst, nst, pdp[idp], ndp[idp], fdp[idp], fst[0] == fst[1]);
		tel = (clock_t)((double) (clock() - each_time) / CLOCKS_PER_SEC);
		each_time = (int32_t)(clock());
		wr_dat_rec(fout, nd, tel, rep, fdp, pdp, adp, ndp, fst, pst, ast, nst);
		if (xtype.at == 1)
		    f = fst[0];
		else if (xtype.at == 2)
		    f = fdp[0];
		else
		    f = fst[1];
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
		else if (fst[0] == fst[1])
		    sf = 1;
		else
		    sf = 0;
		show_dp(f, lev, flag, flag, -1, sf, plot_yet++);
		flag++;
	    }
	    if (j)
		break;
	    f = fst[1];
	    if (bk_flg < 0) {
		if (cnd) {
		    datptr = previous_ptr();
		    cnd -= 2;
		} else {
		    datptr = dtlnk;
		    cnd = -1;
		}
		if (bk_flg == -2 && decide(1, 2,
			"Program paused for re-calibration",
			"Do you want to re-calibrate (Y/N)?")) {
		    recalibrate();
		    bk_flg = 2;
		} else {
		    bk_flg = 1;
		}
	    } else {
		datptr = datptr->next;
            }
	}
	if (j)
	    break;
	if (xtype.at != 6) {
	    flag = 0;
	    fprintf(fout, ";\n");
	}
    }

    if(fbin != NULL) {
	fclose(fbin);
    }
    run_time -= clock();
    bk_flg = 0;
    gr_beep();
    if (j) {
	decide(0, 2, 
	    "DPOAE test aborted!", "Press any key to continue");
    }
    i = (int) ((float) -run_time / CLOCKS_PER_SEC);
    pct = (total > 0) ? acpt * 100.0 / total : 0;
    if (total < 1000)
        sprintf(outmsg[1], "%.0f%% (%d/%d)", pct, acpt, total);
    else
        sprintf(outmsg[1], "%.0f%%", pct);
    fprintf(fout, ";FileName: %s\n;Time: %d\n;Accepted: %s\n", file_name,
	i, outmsg[1]);
    fprintf(fout, ";limit: %f\n", limit_mv);
    sprintf(outmsg[2], "%d sweeps", swp1set);
    flag = (int)(strlen(dpoae_file));
    if (!flag) {
	fprintf(fout, ";Comment F1: %.0f/%.0f  L1: %.0f/%.0f\n", F1b, F1e,
	    L1b, L1e);
	fprintf(fout, ";Comment F2: %.0f/%.0f  L2: %.0f/%.0f\n", F2b, F2e,
	    L2b, L2e);
	fprintf(fout, ";Comment conditions=%d   repeat=%d\n", dp_items, 
	    dp_repeat);
    }
    fclose(fout);

    if (acpt > 0 && decide(1, 2, "Save the file", file_name)) {
	rename(tmp_dat_file, file_name);
	if(fbin) {
	    newext(file_name, "BIN");
	    rename(tmp_bin_file, file_name);
	    newext(file_name, "DAT");
	}
	if(fswp && r_mode.at == 1) {
	    close_swp();
    	    strcpy(swp_file, file_name);
	    newext(swp_file, "SWP");
	    rename(tmp_swp_file, swp_file);
	}
	strcpy(o_file_name, cali_file);
	dpoae_counter++;
	write_counter_file();
    } else {
	newext(file_name, "CAL");
	if (strcmp(file_name, cali_file) == 0) {
	    strcpy(o_file_name, not_saved);
	} else {
	    strcpy(o_file_name, cali_file);
	}
	strcpy(file_name, not_saved);
    }
    fileflag = 1;
    out_msg(i, outmsg);

    j = draw_w(&w_info);
    k = dis_patient() + 2;
    gprintf(j, k, "Fd frequency:  %s", dpfrqtyp[dpftype.at]);
    k += txtpar.font_height;
    gprintf(j, k, "Calibration:  %s", o_file_name);
    k += txtpar.font_height;
    if (flag)
	gprintf(j, k, "List file:  %s", dpoae_file);
    else {
	gprintf(j, k, "F1: %.0f/%.0f  L1: %.0f/%.0f", F1b, F1e,
	    L1b, L1e);
	k += txtpar.font_height;
	gprintf(j, k, "F2: %.0f/%.0f  L2: %.0f/%.0f", F2b, F2e,
	    L2b, L2e);
	k += txtpar.font_height;
	gprintf(j, k, "conditions=%d   repeat=%d", dp_items, dp_repeat);
    }
    dp_items = 0;
    del_data_lnk();
    thev_adj_free();
    return (1);
}

static void
inf_msg(char *s)
{
    text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / 8 - 1;
    gprintf(w_info.xtop + 4, w_info.ybot - 4, s);
    check_event();
}

int
chkfit_calibr()
{
    char *ext;
    int icav = 0, cal_ok = 0, ybot;

    ybot = w_spec.ybot;
    w_spec.ybot = ypix - 1;
    if (probe_flag) {
	ext = (numcav > 1) ? "PRB" : "CAL";
	genfn("P", ext, cali_file, &probe_counter);
    } else if (cali_save[0]) {
	strcpy(cali_file, cali_save);
    } else {
	cali_file[0] = 0; // turns off Test option
    }
    while (!pgm_done()) {
        open_stim_w(dpoae.calibrate);
	set_att(chk_attn);
	dis_cav(icav);
	if (check_fit() == 0) {
	    cal_ok = 0;
	    inf_msg("Calibration Aborted.        ");
	    break;
	}
	display_reset = 1;
	R_clear();
        open_stim_w(dpoae.calibrate);
	set_att(cal_attn);
	w_dis = &w_sig;
	cal_ok = calibrate_it(icav);
	if (cal_ok < 0) {
	    cal_ok = 0;
	    break;
	} else if (cal_ok) {
	    icav++;
	    if (icav >= numcav)
		break;
	}
    }
    if (!cal_ok) {
        if (icav > 0) 
	    remove(cali_file);
	cali_file[0] = 0;
    }
    w_spec.ybot= ybot;
    return (cal_ok);
}

static int
getcal()
{
    char tmp_id[8];
    int ok = 0, swpver = 0;
    static char swp_id[8] = "DPSW1";

    if (r_mode.at == 2) {
        open_stim_w(dpoae.calibrate);
	R_clear();
        pre_dis_ind("Artifact", "Limit");
        dis_artifact();
        draw_w(&w_info);
	if (!*swp_file)
	    strcpy(swp_file, tmp_swp_file);
	fswp = fopen(swp_file, "rb");
	if (fswp) {
            fread(tmp_id, 1, (size_t)(8), fswp);
	    if (strcmp(tmp_id, "DPSW1") == 0)
		swpver = 1;
    	    fread(&buflen, sizeof(short), (size_t)(1), fswp);
	    fread(&swp1set, sizeof(short), (size_t)(1), fswp);
	    fread(&rate, sizeof(int32_t), (size_t)(1), fswp);
	    if (swpver >= 1)
    		fread(&dspnic, sizeof(short), (size_t)(1), fswp);
	    fread(&pas_per_vlt, sizeof(float), (size_t)(1), fswp);
	    fread(&vlt_per_cnt, sizeof(float), (size_t)(1), fswp);
	    fread(cali_file, 1, (size_t)(MAXNAME), fswp);
	    if (cal_read(cali_file, 0))
		cal_disp(cali_file, 0);
	    compute_calgain(0);
	    compute_calgain(1);
	    change_nic(dspnic);
	    check_event();
	    ok = 1;
	}
    } else {
	ok = chkfit_calibr();
        if (r_mode.at == 1 && ok) {
	    strcpy(swp_file, tmp_swp_file);
	    fswp = fopen(swp_file, "wb");
	    if (fswp) {
		fwrite(swp_id, 1, (size_t)(8), fswp);
		fwrite(&buflen, sizeof(short), (size_t)(1), fswp);
		fwrite(&swp1set, sizeof(short), (size_t)(1), fswp);
		fwrite(&rate, sizeof(int32_t), (size_t)(1), fswp);
		fwrite(&dspnic, sizeof(short), (size_t)(1), fswp);
		fwrite(&pas_per_vlt, sizeof(float), (size_t)(1), fswp);
		fwrite(&vlt_per_cnt, sizeof(float), (size_t)(1), fswp);
		fwrite(cali_file, 1, (size_t)(MAXNAME), fswp);
	    }
	} else {
	    fswp = NULL;
	}
    }
    return (ok);
}

void
DPOAE_wind()
{
    if (r_mode.at != 2) {
	if (dsp_init() > 0)
	    return;
    }
    top_message("DPOAE      ");
    dpoae.rate = adjust_rate(dpoae.rate);
    set_test_opt(1, 0, 0);
    dpoae_flag = 1;
    grab_dpoae();
    draw_w(&w_msg);
    draw_w(&w_info);
    FILE_SAVE.status = 0;
    if (getcal()) {
        swp1set *= 2;
	display_reset = 1;
	dfgc = scrn_c[C_CHNA];
	dis_stim(0, accbuf_a, 1, swp1set, acc_sets);
	display_reset = 0;
	dfgc = scrn_c[C_CHNB];
	dis_stim(0, accbuf_b, 1, swp1set, acc_sets);
        do_dpoae_task();
        close_swp();
    }
    set_test_opt(1, 1, 1);
    dfgc = scrn_c[C_WAVE];
    txtpar.text_wind_len = 34;
    dpoae_flag = 0;
    top_message("           ");
    if (r_mode.at != 2) {
        dsprst(TRUE);
    }
}
