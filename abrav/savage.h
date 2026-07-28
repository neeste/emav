/* Defaults and Constants */
#undef TRUE
#undef FALSE
#define TRUE		(1)
#define FALSE		(0)
#define BITDIF		4
#define ESC		27
#define MAXLINE		400
#define MAXPATH		256
#define MAXNAME	        128
#define MAXTOK	        80
#define MAXNPTS		8192
#define MAXNSWP		256
#define MAXNMP		32
#define MAXSPTS		128
#define MAXDISY	        2
#define MINDISY	        1e-9
#define MAXFULLSCALE 	32000
#define RATE		25
#define MAXPOSINT 	32767
#define ADFULLSCALE 	10000.0		/* mV */
#define DAC_DELAY 	0
#define limit(min,val,max)	((val<min)?min:((val>max)?max:val))
#define pv(val)		((val<0)?0:val)

#define D_RMDC   1
#define D_GAIN   2
#define D_NOUP   4
#define D_FLIP   8

#define EC_OOK   0
#define EC_NSW   1
#define EC_RA1   2
#define EC_RB1   3
#define EC_RA2   4
#define EC_RB2   5
#define EC_OTN   6
#define EC_FA1   7
#define EC_FB1   8
#define EC_FA2   9
#define EC_FB2   10
#define evb(ec)  ((ai.events & (1 << ec)) != 0)

struct _upload {
    short   mode;
    short   npts;
    short   sweeps;
    unsigned short dac_a;
    unsigned short dac_b;
    unsigned short accbuf_a;
    unsigned short accbuf_b;
    short   greg;
    short   spidx;
    short   skip;
    short   code;
};

struct _result {
    short     max_a;
    short     min_a;
    short     max_b;
    short     min_b;
    float   fspsq_a;
    float   fspsq_b;
};

struct _sresult {
    short     s1[MAXSPTS];
    short     s2[MAXSPTS];
};

struct _lev_dep {
    short     time1;
    short     time2;
    float   VR1;
    float   VR2;
    float   repr1;
    float   repr2;
};

struct abr {
    short swp1set;
    short sets;
    short size;
    short fspt;
    short show_track;
    int32_t rate;
    short fft_xrange;
    short fft_yrange;
    short levref;
    short chk_size;
    short chk_swps;
    short cal_swps;
    float chk_atten;
    float cal_atten;
    char stim_file[42];
    char list_file[42];
    float stim_lev;
    float clk_lev;
    float sig_lev;
    float msk_lev;
    float clk_dur;
    float sig_dur;
    float msk_dur;
    float sig_freq;
    float msk_freq;
    float sig_start;
    float msk_ramp;
};

struct tone {
    int32_t    rate;
    short   size;
    short   ramp;
    short   skips;
    short   fft_xrange;
    short   fft_yrange;
    short   levref;
    float freq; 
    float level;
    float duration;
};

struct stim {
    int32_t rate;
    short size;
    short type;
    short sig_type[2];
    short sig_win[2];
    float sig_freq[2];
    float sig_level[2];
    float sig_start[2];
    float sig_dur[2];
    float sig_phase[2];
    float sig_ramp[2];
    float sig_atten[2];
};

struct stimParam
{
    float freq1;    // freq of signal (Hz)
    float freq2;    // freq of masker (Hz)
    float level1;   // level of signal (dB SPL)
    float level2;   // level of masker (dB SPL)
    float dur1;	    // duration of signal (msec)
    float dur2;	    // duration of masker (msec)
    float start;    // time where signal starts (msec)
    short sweeps;
    short index;      //index of the param in the list
    struct stimParam *next;
};

struct PATH {
    char drive[3];
    char dir[123];
    char name[23];
    char ext[9];
};

struct EVNT_DESC_0 {
    int32_t run_time;
    short swp_acc, swp_tot;
    float repr_A, repr_B;
};

struct AUX_INF_0 {
    short level, events;
    short time1, time2;
    short repr1, repr2;
    short pk[2][6];
    struct EVNT_DESC_0 ed[10];
};

struct EVNT_DESC {
    int32_t run_time;
    short swp_acc, swp_tot;
    float repr_A, repr_B;
    float Fsp_A, Fsp_B;
};

struct AUX_INF {
    short level, events;
    short time1, time2;
    short ifsp, minswps;
    short pk[2][6];
    float repr1, repr2;
    float VR1, VR2;
    struct EVNT_DESC ed[12];
    float twt_A, twt_B;
    float tsssp_A, tsssp_B;
    int32_t check_fit_time;
    float Nsp_A, Nsp_B;
    float Nmn_A, Nmn_B;
};

struct LEV_DEP {
    short     time1;
    short     time2;
    float   VR1;
    float   VR2;
    float   repr1;
    float   repr2;
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
	float MP, AD, DA[2], LS[2];
} sensitivity;

struct BINhdr 
{
    char identity[8];
    uint16_t headsize;
    uint32_t blocksize;
    uint16_t totalblocksize;
    int32_t srate;
    int32_t bufflen;
    short numchan;
    short ch1type;
    short ch2type;
    short list_type;
    short numblocks;
    short epampgain;
    char list[42];
    short blockscompleted;
    float epa_gain;
    char date[16];
    short totaltime;
    short version;
    short sweepsperset;
    PATIENT patient;
    sensitivity sens;
    struct AUX_INF ai;
};

struct BINwav
{
    struct stimParam param;
    short numsweeps;
    float numsweepscomp;
    short abr_pk[2][7];
};

typedef struct {
	float vpp, mvdc1, mvdc2;
	float rms, ppk, npk, dco;
} SIG_V;

typedef struct {
	float att1, att2;
	float intern_att;
	short att_dev;
} ATTENUATION;

typedef struct {
    short npts;
    int32_t rate;
    float *rp;
} SAV_FFT;

typedef struct {
    short beg;
    short *loc;
    char **sym;
} MARK;

typedef struct {
    short   top, bot, old, old1;
    float   new, lim;
} REJ_IND;

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

extern sensitivity Sen;
extern SIG_V sig_v;
extern ATTENUATION attenu;
extern struct stimParam *curr_param;
extern struct stimParam *head_param;
extern struct _upload dsppar;

extern char exedir[];
extern char ilstim[];
extern char o_file_name[];
extern char file_name[];
extern char record_file[];
extern char exp_comnt[];
extern char tcomment[];
extern float att1, att2;
extern float *accbuf;		/* 2K * sizeof(float) */
extern float *accbuf_1;		/* 2K * sizeof(float) */
extern float *accbuf_2;		/* 2K * sizeof(float) */
extern float *sav_stim;		/* 4K * sizeof(float) */
extern float *longptr;
extern float target_db;
extern float intern_att;
extern float spl_ref;
extern int buflen;
extern int nsweeps;
extern int upperv;
extern int itime1, itime2;
extern int sum_mode;
extern int unattended;
extern int nchan;
extern int att_dev;
extern int tfgc, tbgc;
extern int dis_flag;
extern int swp1set;
extern int limits;
extern int display_reset;
extern int xpix, ypix, dfgc;
extern int32_t rate;
extern short *fsprepro;
extern short *outbuf;			/* 4K * sizeof(int) */
extern short *stimulus;			/* 4K * sizeof(int) */
extern short abr_pk[2][7];
extern unsigned int dspprm;
extern unsigned int dspspts;
extern unsigned int dspmnmx;
