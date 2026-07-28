/* ANSI function prototypes */
#include <stdint.h>

/* ======================= emav.c ======================= */

int32_t	adjust_rate(int32_t);
int	testexist(char *n);
int	is_delim(char c);
char *	atline(void);
int	check_list(char *s, char **lp);
int	check_starter(void);
int	check_token(int flag);
void	change_nic(int);
void	defpar();
void    get_file_name();
void	get_token(int reset);
void	init_alloc_space(void);
void	proc_batch(char *fn);
void	read_args(void);
void	read_init_file(void);
void	read_counter_file(void);
void	set_system(int c, double num);
void	set_printer(int c, double num);
void	set_teoae(int c, double num);
void	set_dpoae(int c, double num);
void	set_tone(int c, double num);
void    set_test_opt(int t1, int t2, int t3);
void    top_message(char *);
void	write_counter_file(void);

/* ========================= dpoae_w.c ========================= */

double	adjustf(double f);
int	add_data_lnk(int set);
int	cali_A(void);
int	cali_B(void);
int     chkfit_calibr(void);
int	create_list(void);
int     skip_num(char *, int);
void	DPOAE_wind(void);
void	grab_dpoae(void);
void	zero_acc(void);
void	zero_out(void);
void	zero_sav(void);
void	dis_result(float *, float *, double, double, double, int);
void	del_data_lnk(void);
void	out_msg(int seconds, char outmsg[][16]);
void	proc_hdr(void);

/* ========================= probe_w.c ========================= */

double  cavity_length(float *pc, int nf, double vs);
int	create_load(void);
int     thev_load(char *cfn, char *sfn);
int     thev_source(char *fn);
void	Cavity_wind(void);
void	Probe_wind(void);
void	grab_probe(void);
void	dis_cal_cav(int icav);
void	thev_adj_cal(float *fst, float *pst, float *fpl, float *sil, float *rfl);
void    thev_adj_free();
void    thev_adj_show(char *fn);
void    thev_load_show(char *fn, int sil);

/* ======================= teoae_w.c =========================== */

double	stability(int show, int32_t *p, int32_t *q, int num);
double	units_conv(double in);
double	vtodb(double v);
double	dbv(double v);
int	dis_stim(int load, void *p, int type, int swps, int sets);
int	dis_patient(void);
int	O_options(char *no_use);
int	I_info(char *no_use);
int	R_clear(void);
int	C_continue(char *no_use);
int	E_continue(char *no_use);
int	E_cancel(char *no_use);
int	A_auto(char *s);
int	S_set_tar(char *no_use);
int	proc_art(int i);
int	toggle_correct(char **s);
int	check_fit(void);
int	calculate_it(void);
void	TEOAE_wind(void);
void	grab_teoae(void);
void	stim_title(char *file);
void	open_stim_w(char *file);
void	init_wind(void);
void	dis_info(void);
void	dis_cali(void);
void	rd_target(char *str);
void	zero_rec(void);
void	compute_rec(int32_t *rsp, int n);

/* ======================== tone.c ==================================== */

void	TONE_wind(void);
void	grab_tone(void);

/* ======================= showdp.c =================================== */

double  pick_phase(double, int);
double  show_dp_fft(float *, int, float *, int, float *, float *, float *, 
            float *, float *, float *, int);
int 	show_dp_file(char *, int, int);
void	initial_dp(void);
void	show_dp(double, float *, int, int, int, int, int);

/* ============================ naverage.c ============================== */

void	pre_dis_ind(char *s, char *m);
void	dis_artifact(void);
void	compute_resid(int32_t *p);
int	check_rej(int32_t *p);
int	init_aver(void);
void	show_A_B(int c, int m, int nss, int chan);
int	message_out(int *x, int *y);
void	filter_it(int32_t *lptr, int n);
int	fold_it(void);
void	take_the_aver(int len, int32_t *p);
void	teoae_aver(void);

/* ============================ savage.c =============================== */

double	dpfreq(double, double);
int	open_text_w(int *i);
int	Option_wind(void);
int	check_t1_t2(void);
int	chk_long(int32_t **r);
int	chk_int(int **i);
int     chk_ramp_dpoae();
int     chk_ramp_tone();
int	chk_short(short **i);
int	chk_size(short **n);
int	dec_att(float **f);
int	dsp_init(void);
int	getstim(char *stimfn, int np);
int	high_pass(float **f);
int	set_rej(char *no_use);
int	dpindx();
void	close_text_w(void);
void    input_filter(int t, int o, float *f, int32_t rate);
void	tok_init(int ntok, int mode);
void	tok_store(int c, int32_t *p);
void	read_next_car(void);
void	write_next_car(void);

/* ============================== disadbuf.c =========================== */

int	display(short *bptr, int np, int reset, int yrange, int xrange);
int	disply(int N, int32_t *p, int np, int reset);
int	most_pos(int a, int b, int c, int d);
int32_t	most_pos_L(int32_t a, int32_t b, int32_t c, int32_t d);
void    dis_units(double, double);
void    show_xlabel(double, double, int, char *);

/* ============================= infowind.c ============================ */

int	reset_cali(void);
int	Info_wind(void);

/* =============================== event.c ============================= */

int	check_chkfit_time(void);
int	check_refresh_time(void);
void    reset_chkfit_time(void);
void    reset_refresh_time(void);
void	proc_event(int c);
void	init_graphics(void);

/* ================================ rdils.c ============================ */

int	rdils(char *, short *, int32_t, int32_t *, int32_t *);

/* ================================ dcremo.c ============================ */

int	dcremo(int32_t *a, int32_t n);

/* ================================ setatt.c ============================ */

void	setatt(int devno, int attenval);
void	senddata(int value);

/* =============================== dofft.c =============================== */

double	pickat(double f);
double  pick_min(double f1, double f2);
double  pick_max(double f1, double f2);
double  pkmn(int i1, int i2, int di, float *buf);
double  pkmx(int i1, int i2, int di, float *buf);
int	alloc_space(int create);
int	check_noise(double f, double df, double nfdb, short nfsb);
int	fft_sig(int32_t *p, int sets);
int	fft_stim(int32_t *p, int sets, int save);
int	fft_short(short *p, int div_flg);
void    mp_transfer(char *);
void    get_level(double, int, int, double, float *, float *, float *);
void	show_fft(int xrange, int yrange, int levref, int color, int reset);
void	fft_A_B(int32_t *p, int32_t *q, int addflg, int sets, int flag, int chan);
void 	modulesqr(void);
void	pick_A_B(double, float *, float *, float *, float *);
void    retrieve_fft(float *p, int n);


/* ================================ param.c =============================== */

int	TOG_axis(char **s);
int	chk_neg(float **f);
int	savethelist(void);
int	gen_param_list(void);
void	cal_conds(void);

/* =============================== repro.c ================================ */

void	repro(int32_t *a, int32_t *b, int n, float *rb);

/* ============================== rdwrdram.c ============================== */

int	num_sets(int mode);
int	recalstim(int mode);
void	downldpar(int mode, int np, int skip, int ramp, int swps);
void	dwnld_dac(int chan, int ns, double scale, int type);
void	modify(int *p);
void	ld_stim(int chan, int mode);
void	set_intern_att(double);
void	set_att(double);

/* ============================== tone.c ================================= */

int	adjust_f(void);
int	chan_alt(void);
int	chk_due(void);
int	toggle_mod(char **s);
void	dis_tone(void);
void	show_tone(void);
void	Tone_wind(void);
void	gen__stim(float f);
int	do_tone(void);

/* ============================== file.c ================================ */

int	chg_dir(char *no_use);
int	display_version(void);
int	eventintime(void);
int	extract_sfoae(void);
int	file_items(char *no_use);
int	File_wind(void);
int	open_file(char *use2chk);
int	rd_teoae_file(void);
int	rd_dpoae_file(void);
int	rd_dpswp_file(void);
int	rd_dpcal_file(void);
int	rd_probe_file(void);
int	savedir(char **s);
int	savefile(char *no_use);
int	Start_New(void);
int 	set_level_unit(char *s);
void	file_trailer(void);
void	fillinblank(char *s, int n);
void	scaleback(int tokmax, int32_t maxval, short *sbuf, int32_t *lbuf);
void	split_path(char *s, char *drv, char *dir, char *nam, char *ext);
void	trim(char *s);

/* ============================= view.c ================================ */

void	view_file(void);

/* ============================= file.c ================================ */

int     count_files(void);
int     today_file(char *);

/* ============================ sfile.c ================================ */

int	printer_ready(void);
void	set_binary(int fh);

/* ============================ print.c ================================ */

int     prn_screen_select(char *);
int     prn_setup(char *);
void    prn_screen(void);
void    set_prn_label(char *);
void    set_prn_port(char *);
void    set_prn_orient(int);
void    set_prn_title(char *);
void    set_prn_type(int);

/* ============================ sfextr.c ================================ */

int     dp2sf(char *, char *, char *);

/* ============================ calfile.c ================================ */

int     cal_read(char *, int);
int     cal_save(char *, int);
int     mptrans_open(char *, int *);
int     mptrans_read(int, float *, float *, float *);
int     thev_src_read(char *fn, 
	    float *zs1, float *ps1, float *px1,
	    float *zs2, float *ps2, float *px2);
int     thev_src_sav(char *tfn, 
	    float *zs1, float *ps1, float *px1, float *lc1, 
	    float *zs2, float *ps2, float *px2, float *lc2, 
	    int nf, int nc, double dc, double tc, double at, short sets);
int     thev_ld_sav(char *tfn, float *z0,
	    float *zl1, float *pl1, float *pr1,
	    float *zl2, float *pl2, float *pr2,
	    int nf, double at, short sets);
int	thev_ld_read(char *fn, float *z0,
	    float *zl1, float *pl1, float *pr1,
	    float *zl2, float *pl2, float *pr2,
	    int n);
void    cal_disp(char *, int);
void    cal_mesg(char *);
void    cal_update(char *, int);
