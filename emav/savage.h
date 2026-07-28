/* Defaults and Constants */
#include <stdint.h>
#undef TRUE
#undef FALSE
#define TRUE		(1)
#define FALSE		(0)
#define BITDIF		4
#define ESC		27
#define NEX             11
#define NST             4
#define NAB             7
#define MAXNCAV         8
#define MINNCAV         2
#define MAXLINE		400
#define MAXPATH		256
#define MAXNAME	        128
#define MAXNNSB	        8
#define MAXTOK	        80
#define MAXSPM		6
#define MAXFULLSCALE 	32000
#define MINNPTS 	128
#define RATE		25
#define MAXPOSINT 	32767
#define DAC_DELAY 	0
#define CALFIX          2.0	// fix calibration error in v2.98
#define FILE_SAVE       file_opt[5]
#define limit(min,val,max)	((val<min)?min:((val>max)?max:val))

typedef struct {
    char Comment[70];
    char Threshold[12];
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
    float MP, AD, AT, DA[2], LS[2];
    char MPfn[80];
} sensitivity;

typedef struct {
    float att1, att2;
    float intern_att;
} ATTENUATION;

typedef struct {
    short npts;
    int32_t rate;
    float *rp;
    float *ph;
} SAV_FFT;

typedef struct {
    short nset, smax, stim[2][6], fold_stm[6], fold_res[6], res_div;
} STIM_PRES_MODE;

typedef struct {
    short   top, bot, old, old1;
    float   new, lim;
} INDICATOR;

extern PATIENT patient;
extern SIG_V sig_v;
extern sensitivity Sys, Sen;
extern ATTENUATION attenu;
extern STIM_PRES_MODE spm[MAXSPM];

struct datlnk {
    float   f2, f1, d2, d1, tm, noise, snr, f3, d3, f4, d4, p1, at;
    short   ctrl;
    struct datlnk *next;
};

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

struct _dpoae {
    float   chk_atten;
    float   cal_atten;
    float   mvmax;
    float   ramp_ms;
    float   nfdb;
    float   hpff[2];
    int32_t    rate;
    int32_t    seed;
    short   swp1set;
    short   sets;
    short   size;
    short   ramp;
    short   fft_xrange;
    short   fft_yrange;
    short   ord_min;
    short   ord_max;
    short   abs_min;
    short   abs_max;
    short   oct_min;
    short   oct_max;
    short   levref;
    short   chk_swps;
    short   cal_swps;
    short   nnsb;
    short   nfsb;
    short   skips;
    short   nic;
    short   modcyc;
    short   modper;
    short   level_unit;
    short   contav;
    char    checkfit[42];
    char    calibrate[42];
    char    file[42];
    char    ths_file[42];
};

struct _teoae {
    int32_t    rate;
    short   swp1set;
    short   sets;
    short   size;
    short   fft_xrange;
    short   fft_yrange;
    short   levref;
    short   chk_swps;
    float   chk_atten;
    float   mvmax;
    float   hpff[2];
    char    checkfit[42];
    char    stim_file[42];
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
    short   surge;
    char    checkfit[42];
    char    calibrate[42];
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
    float   hpff[2];
};

struct PATH {
    char drive[3];
    char dir[123];
    char name[9];
    char ext[5];
};

struct BINhdr {
    unsigned short total;	/* total number of waves           */
    unsigned short wav_len;	/* wave form length in points      */
    float samp_rate;		/* sample rate                     */
    float int2volt;		/* long to volt conversion scale   */
    float volt2pas;		/* volt to pascal conversion scale */
    unsigned short swp1set;	/* sweeps per set */
    unsigned short nic;		/* number of input channels */
    char reserved[12];
};

struct BINwav {
    float f2, L2;
    float f1, L1;
    unsigned short N, T;
    float f3, L3;
    unsigned short f4;
    short L4;
};

extern WIND *w_dis;
extern WIND w_stim;
extern WIND w_sig;
extern WIND w_ind;
extern WIND w_info;
extern WIND w_text;
extern WIND w_msg;
extern WIND w_spec;
extern WIND w_stim_fft;

extern MENUITEM info[];
extern MENUITEM menudata[];

extern struct _upload dsppar;

extern char exedir[];
extern char stmfn[];
extern char cali_file[];
extern char ths_file[];
extern char dpoae_fn[];
extern char o_file_name[];
extern char file_name[];
extern char exp_comnt[];
extern char tcomment[];
extern double spl_ref;
extern double target_voltage;
extern float att1, att2;
extern float intern_att;
extern float mstime1, mstime2, msramp;
extern float tmpcav;
extern float diacav;
extern float rflcav;
extern int buflen;
extern int dspnic;
extern int ntokens, tcount;
extern int acc_sets;
extern int itime1, itime2, iramp;
extern int sum_mode;
extern int dis_flag;
extern int swp1set;
extern int limits;
extern int dpoae_flag;
extern int display_reset;
extern int xpix, ypix, dfgc;
extern int numadc;
extern int numcav;
extern int numsrc;
extern int fft_xrange;
extern int fft_yrange;
extern int fft_levref;
extern int hpf_type, hpf_ord;
extern int32_t rate;
extern int32_t *accbuf;		/* 4K * sizeof(int32_t) */
extern int32_t *accbuf_a;	/* 2K * sizeof(int32_t) */
extern int32_t *accbuf_b;	/* 2K * sizeof(int32_t) */
extern int32_t *savbuf;		/* 4K * sizeof(int32_t) */
extern int32_t *swpbuf;
extern short *outbuf;		/* 4K * sizeof(int) */
extern short *stmbuf;		/* 4K * sizeof(int) */
