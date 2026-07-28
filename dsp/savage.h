/* savage.h */
#include <stdint.h>

/* Defaults and Constants */
#define TRUE		(1)
#define FALSE		(0)
#define BITDIF		4
#define ESC		27
#define MAXNPTS		8192
#define MAXFULLSCALE 	32000
#define RATE		25
#define MAXPOSINT 	32767
#define ADFULLSCALE 	10000.0		/* mV */
#define MAXLINE 	1024
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

extern WIND *w_dis;
extern WIND w_stim;
extern WIND w_sig;
extern WIND w_ind;
extern WIND w_info;
extern WIND w_text;
extern WIND w_msg;
extern WIND w_spec;
extern WIND sub;

extern MENUITEM *submenu;
extern MENUITEM info[];
extern MENUITEM option[];
extern MENUITEM menudata[];
extern MENUITEM bwt[];

extern sensitivity Sen;
extern SIG_V sig_v;
extern ATTENUATION attenu;

extern struct _upload dsppar;

extern char exedir[];
extern char ilstim[];
extern char exp_comnt[];
extern char tcomment[];
extern float att1, att2;
extern float *accbuf;		        /* 4K * sizeof(float) */
extern float *accbuf_a;		        /* 2K * sizeof(float) */
extern float *accbuf_b;		        /* 2K * sizeof(float) */
extern float *sav_stim;		        /* 4K * sizeof(float) */
extern float *fltbuf;
extern float intern_att;
extern float target_voltage;
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
