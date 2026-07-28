/* naverage.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <menu.h>
#include "savage.h"
#include "color.h"
#include "screen.h"
#include "putt.h"

int     lookahead();
void    hifilter(int32_t *, int, double);
void    lofilter(int32_t *, int, double);
void    close_text_w(void);
void    bwfilt(float *, int, double, int, int);

extern int fileflag;

float   F1_cutoff = 2000.0F, F2_cutoff = 5656.9F;
float   resid_vpp = 0;

struct {
    int     top, bot, old, old1;
    float   new, lim;
}       t_ind;

void
pre_dis_ind(char *s, char *m)
{
    int     i, c;

    w_ind.ybot = ypix - 10 - txtpar.menu_height;
    w_ind.ytop = w_ind.ybot - txtpar.menu_height;
    w_ind.ytop = w_ind.ybot;
    w_ind.ybot = ypix - 1;
    i = draw_w(&w_ind);
    c = w_ind.ytop + txtpar.font_height;
    gprintf(i, c, m);
    i = w_ind.ybot - 10;
    mouse_exclude(w_ind.xtop, i, w_ind.xbot, i);
    gr_line(w_ind.xtop, i, w_ind.xbot, i, menucolor.boxc);
    mouse_show(1);
    t_ind.top = ++i;
    t_ind.bot = w_ind.ybot - 1;
    t_ind.old = t_ind.old1 = w_ind.xtop + 1;
    t_ind.lim = (float) (upperv * 1000.0 / (Sen.MP * Sen.AD));
}

void
dis_artifact()
{
    float   mvmax;
    int     i, j, k, tfgc_s, tbgc_s;
    int32_t    range;

    mvmax = (float) (t_ind.new * 1000.0 / (Sen.MP * Sen.AD));

    rd_text_color(&tfgc_s, &tbgc_s);
    text_color(w_ind.wfgc, w_ind.wbgc);
    j = w_ind.xtop + 54;
    k = w_ind.ytop + txtpar.font_height;
    txtpar.text_wind_len = 12;
    if (t_ind.lim < 100)
	i = 1;
    else
	i = 0;
    gprintf(j, k, "%4.*f mPa", i, t_ind.lim);
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

int
check_rej(float *p)
{
    int     i, flag, beg, end;
    float   tm, m, U;

    m = 0;
    flag = 0;
    beg = 0;
    end = buflen;
    U = ((float) upperv) * swp1set;
    for (i = beg; i < end; i++) {
	tm = p[i];
	if (tm < 0)
	    tm = -tm;
	if (tm > U)
	    flag = 1;
	if (tm > m)
	    m = tm;
    }
    t_ind.new = m / swp1set;
    dis_artifact();
    return (flag);
}

int
init_aver()
{
    int     i;

    for (i = 0; i < buflen; i++) {
	accbuf_a[i] = 0;
	accbuf_b[i] = 0;
    }
    return (buflen);
}

#define NDC	8

void
take_the_aver(int len, int32_t *p)
{
    int32_t    tmp;
    int     i;

    tmp = 0;
    for (i = 0; i < NDC; i++) {
	tmp += p[i];
	tmp += p[len - NDC + i];
    }
    tmp /= (NDC * 2);
    for (i = 0; i < len; i++)
	p[i] -= tmp;
}
