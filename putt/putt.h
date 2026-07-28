/* putt.h */

double  adjustf(double);
double  pickat(double);
double  vtodb(double);
int     calibrate_it(char *);
int     check_fit(void);
int     display(short *, int, int, int, int);
int     disply(int, float *, int, int, int);
int     dsp_init();
int     fft_sig(float *, int);
int     fft_stim(float *, int, int);
int     R_clear(void);
int32_t    adjust_rate(int32_t);
void    defpar(void);
void    dis_artifact(void);
void    dis_units(double, double);
void    downldpar(int, int, int, int);
void    get_token(int);
void    init_wind(void);
void    ld_stim(int, short *, int);
void    modulesqr(void);
void    pre_dis_ind(char *, char *);
void    read_args(void);
void    read_init_file(void);
void    show_fft(int, int, int, int, int);
void    stim_title(char *);
void    top_message(char *s);
void    write_counter_file(void);
void    zero_all(void);


/* ============================ calfile.c ================================ */

int     cal_open(char *, char *, int);
int     thev_ld_sav(char *, char *, float *, float *, float *, int);
int     thev_ld_read(char *, float *, float *, int);
int     thev_src_read(char *, float *, float *, float *, int);
int     thev_src_sav(char *, char *, float *, float *, float *, 
            int, double, double, int, float *, float *);
void    cal_info(char *, char *, char [][16], short *, short *);
void    cal_read(int);
void    cal_close(void);

/* ============================ file.c ================================ */

double  thev_cond(double);
int     open_file(char *);
int     check_cal_file(char *);
int     check_hearing_file(char *, int);
void    split_path(char *, char *, char *, char *, char *);
void	rd_cal_file();
void 	rd_hearing_file();
void    thev_adjust(double, double, double *, double *, double *);


/* ============================ sfile.c =============================== */

int	printer_ready(void);
void	set_binary(int fh);

/* ============================ print.c =============================== */

int     prn_screen_select(char *);
int     prn_setup(char *);
void    prn_screen(void);
void    set_prn_label(char *);
void    set_prn_port(char *);
void    set_prn_orient(int);
void    set_prn_title(char *);
void    set_prn_type(int);

/* =========================== hear_w.c =============================== */

void 	show_hearing_result(float, float, float, float, int);

/* ========================== probe_w.c =============================== */

int     probe_test();
void    thev_free();
