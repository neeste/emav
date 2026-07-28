/* naverage.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <dsp.h>
#include <menu.h>
#include <token.h>
#include <util.h>
#include "savage.h"
#include "color.h"
#include "screen.h"
#include "version.h"

#ifndef O_BINARY
#define O_BINARY	0
#endif
#define C_MODE          (O_CREAT | O_RDWR | O_TRUNC | O_BINARY)
#define O_MODE          (O_RDONLY| O_BINARY)
#define P_MODE          (S_IREAD | S_IWRITE)
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#define _access access
#define _close close
#define _lseek lseek
#define _open open
#define _read read 
#define _write write 
#endif /* WIN32 */

#define TRIGGER_LEVEL	3277
#define MAXULONG	(4294967296.0)
#define SQRT_2		(1.414213562)
#define QM_FSP         0
#define QM_SCOR        1
#define QM_REPR        2
#define QM_SNR         3
#define QM_FMP         4

double  db_(double), dbv(double);
double  def_time();
double  var_dif(float *, float *);
double  var_sum(float *, float *);
double  cor_sum(float *, float *);
double  var_mp(float *, float *, double, double, int, int);
double  stability(int, float *, float *, int);
int     alloc_space(int);
int     check_timing();
int     dis_stim(int, void *, enum TYPE);
int     disply(double, float *, int, int, double, MARK *, int);
int     num_sets(int);
int	nxt_lst(void);
int     open_text_w(int *);
int     proc_art(int);
int     Info_wind(), lookahead();
int     Quit_wind(char *);
void    clear_display(void);
void    cursor_line(int, int, int);
void    close_text_w(void);
void    downldpar(int, int, int, int);
void    fft_stim(float *, int, int);
void    tok_store(int, int, float *);
void    show_fft(int, int, int, int, int);
void    out_msg(int);
void    zpf(float *, int, double, double);
void    text_color(int, int);
void    reset_timing();
void    store_evnt(int, int32_t, int, int);
void    wr_aux(TOKENFILE *, struct AUX_INF *);
void    rd_aux(TOKENFILE *, struct AUX_INF *);
void    init_spec();
void    top_message(char *);
void    tok_init(int);
void    sum_dif(float *, float *, int, float);
void    Tem_A_B();
void    Fmp_A_B(int, float *, float *, float *, float *);
void    repr_A_B();
void    init_mp();
void    dis_units(double, double, double);
void	epamp_init(int, int, int, int);
void    edit_control(int, int, int *, int *, int *, int *, int *, int *, int *);
void    show_xlabel(double, double, int, char *);
void    write_counter_file();
void    dis_pos_info(double, double, double, double, double, double, double);

extern char line[];
extern char record_file[];
extern char *abr_tmp_name, *not_saved;
extern char *month_name[12];
extern double response_level[2];
extern double dsp_scale;
extern float time1_ms, time2_ms, rej_mv;
extern float max_level, test_level;
extern float repr_crit1, repr_crit2, VR_crit1, VR_crit2;
extern float def_xrange, def_yrange;
extern float ep_start, ad_sens;
extern int min_sweeps, min_swps;
extern int pqm_type;
extern int aqm_type;
extern int weight_type;
extern int ver_num;
extern int pre_gain;
extern int abr_count;
extern int recflg;
extern int term_code;
extern int lstflg;
extern int nmp;
extern int series_flag;
extern int ep_chan;
extern int make_template, trkbuf;
extern int32_t check_fit_run_time;
extern struct abr abr_;
extern struct BINhdr binhdr;
extern struct stim stim_;
extern unsigned shift_status;
extern FILE *binfp;
extern TOGGLETYPE distype, r_mode, pqm_mode, ok_mode, filter_type, weight;
extern TOGGLETYPE monitor, few, polar, sqm, pqm, aqm;
extern TOGGLETYPE epa_hpf, epa_gain, epa_lpf, epa_acc, epa_pgn, epa_notch, epa_pol; 
extern TOKENFILE *t;

char    outmsg[3][16];
float   F1_cutoff = 0.0, F2_cutoff = 0.0;
float   sh_of[2][2] = {{0}};
float   sh_range = 1.0, sh_shift = 0.0;
float   aver_ofst_a1 = 0.0, aver_ofst_a2 = 0.0;
float   aver_ofst_b1 = 0.0, aver_ofst_b2 = 0.0;
float   twt_A = 0, twt_B = 0;
float  *accbuf_a1, *accbuf_a2, *accbuf_b1, *accbuf_b2;
float   *outbuf_a, *outbuf_b;
short     ifsp = 0;
int     xrange = 64, xbegin = 0;
int     oper_ok = 0, nswpsets = 0, rts = 0, reset_count = 0;
int     sh_A = 1, sh_B = 1, sh_ab = 0;
int     sh_12 = 0, sh_ul = 0, sh_mk = 1;
int     sh_om = 1, sh_sd = 0;
int     trklen, nos_type = 0;
int     stored, nsets_i, nsets_f;
int     ch2 = 1;
int     xmrgn = 20;
int     rej_mod = 0;
struct AUX_INF ai = {0};
struct _result result;
struct _sp {float a, b;} sp[2] = {{0, 0}, {0, 0}};
struct _sresult sresult;
REJ_IND t_ind;

static char *qm_nam[] = {
    "Fsp", "Scor",
    "Repr", "SNR", "Fmp", "Fxx",
};
static char *qm_fmt[] = {
    "%s  A=%4.1f%c%c  B=%4.1f%c%c  C=%4.1f,%4.1f  ",
    "%s  A=%4.1f%c%c  B=%4.1f%c%c  C=%4.1f,%4.1f  ",
    "%s  A=%3.0f%%%c%c  B=%3.0f%%%c%c  C=%3.0f%%,%3.0f%%  ",
    "%s  A=%4.1f%c%c  B=%4.1f%c%c  C=%4.1f,%4.1f  ",
    "%s  A=%4.1f%c%c  B=%4.1f%c%c  C=%4.1f,%4.1f  ",
    "%s  A=%4.1f%c%c  B=%4.1f%c%c  C=%4.1f,%4.1f  ",
};
static float wt_A = 0, wt_B = 0;
static float ts1sp_A = 0, ts1sp_B = 0;
static float tsssp_A = 0, tsssp_B = 0;
static float sqsp_A[2] = {0}, sqsp_B[2] = {0};
static float Aaqm = 0, Baqm = 0, Apqm = 0, Bpqm = 0;
static float Ansp = 0, Bnsp = 0, Anmn = 0, Bnmn = 0;
static float pqm_crit1 = 0, pqm_crit2 = 0;
static float aqm_crit1 = 0, aqm_crit2 = 0;
static float A_qm[5], B_qm[5];
static int fhd = -1;
static short *Aaqm_trk, *Baqm_trk, *Apqm_trk, *Bpqm_trk;
static short *Ansp_trk, *Bnsp_trk, *Anmn_trk, *Bnmn_trk;

void
points_msg()
{
    text_color(scrn_c[C_MSGF], scrn_c[C_MSGB]);
    txtpar.text_wind_len = 4;
    (void) gprintf(490, txtpar.font_height, few.strs[few.at]);
}

void
pre_dis_ind(char *s, char *m)
{
    char    rm[3] = {'N', 'M', 'R'};
    int     i, j;

    w_ind.ybot = ypix - 10 - txtpar.menu_height;
    w_ind.ytop = w_ind.ybot - txtpar.menu_height;
    i = draw_w(&w_ind);
    j = w_ind.ytop + txtpar.font_height;
    (void) gprintf(i, j, "%s", s);
    txtpar.text_wind_len = 1;
    (void) gprintf(i + 104, j, "%c", rm[rej_mod]);
    w_ind.ytop = w_ind.ybot;
    w_ind.ybot = ypix - 1;
    i = draw_w(&w_ind);
    j = w_ind.ytop + txtpar.font_height;
    (void) gprintf(i, j, m);
    i = w_ind.ybot - 10;
    mouse_exclude(w_ind.xtop, i, w_ind.xbot, i);
    gr_line(w_ind.xtop, i, w_ind.xbot, i, menucolor.boxc);
    mouse_show(1);
    t_ind.top = ++i;
    t_ind.bot = w_ind.ybot - 1;
    t_ind.old = t_ind.old1 = w_ind.xtop + 1;
    t_ind.lim = rej_mv;
}

void
dis_artifact()
{
    double  mvmax;
    int     i, j, k, tfgc_s, tbgc_s;
    int32_t    range;

    t_ind.lim = rej_mv;
    mvmax = t_ind.new * 1000.0 / Sen.AD;

    rd_text_color(&tfgc_s, &tbgc_s);
    text_color(w_ind.wfgc, w_ind.wbgc);
    j = w_ind.xtop + 54;
    k = w_ind.ytop + txtpar.font_height;
    txtpar.text_wind_len = 12;
    if (t_ind.lim < 10000.0)
	i = 2;
    else
	i = 1;
    if(t_ind.lim < 0.01)
        (void) gprintf(j, k, "%5.*f nV", i, t_ind.lim / 0.001);
    else if(t_ind.lim < 1000.0)
        (void) gprintf(j, k, "%5.*f uV", i, t_ind.lim );
    else
        (void) gprintf(j, k, "%5.*f mV", i, t_ind.lim / 1000.0);
    if (mvmax < t_ind.lim)
	range = (int32_t) (t_ind.lim / 5.0);
    else
	range = (int32_t) (mvmax / 5.0);
    range++;
    range *= 5;
    j = (w_ind.xbot - w_ind.xtop);
    k = (int) (w_ind.xtop + t_ind.lim / range * j);
    mouse_exclude(w_ind.xtop, w_ind.ytop, w_ind.xbot, w_ind.ybot);
    gr_rectf(w_ind.xtop + 1, t_ind.top + 4, t_ind.old1, t_ind.bot, scrn_c[C_INDB]);
    gr_rectf(t_ind.old, t_ind.top, t_ind.old, t_ind.bot, scrn_c[C_INDB]);
    t_ind.old = k;
    k = (int) (w_ind.xtop + mvmax / range * j);

    if (k > w_ind.xbot)
	k = w_ind.xbot;
    t_ind.old1 = k;
    gr_rectf(w_ind.xtop + 1, t_ind.top + 4, k, t_ind.bot, scrn_c[C_INDN]);
    gr_rectf(t_ind.old, t_ind.top, t_ind.old, t_ind.bot, scrn_c[C_INDL]);
    mouse_show(1);
    text_color(tfgc_s, tbgc_s);
}

static int pause_collect = 0;

int
check_rej(float *p)
{
    int     i, flag, start, end;
    double  tm, m, U;

    if (pause_collect)
	return (1);
    if (rej_mod == 1) {             /* MEAN reject */
	m = 0;
	flag = 0;
	U = (float) upperv * swp1set;
	start = itime1;
	end = itime2;
	for (i = start; i < end; i++) {
	    tm = p[i];
	    if (tm < 0)
		tm = -tm;
	    if (tm > U)
		flag = 1;
	    if (tm > m)
		m = tm;
	}
	start += buflen;
	end += buflen;
	for (i = start; i < end; i++) {
	    tm = p[i];
	    if (tm < 0)
		tm = -tm;
	    if (tm > U)
		flag = 1;
	    if (tm > m)
		m = tm;
	}
	t_ind.new = (float) m / swp1set;
    } else if (rej_mod == 2) {      /* RAW reject */
    	m = 0;
        if (ep_chan & 1) {
            if (m < result.max_a)
                m = result.max_a;
            if (m < -result.min_a)
                m = -result.min_a;
         }
        if (ep_chan & 2) {
            if (m < (result.max_b * 256))
                m = (result.max_b * 256);
            if (m < (-result.min_b * 256))
                m = (-result.min_b * 256);
        }
        flag = (m > upperv);
        t_ind.new = (float) m;
    } else {                        /* No reject */
        flag = 0;
    }
    dis_artifact();
    return (flag);
}

int
init_aver()
{
    int     i, n;

    accbuf_a1 = accbuf_1;
    accbuf_a2 = accbuf_2;
    accbuf_b1 = accbuf_1 + buflen;
    accbuf_b2 = accbuf_2 + buflen;
    outbuf_a = (float *) outbuf;
    outbuf_b = (float *) outbuf + buflen;

    if (xrange > buflen)
	xrange = buflen;
    else if (xrange < 16)
	xrange = 16;
    n = buflen * 2;

    trklen = (trkbuf * buflen) / 8;
    Aaqm_trk = (short *) fsprepro;
    Baqm_trk = (short *) fsprepro + 2 * trklen;
    Apqm_trk = (short *) fsprepro + 4 * trklen;
    Bpqm_trk = (short *) fsprepro + 6 * trklen;
    Ansp_trk = (short *) fsprepro + 1 * trklen;
    Bnsp_trk = (short *) fsprepro + 3 * trklen;
    Anmn_trk = (short *) fsprepro + 5 * trklen;
    Bnmn_trk = (short *) fsprepro + 7 * trklen;
    if (aqm_type == QM_REPR) {
	aqm_crit1 = repr_crit1 * 100;
	aqm_crit2 = repr_crit2 * 100;
    } else {
	aqm_crit1 = VR_crit1;
	aqm_crit2 = VR_crit2;
    }
    if (pqm_type == QM_REPR) {
	pqm_crit1 = repr_crit1 * 100;
	pqm_crit2 = repr_crit2 * 100;
    } else {
	pqm_crit1 = VR_crit1;
	pqm_crit2 = VR_crit2;
    }

    Aaqm = Baqm = Apqm = Bpqm = Ansp = Bnsp = Anmn = Bnmn = 0;
    for (i = 0; i < n; i++) {
	accbuf_1[i] = 0.0;
	accbuf_2[i] = 0.0;
    }

    return (n);
}

void
init_wt()
{
    ts1sp_A = ts1sp_B = 0;
    tsssp_A = tsssp_B = 0;
    init_mp();
    twt_A = twt_B = 0;
    stored = nsets_i = nsets_f = 0;
}

void
init_ai(double dt)
{
    memset(&ai, 0, sizeof(ai));
    abr_pk[0][0] = nint(ep_start / dt);
}

void
init_sh()
{
    sh_of[0][0] = sh_of[0][1] = sh_of[1][0] = sh_of[1][1] = 0;
    sh_A = ep_chan & 1;
    sh_B = ep_chan & 2;
    sh_ab = (ep_chan & 1) ? 0 : 1;
    sh_mk = 1;
    sh_range = def_yrange;
    sh_shift = 0;
    sh_ul = 0;
    sh_om = 1;          /* offset mode */
    oper_ok = 0;
}

static float t_stab = 0;
static double a1bl[2][3] = {{0.2, 0.0, 0.1},{0.0, 0.2, 0.3}};
static double a2bl[2][3] = {{-0.2, 0.0, -0.1},{0.0, 0.2, 0.1}};
static double b1bl[2][3] = {{0.2, 0.0, 0.1},{0.0, -0.2, -0.1}};
static double b2bl[2][3] = {{-0.2, 0.0, -0.1},{0.0, -0.2, -0.3}};

void
show_A_B_labels()
{
    int     a1lb = 0, a2lb = 0, b1lb = 0, b2lb = 0;
    int     xxlb, oklb;
    int     i, h, m, b, c;
    static char *syms[] = {"12", "+-",};

    i = (w_sig.ytop + w_sig.ybot) / 2;
    h = txtpar.font_height;
    m = h / 2;
    switch (sh_om) {
    case 0:
	a1lb = (int) (i - 2 * h + m);
	a2lb = (int) (i - h + m);
	b1lb = (int) (i + h + m);
	b2lb = (int) (i + 2 * h + m);
	break;
    case 1:
	a1lb = (int) (i - h - a1bl[ch2][1] * i + m);
	a2lb = (int) (i + h - a2bl[ch2][1] * i + m);
	b1lb = (int) (i - h - b1bl[ch2][1] * i + m);
	b2lb = (int) (i + h - b2bl[ch2][1] * i + m);
	break;
    case 2:
	a1lb = (int) (i - h - a1bl[ch2][2] * i + m);
	a2lb = (int) (i - h - a2bl[ch2][2] * i + m);
	b1lb = (int) (i - h - b1bl[ch2][2] * i + m);
	b2lb = (int) (i - h - b2bl[ch2][2] * i + m);
	break;
    }

    w_dis = &w_sig;
    xxlb = w_sig.xtop + 3;
    oklb = w_sig.ytop + h;
    i = sh_sd ? 1 : 0;
    b = w_sig.wbgc;
    c = -1;
    if (sh_A) {
        text_color(scrn_c[C_CHA1], b);
        (void) gprintf(xxlb, a1lb, "A%c", syms[i][0]);
        if (sh_ul && sh_ab == 0 && sh_12 == 0) {
            text_color(scrn_c[C_CHA1], c);
            (void) gprintf(xxlb, a1lb, "__");
        }
        if (sh_sd != 2) {
            text_color(scrn_c[C_CHA2], b);
            (void) gprintf(xxlb, a2lb, "A%c", syms[i][1]);
            if (sh_ul && sh_ab == 0 && sh_12 == 1) {
                text_color(scrn_c[C_CHA2], c);
                (void) gprintf(xxlb, a2lb, "__");
            }
        }
    }
    if (sh_B) {
        text_color(scrn_c[C_CHB1], b);
        (void) gprintf(xxlb, b1lb, "B%c", syms[i][0]);
        if (sh_ul && sh_ab == 1 && sh_12 == 0) {
            text_color(scrn_c[C_CHB1], c);
            (void) gprintf(xxlb, b1lb, "B%c", syms[i][0]);
//            (void) gprintf(xxlb, b1lb, "__");
        }
        if (sh_sd != 2) {
            text_color(scrn_c[C_CHB2], b);
            (void) gprintf(xxlb, b2lb, "B%c", syms[i][1]);
            if (sh_ul && sh_ab == 1 && sh_12 == 1) {
                text_color(scrn_c[C_CHB2], c);
                (void) gprintf(xxlb, b2lb, "B%c", syms[i][1]);
 //               (void) gprintf(xxlb, b2lb, "__");
            }
        }
    }
    if (oper_ok) {
	text_color(scrn_c[C_OKTX], b);
	(void) gprintf(xxlb, oklb, "OK");
    }
}

static char *symbols[] = {
    "0", "I", "II", "III", "IV", "V", "V'", "S"
};

void
show_A_B(int ss)     //ss indicates to show stimulus
{
    double  a1of, a2of, b1of, b2of;
    int     i, drst, dflg;
    float   wa, wb;
    short   loc_A[8], loc_B[8];
    MARK    mark, *dmrk;
    static double eps = 1e-30;

    display_reset = 1;
    dfgc = scrn_c[C_WAVE];
    if (ss) 
    {
	distype.at = 1;
	(void) dis_stim(0, sav_stim, LONG);
    }
    distype.at = 0;

    w_dis = &w_sig;
    clear_display();
    ch2 = (sh_A && sh_B) ? 1 : 0;
    show_A_B_labels();
    text_color(w_sig.wfgc, w_sig.wbgc);
    w_sig.xtop += xmrgn;

    show_xlabel(xbegin * 1000.0 / rate, xrange * 1000.0 / rate, 1, "ms");
    w_sig.ybot -= txtpar.font_height;
    gr_recto(w_sig.xtop, w_sig.ytop, w_sig.xbot, w_sig.ybot, menucolor.boxc);

    wa = (twt_A > eps) ? twt_A : 0;
    wb = (twt_B > eps) ? twt_B : 0;
    a1of = a1bl[ch2][sh_om] + sh_shift + sh_of[0][0];
    a2of = a2bl[ch2][sh_om] + sh_shift + sh_of[0][1];
    b1of = b1bl[ch2][sh_om] + sh_shift + sh_of[1][0];
    b2of = b2bl[ch2][sh_om] + sh_shift + sh_of[1][1];
    if (sh_sd) {
	sum_dif(accbuf_a1, accbuf_a2, buflen, 2.0);
	sum_dif(accbuf_b1, accbuf_b2, buflen, 2.0);
    }
    mark.beg = xbegin;
    mark.sym = symbols;
    loc_A[0] = loc_B[0] = abr_pk[0][0];
    for (i = 1; i < 7; i++) {
    	loc_A[i] = abr_pk[0][i];
    	loc_B[i] = abr_pk[1][i];
    }
    loc_A[7] = loc_B[7] = abr_pk[1][0];
    sig_v.vpp = sh_range;
    drst = 1;
    if (polar.at == epa_pol.at) {
        dflg = D_NOUP | D_GAIN;
    } else {
        dflg = D_NOUP | D_GAIN | D_FLIP;
    }

    dmrk = sh_mk ? &mark : NULL;
    if (sh_sd == 2) {
        if (sh_B) {
            mark.loc = loc_B;
            dfgc = scrn_c[C_CHB1];
            (void) disply(wb, &accbuf_b1[xbegin], xrange, drst, b1of, dmrk, dflg);
            display_reset = drst = 0;
        }
        if (sh_A) {
            mark.loc = loc_A;
            dfgc = scrn_c[C_CHA1];
            (void) disply(wa, &accbuf_a1[xbegin], xrange, drst, a1of, dmrk, dflg);
            display_reset = drst = 0;
        }
    } else {
        if (sh_B) {
            dfgc = scrn_c[C_CHB2];
            (void) disply(wb, &accbuf_b2[xbegin], xrange, drst, b2of, NULL, dflg);
            display_reset = drst = 0;
        }
        if (sh_A) {
            dfgc = scrn_c[C_CHA2];
            (void) disply(wa, &accbuf_a2[xbegin], xrange, drst, a2of, NULL, dflg);
            display_reset = drst = 0;
        }
        if (sh_B) {
            mark.loc = loc_B;
            dfgc = scrn_c[C_CHB1];
            (void) disply(wb, &accbuf_b1[xbegin], xrange, drst, b1of, dmrk, dflg);
            display_reset = drst = 0;
        }
        if (sh_A) {
            mark.loc = loc_A;
            dfgc = scrn_c[C_CHA1];
            (void) disply(wa, &accbuf_a1[xbegin], xrange, drst, a1of, dmrk, dflg);
            display_reset = drst = 0;
        }
    }
    if (sh_sd) {
	sum_dif(accbuf_a1, accbuf_a2, buflen, 1.0);
	sum_dif(accbuf_b1, accbuf_b2, buflen, 1.0);
    }
    w_sig.xtop -= xmrgn;
    w_sig.ybot += txtpar.font_height;
}

void
Fmp_A_B(int nset, float *tp_a1, float *tp_b1, float *tp_a2, float *tp_b2)
{
    float   vsa = 0, vda = 0, vsb = 0, vdb = 0;
    float   dbuv_ref;

    if (ep_chan & 1) {
        vsa = (float) var_sum(accbuf_a1, accbuf_a2) / (twt_A * twt_A * 4);
        vda = (float) var_mp(tp_a1, tp_a2, wt_A, twt_A, nset, 0);
    }
    if (ep_chan & 2) {
        vsb = (float) var_sum(accbuf_b1, accbuf_b2) / (twt_B * twt_B * 4);
        vdb = (float) var_mp(tp_b1, tp_b2, wt_B, twt_B, nset, 1);
    }
    A_qm[QM_FMP] = vda > 0 ? vsa / vda : 0;
    B_qm[QM_FMP] = vdb > 0 ? vsb / vdb : 0;
    if (aqm_type == QM_SCOR || pqm_type == QM_SCOR) {
        A_qm[QM_SCOR] = A_qm[QM_FMP] * (float) cor_sum(accbuf_a1, accbuf_a2);
        B_qm[QM_SCOR] = B_qm[QM_FMP] * (float) cor_sum(accbuf_b1, accbuf_b2);
    }

/* noise parameters */

    dbuv_ref = (float) dbv(Sen.AD * atof(epa_gain.strs[epa_gain.at]) * 1e-6);
    nos_type = 3;
    Anmn = (float) db_(vda) - dbuv_ref;
    Bnmn = (float) db_(vdb) - dbuv_ref;
}

void
repr_A_B()
{
    int     i, n;
    float   sa1 = 0, sa2 = 0, sa11 = 0, sa22 = 0, sa12 = 0;
    float   sb1 = 0, sb2 = 0, sb11 = 0, sb22 = 0, sb12 = 0;
    float   A1, A2, ea11, ea22, ea12, Arep;
    float   B1, B2, eb11, eb22, eb12, Brep;

    for (i = itime1; i < itime2; i++) {
	if (ep_chan & 1) {
	    A1 = accbuf_a1[i];
	    A2 = accbuf_a2[i];
	    sa1 += A1;
	    sa2 += A2;
	    sa11 += A1 * A1;
	    sa22 += A2 * A2;
	    sa12 += A1 * A2;
	}
	if (ep_chan & 2) {
	    B1 = accbuf_b1[i];
	    B2 = accbuf_b2[i];
	    sb1 += B1;
	    sb2 += B2;
	    sb11 += B1 * B1;
	    sb22 += B2 * B2;
	    sb12 += B1 * B2;
	}
    }

    n = itime2 - itime1;
    if (n == 0)
	return;
    ea11 = (sa11 - sa1 * sa1 / n) / n;
    ea22 = (sa22 - sa2 * sa2 / n) / n;
    ea12 = (sa12 - sa1 * sa2 / n) / n;
    Arep = (float) ((ea11 <= 0 || ea22 <= 0) ? 0 : ea12 / sqrt(ea11 * ea22));
    eb11 = (sb11 - sb1 * sb1 / n) / n;
    eb22 = (sb22 - sb2 * sb2 / n) / n;
    eb12 = (sb12 - sb1 * sb2 / n) / n;
    Brep = (float) ((eb11 <= 0 || eb22 <= 0) ? 0 : eb12 / sqrt(eb11 * eb22));

    A_qm[QM_REPR] = Arep * 100;
    B_qm[QM_REPR] = Brep * 100;
}

void
Fsp_A_B(int nset)
{
    int     i, n;
    float   sa1 = 0, sa11 = 0, da1 = 0, da11 = 0;
    float   sb1 = 0, sb11 = 0, db1 = 0, db11 = 0;
    float   A1, ea11, na11, wa;
    float   B1, eb11, nb11, wb;
    float   tnswp, dbuv_ref;
    float   varsp_A, varsp_B;

/* single point */

    wa = twt_A * 2;
    wb = twt_B * 2;
    tnswp = (float) (nset * 2.0 * swp1set);
    ts1sp_A += (sp[0].a + sp[1].a) * wt_A;
    ts1sp_B += (sp[0].b + sp[1].b) * wt_B;
    tsssp_A += (sqsp_A[0] + sqsp_A[1]) * wt_A * wt_A;
    tsssp_B += (sqsp_B[0] + sqsp_B[1]) * wt_B * wt_B;
    varsp_A = (tsssp_A * tnswp - ts1sp_A * ts1sp_A) / (wa * wa);
    varsp_B = (tsssp_B * tnswp - ts1sp_B * ts1sp_B) / (wb * wb);

/* window interval */

    for (i = itime1; i < itime2; i++) {
        if (ep_chan & 1) {
	    A1 = (accbuf_a1[i] + accbuf_a2[i]) / wa;
	    sa1 += A1;
	    sa11 += A1 * A1;
	    A1 = (accbuf_a1[i] - accbuf_a2[i]) / wa;
	    da1 += A1;
	    da11 += A1 * A1;
	}
        if (ep_chan & 2) {
	    B1 = (accbuf_b1[i] + accbuf_b2[i]) / wb;
	    sb1 += B1;
	    sb11 += B1 * B1;
	    B1 = (accbuf_b1[i] - accbuf_b2[i]) / wb;
	    db1 += B1;
	    db11 += B1 * B1;
	}
    }
    n = itime2 - itime1;
    if (n <= 0)
	return;
    ea11 = (sa11 - sa1 * sa1 / n) / n;
    eb11 = (sb11 - sb1 * sb1 / n) / n;

/* ratio of variances */

    A_qm[QM_FSP] = (varsp_A > 0) ? (ea11 / varsp_A) * tnswp : 0;
    B_qm[QM_FSP] = (varsp_B > 0) ? (eb11 / varsp_B) * tnswp : 0;

/* noise parameters */

    dbuv_ref = (float) dbv(Sen.AD * atof(epa_gain.strs[epa_gain.at]) * 1e-6);
    nos_type = 2;

/* single-point noise */

    Ansp = (float) db_(varsp_A / tnswp) - dbuv_ref;
    Bnsp = (float) db_(varsp_B / tnswp) - dbuv_ref;

/* averaged noise */

    na11 = (da11 - da1 * da1 / n) / n;
    nb11 = (db11 - db1 * db1 / n) / n;
    Anmn = (float) db_(na11) - dbuv_ref;
    Bnmn = (float) db_(nb11) - dbuv_ref;

/* SNR */

    A_qm[QM_SNR] = (na11 > 0) ? ea11 / na11 : 0;
    B_qm[QM_SNR] = (nb11 > 0) ? eb11 / nb11 : 0;
}

void
qm_info()
{
    char    name[8], *u, *n1, *n2;
    int     j, k;

    txtpar.text_wind_len = 60;
    text_color(w_info.wfgc, w_info.wbgc);

    strcpy(name, qm_nam[pqm_type]);
    strcat(name, (strlen(name) < 4) ? ": " : ":");
    j = w_info.xtop + 4;
    k = w_info.ytop + txtpar.font_height;
    (void) gprintf(j, k, qm_fmt[pqm_type],
        name,
	Apqm, evb(EC_FA1) ? '*' : ' ', evb(EC_FA2) ? '*' : ' ',
	Bpqm, evb(EC_FB1) ? '*' : ' ', evb(EC_FB2) ? '*' : ' ',
	pqm_crit1, pqm_crit2);

    strcpy(name, qm_nam[aqm_type]);
    strcat(name, (strlen(name) < 4) ? ": " : ":");
    k = w_info.ytop + txtpar.font_height * 2;
    (void) gprintf(j, k, qm_fmt[aqm_type], 
        name,
	Aaqm, evb(EC_RA1) ? '*' : ' ', evb(EC_RA2) ? '*' : ' ',
	Baqm, evb(EC_RB1) ? '*' : ' ', evb(EC_RB2) ? '*' : ' ',
	aqm_crit1, aqm_crit2);

    n1 = "Nsp";
    n2 = (nos_type == 3) ? "Nbb" : "Nav";
    u = (nos_type == 0) ? "dBV " : "dBuV";
    k = w_info.ytop + txtpar.font_height * 3;
    if (monitor.at == 2) {
        Ansp = limit(-320, Ansp, 320);
        Bnsp = limit(-320, Bnsp, 320);
	(void) gprintf(j, k, "%s:%5.0f%5.0f %s", n1, Ansp, Bnsp, u);
    } else {
        Anmn = limit(-320, Anmn, 320);
        Bnmn = limit(-320, Bnmn, 320);
	(void) gprintf(j, k, "%s:%5.0f%5.0f %s", n2, Anmn, Bnmn, u);
    }
}

int
int_limit(double v1, double v, double v2)
{
    return ((int) limit(v1, v, v2));
}

void
Monitor_track(int nset)
{
    char   *title = NULL;
    int     x, y, x1, y1, x2, y2;
    float   dx, dy, yr = 0, yya = 0, yyb = 0, yy0 = 0, yy1 = 0, yy2 = 0, yt = 0;
    extern WIND w_stim, w_stim_fft;
    static int xa, ya, xb, yb;
    static unsigned char fm[] = {
	0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55
    };

    if (!abr_.show_track)
        return;

    /* plot the Fsp track */

    switch (monitor.at) {
    case 0:
	title = qm_nam[pqm_type];
	yy0 = 1;
	yy1 = pqm_crit1;
	yy2 = pqm_crit2;
	yya = Apqm;
	yyb = Bpqm;
	yr = yt = 10;
	break;
    case 1:
	title = qm_nam[aqm_type];
	yy0 = (float) ((aqm_type == QM_REPR) ? 50 : 1);
	yy1 = aqm_crit1;
	yy2 = aqm_crit2;
	yya = Aaqm;
	yyb = Baqm;
	yr = yt = (float) ((aqm_type == QM_REPR) ? 100 : 10);
	break;
    case 2:
	title = "Nsp";
	yy0 = 20;
	yy1 = 10;
	yy2 = 30;
	yt = (float) ((nos_type == 0) ? 20 : 0);
	yya = Ansp + 40 - yt;
	yyb = Bnsp + 40 - yt;
	yr = 40;
	break;
    case 3:
	title = (nos_type == 3) ? "Nbb" : "Nav";
	yy0 = 20;
	yy1 = 10;
	yy2 = 30;
	yt = (float) ((nos_type == 0) ? -20 : 0);
	yya = Anmn + 40 - yt;
	yyb = Bnmn + 40 - yt;
	yr = 40;
	break;
    }
    w_ind.ybot = ypix - 10 - txtpar.menu_height;
    x1 = w_stim.xtop;
    y1 = w_stim_fft.ybot;
    x2 = w_stim_fft.xbot;
    y2 = txtpar.menu_height + 1;
    dx = (x2 - x1) / (float) nsweeps;
    dy = (y2 - y1) / yr;
    if (nset == 1) {
	text_color(w_stim.wfgc, w_stim.wbgc);
	gr_rectf(x1, y1, x2, y2, w_stim.wbgc);
	(void) gprintf(x1 + 4, y2 + txtpar.font_height, title);
	(void) gprintf(x2 - 24, y2 + txtpar.font_height, "%.0f", yt);
	gr_setfillmask(fm);
	y = int_limit(y2, yy0 * dy + y1, y1);
	gr_rectf(x1, y, x2, y, scrn_c[C_CHA2]);
	y = int_limit(y2, yy1 * dy + y1, y1);
	gr_rectf(x1, y, x2, y, scrn_c[C_CHB2]);
	y = int_limit(y2, yy2 * dy + y1, y1);
	gr_rectf(x1, y, x2, y, scrn_c[C_CHB2]);
	x = int_limit(x1, min_sweeps * dx + x1, x2);
	gr_rectf(x, y1, x, y2, scrn_c[C_CHB2]);
	gr_setfillmask(NULL);
	xa = xb = x1;
        ya = int_limit(y2, yya * dy + y1, y1);
        yb = int_limit(y2, yyb * dy + y1, y1);
    }
    x = int_limit(x1, nset * dx + x1, x2);
    if (sh_A) {
        y = int_limit(y2, yya * dy + y1, y1);
        gr_line(xa, ya, x, y, scrn_c[C_CHA1]);
	xa = x;
	ya = y;
    }
    if (sh_B) {
        y = int_limit(y2, yyb * dy + y1, y1);
        gr_line(xb, yb, x, y, scrn_c[C_CHB1]);
	xb = x;
	yb = y;
    }
}

void
out_spectrum()
{
    int     i;

    init_spec();
    (void) draw_w(&w_spec);
    if (sh_ab == 0) {
	for (i = 0; i < buflen; i++)
	    outbuf_a[i] = accbuf_a1[i] + accbuf_a2[i];
	fft_stim(outbuf_a, nswpsets, 0);
	show_fft(abr_.fft_xrange, abr_.fft_yrange, abr_.levref, scrn_c[C_CHA1], 1);
	for (i = 0; i < buflen; i++)
	    outbuf_a[i] = accbuf_a1[i] - accbuf_a2[i];
	fft_stim(outbuf_a, nswpsets, 0);
	show_fft(abr_.fft_xrange, abr_.fft_yrange, abr_.levref, scrn_c[C_CHA2], 0);
    } else {
	for (i = 0; i < buflen; i++)
	    outbuf_b[i] = accbuf_b1[i] + accbuf_b2[i];
	fft_stim(outbuf_b, nswpsets, 0);
	show_fft(abr_.fft_xrange, abr_.fft_yrange, abr_.levref, scrn_c[C_CHB1], 1);
	for (i = 0; i < buflen; i++)
	    outbuf_b[i] = accbuf_b1[i] - accbuf_b2[i];
	fft_stim(outbuf_b, nswpsets, 0);
	show_fft(abr_.fft_xrange, abr_.fft_yrange, abr_.levref, scrn_c[C_CHB2], 0);
    }
}

void
reconstruct_monitor(int e)
{
    float   dbref;
    int     i, n;

    if (e)
	nsweeps = nswpsets;
    n = (nswpsets <= trklen) ? nswpsets : trklen;
    for (i = 0; i < n; i++) {
	Aaqm = (float) Aaqm_trk[i] / 100;
	Baqm = (float) Baqm_trk[i] / 100;
	Apqm = (float) Apqm_trk[i] / 100;
	Bpqm = (float) Bpqm_trk[i] / 100;
	if (nos_type == 0) {
            Ansp = (float) Ansp_trk[i] / 100;
            Bnsp = (float) Bnsp_trk[i] / 100;
            Anmn = (float) Anmn_trk[i] / 100;
            Bnmn = (float) Bnmn_trk[i] / 100;
	} else if (nos_type == 1) {
	    dbref = (float) (20 * log10(2e4 / 1e6));
            Ansp = (float) Ansp_trk[i] / 10 - dbref;
            Bnsp = (float) Bnsp_trk[i] / 10 - dbref;
            Anmn = (float) Anmn_trk[i] / 10 - dbref;
            Bnmn = (float) Bnmn_trk[i] / 10 - dbref;
	} else {    /* nos_type == 2 || nos_type == 3 */
            Ansp = (float) Ansp_trk[i] / 10;
            Bnsp = (float) Bnsp_trk[i] / 10;
            Anmn = (float) Anmn_trk[i] / 10;
            Bnmn = (float) Bnmn_trk[i] / 10;
	}
	Monitor_track(i + 1);
    }
}

double
var_fsp(int aflg)
{
    float   v, sqspt = 0, spt = 0, n;

    n = (float) swp1set * 2;
    if (aflg) {
        if (ep_chan & 1) {
            spt = sp[0].a + sp[1].a;
            sqspt = sqsp_A[0] + sqsp_A[1];
        }
    } else {
        if (ep_chan & 2) {
            spt = sp[0].b + sp[1].b;
            sqspt = sqsp_B[0] + sqsp_B[1];
        }
    }
    if (n > 0) {
	v = (sqspt - (spt * spt) / n) / n;
	if (v > 0)
	    return (v);
    }
    return (1);
}

int
message_out(int *x, int *y)
{
    double  dt;
    int     j, k, m;

    dt = 1000.0 / rate;
    j = draw_w(&w_msg);
    k = w_msg.ytop + txtpar.font_height;
    if(head_param)      //this will display the parameter number if we're using a BIN file
    {
        (void) gprintf(j, k, "LevSig=%3.0f dB", curr_param->level1);
        k += txtpar.font_height;
        (void) gprintf(j, k, "LevMask=%3.0f dB", curr_param->level2);
        k += txtpar.font_height;
        (void) gprintf(j, k, "T1=%4.1f ms", itime1 * dt);
        k += txtpar.font_height;
        (void) gprintf(j, k, "T2=%4.1f ms", itime2 * dt);
        k += txtpar.font_height;
        (void) gprintf(j, k, "Parameter #%d", curr_param->index);
    }
    else
    {
        (void) gprintf(j, k, "Level=%3.0f dB  ", test_level);
        k += txtpar.font_height;
        (void) gprintf(j, k, "T1=%4.1f ms", itime1 * dt);
        k += txtpar.font_height;
        (void) gprintf(j, k, "T2=%4.1f ms", itime2 * dt);
        k += txtpar.font_height;
        (void) gprintf(j, k, "SP=%4.1f ms", ifsp * dt);
        k += txtpar.font_height;
        (void) gprintf(j, k, "Wt.Av. = %s", weight.strs[weight_type]);
    }
    k += txtpar.font_height + 8;
    (void) gprintf(j, k, "Sets left:");
    k += txtpar.font_height;
    (void) gprintf(j, k, "%5d (%2d)", nsweeps * 2, swp1set);
    m = k + txtpar.font_height;
    (void) gprintf(j, m, "Sets accepted:");
    m += txtpar.font_height;
    
    *x = j;
    *y = k;
    return (m);
}

void
filter_it(float *lptr, int n)
{
    if (filter_type.at == 1 && (F1_cutoff > 0 || F2_cutoff > 0)) {
        if (pqm_type == QM_FSP || aqm_type == QM_FSP ) {
	    (void) decide(0, 2, "Fsp invalid with zero phase filter.",
		"ZPF will be disabled.");
            filter_type.at = 0;
        } else {
	    zpf(lptr, n, F1_cutoff / rate, F2_cutoff / rate);
        }
    }
}

void
check_stored(int c)
{
    int     i;
    float   w_a = 0, w_b = 0;

    if (!stored) {
	if (oper_ok)
	    stored = 3;
	else if (evb(EC_FA2))
	    stored = 1;
	else if (evb(EC_FB2))
	    stored = 2;
	if (stored) {
	    if (weight_type != 0 && c > 0 && twt_A > 0 && twt_B > 0) {
		w_a = twt_A / (float) c / (float) swp1set;
		w_b = twt_B / (float) c / (float) swp1set;
		for (i = 0; i < buflen; i++) {
		    accbuf_a1[i] /= w_a;
		    accbuf_a2[i] /= w_a;
		    accbuf_b1[i] /= w_b;
		    accbuf_b2[i] /= w_b;
		}
	    }
            if(t)
            {
	        tok_store(7, c, accbuf_a1);
	        tok_store(8, c, accbuf_a2);
	        tok_store(9, c, accbuf_b1);
	        tok_store(10, c, accbuf_b2);
            }
	    if (weight_type != 0 && c > 0 && twt_A > 0 && twt_B > 0) {
		for (i = 0; i < buflen; i++) {
		    accbuf_a1[i] *= w_a;
		    accbuf_a2[i] *= w_a;
		    accbuf_b1[i] *= w_b;
		    accbuf_b2[i] *= w_b;
		}
	    }
            nsets_i = c;
	}
    }
}

char *term[] = {
    "for unknown reason",
    "because operator quit",
    "because max. sweeps reached",
    "with VR2 on A & OK",
    "with VR2 on B & OK",
    "with operator OK",
    "with VR1 on A",
    "with VR2 on A",
    "with VR1 on B",
    "with VR2 on B",
};

int
check_term()
{
    if (evb(EC_OTN))
	return (1);
    if (evb(EC_NSW))
	return (2);
//    if (evb(EC_OOK) && evb(EC_FA2))     /* obselete: used for R10 */
//	return (3);
//    if (evb(EC_OOK) && evb(EC_FB2))     /* obselete: used for R10 */
//	return (4);
    if (evb(EC_OOK) && (ok_mode.at == 1))
	return (5);
    if (evb(EC_FA1) && (pqm_mode.at == 1))
	return (6);
    if (evb(EC_FA2) && (pqm_mode.at == 2))
	return (7);
    if (evb(EC_FB1) && (pqm_mode.at == 3))
	return (8);
    if (evb(EC_FB2) && (pqm_mode.at == 4))
	return (9);
    return (0);
}

void
inform_term()
{
    int     x1, y1, x2, y2;
    int     j, k;

    x1 = w_info.xtop + 1;
    y1 = w_info.ytop + txtpar.font_height * 6;
    x2 = w_info.xbot - 1;
    y2 = w_info.ybot - 1;
    gr_rectf(x1, y1, x2, y2,  scrn_c[C_INFB]);
    txtpar.text_wind_len = 60;
    text_color(scrn_c[C_INFH], scrn_c[C_INFB]);
    j = w_info.xtop + 4;
    k = w_info.ytop + txtpar.font_height * 8;
    (void) gprintf(j, k, " Terminated %s.", term[check_term()]);
}

int
chk_swp_fmt(char *key)
{
    int     f;
    static char *key_str[] = {"SWP", "SW1", "SW2", "SW3", "SW4", "SW5", "SW6"};
    static int nfmt = sizeof(key_str) / sizeof(key_str[0]);

    for (f = 0; f < nfmt && strcmp(key, key_str[f]); f++)
	continue;
    return ((f < nfmt) ? f : -1);
}

void
reverse_polarity(int alt)
{
    int i;
    
    for (i = 0; i < buflen; i++) {
        outbuf_a[i] = -outbuf_a[i];
        outbuf_b[i] = -outbuf_b[i];
    }
    result.max_a = -result.min_a;
    result.max_b = -result.min_b;
    result.min_a = -result.max_a;
    result.min_b = -result.max_b;
    sp[alt].a = -sp[alt].a;
    sp[alt].b = -sp[alt].b;
    for (i = 0; i < MAXSPTS; i++) {
        sresult.s1[i] = -sresult.s1[i];
        sresult.s1[i] = -sresult.s2[i];
    }
}

void
check_spl(double twt)
{
    double s, sm, ss, ms, dc, rms;
    int i;
    static double spl = -999;
    static double eps = 1e-20;

    if (twt > 0 && buflen > 0) {
        sm = ss = 0;
        for (i = 0; i < buflen; i++) {
	    s = accbuf_a1[i] + accbuf_a2[i];
	    sm += s;
	    ss += s * s;
        }
	dc = sm / buflen;
	ms = ss / buflen;
        rms = sqrt(ms - dc * dc) / (2 * twt * Sen.AD * Sen.MP * spl_ref);
	if (rms < eps)
	    rms = eps;
        spl = 20 * log10(rms);	// dB SPL

        text_color(w_info.wfgc, w_info.wbgc);
    } else {
        text_color(scrn_c[C_MSGH], w_info.wbgc);
    }
    response_level[1] = spl;
}

void
abr_aver(int condition)
{
    char    key[4];
    double  dt = 0, sc, scb;
    int     swpfmt = 0, compskip = 0, dflg;
    int     np, npr, nbr, c, i, sweep, m, cnt = 0, sweeps = 0, jj, kk;
    int     saved, up, set_ticks = 0, read_ticks = 0, omode = 0;
    int     alt = 0, terminate = 0;
    int     mo, da, yr, t2, t1, skip = 0;
    int     cp = 0, lp = 0, cu = 1, pu = 1, go = 0, es = 0;
    int     x = 0, y1, y2, fix_rev_swp = 0;
    int32_t    run_time, read_time = 0, swpsiz = 0, hdrsiz = 0;
    double  t0, t5, t6;
    float   smsq_a, smsq_b, v_a, v_b;
    float  *back_a, *back_b, *tp_a1, *tp_a2, *tp_b1, *tp_b2, *recbuf;
    short   data;

    top_message("ABR test     ");
    pause_collect = 0;

    if (nsweeps <= 0 || buflen <= 0)
	return;

    t2 = stim_.sig_type[0];
    t1 = stim_.sig_type[1];
    if (t2 == 5 || t2 == 6 || t1 == 5 || t1 == 6)
        skip = 1;
    ver_num = VERSION_NUM;
    weight_type = weight.at;
    aqm_type = aqm.at;
    pqm_type = pqm.at;
    swp1set = abr_.swp1set;
    upperv = (int) (rej_mv * 0.001 * Sen.AD);
    //pre_gain = (epa_pgn.at == 0) ? 1000 : 50;
    //epa_gain = epa_agn * pre_gain;

    if (r_mode.at != 2) {
        dt = def_time();
        ifsp = limit(0, (itime1 + itime2) / 2, buflen);
    }
    if (r_mode.at == 0) {
	fhd = -1;
    } else {
	if (r_mode.at == 1) {
            if (recflg == 1) {
        	strcpy(record_file, file_name);
        	newext(record_file, "swp");
            }
	    omode = (int) C_MODE;
	    if (_access(record_file, 0) == 0) {
		if (!decide(1, 2, "Do you want to overwrite existing file?",
			record_file))
		    return;
	    }
	} else {
	    omode = (int) O_MODE;
	}
	fhd = _open(record_file, omode, P_MODE);
	if (fhd == -1) {
	    (void) decide(0, 3, "Sweep file open error!", record_file,
		"ABR Test skipped.");
	    return;
	}
    	if (r_mode.at == 1) {           /* write sweeps to disk */
	    _write(fhd, "SW6", 4);
	    _write(fhd, &test_level, sizeof(float));
	    _write(fhd, &ifsp, sizeof(short));
	    _write(fhd, &buflen, sizeof(short));
	    _write(fhd, &swp1set, sizeof(short));
	    _write(fhd, &rate, sizeof(int32_t));
	    _write(fhd, &compskip, sizeof(short));
	    _write(fhd, &ep_chan, sizeof(short));
	    _write(fhd, &epa_gain, sizeof(float));
	    _write(fhd, &ad_sens, sizeof(float));
	    _write(fhd, sav_stim, buflen * sizeof(float));
	    hdrsiz = 26L + (int32_t) buflen * sizeof(float);
	} else if (r_mode.at == 2) {    /* read sweeps from disk */
	    _read(fhd, key, 4);
	    swpfmt = chk_swp_fmt(key);
	    if (swpfmt < 0) {
		(void) decide(0, 3, "Not a valid sweep file!", record_file,
		    "ABR test skipped.");
		return;
	    }
	    if (swpfmt == 0 && weight_type == 3) {
		(void) decide(0, 2, "File can't do Fsp!", "ABR test skipped.");
		return;
	    }
	    text_color(scrn_c[C_BTXF], scrn_c[C_BTXB]);
	    txtpar.text_wind_len = 60;
	    (void) gprintf(530, txtpar.font_height, record_file);
	    set_ticks = (int) (CLOCKS_PER_SEC * (swp1set * ((float) buflen) / rate));
            read_ticks = lstflg ? 0 : set_ticks;
	    _read(fhd, &test_level, sizeof(float));
	    _read(fhd, &ifsp, sizeof(short));
            hdrsiz = 4 + sizeof(float) + sizeof(short);
	    if (swpfmt >= 2) {
		_read(fhd, &buflen, sizeof(short));
		_read(fhd, &swp1set, sizeof(short));
		_read(fhd, &rate, sizeof(int32_t));
                hdrsiz += 2 * sizeof(short) + sizeof(int32_t);
	    } else {
		buflen = 2048;
		swp1set = 20;
		rate = 50000;
	    }
	    if (swpfmt >= 3) {
		_read(fhd, &compskip, sizeof(short));
                hdrsiz += sizeof(short);
		if (compskip > 0)
                    ifsp /= compskip;
	    } else {
	    	compskip = 0;
	    }
	    if (swpfmt >= 5) {
		_read(fhd, &ep_chan, sizeof(short));
		_read(fhd, &epa_gain, sizeof(float));
                hdrsiz += sizeof(short) + sizeof(float);
	    } else {
	    	ep_chan = 1 | 2;
	    	//epa_gain = 2e5;
	    	rej_mod = 0;
            }
	    if (swpfmt >= 6) {
		_read(fhd, &ad_sens, sizeof(float));
                hdrsiz += sizeof(float);
	    } else if (swpfmt == 5) {
	    	ad_sens = (float) -8302.3;  /* TB Pinnacle */
	    } else {
	    	ad_sens = (float) 3267.7;   /* Ariel DSP-16+ */
            }
	    _read(fhd, sav_stim, buflen * sizeof(float));
            hdrsiz += buflen * sizeof(float);
	}
    }
    if (r_mode.at == 2)    /* read sweeps from disk */
    {
        dt = def_time();
        ifsp = limit(0, ifsp, buflen);
        fix_rev_swp = 0;
        epa_pol.at = 1;
    } else {
        //epamp_init(epa_hpf.at, epa_acc.at, epa_pgn.at, epa_ntf.at);
        epa_pol.at = 0;
    }
    if (polar.at == epa_pol.at) {
        dflg = D_NOUP | D_GAIN;
    } else {
        dflg = D_NOUP | D_GAIN | D_FLIP;
    }
    dis_units(ad_sens, 1.0, atof(epa_gain.strs[epa_gain.at]));
    reset_count = 0;
    init_sh();
    np = init_aver();
    if (np <= 0)
	return;
    init_ai(dt);
    recbuf = (ep_chan & 1) ? outbuf_a : outbuf_b;
    npr = ((ep_chan & 1) && (ep_chan & 2)) ? np : np / 2;
    nbr = npr * sizeof(float);
    memset(outbuf_a, 0, buflen * sizeof(float));
    memset(outbuf_b, 0, buflen * sizeof(float));

    back_a = (float *) calloc(np, sizeof(float));
    back_b = back_a + buflen;
    (void) alloc_space(1);
    tp_a1 = back_a;
    tp_a2 = outbuf_a;
    tp_b1 = back_b;
    tp_b2 = outbuf_b;

    init_wt();

    if(t)
    {
        tok_init(10);
        tok_store(1, 1, (float *) sav_stim);
    }

    saved = save_w(&w_msg);
    m = message_out(&jj, &kk);

    pre_dis_ind("Artifact", "Limit");
    dis_artifact();

    y1 = w_sig.ytop;
    y2 = w_sig.ybot - txtpar.font_height;

    if (r_mode.at == 2) {
	read_time = clock();
    } else {
	(void) dsprst(TRUE);
	downldpar(2, buflen, 0, 0);
	(void) dsprst(FALSE);
    }
    run_time = clock();
    up = 0;
    sweep = nsweeps;
    w_spec.ytop = w_msg.ybot + 1;
    //initalize the waveform display
    show_A_B(1);
    //this is the main loop when ABR is running
    while (!terminate && !pgm_done()) 
    {
        //static float t5_val, t6_val;
        if ((check_timing() || up) && (sweeps > 0))
        {
            //update the display waveform...
            show_A_B(0);
	    if (cp >= 0 && cp < xrange) 
            {
                x = (int) ((double) (w_sig.xbot - w_sig.xtop - 20)
                    * ((double) cp) / xrange) + 21;
                cursor_line(x, y1, y2);
                lp = cp;
            }
	    reset_timing();
	    up = 0;
	}
	if (cu || pu)     //cursor or peak update
        {
            static double t5_val, t6_val;
	    sc = 1 /  Sen.AD / (binhdr.epa_gain);
	    scb = ((twt_B > 0) && (ep_chan & 2)) ? sc / twt_B : 0;
	    text_color(w_info.wfgc, w_info.wbgc);
	    txtpar.text_wind_len = 60;
	    i = cp + xbegin;
	    //k = w_info.ytop + txtpar.font_height * 8;

            if(cu || (cp != lp && cp >= 0 && cp < xrange) )
            {
                mouse_show(0);
	        cursor_line(x, y1, y2);
	        x = (int) ((double) (w_sig.xbot - w_sig.xtop - 20)
	            * ((double) cp) / xrange) + 21;
	        cursor_line(x, y1, y2);
                mouse_show(1);
            }
            i = cp + xbegin;
            t0 = abr_pk[0][0] * dt;
            if(abr_pk[1][5] == 0)
            {
                t5_val = 0;
                t5 = 0;
            }
            else
            {
                t5_val = (accbuf_b1[abr_pk[1][5]] * scb) + (accbuf_b2[abr_pk[1][5]] * scb);
                t5_val /= 2;
	        t5 = abr_pk[1][5] * dt - t0;
            }
            if(abr_pk[1][6] == 0)
            {
                t6_val = 0;
                t6 = 0;
            }
            else
            {
                t6_val = (accbuf_b1[abr_pk[1][6]] * scb) + (accbuf_b2[abr_pk[1][6]] * scb);
                t6_val /= 2;
	        t6 = abr_pk[1][6] * dt - t0;
            }
            //the volt values are passed as V, not nV
            dis_pos_info( ((cp + xbegin) * dt) - t0, accbuf_b1[i] * scb, t5, t5_val, t6, 
                t6_val, t0);
            lp = cp;
	    cu = 0;
            pu = 0;
        } // end of peak and cursor update
	if (r_mode.at == 2) {
	    c = read_time - clock();
	} else if (dspdget()) {
	    (void) dspdread(&data);
	    c = data;
	} else {
	    c = swp1set;
	}
	if (c <= 0) 
        {
	    if (r_mode.at == 2)     //reading data from file
            {
                memset(outbuf_a, 0, buflen * sizeof(float));
                memset(outbuf_b, 0, buflen * sizeof(float));
		if (!pause_collect) 
                {
		    if (_read(fhd, recbuf, nbr) != nbr) 
                    {
                        _close(fhd);
                        fhd = -1;
                        text_color(scrn_c[C_BTXF], scrn_c[C_BTXB]);
			txtpar.text_wind_len = 60;
                        if (lstflg && nxt_lst()) 
                        {
	    		    (void) gprintf(530, txtpar.font_height, record_file);
                            fhd = _open(record_file, omode, P_MODE);
			    (void) _lseek(fhd, hdrsiz, SEEK_SET);
    		            if (fhd != -1 && _read(fhd, recbuf, nbr) != nbr) 
                            {
                                _close(fhd);
    		                fhd = -1;
			    }
                        }
                        if (fhd == -1) 
                        {
			    (void) gprintf(530, txtpar.font_height, "END OF FILE ");
			    memset(recbuf, 0, nbr);
			    if (lstflg)
		                nswpsets = sweeps;
		            else
			        pause_collect = 1;
			}
		    }
		    c = sizeof(result);
		    if (swpfmt == 0 || _read(fhd, &result, c) != c) {
			memset(&result, 0, c);      /* No result from DSP */
		    }
		    if (swpfmt == 3) {
			_read(fhd, &sp[alt], sizeof(struct _sp));
                    } else if (compskip == 0) {
			sp[alt].a = outbuf_a[ifsp];
			sp[alt].b = outbuf_b[ifsp];
		    }
	            c = swp1set * sizeof(short);
		    if(swpfmt >= 4 && (ep_chan & 1))
	                _read(fhd, sresult.s1, c);
	            else
	                memset(sresult.s1, 0, c);
		    if(swpfmt >= 4 && (ep_chan & 2))
	                _read(fhd, sresult.s2, c);
	            else
	                memset(sresult.s2, 0, c);
		}
		if (fix_rev_swp)
		    reverse_polarity(alt);
	        read_time = clock() + read_ticks;
	    } else {
		(void) dspuarr(dsppar.accbuf_a, np, outbuf);
		(void) dspuarr(dsppar.accbuf_b, np, outbuf + np);
		(void) dspuarr(dspmnmx, 2 * 2, (short *) &result);
		(void) dspuarr(dspspts, 2 * MAXSPTS, (short *) &sresult);
		(void) dsprst(TRUE);

                if(stim_.type != 0)  //recalculate the masker with different phase
                {
		    if(! (sweeps % 2) ) {
                        downldpar(2, buflen, skip, 1);
		    } else {
                        downldpar(2, buflen, skip, 0);
		    }
                }

		(void) dsprst(FALSE);
		long2float((int32_t *) outbuf, np);
		sp[alt].a = outbuf_a[ifsp];
		sp[alt].b = outbuf_b[ifsp];
	    }
            smsq_a = smsq_b = 0;
            for (i = 0; i < swp1set; i++) {
                v_a = sresult.s1[i];
                v_b = sresult.s2[i];
                smsq_a += v_a * v_a;
                smsq_b += v_b * v_b;
            }

	    if (r_mode.at == 1) {
	        _write(fhd, recbuf, nbr);
		_write(fhd, &result, sizeof(result));
                swpsiz = nbr + sizeof(result) ;
		c = swp1set * sizeof(short);
                if (ep_chan & 1) {
		    _write(fhd, sresult.s1, c);
                    swpsiz += c;
		}
                if (ep_chan & 2) {
		    _write(fhd, sresult.s2, c);
                    swpsiz += c;
		}
	    }
	    if (!pause_collect)
		cnt++;
	    filter_it(outbuf_a, buflen);        /* A */
	    filter_it(outbuf_b, buflen);        /* B */
	    (void) draw_w(&w_spec);
	    w_dis = &w_spec;
	    sig_v.vpp = (float)  1e-6 * t_ind.lim;
	    if (sig_v.vpp < 1e-6)
		sig_v.vpp = (float) 1e-6;
            c = 1;
            //this is where we display the recieved signal
            //and alternate colors
	    if (ep_chan & 1) {
	        dfgc = alt ? scrn_c[C_CHA2] : scrn_c[C_CHA1];
	        (void) disply((float) swp1set, outbuf_a, itime2, c, 0.0, NULL, dflg);
	        c = 0;
	    }
	    if (ep_chan & 2) {
	        dfgc = alt ? scrn_c[C_CHB2] : scrn_c[C_CHB1];
	        (void) disply((float) swp1set, outbuf_b, itime2, c, 0.0, NULL, dflg);
	        c = 0;
	    }
            //check rejection
	    if (check_rej(outbuf_a)) {
		if (r_mode.at == 1) {
                    (void) _lseek(fhd, -swpsiz, SEEK_CUR);
		}
	    } else {    //if buffer accepted we do this stuff
		if (alt == 0) {
	            for (i = 0; i < buflen; i++) {
		        back_a[i] = outbuf_a[i];    //copy both buffers to backup every other set
		        back_b[i] = outbuf_b[i];
		    }
		    sqsp_A[0] = (float) smsq_a;
		    sqsp_B[0] = (float) smsq_b;
		    alt = 1;
		} else {    //we do this stuff every other set
		    sqsp_A[1] = (float) smsq_a;
		    sqsp_B[1] = (float) smsq_b;
		    if (weight_type == 0) {     //apply weight depending on type
			wt_A = 1;
			wt_B = 1;
		    } else if (weight_type == 1) {
			wt_A = (float) (1 / sqrt(var_dif(tp_a1, tp_a2)));
			wt_B = (float) (1 / sqrt(var_dif(tp_b1, tp_b2)));
		    } else if (weight_type == 2) {
			wt_A = (float) (1 / var_dif(tp_a1, tp_a2));
			wt_B = (float) (1 / var_dif(tp_b1, tp_b2));
		    } else if (weight_type == 3) {
			wt_A = (float) (1 / var_fsp(1));
			wt_B = (float) (1 / var_fsp(0));
		    } else if (weight_type == 4) {
			wt_A = (float) (1 / sqrt(var_fsp(1)));
			wt_B = (float) (1 / sqrt(var_fsp(0)));
		    }
                    for (i = 0; i < buflen; i++) {   //then finally add values to accumulator
			accbuf_a1[i] += tp_a1[i] * wt_A;
			accbuf_a2[i] += tp_a2[i] * wt_A;
			accbuf_b1[i] += tp_b1[i] * wt_B;
			accbuf_b2[i] += tp_b2[i] * wt_B;
		    }
		    twt_A += swp1set * wt_A;
		    twt_B += swp1set * wt_B;
		    sweep--;
		    sweeps = nsweeps - sweep;
		    text_color(scrn_c[C_MSGH], w_msg.wbgc);
		    (void) gprintf(jj, kk, "%5d", sweep * 2);
		    Fsp_A_B(sweeps);
		    if (aqm_type == QM_REPR || pqm_type == QM_REPR)
			repr_A_B();
		    if (aqm_type == QM_SCOR || aqm_type == QM_FMP ||
		        pqm_type == QM_SCOR || pqm_type == QM_FMP)
			Fmp_A_B(sweeps, tp_a1, tp_b1, tp_a2, tp_b2);
	            if (ep_chan & 1) {
	                Aaqm = (float) limit(0, A_qm[aqm_type], 99.9);
	                Apqm = (float) limit(0, A_qm[pqm_type], 99.9);
	            }
	            if (ep_chan & 2) {
                        Baqm = (float) limit(0, B_qm[aqm_type], 99.9);
                        Bpqm = (float) limit(0, B_qm[pqm_type], 99.9);
                    }
		    i = sweeps - 1;
		    if (i >= trklen)
		        i = trklen - 1;
		    Aaqm_trk[i] = (int) (limit(-32, Aaqm , 32) * 100);
		    Baqm_trk[i] = (int) (limit(-32, Baqm , 32) * 100);
		    Apqm_trk[i] = (int) (limit(-32, Apqm , 32) * 100);
		    Bpqm_trk[i] = (int) (limit(-32, Bpqm , 32) * 100);
		    Ansp_trk[i] = (int) (limit(-320, Ansp , 320) * 10);
		    Bnsp_trk[i] = (int) (limit(-320, Bnsp , 320) * 10);
		    Anmn_trk[i] = (int) (limit(-320, Anmn , 320) * 10);
		    Bnmn_trk[i] = (int) (limit(-320, Bnmn , 320) * 10);
		    if (sqm.at) {
			qm_info();
			Monitor_track(sweeps);
		    }
		    if (sweeps == 1)
			up = 1;
		    if (sweeps >= nsweeps)
			store_evnt(EC_NSW, run_time, sweeps, cnt);
		    if (sweeps >= min_sweeps) {
			if ((Aaqm >= aqm_crit1) && (ep_chan & 1))
			    store_evnt(EC_RA1, run_time, sweeps, cnt);
			if ((Aaqm >= aqm_crit2) && (ep_chan & 1))
			    store_evnt(EC_RA2, run_time, sweeps, cnt);
			if ((Baqm >= aqm_crit1) && (ep_chan & 2))
			    store_evnt(EC_RB1, run_time, sweeps, cnt);
			if ((Baqm >= aqm_crit2) && (ep_chan & 2))
			    store_evnt(EC_RB2, run_time, sweeps, cnt);
			if ((Apqm >= pqm_crit1) && (ep_chan & 1))
			    store_evnt(EC_FA1, run_time, sweeps, cnt);
			if ((Apqm >= pqm_crit2) && (ep_chan & 1))
			    store_evnt(EC_FA2, run_time, sweeps, cnt);
			if ((Bpqm >= pqm_crit1) && (ep_chan & 2))
			    store_evnt(EC_FB1, run_time, sweeps, cnt);
			if ((Bpqm >= pqm_crit2) && (ep_chan & 2))
			    store_evnt(EC_FB2, run_time, sweeps, cnt);
		    }
		    alt = 0;
		}
	    }
	    text_color(scrn_c[C_MSGH], w_msg.wbgc);
	    (void) gprintf(jj, m, "%d / %d", 2 * sweeps + alt, cnt);
        }  //end of if(c <= 0)
	while (check_event()) {
            int changed;
	    i = getevent();
            nswpsets = sweeps;
	    txtpar.text_wind_len = 60;
            edit_control(i, 0, &es, &up, &cp, &cu, &go, &pu, &changed);
	    switch (i) {
	    case (FN | 66):	        /* F8 */
		if (decide(1, 1, "Is this ABR waveform acceptable?")) {
		    store_evnt(EC_OOK, run_time, sweeps, cnt);
		    oper_ok = 1;
		}
		up = 1;
		break;
	    case (FN | 68):	        /* F10 */
		if (pause_collect) {
                    top_message("ABR test     ");
		    pause_collect = 0;
		} else {
		    top_message("PAUSE COLLECT");
		    pause_collect = 1;
		}
		break;
	    case ',':                   /* , */
		if (upperv >= MAXPOSINT / 20) 
		    upperv -= MAXPOSINT / 20;
		else
		    upperv = 0;
		rej_mv = (float) (upperv * 1000.0 / Sen.AD);
		break;
	    case '.':                   /* . */
		//if (upperv <= MAXPOSINT - MAXPOSINT / 20)
    		    upperv += MAXPOSINT / 20;
		//else
		//    upperv = MAXPOSINT;
		rej_mv = (float) (upperv * 1000.0 / Sen.AD);
		break;
            case '/':                   /* / to set reject limit at the current peak */
                upperv = (int)  t_ind.new;
                rej_mv = (float) (upperv * 1000.0 / Sen.AD);
                break;
	    case '`':                   /* ` */
		read_ticks = read_ticks ? 0 : set_ticks;
		break;
	    case 'X':                   /* X */
		nsweeps = 32000;
		sweep = nsweeps - sweeps;
		up = 1;
		break;
	    case 'Z':                   /* Z */
		(void) init_aver();
		(void) draw_w(&w_msg);
		m = message_out(&jj, &kk);
		sweep = nsweeps;
		sweeps = 0;
		cnt = 0;
		alt = 0;
		init_wt();
		if (fhd != -1)
		    (void) _lseek(fhd, hdrsiz, SEEK_SET);
                reset_count++;
                break;
	    }
	    if (es) {
		if (decide(1, 1, "Do you want to terminate this test?")) {
		    store_evnt(EC_OTN, run_time, sweeps, cnt);
		    series_flag = 0;
		}
	    }   //end of while(check_event())
	}   //end of main sweep loop
	terminate = check_term();
	if (!make_template)
	    check_stored(sweeps);
    }
    (void) dsprst(TRUE);
    if (fhd != -1) {
	_close(fhd);
	fhd = -1;
    }
    top_message("             ");
    pause_collect = 0;
    inform_term();
    w_spec.ytop = w_msg.ybot + 1 + txtpar.menu_height;
    gr_beep();
    if (saved)
	close_w(&w_msg);
    sweeps = nsweeps - sweep;
    if (!sweeps)
	return;
    if (weight_type != 0 && sweeps > 0 && twt_A > 0 && twt_B > 0) {
        twt_A /= (float) sweeps * swp1set;
        twt_B /= (float) sweeps * swp1set;
        for (i = 0; i < buflen; i++) {
            accbuf_a1[i] /= twt_A;
            accbuf_a2[i] /= twt_A;
	    accbuf_b1[i] /= twt_B;
	    accbuf_b2[i] /= twt_B;
	}
    }
    twt_A = twt_B = (float) sweeps * swp1set;

    if (sqm.at) {
        qm_info();
    } else if (twt_A > 0) {
	check_spl(twt_A);
    }
    show_A_B(0);

    if(t) {
        tok_store(2, sweeps, accbuf_a1);
        tok_store(3, sweeps, accbuf_a2);
        tok_store(4, sweeps, accbuf_b1);
        tok_store(5, sweeps, accbuf_b2);
        (void) twrite(fsprepro, 6, t);

        if (make_template) {
            (void) twrite(fsprepro + buflen, 7, t);
            (void) twrite(fsprepro + 2 * buflen, 8, t);
	    if (ep_chan & 1) {
  	        sum_dif(accbuf_a1, accbuf_a2, buflen, 2.0);
	        tok_store(9, sweeps, accbuf_a1);
                tok_store(10, sweeps, accbuf_a2);
	        sum_dif(accbuf_a1, accbuf_a2, buflen, 1.0);
            } else {
	        sum_dif(accbuf_b1, accbuf_b2, buflen, 2.0);
	        tok_store(9, sweeps, accbuf_b1);
                tok_store(10, sweeps, accbuf_b2);
	        sum_dif(accbuf_b1, accbuf_b2, buflen, 1.0);
            }
        }
    }
    free(back_a);

    (void) genfn("A", "ABR", file_name, &abr_count);
    get_date(&mo, &da, &yr);
    (void) sprintf(outmsg[0], "%02d-%s-%02d", da, month_name[mo], yr);
    rts = (int) ((float) (clock() - run_time) / CLOCKS_PER_SEC + 0.5);
    (void) sprintf(outmsg[2], "%d swp/set", swp1set);
    (void) sprintf(outmsg[1], "%d/%d %.0f%%", sweeps * 2, cnt,
	(sweeps * 2.0 / cnt) * 100);
    if(t) {
        t->hitch.time = rts;
        t->hitch.stab = t_stab;
        t->hitch.rejlmt = t_ind.lim;
        t->hitch.resid_vpp = 0;
        t->hitch.mode = stored;
        memcpy(t->hitch.outmsg, outmsg, 3 * 16);
        twrhitch(t);
        term_code = terminate;
        min_swps = min_sweeps;
        nswpsets = nsets_f = sweeps;
        wr_aux(t, &ai);
        tclose(t);
        t = NULL;
        //put this stuff in the conditional so no matter what we don't save a token file if 
        //we don't want to use the whole token file thingie...
        if (series_flag || decide(1, 2, "Save the file", file_name)) {
	    (void) rename(abr_tmp_name, file_name);
	    abr_count++;
	    write_counter_file();
        } else {
	    strcpy(file_name, not_saved);
            if (r_mode.at == 1 || recflg)
                write_counter_file();
        }
        out_msg(0);
    }
    
    if(binfp) {
        int q, x;
        struct BINwav bw;
        bw.numsweepscomp = twt_A;
        memcpy(&bw.param, curr_param, sizeof(struct stimParam) );
        for(q = 0; q < 2; ++q)
        {
            for(x = 0; x < 7; ++x)
                bw.abr_pk[q][x] = abr_pk[q][x];
        }
        fwrite(&bw, sizeof(struct BINwav), 1, binfp);
        fwrite(accbuf_a1, buflen * sizeof(float), 1, binfp);
        fwrite(accbuf_a2, buflen * sizeof(float), 1, binfp);
        fwrite(accbuf_b1, buflen * sizeof(float), 1, binfp);
        fwrite(accbuf_b2, buflen * sizeof(float), 1, binfp);

    }
}

void
store_evnt(int evnt_code, int32_t run_time, int swp_pr_acc, int swp_tot)
{
    if (!(ai.events & (1 << evnt_code))) {
	ai.ed[evnt_code].run_time = clock() - run_time;
	ai.ed[evnt_code].swp_acc = swp_pr_acc * 2;
	ai.ed[evnt_code].swp_tot = swp_tot;
	ai.ed[evnt_code].repr_A = Aaqm;
	ai.ed[evnt_code].repr_B = Baqm;
	ai.ed[evnt_code].Fsp_A = Apqm;
	ai.ed[evnt_code].Fsp_B = Bpqm;
	ai.events |= (1 << evnt_code);
    }
}

void
wr_aux(TOKENFILE * tfp, struct AUX_INF * pai)
{
    int i;
    tfp->header.blank1[0] = weight_type;
    tfp->header.blank1[1] = 2;	        /* AUX_INF version */
    tfp->header.blank1[2] = aqm_type;
    tfp->header.blank1[3] = nos_type;
    tfp->header.blank1[4] = ver_num;
    tfp->header.blank2[0] = pqm_type;
    tfp->header.blank2[1] = nsets_i;
    tfp->header.blank2[2] = nsets_f;
    tfp->header.blank2[3] = reset_count;
    tfp->header.blank2[4] = ep_chan;
    tfp->header.blank2[5] = pre_gain;
    tfp->header.blank2[6] = nint(10 * atof(epa_gain.strs[epa_gain.at]) / pre_gain);
    tfp->header.blank2[7] = trkbuf;
    tfp->header.blank2[8] = (short) epa_pol.at;
    pai->level = (short) test_level;
    pai->time1 = itime1;
    pai->time2 = itime2;
    pai->ifsp = ifsp;
    pai->repr1 = repr_crit1;
    pai->repr2 = repr_crit2;
    pai->VR1 = VR_crit1;
    pai->VR2 = VR_crit2;
    pai->twt_A = twt_A;
    pai->twt_B = twt_B;
    pai->tsssp_A = tsssp_A;
    pai->tsssp_B = tsssp_B;
    pai->minswps = min_swps;
    pai->check_fit_time = check_fit_run_time;
    pai->Nsp_A = Ansp;
    pai->Nsp_B = Bnsp;
    pai->Nmn_A = Anmn;
    pai->Nmn_B = Bnmn;
    for(i = 0; i < 6; ++i)
    {
	ai.pk[0][i] = abr_pk[0][i];
	ai.pk[1][i] = abr_pk[1][i];
    }
    memcpy(tfp->header.pattern + 4, pai, sizeof(*pai));
    twrheader(tfp);
}

void
rd_aux(TOKENFILE * tfp, struct AUX_INF * pai)
{
    struct AUX_INF_0 *pai0;
    int     i, ai_type;
    short r;

    trdcar(1, tfp);
    swp1set = tfp->car.reps;
    trdheader(tfp);
    memcpy(pai, tfp->header.pattern + 4, sizeof(*pai));
    weight_type = tfp->header.blank1[0];
    ai_type = tfp->header.blank1[1];
    if (ai_type == 0) {	                /* check AUX_INF version */
	pai0 = (struct AUX_INF_0 *) pai;
	for (i = 11; i >= 0; i--) {
	    pai->ed[i].run_time = (i > 6) ? 0 : pai0->ed[i].run_time;
	    pai->ed[i].swp_acc = (i > 6) ? 0 : pai0->ed[i].swp_acc;
	    pai->ed[i].swp_tot = (i > 6) ? 0 : pai0->ed[i].swp_tot;
	    pai->ed[i].repr_A = (i > 6) ? 0 : pai0->ed[i].repr_A;
	    pai->ed[i].repr_B = (i > 6) ? 0 : pai0->ed[i].repr_B;
	    pai->ed[i].Fsp_A = 0;
	    pai->ed[i].Fsp_B = 0;
	}
	pai->ifsp = 0;
	pai->repr1 = pai0->repr2;	/* reverse meaning of repr1 & repr2 */
	pai->repr2 = pai0->repr1;
	pai->VR1 = 0;
	pai->VR2 = 0;
    }
    aqm_type = tfp->header.blank1[2];
    nos_type = tfp->header.blank1[3];
    ver_num = tfp->header.blank1[4];
    pqm_type = tfp->header.blank2[0];
    if (ver_num >= 112) {
        nsets_i = tfp->header.blank2[1];
        nsets_f = tfp->header.blank2[2];
    } else {
        trdcar(7, tfp);
        r = tfp->car.reps;
        nsets_i = r / swp1set;
        trdcar(2, tfp);
        r = tfp->car.reps;
        nsets_f = r / swp1set;
    }
    reset_count = tfp->header.blank2[3];
    if (ver_num >= 117) {
        ep_chan = tfp->header.blank2[4];
    } else {
        ep_chan = 1 | 2;
        aqm_type += QM_REPR;
    }
    if (ver_num >= 125) {
        pre_gain = tfp->header.blank2[5];
        //epa_gain = (float) (pre_gain * 0.1 * tfp->header.blank2[6]);
    } else if (ver_num >= 117) {
//        epa_gain = 2e4;
    } else {
//        epa_gain = 2e5;
    }
    if (ver_num >= 119) {
        trkbuf = tfp->header.blank2[7];
    } else if (ver_num >= 117) {
        trkbuf = 2;
    } else {
        trkbuf = 1;
    }
    if (ver_num >= 124) {
        epa_pol.at = tfp->header.blank2[8];
    } else if (ver_num == 117 || ver_num == 122  || ver_num == 123) {
        epa_pol.at = 1;
    } else {
        epa_pol.at = 0;
    }
    stored = tfp->hitch.mode;
    test_level = pai->level;
    itime1 = pai->time1;
    itime2 = pai->time2;
    ifsp = pai->ifsp;
    repr_crit1 = pai->repr1;
    repr_crit2 = pai->repr2;
    VR_crit1 = pai->VR1;
    VR_crit2 = pai->VR2;
    twt_A = pai->twt_A;
    twt_B = pai->twt_B;
    tsssp_A = pai->tsssp_A;
    tsssp_B = pai->tsssp_B;
    min_swps = pai->minswps;
    check_fit_run_time = pai->check_fit_time;
    Ansp = pai->Nsp_A;
    Bnsp = pai->Nsp_B;
    Anmn = pai->Nmn_A;
    Bnmn = pai->Nmn_B;
    for(i = 0; i < 6; ++i)
    {
	abr_pk[0][i] = ai.pk[0][i];
	abr_pk[1][i] = ai.pk[1][i];
    }
    abr_pk[0][6] = abr_pk[1][6] = 0;

}

int
rd_level(TOKENFILE * tfp)
{
    int     level;

    trdheader(tfp);
    memcpy(&level, tfp->header.pattern + 4, sizeof(short));
    return (level);
}
