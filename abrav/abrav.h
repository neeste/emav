/* abrav.h */

/* ============================ abr_w.c ================================ */
double  vtodb(double), dbv(double);
double  units_conv(double);
void    fft_A_B(float *p, float *q, int sets, int addflg, int flag);

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

/* ============================== disadbuf.c =========================== */
void    dis_units(double, double, double);
void    show_xlabel(double, double, int, char *);

/* ============================== dofft.c =========================== */
float   pickat(float f, int nnsb), calnswp(float tm);
int     fft_stim(float *, int, int);
void    show_fft(int, int, int, int, int);

/* ============================== savage.c =========================== */
int     dsp_init(void);
