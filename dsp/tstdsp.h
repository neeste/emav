// tstdsp.h
#include <stdint.h>

double  adjustf(double);
int     chk_ramp_tone();
int     disply(int, float *, int, int, int);
int     dsp_init();
int     do_tone();
int     fft_stim();
int     R_clear();
int     tone_opt();
double  pickat(double);
double  units_conv(double);
double  vtodb();

int dspdget();
int dspdread ();
int dspuarr ();
int dsprst ();

int32_t    adjust_rate(int32_t);
void    ld_stim(int, int, short *, int);
void    downldpar(int, int, int, int), zero_all();
void    show_fft(int, int, int, int, int);
void    text_color(int i, int n);
void    dis_units(double, double);
void    title(char *);
void    top_message(char *);
void    show_xlabel(double vb, double vr, int bv, char *u);
