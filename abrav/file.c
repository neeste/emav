/* file.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <token.h>
#include <time.h>
#include <dsp.h>
#include <menu.h>
#include <fftlib.h>
#include <util.h>
#include "savage.h"
#include "color.h"
#include "screen.h"
#include "version.h"
#include "abrav.h"

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif /* WIN32 */

#define limit(min,val,max)	((val<min)?min:((val>max)?max:val))
#define TICKNUM                 (CLOCKS_PER_SEC/3)

char   *dspdev();
double  dbv(double);
int     check_term(void);
int     check_list(char *, char **);
int     open_file(char *), file_items(char *);
int     testexist(char *), savedir(char **);
int     dis_stim(int load, void *p, enum TYPE type);
int     dis_patient();
int     disply(double, float *, int, int, double, MARK *, int);
int     chk_swp_fmt(char *);
int     rd_level(TOKENFILE *);
int     init_aver();
int     printer_ready(void);
int     select_param_win(char *);
int     check_bin_file(char *fn);
int     add_param_list(struct stimParam);
int     numb_params();
void    select_param(void);
void    clear_display(void);
void    close_w(WIND *);
void    get_token(int);
void    dis_artifact(), pre_dis_ind(char *, char *);
void    show_fft(int, int, int, int, int);
void    show_result(float, float, float, float, float, int);
void    rd_abr_file(), cp_swp_file(), rd_cal_file(), rd_bin_file(), initial();
void    stim_title(char *), set_binary(int), wind_color(WIND *, int, int);
void    wr_aux(TOKENFILE *, struct AUX_INF *);
void    rd_aux(TOKENFILE *, struct AUX_INF *);
void    edit_abr(), dis_events();
void    edit_bin();
void    wr_abr_log();
void    dis_info(int);
void    init_spec();
void    show_A_B(int);
void    reconstruct_monitor(int);
void    out_spectrum();
void    top_message(char *);
void    points_msg(void);
void    init_sh();
void    dis_units(double, double, double);
void    edit_control(int, int, int *, int *, int *, int *, int *, int *, int *);
void    write_counter_file();
void    take_files_bwt(int, int, char *);
void	qm_info(void);
void    open_stim_w(char *); 
void    display_param(int);
void    wr_bin_dat();
void    delete_param_list();
void    dis_pos_info(double, double, double, double, double, double, double);

extern char outmsg[3][16];
extern char *abr_tmp_name;
extern char data_file[128];
extern char tokstr[], line[], exedir[], record_file[];
extern char *term[];
extern float *accbuf_a1, *accbuf_a2, *accbuf_b1, *accbuf_b2;
extern float time1_ms, time2_ms;
extern float test_level;
extern float def_xrange, def_yrange;
extern float sh_range, sh_shift;
extern float F1_cutoff, F2_cutoff;
extern float twt_A, twt_B;
extern float sh_of[2][2];
extern int min_sweeps, min_swps;
extern int aqm_type, pqm_type;
extern int weight_type;
extern int nswpsets, rts, ifsp, reset_count;
extern int dfgc, ep_chan, ncfn;
extern int swp1set, abr_count;
extern int sh_12, sh_ab, sh_ul, sh_mk, sh_A, sh_B;
extern int stored, nsets_i, nsets_f;
extern int term_code;
extern int xrange, xbegin;
extern int trkbuf;
extern int32_t check_fit_run_time;
extern struct AUX_INF ai;
extern struct abr abr_;
extern unsigned shift_status;
extern MENUITEM file_opt[];
extern REJ_IND t_ind;
extern TOGGLETYPE weight, r_mode, monitor;
extern TOGGLETYPE xtype, dpftype, datafmt, Sbin, few, epa_gain;
extern TOKENFILE *t;
extern WIND w_stim_fft;
extern char tmpbatchfile[];
extern int calibr_flag, differ_flag;
extern struct BINhdr binhdr;
extern struct BINwav current_wav;

char   *not_saved = {"Not Saved"};

MENUITEM fft_opt[] = {
    {"Spectrum Frequency Range (kH&z) :", (char *) &abr_.fft_xrange, SHORT, 6, 0, 1, NULL},
    {"Spectrum Magnitude Range (&dB) :", (char *) &abr_.fft_yrange, SHORT, 5, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 1, NULL},
};

/* split the complete path name into drive, dir, name & ext */
void
split_path(char *s, char *drv, char *dir, char *nam, char *ext)
{
#ifdef WIN32
    _splitpath(s, drv, dir, nam, ext);
#else 
    if (strchr(s, ':')) {
	do {
	    *drv++ = *s++;
	} while (*s != ':');
    }
    *drv = '\0'; 
    if (strchr(s, '/') || strchr(s, '\\')) {
	do {
	    *dir++ = *s++;
	} while (strchr(s, '/') || strchr(s, '\\'));
    }
    *dir = '\0'; 
    while (*s && *s != '.') {
	*nam++ = *s++;
    }
    *nam = '\0'; 
    while (*s) {
	*ext++ = *s++;
    }
    *ext = '\0'; 
#endif
}

void
out_msg(int i)
{
    double  dt;
    int     j, k;
    struct PATH path;
    static char *pmsg[] = {
	"Final Wave",
	"A Fsp Pass",
	"B Fsp Pass",
	"Oper  OKed"
    };

    split_path(file_name, path.drive, path.dir, path.name, path.ext);
    file_opt[4].status = 1;
    file_opt[5].status = strcmp(file_name, not_saved);
    w_spec.ytop = w_msg.ybot + 1;
    j = draw_w(&w_spec);
    k = w_spec.ytop + txtpar.font_height;
    (void) gprintf(j, k, "Filename:");
    k += txtpar.font_height;
    (void) gprintf(j, k, " %s%s", path.name, path.ext);
    k += txtpar.font_height;
    (void) gprintf(j, k, "Test Date:");
    k += txtpar.font_height;
    (void) gprintf(j, k, " %s", outmsg[0]);
    k += txtpar.font_height;
    (void) gprintf(j, k, "Test Time:");
    k += txtpar.font_height;
    (void) gprintf(j, k, " %d sec", rts);
    k += txtpar.font_height + 2;
    (void) gprintf(j, k, pmsg[i]);

    dt = 1000.0 / rate;
    j = draw_w(&w_msg);

    if(head_param)      //this will display the parameter number if we're using a BIN file
    {
        k = w_msg.ytop + txtpar.font_height;
        (void) gprintf(j, k, "LevSig=%3.0f dB", curr_param->level1);
        k += txtpar.font_height;
        (void) gprintf(j, k, "LevMask=%3.0f dB", curr_param->level2);
        k += txtpar.font_height;
        (void) gprintf(j, k, "T1=%4.1f ms", itime1 * dt);
        k += txtpar.font_height;
        (void) gprintf(j, k, "T2=%4.1f ms", itime2 * dt);
        k += txtpar.font_height;
        (void) gprintf(j, k, "Parameter #%d", curr_param->index);
        k += txtpar.font_height + 8;
    }
    else
    {
        k = w_msg.ytop + txtpar.font_height;
        (void) gprintf(j, k, "Level=%3.0f dB", test_level);
        k += txtpar.font_height;
        (void) gprintf(j, k, "T1=%4.1f ms", itime1 * dt);
        k += txtpar.font_height;
        (void) gprintf(j, k, "T2=%4.1f ms", itime2 * dt);
        k += txtpar.font_height;
        (void) gprintf(j, k, "SP=%4.1f ms", ifsp * dt);
        k += txtpar.font_height;
        (void) gprintf(j, k, "Wt.Av. = %s", weight.strs[weight_type]);
        k += txtpar.font_height + 8;
    }
    (void) gprintf(j, k, "Accepted:");
    k += txtpar.font_height;
    (void) gprintf(j, k, " %s", outmsg[1]);
    k += txtpar.font_height;
    (void) gprintf(j, k, " %s", outmsg[2]);
}

int
Quit_wind(char *no_use)
{

    if (t != NULL)
	tclose(t);
    if (tmpbatchfile[0])
	(void) remove(tmpbatchfile);
    end_gr();
    (void) dsprst(TRUE);	/* Assert the DSP reset	 */
    pgm_exit();
    return (0);
}

char    o_file_name[MAXPATH] = {0};

static char ofiledir[MAXPATH] = {0};
static char *sf[] = { "*.ABR", "*.BIN", "*.CAL", "*.SWP"};
static void (*rd_and_show[]) () = {
    rd_abr_file, rd_bin_file, rd_cal_file, cp_swp_file
};

static TOGGLETYPE filetypes = {
    0,
    { "ABR", "BIN", "CAL", "SWP", NULL },
};

/* display the verison number and copyright info */
int
display_version()
{
    decide(0, 6, VERSION, PGM_NAME, COPYRIGHT, BTNRH, dspapi(), dspdev());
    return (0);
}

int
writelog(char *no_use)
{
    wr_abr_log();
    return (27);
}

int writedat(char *no_use)
{
    wr_bin_dat();
    return(27);
}


int
savefile(char *no_use)
{
    TOKENFILE *tfp;
    int     renflg = 0;

    if (strcmp(file_name, not_saved) == 0) {
	(void) genfn("A", "ABR", file_name, &abr_count);
	abr_count++;
	write_counter_file();
	tfp = topen(abr_tmp_name);
	renflg = 1;
    } else {
	tfp = topen(file_name);
    }

    if (tfp == NULL)
	return (0);
    memcpy(tfp->hitch.comment, &patient, sizeof(PATIENT));
    twrhitch(tfp);
    if(tfp->hitch.tokens > 1) {
	wr_aux(tfp, &ai);
	out_msg(0);
    }
    tclose(tfp);
    if (renflg)
	(void) rename(abr_tmp_name, file_name);

    (void) decide(0, 2, "File is saved in", file_name);
    return (1);
}

int
Start_New()
{
    file_opt[4].status = 0;
    file_opt[5].status = 0;
    file_name[0] = 0;
    patient.Comment[0] = 0;
    patient.Lastname[0] = 0;
    patient.Firstname[0] = 0;
    patient.ID[0] = 0;
    patient.Dob[0] = 0;
    mainmenu_exit();
    set_trailer(NULL);
    return (27);
}

MENUITEM file_opt[] = {
    {"&Open...", NULL, NONE, 0, 0, 1, file_items},
    {"&New", NULL, NONE, 0, 0, 1, Start_New},
    {"&Print Screen ( ^W )", NULL, NONE, 0, 0, 1, prn_screen_select},
    {"P&rinter Setup...", NULL, NONE, 0, 0, 1, prn_setup},
    {"&Save File", NULL, NONE, 0, 0, 0, savefile},
    {"&Write Log File", NULL, NONE, 0, 0, 0, writelog},
    {"S&elect Param", NULL, NONE, 0, 0, 0, select_param_win},
    {"Write &Dat File", NULL, NONE, 0, 0, 0, writedat},
    {"&About ABRAV... ( F1 )", NULL, NONE, 0, 0, 1, display_version},
    {"E&xit ( ^C )", NULL, NONE, 0, 0, 1, Quit_wind},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

int
toggle_type(char **s)
{
    strcpy(o_file_name, ofiledir);
    strcat(o_file_name, sf[filetypes.at]);
    return (0);
}

MENUITEM sub_file[] = {
    {"&Open File", NULL, NONE, 0, 0, 1, open_file},
    {"File &Type =", (char *) &filetypes, TOGGLE, 0, 0, 1, toggle_type},
    {"File &Name :=", o_file_name, STRING, 127, 0, 1, savedir},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

int
savedir(char **s)
{
    struct PATH path;
    int     len, i, c;

    len = i = strlen(*s);
    c = 0;
    if (i) {
	while (i--) {
	    c = (int) (*(*s + i));
	    if (c == '.' || c == '\\' || c == '/')
		break;
	}
	if (i && c == '.') {
	    i += 4;
	    if (i < len)
		*(*s + i) = 0;
	}
    }
    split_path(*s, path.drive, path.dir, path.name, path.ext);
    strcpy(ofiledir, path.drive);
    strcat(ofiledir, path.dir);
    return (0);
}

void
file_trailer()
{
    strcpy(o_file_name, ofiledir);
    strcat(o_file_name, sf[filetypes.at]);
    (void) simple_submenu(menu_step(2), txtpar.menu_height * 2, sub_file);
}

int
file_items(char *no_use)
{
    set_trailer((void (*)(void)) file_trailer);
    return (27);
}

int
Edit_wind()
{
    if (file_opt[4].status) 
    {
        if( check_bin_file(file_name) )
        {
            edit_bin();
            rd_bin_file();
        }
        else
        {
	    edit_abr();
	    rd_abr_file();
        }
    }
    return (0);
}

int
File_wind()
{
    return (simple_submenu(0, txtpar.menu_height, file_opt));
}

#define ATTRIB (_A_NORMAL | _A_RDONLY | _A_HIDDEN)

void
scaleback(int tokmax, int32_t maxval, float *out)
{
    int     i;
    float   scale;

    if (tokmax > 0)
	scale = (float) maxval / tokmax;
    else
	scale = 1.0;
    for (i = 0; i < buflen; i++)
	out[i] = (float) (scale * (float) outbuf[i]);
}

TOKENFILE *
check_abr_file(char *fn, int flag)
{
    TOKENFILE *tfp;

    tfp = topen(fn);
    if (tfp == NULL) {
	if (flag)
	    (void) decide(0, 2, "Can't open the file:", fn);
    } else {
	if (tfp->header.stype != 11111 || strcmp((char *)tfp->header.pattern, "ABR")) {
	    tclose(tfp);
	    tfp = NULL;
	if (flag)
	    (void) decide(0, 3, o_file_name, "is not a valid ABR token file.",
		"Press any key or mouse to continue.");
	}
    }
    return (tfp);
}

int 
check_bin_file(char *fn)
{
    FILE *fp;
    struct BINhdr bh;
    int rval;
    fp = fopen(fn, "rb");
    if(fp == NULL)
        rval = 0;
    else
    {
        fread(&bh, sizeof(struct BINhdr), 1, fp);
        if(!strcmp(bh.identity, "ABRAV") )
            rval = 1;
        else
            rval = 0;
        fclose(fp);
    }
    return rval;

}

void
sum_dif(float *x, float *y, int n, float s)
{
    double  u, v;
    int     i;

    for (i = 0; i < n; i++) {
	u = x[i];
	v = y[i];
	x[i] = (float) ((u + v) / s);
	y[i] = (float) ((u - v) / s);
    }
}

void
cursor_line(int x, int y1, int y2)
{
    gr_dotty(3);
    gr_line(x, y1, x, y2, 15);
    gr_dotty(0);
}

void
init_edit_msg()
{
    int     j, k;

    j = draw_w(&w_info);
    text_color(w_info.wfgc, w_info.wbgc);
    k = w_info.ytop + txtpar.font_height * 4;
    (void) gprintf(j, k, "           I     II    III     IV      V ");
    k = w_info.ytop + txtpar.font_height * 7;
    (void) gprintf(j, k, "   Lat     A1     A2     B1     B2      S");
}

void init_edit_msg2()
{
    int     j, k;

    j = draw_w(&w_info);
    text_color(w_info.wfgc, w_info.wbgc);
    k = w_info.ytop + txtpar.font_height * 4;
    (void) gprintf(j, k, "           V      V'");
    k = w_info.ytop + txtpar.font_height * 7;
    (void) gprintf(j, k, "   Lat     A1     A2     B1     B2      S");
}


void
zpf(float *x, int n, double f1, double f2)
{
    double s1, s2, wt;
    int    i, j, nf, nz;

    x[0] = (x[1] + x[n - 1]) / 2;           /* fix glitch at t=0 */
    f1 *= n;
    f2 *= n;
    nf = n / 2;
    nz = (f2 > 0) ? limit(nf / 8, (int) (f2 * 4), nf) : nf;
    faip(x, n);
    x[0] = x[1] = 0;
    for (i = 1, j = 2; i < nz; i++) {
        s1 = (f1 > 0) ? f1 / i : 0;
        s2 = (f2 > 0) ? i / f2 : 0;
        s2 *= s2;                       /* s2^2 */
        s2 *= s2;                       /* s2^4 */
        s2 *= s2;                       /* s2^8 */
        s2 *= s2;                       /* s2^16 */
        wt = 1 / (1 + s1 + s2);
        x[j++] *= (float) wt;
        x[j++] *= (float) wt;
    }
    while(j < n)
        x[j++] = 0;
    fsip(x, n);
}

double 
def_time()
{
    itime1 = nint(0.001 * time1_ms * rate);
    itime2 = nint(0.001 * time2_ms * rate);
    itime1 = limit(0, itime1, buflen);
    itime2 = limit(0, itime2, buflen);
    xrange = limit(0, (int) (0.001 * def_xrange * rate + 1), buflen);
    sh_range = def_yrange;
    sh_shift = 0;

    return (1000.0 / rate);
}

void
edit_abr()
{
    TOKENFILE *tfp;
    struct AUX_INF bi;
    double  dt, sc, sca, scb, scs;
    int     cp = 0, lp = 0, cu = 1, pu = 1, wave = 0;
    int     i, up = 1, j = 0, k, go = 0, es = 0, changed = 0;
    int     x = 0, y1, y2, terminate = 0, msg_spec = 0;
    double t0, t1, t2, t3, t4, t5;

    if (strcmp(file_name, not_saved) == 0) 
    {
        tfp = check_abr_file(abr_tmp_name, 1);
    } else 
    {
        tfp = check_abr_file(file_name, 1);
    }
    if (tfp == NULL) 
    {
	top_message("Open Error");
	while (!check_event());
	top_message("          ");
	return;
    }
    if(tfp->hitch.tokens == 1) 
    {
	tclose(tfp);
	return;
    }
    strcpy(line, outmsg[1]);
    bi = ai;
    init_sh();
    top_message("Edit");

    y1 = w_sig.ytop;
    y2 = w_sig.ybot - txtpar.font_height;
    dt = 1000.0 / rate;
    init_edit_msg();

    out_msg(wave);
    reconstruct_monitor(1);
    qm_info();

    while (!terminate) 
    {
	if (pu)    //peak update flag
        {
	    text_color(w_info.wfgc, w_info.wbgc);
	    txtpar.text_wind_len = 100;
	    j = (w_info.xtop + w_info.xbot) / 2 + 4;
	    k = w_info.ytop + txtpar.font_height * 3;
	    (void) gprintf(j, k, " t0,st=%5.2f,%5.2f ",
		abr_pk[0][0] * dt, abr_pk[1][0] * dt);
	    t0 = abr_pk[0][0] * dt,
		t1 = abr_pk[0][1] * dt - t0,
		t2 = abr_pk[0][2] * dt - t0,
		t3 = abr_pk[0][3] * dt - t0,
		t4 = abr_pk[0][4] * dt - t0,
		t5 = abr_pk[0][5] * dt - t0,
		j = w_info.xtop + 4;
	    k = w_info.ytop + txtpar.font_height * 5;
	    (void) gprintf(j, k, " %c A   %6.2f %6.2f %6.2f %6.2f %6.2f ",
		sh_ab ? ' ' : '>', pv(t1), pv(t2), pv(t3), pv(t4), pv(t5));
	    t1 = abr_pk[1][1] * dt - t0,
		t2 = abr_pk[1][2] * dt - t0,
		t3 = abr_pk[1][3] * dt - t0,
		t4 = abr_pk[1][4] * dt - t0,
		t5 = abr_pk[1][5] * dt - t0,
		k = w_info.ytop + txtpar.font_height * 6;
	    (void) gprintf(j, k, " %c B   %6.2f %6.2f %6.2f %6.2f %6.2f ",
		sh_ab ? '>' : ' ', pv(t1), pv(t2), pv(t3), pv(t4), pv(t5));
	    pu = 0;
	}
	if (up)     //update position
        {
	    show_A_B(0);
	    if (cp >= 0 && cp <= xrange) {
		x = (int) ((double) (w_sig.xbot - w_sig.xtop - 20)
		    * ((double) cp) / xrange) + 21;
		cursor_line(x, y1, y2);
		lp = cp;    //last position = current pos
	    }
	    up = 0;
	}
	if (cu) 
        {
	    sc = 1e9 /  Sen.AD / atof(epa_gain.strs[epa_gain.at]);
	    sca = ((twt_A > 0) && (ep_chan & 1)) ? sc / twt_A : 0;
	    scb = ((twt_B > 0) && (ep_chan & 2)) ? sc / twt_B : 0;
	    scs = 0.001;
	    text_color(w_info.wfgc, w_info.wbgc);
	    txtpar.text_wind_len = 60;
	    i = cp + xbegin;
	    k = w_info.ytop + txtpar.font_height * 8;
            (void) gprintf(j, k, "%6.2f %6.0f %6.0f %6.0f %6.0f %6.0f",
		i * dt,
		accbuf_a1[i] * sca, accbuf_a2[i] * sca,
		accbuf_b1[i] * scb, accbuf_b2[i] * scb,
		sav_stim[i] * scs);
	    if (cp != lp && cp >= 0 && cp < xrange) {
	    	mouse_show(0);
		cursor_line(x, y1, y2);
		x = (int) ((double) (w_sig.xbot - w_sig.xtop - 20)
		    * ((double) cp) / xrange) + 21;
		cursor_line(x, y1, y2);
	    	mouse_show(1);
		lp = cp;
	    }
	    cu = 0;
	}
	i = getevent();
	if (i == 3)
	    break;
        edit_control(i, 1, &es, &up, &cp, &cu, &go, &pu, &changed);
	if (i == (FN | 62)) {	        /* F4 */
	    if (msg_spec) {
		out_msg(wave);
		msg_spec = 0;
	    } else {
		out_spectrum();
		msg_spec = 1;
	    }
	} else if (i == (FN | 63)) {	/* F5 */
	    (void) draw_w(&w_info);
	    (void) dis_patient();
	    dis_events();
	    while (!check_event());
	    init_edit_msg();
	    qm_info();
	    pu = 1;
	    cu = 1;
	} else if (i == (FN | 64)) {	/* F6 */
	    if (tfp->hitch.tokens >= 10 && stored) {
		if (!wave) {
		    (void) tread(outbuf, 7, tfp);
		    scaleback(tfp->car.tokmax, tfp->car.maxval, accbuf_a1);
                    nswpsets = nsets_i;
                    twt_A = twt_B = (float) nswpsets * swp1set;
		    (void) tread(outbuf, 8, tfp);
		    scaleback(tfp->car.tokmax, tfp->car.maxval, accbuf_a2);
		    (void) tread(outbuf, 9, tfp);
		    scaleback(tfp->car.tokmax, tfp->car.maxval, accbuf_b1);
		    (void) tread(outbuf, 10, tfp);
		    scaleback(tfp->car.tokmax, tfp->car.maxval, accbuf_b2);
		    switch (stored) {
		    case 1:
			i = EC_FA2;
			break;
		    case 2:
			i = EC_FB2;
			break;
		    case 3:
			i = EC_OOK;
			break;
		    }
		    (void) sprintf(outmsg[1], "%d/%d %.0f%%",
			ai.ed[i].swp_acc, ai.ed[i].swp_tot,
			(float) ai.ed[i].swp_acc / ai.ed[i].swp_tot * 100);
		    rts = (int) ((float) ai.ed[i].run_time / CLOCKS_PER_SEC + 0.5);
		    wave = stored;
		} else {
		    (void) tread(outbuf, 2, tfp);
		    scaleback(tfp->car.tokmax, tfp->car.maxval, accbuf_a1);
                    nswpsets = nsets_f;
                    twt_A = twt_B = (float) nswpsets * swp1set;
		    (void) tread(outbuf, 3, tfp);
		    scaleback(tfp->car.tokmax, tfp->car.maxval, accbuf_a2);
		    (void) tread(outbuf, 4, tfp);
		    scaleback(tfp->car.tokmax, tfp->car.maxval, accbuf_b1);
		    (void) tread(outbuf, 5, tfp);
		    scaleback(tfp->car.tokmax, tfp->car.maxval, accbuf_b2);
		    strcpy(outmsg[1], line);
		    rts = tfp->hitch.time;
		    wave = 0;
		}
		if (!msg_spec)
		    out_msg(wave);
		up = 1;
		pu = 1;
		cu = 1;
	    }
	} else if (i == (FN | 87)) {  /* Shift-F4 */
	    (void) simple_submenu(150, 250, fft_opt);
	    if(msg_spec)
	        out_spectrum();
	} else if (i == 'F') {	/* F for (zero-phase) filter */
	    zpf(accbuf_a1, buflen, F1_cutoff / rate, F2_cutoff / rate);
	    zpf(accbuf_a2, buflen, F1_cutoff / rate, F2_cutoff / rate);
	    zpf(accbuf_b1, buflen, F1_cutoff / rate, F2_cutoff / rate);
	    zpf(accbuf_b2, buflen, F1_cutoff / rate, F2_cutoff / rate);
	    out_spectrum();
	    msg_spec = 1;
	    up = 1;
	}
	terminate = es;
    }
    top_message("    ");
    if (memcmp(&ai, &bi, sizeof(ai)) != 0 && strcmp(file_name, not_saved)) {
	if (decide(1, 1, "Do you want to save changes to the file?")) {
	    wr_aux(tfp, &ai);
	}
    }
    tclose(tfp);
}

void save_bin()
{
    struct BINhdr bh;
    struct BINwav bw;
    FILE *fp;
    fp = fopen(file_name, "r+b");
    fseek(fp, 0, SEEK_SET);
    fread(&bh, sizeof(struct BINhdr), 1, fp);
    //theoreticlly we should be in the right place here
    fseek(fp, sizeof(struct BINhdr) + (bh.blocksize * (curr_param->index - 1) ), SEEK_SET);
    fread(&bw, sizeof(struct BINwav), 1, fp);
    memcpy(&bw.abr_pk, abr_pk, sizeof(abr_pk) );
    fseek(fp,  sizeof(struct BINhdr) + (bh.blocksize * (curr_param->index - 1) ), SEEK_SET);
    fwrite(&bw, sizeof(struct BINwav), 1, fp);
    fclose(fp);
    //every time we make changes to a BIN file we save those changes to a DAT file
    wr_bin_dat();
}

void edit_bin()
{
    struct AUX_INF bi;
    double  dt, sc, scb;
    int     cp = 0, lp = 0, cu = 1, pu = 1, wave = 0;
    int     i, up = 1,  go = 0, es = 0, changed = 0;
    int     x = 0, y1, y2, terminate = 0, msg_spec = 0;
    double t0, t5, t6;

    if (!check_bin_file(file_name)) {
	top_message("BIN file Open Error");
	while (!check_event());
	top_message("          ");
	return;
    }

    display_param(1);    //display the first parameter, and don't show patient info
    init_sh();
    top_message("Edit BIN");

    y1 = w_sig.ytop;
    y2 = w_sig.ybot - txtpar.font_height;
    dt = 1000.0 / rate;
    //init_edit_msg2();

    out_msg(wave);
    reconstruct_monitor(1);
    //qm_info();

    while (!terminate) 
    {
	if (up)     //update position
        {
	    show_A_B(0);
	    if (cp >= 0 && cp <= xrange) {
		x = (int) ((double) (w_sig.xbot - w_sig.xtop - 20)
		    * ((double) cp) / xrange) + 21;
		cursor_line(x, y1, y2);
		lp = cp;    //last position = current pos
	    }
	    up = 0;
	}
	if (cu || pu)     //cursor update
        {
            static double t5_val, t6_val;
	    sc = 1 /  Sen.AD / (binhdr.epa_gain);
	    //sca = ((twt_A > 0) && (ep_chan & 1)) ? sc / twt_A : 0;
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
                t6_val /= 2;
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
            dis_pos_info((float) ((cp + xbegin) * dt) - t0, accbuf_b1[i] * scb, t5, t5_val, t6, 
                t6_val, t0);
		lp = cp;
	    cu = 0;
            pu = 0;
	}
	i = getevent();
	if (i == 3)
	    break;
        //flags are end signal, update position, current pos, 
        //cursor update, go to that position, and peak update
        edit_control(i, 1, &es, &up, &cp, &cu, &go, &pu, &changed);
	if (i == (FN | 62)) {	        /* F4 */
	    if (msg_spec) {
		out_msg(wave);
		msg_spec = 0;
	    } else {
		out_spectrum();
		msg_spec = 1;
	    }
	} else if (i == (FN | 63)) {	/* F5 */
	    (void) draw_w(&w_info);
	    (void) dis_patient();
	    dis_events();
	    while (!check_event());
	    init_edit_msg();
	    qm_info();
	    pu = 1;
	    cu = 1;
	} 
        else if (i == (FN | 64)) 	/* F6 */
        {
            //do something here?
        }

        else if (i == (FN | 87))   /* Shift-F4 */
        {
	    (void) simple_submenu(150, 250, fft_opt);
	    if(msg_spec)
	        out_spectrum();
	} 
        else if (i == 'F') 	/* F for (zero-phase) filter */
        {
	    zpf(accbuf_a1, buflen, F1_cutoff / rate, F2_cutoff / rate);
	    zpf(accbuf_a2, buflen, F1_cutoff / rate, F2_cutoff / rate);
	    zpf(accbuf_b1, buflen, F1_cutoff / rate, F2_cutoff / rate);
	    zpf(accbuf_b2, buflen, F1_cutoff / rate, F2_cutoff / rate);
	    out_spectrum();
	    msg_spec = 1;
	    up = 1;
	}
        else if( i == 'P')     /* P for selecting new parameter */
        {
            if(changed)
            {
	        if (decide(1, 1, "Do you want to save changes to the file?")) 
                    save_bin();
            }
            select_param();
            init_edit_msg2();
            up = pu = cu = 1;
            changed = 0;
        }
	terminate = es;
    }
    top_message("    ");
    if ((memcmp(&ai, &bi, sizeof(ai)) != 0 && strcmp(file_name, not_saved) ) && changed) 
    {
	if (decide(1, 1, "Do you want to save changes to the file?")) 
            save_bin();
    }
    delete_param_list();
    
}



void
batch_format()
{
    TOKENFILE *tfp;
    double  dt, sc = 0.001;
    int     cp = 0, wave = 0;
    int     i, j, k;
    int     x, y1, y2;
    double t0, t1, t2, t3, t4, t5;

    tfp = check_abr_file(file_name, 1);
    if (tfp == NULL) {
	return;
    }
    if(tfp->hitch.tokens == 1) {
	tclose(tfp);
	return;
    }
    text_color(scrn_c[C_BTXF], scrn_c[C_BTXB]);
    txtpar.text_wind_len = 60;
    (void) gprintf(530, txtpar.font_height, file_name);

    strcpy(line, outmsg[1]);
    init_sh();
    y1 = w_sig.ytop;
    y2 = w_sig.ybot - txtpar.font_height;
    dt = 1000.0 / rate;
    init_edit_msg();

    out_msg(wave);
    reconstruct_monitor(1);
    qm_info();

    out_spectrum();

    text_color(w_info.wfgc, w_info.wbgc);
    txtpar.text_wind_len = 100;
    j = (w_info.xtop + w_info.xbot) / 2 + 4;
    k = w_info.ytop + txtpar.font_height * 3;
    (void) gprintf(j, k, " t0,st=%5.2f,%5.2f ",
	abr_pk[0][0] * dt, abr_pk[1][0] * dt);
    t0 = abr_pk[0][0] * dt,
	t1 = abr_pk[0][1] * dt - t0,
	t2 = abr_pk[0][2] * dt - t0,
	t3 = abr_pk[0][3] * dt - t0,
	t4 = abr_pk[0][4] * dt - t0,
	t5 = abr_pk[0][5] * dt - t0,
	j = w_info.xtop + 4;
    k = w_info.ytop + txtpar.font_height * 5;
    (void) gprintf(j, k, " %c A   %6.2f %6.2f %6.2f %6.2f %6.2f ",
	sh_ab ? ' ' : '>', pv(t1), pv(t2), pv(t3), pv(t4), pv(t5));
    t1 = abr_pk[1][1] * dt - t0,
	t2 = abr_pk[1][2] * dt - t0,
	t3 = abr_pk[1][3] * dt - t0,
	t4 = abr_pk[1][4] * dt - t0,
	t5 = abr_pk[1][5] * dt - t0,
	k = w_info.ytop + txtpar.font_height * 6;
    (void) gprintf(j, k, " %c B   %6.2f %6.2f %6.2f %6.2f %6.2f ",
	sh_ab ? '>' : ' ', pv(t1), pv(t2), pv(t3), pv(t4), pv(t5));

    text_color(w_info.wfgc, w_info.wbgc);
    txtpar.text_wind_len = 60;
    i = cp + xbegin;
    k = w_info.ytop + txtpar.font_height * 8;
    (void) gprintf(j, k, "%6.2f %6.0f %6.0f %6.0f %6.0f %6.0f",
	i * dt,
	accbuf_a1[i] * sc, accbuf_a2[i] * sc,
	accbuf_b1[i] * sc, accbuf_b2[i] * sc,
	sav_stim[i] * sc);
    show_A_B(0);
    if (cp >= 0 && cp < xrange) {
	x = (int) ((double) (w_sig.xbot - w_sig.xtop - 20)
	    * ((double) cp) / xrange) + 21;
	cursor_line(x, y1, y2);
    }

    tclose(tfp);
}

void
wr_abr_log()
{
    char    lfn[82], *qm;
    double  dt;
    float   c = 0;
    int     i;
    FILE   *lfp;
    struct PATH path;
    static char *evs[12][2] = {
	{"- Oper. did not accept ABR",
	    "+ Oper. accepted ABR"},
	{"- Max. sweeps not reached",
	    "+ Max. sweeps reached."},
	{"- %s >= %.2f not reached on chan. A ",
	    "+ %s >= %.2f reached on chan. A "},
	{"- %s >= %.2f not reached on chan. B ",
	    "+ %s >= %.2f reached on chan. B "},
	{"- %s >= %.2f not reached on chan. A ",
	    "+ %s >= %.2f reached on chan. A "},
	{"- %s >= %.2f not reached on chan. B ",
	    "+ %s >= %.2f reached on chan. B "},
	{"- Oper. did not force termination ",
	    "+ Oper. forced termination "},
	{"- %s >= %.2f not reached on chan. A ",
	    "+ %s >= %.2f reached on chan. A "},
	{"- %s >= %.2f not reached on chan. B ",
	    "+ %s >= %.2f reached on chan. B "},
	{"- %s >= %.2f not reached on chan. A ",
	    "+ %s >= %.2f reached on chan. A "},
	{"- %s >= %.2f not reached on chan. B ",
	    "+ %s >= %.2f reached on chan. B "},
	{"- ", "+ "},
    };
    static char *aqm_s2[] = {"Repr", "SNR", "Fmp", "Fxx"};
    static char *pqm_s2[] = {"Fsp", "Scor"};

    strcpy(lfn, file_name);
    newext(lfn, "log");
    lfp = fopen(lfn, "w");
    if (lfp == NULL) {
	(void) decide(0, 2, "Can't open log file", "Press any key continue");
	return;
    }
    fprintf(lfp, ";LastName:  %s\n;FirstName: %s\n",
	patient.Lastname, patient.Firstname);
    fprintf(lfp, ";Ear: %5s\n;ID: %s\n", patient.Ear == 'R' ?
	"Right" : (patient.Ear == 'L' ? "Left" : "?"), patient.ID);
    fprintf(lfp, ";DoB: %s\n", patient.Dob);
    fprintf(lfp, ";Comment: %s\n", patient.Comment);
    fprintf(lfp, "\n");

    dt = 1000.0 / rate;
    split_path(file_name, path.drive, path.dir, path.name, path.ext);
    fprintf(lfp, "Filename:  %s%s\n", path.name, path.ext);
    fprintf(lfp, "Test Date:  %s,  ", outmsg[0]);
    fprintf(lfp, "Check Fit Time: %d sec\n",
	(int) ((float) check_fit_run_time / CLOCKS_PER_SEC + 0.5));
    fprintf(lfp, "Test Time:  %d sec,  ", rts);
    fprintf(lfp, "reset count = %d\n", reset_count);
    fprintf(lfp, "Level=%3.0f dB, ", test_level);
    fprintf(lfp, "T1=%4.1f ms, ", itime1 * dt);
    fprintf(lfp, "T2=%4.1f ms, ", itime2 * dt);
    fprintf(lfp, "SP=%4.1f, ", ifsp * dt);
    fprintf(lfp, "Min.Swps.=%d, ", min_swps);
    fprintf(lfp, "Wt.Av. = %s\n", weight.strs[weight_type]);
    fprintf(lfp, "Nsp_A = %4.1f dBV  Nsp_B = %4.1f dBV  ", ai.Nsp_A, ai.Nsp_B);
    fprintf(lfp, "Nmn_A = %4.1f dBV  Nmn_B = %4.1f dBV\n", ai.Nmn_A, ai.Nmn_B);
    fprintf(lfp, "Accepted:  %s %s \n", outmsg[1], outmsg[2]);
    fprintf(lfp, "Terminated %s.\n", term[check_term()]);

    for (i = 0; i < 12; i++) {
	if (i == EC_RA1 || i == EC_RB1) {
	    c = ai.repr1;
	} else if (i == EC_RA2 || i == EC_RB2) {
	    c = ai.repr2;
	} else if (i == EC_FA1 || i == EC_FB1) {
	    c = ai.VR1;
	} else if (i == EC_FA2 || i == EC_FB2) {
	    c = ai.VR2;
	}
        if (i >= 2 && i <= 5) {
            qm = aqm_s2[aqm_type];
        } else if (i >= 7 && i <= 10) {
            qm = pqm_s2[pqm_type];
        } else {
            qm = "";
        }
	fprintf(lfp, evs[i][evb(i) ? 1 : 0], qm, c);
	fprintf(lfp, "\n");
	if (evb(i)) {
	    fprintf(lfp, "  run_time=%4d s", (uint32_t) (ai.ed[i].run_time / CLOCKS_PER_SEC));
	    fprintf(lfp, ", swp_acc=%4d", ai.ed[i].swp_acc);
	    fprintf(lfp, ", swp_tot=%4d", ai.ed[i].swp_tot);
	    fprintf(lfp, "\n");
            qm = aqm_s2[aqm_type];
	    fprintf(lfp, "  %s(A)=%5.2f", qm, ai.ed[i].repr_A);
	    fprintf(lfp, ", %s(B)=%5.2f", qm, ai.ed[i].repr_B);
            qm = pqm_s2[pqm_type];
	    fprintf(lfp, ", %s(A)=%5.2f", qm, ai.ed[i].Fsp_A);
	    fprintf(lfp, ", %s(B)=%5.2f", qm, ai.ed[i].Fsp_B);
	    fprintf(lfp, "\n"); 
	}
    }
    fclose(lfp);
}

void 
wr_bin_dat()
{
    char datfn[82];
    FILE * datfp;
    struct stimParam *my_param;
    struct stimParam *old_param;
    float zerolat, fivelat, sixlat, tmp;
    strcpy(datfn, file_name);
    newext(datfn, "dat");
    my_param = head_param;
    old_param = curr_param;
    curr_param = my_param;
    tmp = 1 / (float) rate;
    datfp = fopen(datfn, "w");
    if (datfp == NULL) 
    {
	(void) decide(0, 2, "Can't open dat file for writing", "Press any key continue");
	return;
    }
    if(my_param == NULL)
    {
        (void) decide(0, 2, "No parameters available for writing", "Press any key to continue");
        return;
    }
    fprintf(datfp, ";ABRAV\n;");
    fprintf(datfp, VERSION);
    fprintf(datfp, "\n");
    fprintf(datfp, ";LastName: %s\n", binhdr.patient.Lastname);
    fprintf(datfp, ";FirstName: %s\n", binhdr.patient.Firstname);
    fprintf(datfp, ";Ear: %s\n", patient.Ear == 'R' ?
	"Right" : (patient.Ear == 'L' ? "Left" : "?") );
    fprintf(datfp, ";ID: %s\n", binhdr.patient.ID);
    fprintf(datfp, ";D.O.B: %s\n", binhdr.patient.Dob);
    fprintf(datfp, ";Comment: %s\n", binhdr.patient.Comment);
    fprintf(datfp, ";Date: %s\n", binhdr.date);
    fprintf(datfp, ";List File: %s\n", binhdr.list);
    fprintf(datfp, ";Parameters: %d\n;\n", numb_params() );
    fprintf(datfp, ";Stimulus_Type=%d\n", binhdr.list_type);
    //fprintf(datfp, ";Signal_Phase=%d\n", binhdr.
    if(binhdr.list_type == 1)   //simutanius masking was used
    { 
       fprintf(datfp, ";  Fs    Ls  Dur_s   Fm     Lm    sweeps   LatV   LatV'           V             V'\n");
    }
    else if(binhdr.list_type == 2)
    {
       fprintf(datfp, ";  Fs    Ls  Dur_s  Start  Fm     Lm  Dur_m  sweeps   LatV   LatV'        V           V'\n");
    }
    while(my_param != NULL)
    {
        display_param(curr_param->index);     //we load current parameter into memory
        zerolat = (tmp * abr_pk[1][0]) * 1000;
        fivelat = tmp * abr_pk[1][5] * 1000;
        sixlat = tmp * abr_pk[1][6] * 1000;

        if(binhdr.list_type == 1)   //simutanius masking was used
        {
            fprintf(datfp, "%5.1f  %3.1f   %2d   %5.1f  %3.1f  %4d    %5.2f   %5.2f%16.3f%16.3f\n", 
                my_param->freq1, my_param->level1, (int) my_param->dur1, my_param->freq2, 
                my_param->level2, my_param->sweeps, (fivelat - zerolat), (sixlat - zerolat), 
                accbuf_b1[abr_pk[1][5]], accbuf_b1[abr_pk[1][6]]);
        }
        else if(binhdr.list_type == 2)      //forward masking used...
        {
            fprintf(datfp, "%5.1f  %3.1f   %2d   %2d   %5.1f  %3.1f   %2d  %4d    %5.2f   %5.2f%16.3f%16.3f\n", 
                my_param->freq1, my_param->level1, (int) my_param->dur1,
		(int) my_param->start,
		my_param->freq2, my_param->level2, (int) my_param->dur2,
		my_param->sweeps,
		(fivelat - zerolat), (sixlat - zerolat), 
                accbuf_b1[abr_pk[1][5]], accbuf_b1[abr_pk[1][6]]);
        }
        my_param = curr_param = my_param->next;
    }
    curr_param = old_param;
    display_param(old_param->index);
    fprintf(datfp, ";Filename: %s\n", datfn);
    fprintf(datfp, ";Time: %d\n", binhdr.totaltime);

    fclose(datfp);
}

void
dis_events()
{
    float   s;
    int     i, k;

    text_color(w_info.wfgc, w_info.wbgc);
    txtpar.text_wind_len = (w_info.xbot - w_info.xtop + 1) / 8 - 1;
    i = w_info.xtop + 4;
    k = w_info.ytop + txtpar.font_height * 4;
    (void) gprintf(i, k, "Terminated %s.", term[check_term()]);
    k = w_info.ytop + txtpar.font_height * 5;
    if (!evb(EC_OOK)) {
	(void) gprintf(i, k, "- No oper. O.K.");
    } else {
	s = (float) ai.ed[EC_OOK].run_time / CLOCKS_PER_SEC;
	(void) gprintf(i, k, "+ Oper. O.K. at %.0f s", s);
    }
    k = w_info.ytop + txtpar.font_height * 6;
    if (!evb(EC_NSW)) {
	(void) gprintf(i, k, "- Not max. sweeps");
    } else {
	s = (float) ai.ed[EC_NSW].run_time / CLOCKS_PER_SEC;
	(void) gprintf(i, k, "+ Max. sweeps at %.0f s", s);
    }
    k = w_info.ytop + txtpar.font_height * 7;
#ifdef NEVER
    if (!evb(EC_RA2)) {
	(void) gprintf(i, k, "- Repr. %.0f%% not reached on A", ai.repr2 * 100);
    } else {
	(void) gprintf(i, k, "+  Repr. %.0f%% reached on A at %.0f s",
	    ai.repr2 * 100, ai.ed[EC_RA2].run_time / CLOCKS_PER_SEC);
    }
    k = w_info.ytop + txtpar.font_height * 8;
    if (!evb(EC_RB2)) {
	(void) gprintf(i, k, "- Repr. %.0f%% not reached on B", ai.repr2 * 100);
    } else {
	(void) gprintf(i, k, "+  Repr. %.0f%% reached on B at %.0f s",
	    ai.repr2 * 100, ai.ed[EC_RB2].run_time / CLOCKS_PER_SEC);
    }
#endif
}

void
cp_swp_file()
{
    r_mode.at = 2;
    strcpy(record_file, o_file_name);
}

void
set_dis_units_car(TOKENFILE *tfp)
{
    double ad_sen, mp_sen;
    struct CAR *c = &tfp->car;

    ad_sen = (MAXPOSINT / 10.0);
    mp_sen = 1;
    if ((fabs(c->adsen) > 1e3) && (fabs(c->adsen) < 1e9) && 
        (fabs(c->mpsen) > 1e-6) && (fabs(c->mpsen) < 1e6)) {
        ad_sen = c->adsen;
        mp_sen = c->mpsen;
    } else if (c->tokmax > 0) {
        mp_sen = ((double) c->maxval / c->tokmax) /
            (c->reps * c->scale * ad_sen);
    }
    dis_units(ad_sen, mp_sen, atof(epa_gain.strs[epa_gain.at]));
}

void
rd_abr_file()
{
    TOKENFILE *tfp;

    tfp = check_abr_file(o_file_name, 1);
    if (tfp == NULL)
	return;
    (void) draw_w(&w_info);
    rate = (int32_t) (tfp->header.isf
	* pow(10.0, (double) tfp->header.power10));
    target_db = tfp->hitch.target_level;
    memcpy(&patient, tfp->hitch.comment, sizeof(PATIENT));
    strcpy(file_name, o_file_name);
    buflen = (int) tfp->hitch.toksiz;
    (void) def_time();
    rd_aux(tfp, &ai);
    (void) init_aver();
    init_sh();
    (void) tread(outbuf, 2, tfp);
    attenu.att1 = (float) tfp->car.atten1 / 100;
    attenu.att2 = (float) tfp->car.atten2 / 100;
    scaleback(tfp->car.tokmax, tfp->car.maxval, accbuf_a1);
    if (nsets_f > 0) {
	(void) tread(outbuf, 3, tfp);
	scaleback(tfp->car.tokmax, tfp->car.maxval, accbuf_a2);
	if (tfp->hitch.tokens >= 5) {
	    (void) tread(outbuf, 4, tfp);
	    scaleback(tfp->car.tokmax, tfp->car.maxval, accbuf_b1);
	    (void) tread(outbuf, 5, tfp);
	    scaleback(tfp->car.tokmax, tfp->car.maxval, accbuf_b2);
	} else {
	    memset(accbuf_b1, 0, buflen * sizeof(float));
	    memset(accbuf_b2, 0, buflen * sizeof(float));
	}
        memset(fsprepro, 0, (2 * MAXNPTS) * sizeof(short));
	if (tfp->hitch.tokens >= 6) {
	    (void) tread(fsprepro, 6, tfp);
	    if (tfp->hitch.tokens >= 7 && trkbuf >= 2)
	        (void) tread(fsprepro + buflen, 7, tfp);
	    if (tfp->hitch.tokens >= 8 && trkbuf >= 3)
	        (void) tread(fsprepro + 2 * buflen, 8, tfp);
	}
	(void) tread(outbuf, 1, tfp);
        set_dis_units_car(tfp);
	stim_title(tfp->car.sname);
	scaleback(tfp->car.tokmax, tfp->car.maxval, sav_stim);
	nswpsets = nsets_f;
	twt_A = twt_B = ((float) nswpsets) * swp1set;
	show_A_B(1);
	t_ind.lim = tfp->hitch.rejlmt;
	pre_dis_ind("Artifact", "Limit");
	dis_artifact();
    }
    memcpy(outmsg, tfp->hitch.outmsg, 3 * 16);
    rts = tfp->hitch.time;
    out_msg(0);
    (void) dis_patient();
    dis_events();
    tclose(tfp);
}

void
rd_cal_file()
{
    TOKENFILE *tfp;
    int     n, t;
    float   twt = 0;

    tfp = check_abr_file(o_file_name, 1);
    if (tfp == NULL)
	return;
    rate = (int32_t) (tfp->header.isf
	* pow(10.0, (double) tfp->header.power10));
    n = (int) (0.005 * rate);
    target_db = tfp->hitch.target_level;
    memcpy(&patient, tfp->hitch.comment, sizeof(PATIENT));
    strcpy(file_name, o_file_name);
    buflen = (int) tfp->hitch.toksiz;
    t = itime1;
    itime1 = buflen;
    if (tfp->hitch.tokens > 0) {
	(void) tread(outbuf, 1, tfp);
        set_dis_units_car(tfp);
        attenu.att1 = (float) tfp->car.atten1 / 100;
        attenu.att2 = (float) tfp->car.atten2 / 100;
        if (tfp->car.reps > 0) {
	    swp1set = tfp->car.reps;
	    nswpsets = 1;
	    twt_A = twt_B = (float) tfp->car.reps;
	    twt = (float) tfp->car.reps * 2;
	    scaleback(tfp->car.tokmax, tfp->car.maxval, accbuf_1);  //convert the saved stim from 16 bit to 32 bit
	    open_stim_w((char *)tfp->car.sname);
	    (void) draw_w(&w_info);     //clear the info window
	    clear_display();
	    dfgc = scrn_c[C_CHNA];
	    (void) disply(twt, accbuf_1, n, 1, 0.0, NULL, 0);    //display the stimulus in it's window
	    dis_info(0);                //display information window
	    init_spec();
	    (void) draw_w(&w_msg);
	    (void) draw_w(&w_spec);
	    fft_stim(accbuf_1, 1, 0);
	    show_fft(abr_.fft_xrange, abr_.fft_yrange, abr_.levref, dfgc, 1);
	    t_ind.lim = tfp->hitch.rejlmt;
	    pre_dis_ind("Artifact", "Limit");
	    dis_artifact();
	}
    }
    if (tfp->hitch.tokens > 1) {
	(void) tread(outbuf, 2, tfp);
        if (tfp->car.reps > 0) {
	    scaleback(tfp->car.tokmax, tfp->car.maxval, accbuf_2);  //convert the saved stim from 16 bit to 32 bit
	    dfgc = scrn_c[C_CHNB];
	    (void) disply(twt, accbuf_2, n, 1, 0.0, NULL, 0);    //display the stimulus in it's window
	    fft_stim(accbuf_2, 1, 0);
	    show_fft(abr_.fft_xrange, abr_.fft_yrange, abr_.levref, dfgc, 0);
	}
    }
    itime1 = t;
    tclose(tfp);
}

void 
rd_bin_file()
{
    struct BINwav bw;
    FILE   *fpt;
    int q, x, i, read;

    //initalizes buffers to make sure the pointers are correct
    init_aver();    
    init_sh();
    strcpy(file_name, o_file_name);
    fpt = fopen(o_file_name, "rb");
    if(fpt == NULL)
        return;
    fread(&binhdr, sizeof(struct BINhdr), 1, fpt);
    memcpy(&patient, &binhdr.patient, sizeof(PATIENT) );
    memcpy(&ai, &binhdr.ai, sizeof(struct AUX_INF) );
    memcpy(&Sen, &binhdr.sens, sizeof(sensitivity) );
    strcpy(outmsg[0], binhdr.date);  //copy date to message holder
    rate = binhdr.srate;
    buflen = binhdr.bufflen;
    xrange = buflen;
    rts = binhdr.totaltime;
    abr_.swp1set = binhdr.sweepsperset;
    strcpy(binhdr.date, outmsg[0]);
    dis_units(Sen.AD, Sen.MP , (double) binhdr.epa_gain);
    delete_param_list();
    for(i = 0; i < binhdr.blockscompleted; ++i) {
        fseek(fpt, sizeof(binhdr) + (binhdr.blocksize * i), SEEK_SET);
        read = fread(&bw, sizeof(struct BINwav), 1, fpt);
        if(read <= 0)    {
            (void) decide(0, 1, "ERROR reading bin file, currupt or outdated file type!");
            delete_param_list();
            return;
        } else {
            add_param_list(bw.param);
	}
    }
    curr_param = head_param;
    fseek(fpt, sizeof(binhdr) , SEEK_SET); //move file pointer to next header
    fread(&current_wav, sizeof(struct BINwav), 1, fpt);
    for(q = 0; q < 2; ++q)  //copy the pk info over
    {
        for(x = 0; x < 7; ++x)
            abr_pk[q][x] = bw.abr_pk[q][x];
    }
    //fseek(fpt, bh.headsize + sizeof(struct BINwav), 0); //should be at accbuf_a1
    fread(accbuf_a1, sizeof(float) * buflen, 1, fpt);   //copy all waveform data to the buffers
    fread(accbuf_a2, sizeof(float) * buflen, 1, fpt);
    fread(accbuf_b1, sizeof(float) * buflen, 1, fpt);
    fread(accbuf_b2, sizeof(float) * buflen, 1, fpt);
    fclose(fpt);    //close the file now that we're done with it
    out_msg(0);
    twt_A = twt_B = bw.numsweepscomp;
    (void) dis_patient();
    dis_events();
    //this actually puts the waveform to the screen, 0 indicates we don't display stim
    show_A_B(0);
    file_opt[6].status = 1;
    file_opt[7].status = 1;

}


void 
display_param(int param)
{
    struct BINhdr bh;
    struct BINwav bw;
    int x, q;
    FILE   *fpt;
    fpt = fopen(o_file_name, "rb");
    fread(&bh, sizeof(struct BINhdr), 1, fpt);
    fseek(fpt, sizeof(bh) + (bh.blocksize * (param - 1) ), SEEK_SET);
    fread(&bw, sizeof(struct BINwav), 1, fpt);

    for(q = 0; q < 2; ++q)  //copy the pk info over
    {
        for(x = 0; x < 7; ++x)
            abr_pk[q][x] = bw.abr_pk[q][x];
    }
    //fseek(fpt, bh.headsize + sizeof(struct BINwav), 0);     //we should be at accbuf_a1
    fread(accbuf_a1, sizeof(float) * buflen, 1, fpt);   //copy all waveform data to the buffers
    fread(accbuf_a2, sizeof(float) * buflen, 1, fpt);
    fread(accbuf_b1, sizeof(float) * buflen, 1, fpt);
    fread(accbuf_b2, sizeof(float) * buflen, 1, fpt);
    fclose(fpt);    //close the file now that we're done with it
    out_msg(0);
    twt_A = twt_B = bw.numsweepscomp;
    (void) dis_patient();
    dis_events();
    //this actually puts the waveform to the screen, 0 indicates we don't display stim
    show_A_B(0);
    file_opt[6].status = 1;
}

void select_param()
{
    WIND    w;
    static int nrow = 10, ncfn = 55;
    int x, j, i, saved = 0, loopflg = 1, mx, my, tmp;
    int curr, start, end, num_params, accept = -1;
    char temp[256];
    w.xtop = menu_step(1);
    w.xbot = w.xtop + ncfn * 8 + 8;
    w.ytop = 3 * txtpar.menu_height;
    w.ybot = w.ytop + (nrow +1) * txtpar.font_height + 1;
    wind_color(&w, menucolor.afg, menucolor.abg);
    saved = save_w(&w);
    x = draw_w(&w) + 8;
    j = w.ytop + txtpar.font_height;
    //hold_j = j;
    start = curr = 1;
    num_params = numb_params();
    end = 10;//limit(1, nrow, num_params);

    do {
        //make sure we don't have an empty param list...
        if(head_param)
        {
            curr_param = head_param;
            j = w.ytop + txtpar.font_height;     //make sure the y axis text starts at the top
            text_color(menucolor.afg, menucolor.abg);
            if(binhdr.list_type == 1)
                (void) gprintf(x, j, "   Parm#  Fsig    Lsig  Dsig  Fmask   Lmask  Sweeps");
            else
                (void) gprintf(x, j, "Parm#  Fsig    Lsig  Dsig  Fmask   Lmask Dmask  Sweeps");
            j += txtpar.font_height;
            for(i = 1; curr_param; i++)
            {
                if(i == curr)
                    text_color(menucolor.mfg, menucolor.mbg);
                else
                    text_color(menucolor.afg, menucolor.abg);
                if(i <= end && i >= start)  //only display up to 10 parameters
                {
                    if(binhdr.list_type == 1)   //type one is sim masking
                        sprintf(temp, "%5d%10.2f%7.2f%4d%9.2f%8.2f%7d   ", curr_param->index, curr_param->freq1, curr_param->level1,
                            (int) curr_param->dur1, curr_param->freq2, curr_param->level2, curr_param->sweeps);
                    else    //other is the forward masking
                        sprintf(temp, "%2d%10.2f%7.2f%4d%9.2f%8.2f%5d%7d ", curr_param->index, curr_param->freq1, curr_param->level1,
                            (int) curr_param->dur1, curr_param->freq2, curr_param->level2, (int) curr_param->dur2, curr_param->sweeps);                    gprintf(x, j, temp);
                    j += txtpar.font_height;
                }
                curr_param = curr_param->next;
            }
            curr_param = head_param;
        }
        else
            gprintf(x, j, "ERROR: could not read parameter list");

    	tmp = getevent();
        switch(tmp)
        {

	case SINGLE_CLICK:
	    mouse_position(&mx, &my);
            //make sure the click was within the window
	    if (mx > w.xtop && mx < w.xbot && my > w.ytop && my < w.ybot) 
            {
                int stuff;
                stuff = start + (my - w.ytop) / txtpar.font_height;
                stuff--;
		if (stuff <= end) 
                {
                    if(curr == stuff)
                    {
                        loopflg = 0;
		        accept = stuff;
                    }
		    curr = stuff;
		}
	    }
            //otherwise we set the flag to exit the loop
            else 
		loopflg = 0;
            break;
	case 13:		/* return */
            loopflg = 0;
            accept = curr;
	    break;
	case FN | 72:		/* arrow up */
	    if (curr > start) {
		curr--;
	    } 
            else if(curr == 1)
            {
                //eh...we do nothing here
            }
            else
            {
		start = limit(0, start - 1, num_params - 1);
		end = limit(1, start + nrow - 1, num_params);
		curr = start;
	    }
	    break;
	case FN | 80:		/* arrow down */
	    if (curr < end) 
            {
		curr++;
	    }
            else if(curr == num_params)
            {
                //yep, do nothing here again
            }
            else 
            {
		end = limit(1, end + 1, num_params);
		start = limit(0, end - nrow + 1, num_params);
		curr = end;
	    }
	    break;
	case FN | 75:		/* arrow left */
	    break;
	case FN | 77:		/* arrow right */
	    break;
	case FN | 73:		/* page up */
	    if (curr > start) 
            {
		curr = start;
	    } 
            else 
            {
		start = limit(0, start - nrow, num_params);
		end = limit(1, start + nrow, num_params);
		curr = start;
	    }

            break;
	case FN | 117:		/* control end */
            if(end != num_params)
            {
                end = num_params;
	        start = limit(0, end - nrow, num_params);
	        curr = end;
            }
            break;
	case FN | 81:		/* page down */
            if (end == num_params) 
            {
		curr = end;
	    } 
            else 
            {
		end = limit(1, end + nrow, num_params);
		start = limit(0, end - nrow, num_params);
		curr = end;
	    }
	    break;
	case FN | 119:		/* control home */
	    start = curr = 1;
	    end = limit(1, start + nrow, num_params);
	    break;
	case FN | 71:		/* home */
	    curr = start;
	    break;
	case FN | 79:		/* end */
	    curr = end;
	    break;

        }
    } while(loopflg);

    if(saved)
        close_w(&w);

    if(accept > 0)
    {
        curr_param = head_param;
        while(curr_param->next)
        {
            if(accept == curr_param->index)
                break;
            curr_param = curr_param->next;
        }
        display_param(accept);
    }

}

int select_param_win(char *nope)
{
    set_trailer((void (*)(void)) select_param);
    return (27);
}

int
eventintime()
{
    int32_t    timer;
    int     rv;

    rv = 0;
    timer = clock() + TICKNUM;
    while (clock() < timer) {
	rv = check_event();
	if (rv)
	    break;
    }
    return (rv);
}

int
open_file(char *use2chk)
{
    WIND    w;
    TOKENFILE *tfp;
    FILE   *fpt;
    int     saved, i, j, k, nf, tmp, acpt, end, rc, x, y, cw;
    int     xx = 0, yy = 0, hold, status, fmt = 0, mx, my;
    int     f_buflen, f_swp1set;
    int32_t    f_rate;
    float   lv;
    PATIENT P = {"", "", "", 0, "", ""};
    char   *files, *beg;
    static int nrow = 15;
    static int start = 0, ptr = 0;
    static char lastfn[MAXPATH] = {0};

    file_opt[0].inputlen = 0;
    //this looks for the number of files in the directory with the same extention
    nf = count_files();
    //we want to return if there aren't any files with that extention
    if (nf == 0) {
	(void) decide(0, 2, "Warning, File not found!",
	    "Press any key or mouse to continue");
	return ('N');
    }
    if (strcmp(lastfn, o_file_name)) {
	ptr = start = 0;
        strcpy(lastfn, o_file_name);
    }
    end = limit(1, start + nrow, nf);
    files = (char *) malloc((ncfn + 1) * (end - start));
    if (files == NULL) {
	(void) decide(0, 2, "Error allocating space for filename holder",
	    "Press any key or mouse to continue");
	return (0);
    }
    take_files_bwt(start, end, files);
//    k = nrow;
    cw = txtpar.font_width;
    w.xtop = menu_step(2);
    w.xbot = w.xtop + (ncfn + 3) * cw;
    w.ytop = 6 * txtpar.menu_height;
    w.ybot = w.ytop + nrow * txtpar.font_height;
    wind_color(&w, menucolor.afg, menucolor.abg);
    saved = save_w(&w);
    x = draw_w(&w) + cw;
    j = w.ytop + txtpar.font_height;
    hold = 0;
    if (use2chk != NULL) {
	xx = w.xbot;
	yy = w.ybot;
	w.xtop = xx + 20;
	w.xbot = w.xtop + 28 * cw;
	w.ybot = w.ytop + 5 * txtpar.font_height;
	wind_color(&w, menucolor.mfg, menucolor.mbg);
	hold = save_w(&w);
	(void) draw_w(&w);
	w.xbot = xx;
	w.ybot = yy;
	xx = w.xtop + 4;
	yy = j;
	w.xtop = menu_step(2);
    }
    do {        //here we just display a list of available files with same exention
	for (k = start; k < end; k++) {
	    if (k == ptr)
		text_color(menucolor.mfg, menucolor.mbg);
	    else
		text_color(menucolor.afg, menucolor.abg);
	    tmp = k - start;
	    y = j + tmp * txtpar.font_height;
	    txtpar.text_wind_len = ncfn + 1;
	    (void) gprintf(x, y, files + tmp * (ncfn + 1));
	}
	if (use2chk != NULL && !eventintime()) {
	    status = 0;
	    text_color(menucolor.mfg, menucolor.mbg);
	    strcpy(line, ofiledir);
	    strcat(line, files + (ptr - start) * (ncfn + 1));
	    trim(line);
            //once we select a file we switch and open the file, get patient info, etc...
	    if (filetypes.at == 3)      //this is for the SWP flie type
            {
		fpt = fopen(line, "rb");
		if (fpt == NULL)
		    status = 1;
		else {
		    strcpy(tokstr, line);
		    fread(line, 4, 1, fpt);
		    line[4] = 0;
		    fread(&lv, sizeof(float), 1, fpt);
		    fread(&i, sizeof(short), 1, fpt);
		    fread(&f_buflen, sizeof(short), 1, fpt);
		    fread(&f_swp1set, sizeof(short), 1, fpt);
		    fread(&f_rate, sizeof(int32_t), 1, fpt);
		    fmt = chk_swp_fmt(line);
		    status = (fmt < 0) ? 4 : 5;
		    fclose(fpt);
		}
	    }
            else if(filetypes.at == 1)      //this if for the new BIN file type
            {
                if(check_bin_file(line))
                {
                    struct BINhdr bh;
                    fpt = fopen(line, "rb");
                    if(fpt == NULL)
                        status = 1;
                    else
                    {
                        status = 3;
                        fread(&bh, sizeof(struct BINhdr), 1, fpt);
                        memcpy(&P, &bh.patient, sizeof(PATIENT) );
                        fclose(fpt);
                    }
                }
                else
                    status = 6;
            }
            else      //this is for the ABR token file type
            {
		tfp = topen(line);
		if (tfp == NULL)
		    status = 1;
		else if (tfp->header.stype != 11111 || strcmp((char *)tfp->header.pattern, "ABR")) 
                {
		    tclose(tfp);
		    status = 2;
		} else {
		    status = 3;
		    memcpy(&P, tfp->hitch.comment, sizeof(PATIENT));
		    //level = rd_level(tfp);
		    tclose(tfp);
		}
	    }

	    beg = (char *) line;
	    for (i = 0; i < 5; i++) {
		beg[0] = 0;
		fillinblank(beg, 26);
		beg += 27;
	    }
	    switch (status) {
	    case 1:
		sprintf(line, "%-26s", "Can't open the file.");
		break;
	    case 2:
		sprintf(line, "%-26s", "Not a valid ABR file.");
		break;
	    case 3:
		beg = (char *) line;
		sprintf(beg, "Last Name : %-14s", P.Lastname);
		beg += 27;
		sprintf(beg, "First Name: %-14s", P.Firstname);
		beg += 27;
		sprintf(beg, "Ear: %c, ID: %-14s", P.Ear, P.ID);
		//beg += 27;
		//sprintf(beg, "Test Level: %d dB", level);
		beg += 27;
		sprintf(beg, "Comment: %-17s", P.Comment);
		break;
	    case 4:
		sprintf(line, "%s", "Not a valid SWP file.");
		break;
	    case 5:
		beg = (char *) line;
		sprintf(beg, "SW%d record file.        ", fmt);
		beg += 27;
		sprintf(beg, "Level: %.1f dB", lv);
		beg += 27;
		if (fmt > 1) {
		    sprintf(beg, "SP at %d", i);
		    beg += 27;
		    if (fmt > 2) {
			sprintf(beg, "Len: %d, Swp/set: %d",
			    f_buflen, f_swp1set);
			beg += 27;
			sprintf(beg, "Rate: %d Hz    ", f_rate);
			beg += 27;
		    }
		}
		break;
            case 6:
                sprintf(line, "%s", "Not a valid BIN file.");
	    }
	    k = yy;
	    beg = (char *) line;
            txtpar.text_wind_len = 27;
	    for (i = 0; i < 5; i++) {
		(void) gprintf(xx, k, "%s", beg);
		beg += 27;
		k += txtpar.font_height;
	    }
	}
	acpt = 0;
	tmp = getevent();
	if (tmp == 3)
	    return (0);
	switch (tmp) {
	case 27:
	    acpt = -1;
	    break;
	case 9:
	    acpt = -2;
	    break;
	case SINGLE_CLICK:
	    mouse_position(&mx, &my);
	    if (mx > w.xtop && mx < w.xbot && my > w.ytop &&
		my < w.ybot) {
		tmp = start + (my - w.ytop) / txtpar.font_height;
		if (tmp < end) {
		    acpt = (ptr == tmp);
		    ptr = tmp;
		}
	    } else {
		acpt = -1;
	    }
	    break;
	case 13:		/* return */
	    acpt = 1;
	    break;
	case FN | 72:		/* arrow up */
	    if (ptr > start) {
		ptr--;
	    } else {
		start = limit(0, start - 1, nf - 1);
		end = limit(1, start + nrow, nf);
		ptr = start;
		take_files_bwt(start, end, files);
	    }
	    break;
	case FN | 80:		/* arrow down */
	    if (ptr < end - 1) {
		ptr++;
	    } else {
		end = limit(1, end + 1, nf);
		start = limit(0, end - nrow, nf - 1);
		ptr = end - 1;
		take_files_bwt(start, end, files);
	    }
	    break;
	case FN | 75:		/* arrow left */
	    break;
	case FN | 77:		/* arrow right */
	    break;
	case FN | 73:		/* page up */
	    if (ptr > start) {
		ptr = start;
	    } else {
		start = limit(0, start - nrow, nf - 1);
		end = limit(1, start + nrow, nf);
		ptr = start;
		take_files_bwt(start, end, files);
	    }
	    break;
	case FN | 117:		/* control end */
	    end = nf;
	    start = limit(0, end - nrow, nf);
	    ptr = end - 1;
	    take_files_bwt(start, end, files);
	    break;
	case FN | 81:		/* page down */
	    if (ptr < end - 1) {
		ptr = end - 1;
	    } else {
		end = limit(1, end + nrow, nf);
		start = limit(0, end - nrow, nf - 1);
		ptr = end - 1;
		take_files_bwt(start, end, files);
	    }
	    break;
	case FN | 119:		/* control home */
	    start = ptr = 0;
	    end = limit(1, start + nrow, nf);
	    take_files_bwt(start, end, files);
	    break;
	case FN | 71:		/* home */
	    ptr = start;
	    break;
	case FN | 79:		/* end */
	    ptr = end - 1;
	    break;
	}
    } while (acpt == 0);

    if (acpt > 0) {
	strcpy(o_file_name, ofiledir);
	strcat(o_file_name, files + (ptr - start) * (ncfn + 1));
	trim(o_file_name);
        //ah-ha! here is where we set which function to be called when we want to
        //read the file and display it!
	if (use2chk != NULL)
	    set_trailer((void (*)(void)) rd_and_show[filetypes.at]);
	rc = 27;
    } else if (acpt == -2) {
	rc = 'N';
    } else {
	rc = 0;
    }
    if (hold) {
	yy = w.xtop;
	w.xtop = xx - 4;
	close_w(&w);
	w.xtop = yy;
    }
    if (saved)
	close_w(&w);
    free(files);
    return (rc);
}
