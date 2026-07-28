/* Defaults and Constants */
#include <stdint.h>
#undef TRUE
#undef FALSE
#define PUTT        	yes
#define TRUE		(1)
#define FALSE		(0)
#define BITDIF		4
#define ESC		27
#define MAXLINE 	1024
#define MAXPATH		256
#define MAXNAME	        128
#define MAXTOK	        80
#define MAXFULLSCALE 	32000
#define RATE		25
#define MAXPOSINT 	32767
#define ADFULLSCALE 	10000.0		/* mV */
#define DAC_DELAY 	0
#define limit(min,val,max)	((val<min)?min:((val>max)?max:val))

struct _upload {
    short   mode;
    short   npts;
    short   sweeps;
    short   skip;
    unsigned short dac_a;
    unsigned short dac_b;
    unsigned short acc;
    short   greg;
    short   scale;
    short   range;
    short   nic;
    short   code;
};

struct _probe {
    float   chk_atten;
    float   cal_atten;
    float   temp;
    float   diacav;
    float   maxlen;
    float   rejlim;
    float   minres;
    float   f1_erf;
    float   f2_erf;
    int32_t    rate;
    int32_t    seed;
    short   size;
    short   fft_xrange;
    short   fft_yrange;
    short   levref;
    short   chk_skps;
    short   chk_swps;
    short   cal_skps;
    short   cal_swps;
    short   cal_sets;
    short   niter;
    short   ncav;
    short   nsrc;
    char    checkfit[42];
    char    calibrate[42];
};

struct _hear {
    float   start;
    float   step;
    float   fin;
    float   dur;
    float   ramp_ms;
    float   f_start;
    float   f_end;
    float   min_gap_s;
    float   max_gap_s;
    float   rsp_tim_s;
    float   sd;
    float   rejlim;
    float   minfz;
    float   maxfz;
    float   minlev;
    float   maxlev;
    float   obn_max;
    short   pts_oct;
    short   max_trial;
    short   protocol;
    short   hfthr;
    char    file[42];
};

struct _tone {
    int32_t    rate;
    short   size;
    short   ramp;
    short   skips;
    short   fft_xrange;
    short   fft_yrange;
    short   levref;
    float   freq; 
    float   level;
    float   duration;
    float   ramp_ms;
};

struct PATH {
    char drive[3];
    char dir[123];
    char name[9];
    char ext[5];
};

typedef struct {
    char Comment[82];
    char Lastname[13];
    char Firstname[12];
    char Ear;
    char ID[12];
    char Dob[12];
} PATIENT;

typedef struct {
    float vpp, mvdc1, mvdc2;
    float rms, ppk, npk, dco;
} SIG_V;

typedef struct {
    float MP, AD, DA[2], LS[2];
} sensitivity;

typedef struct {
    float att1, att2;
    float intern_att;
    int att_dev;
} ATTENUATION;

typedef struct {
    int npts;
    int32_t rate;
    float *rp;
} SAV_FFT;

extern SAV_FFT A_fft, B_fft;

extern WIND *w_dis;
extern WIND w_stim;
extern WIND w_sig;
extern WIND w_ind;
extern WIND w_info;
extern WIND w_text;
extern WIND w_msg;
extern WIND w_spec;
extern WIND sub;

extern PATIENT patient;

extern MENUITEM *submenu;
extern MENUITEM info[];
extern MENUITEM option[];
extern MENUITEM menudata[];
extern MENUITEM bwt[];

extern sensitivity Sen, Sys;
extern SIG_V sig_v;
extern ATTENUATION attenu;

extern struct _upload dsppar;
extern struct _probe probe;
extern struct _hear hear;
extern struct _tone tone;

extern char exedir[];
extern char ilstim[];
extern char cali_file[], tmp_cal_fn[];
extern char o_file_name[];
extern char file_name[];
extern char exp_comnt[];
extern char tcomment[];
extern double rho_c;
extern float att1, att2;
extern float *accbuf;		        /* 4K * sizeof(float) */
extern float *accbuf_a;		        /* 2K * sizeof(float) */
extern float *accbuf_b;		        /* 2K * sizeof(float) */
extern float *sav_stim;		        /* 4K * sizeof(float) */
extern float *fltbuf;
extern float intern_att;
extern float spl_ref;
extern int buflen;
extern int nsets;
extern int upperv;
extern int sum_mode;
extern int unattended;
extern int nchan;
extern int att_dev;
extern int tfgc, tbgc;
extern int dis_flag;
extern int swp1set;
extern int display_reset;
extern int xpix, ypix, dfgc;
extern int32_t rate;
extern short *outbuf;			/* 4K * sizeof(int) */
extern short *dspbuf;			/* 4K * sizeof(int) */
extern short *stimulus;			/* 4K * sizeof(int) */
