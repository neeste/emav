/*                  A Structure for managing token files                */
#include <stdint.h>

#define FNAMESIZE 42
#define COMNTSIZE 82
#define TOKENFILE struct _tokenfile

struct HEADER	{               /* word locs*/
	int16_t blank1[5];      /* 1-5   */
	int16_t blocks;         /* 6     */
	int16_t blank2[52];     /* 7-58 */
	int16_t chans;          /* 59    */
	int16_t blank3;         /* 60    */
	int16_t power10;        /* 61    */
	int16_t isf;            /* 62    */
	int16_t ftype;          /* 63    */
	int16_t stype;          /* 64    */
	char   pattern[128];    /* 65-128 */
	int16_t epoch;          /* 129   */
	int32_t cal[14];        /* 130-157 */
	char   label[196];      /* 158-255 */
	int16_t lablen;         /* 256 words */
};

struct HITCH {			    /* bytes locs */
	int16_t tokens;		    /* 1 - 2 */
	int16_t toksiz;             /* 3 - 4 */
	int16_t xtoksiz;	    /* 5 - 6 */
	int16_t curtok;		    /* 7 - 8 */
	char   intval[6];           /* 9 - 14 */
	char   delay[6];            /* 15 - 20 */
	char   dur[6];              /* 21 - 26 */
	char   comment[COMNTSIZE];  /* 27 - 108 */
	char   lastname[13];        /* 109 - 121 */
	char   firstname[12];       /* 122 - 133 */
	char   ear;		    /* 124 - 134 */
	char   patientID[12];       /* 135 - 146 */
	char   Dob[12];	            /* 147 - 158 */
	float target_level;	    /* 159 - 162 */
	float stab;                 /* 164 - 167 */
	float rejlmt;		    /* 168 - 171 */
	float resid_vpp;	    /* 172 - 175 */
	int16_t time;		    /* 176 - 179 */
	char   outmsg[3][16];       /* 180 - 227 */
	int16_t mode;		    /* 228 - 229 */
};

struct CAR {			    /* words locs*/
	int16_t tokeno;		    /* 1 */
	int32_t block;		    /* 2 - 3 */
	int32_t index;		    /* 4 - 5 */
	int16_t rep;		    /* 6 */
	int16_t reps;		    /* 7 */
	int16_t swprej;		    /* 8 */
	int16_t target;		    /* 9 */
	int32_t toksize;            /* 10 - 11 */
	int16_t atten1;		    /* 12 */
	int16_t atten2;		    /* 13 */
	int32_t dccomp;		    /* 14 - 15 */
	int32_t maxval;		    /* 16 - 17 */
	int16_t tokmax;		    /* 18 */
	int32_t toktime;            /* 19 - 20 */
	char   sname[FNAMESIZE];    /* 21 - 41 */
	char   tokcmt[COMNTSIZE];   /* 42 - 82 */
	int16_t rejt1;		    /* 83 */
	int16_t rejt2;		    /* 84 */
	int16_t rejlo;		    /* 85 */
	int16_t rejhi;		    /* 86 */
	char   tag[6];		    /* 87 - 89 */
	float scale;		    /* 90 - 91 */
	float temp;                 /* 92 - 93 */
	float diacav;		    /* 94 - 95 */
	float lencav;		    /* 96 - 97 */
	float adsen;		    /* 98 - 99 */
	float mpsen;		    /* 100 - 101 */
};

TOKENFILE {	
	short fd;		
	char fname[FNAMESIZE];	
	struct HEADER header;	
	struct HITCH hitch;	
	char htext[512-sizeof(struct HITCH)];   /* place holder */
	struct CAR car;		
	char ctext[512-sizeof(struct CAR)];     /* place holder */
	short *buf;		
	short token;		
};

/*  ANSCII function prototypes */

double getfloat(char *);
int getchoice(char *);
int getchup(char *);
int getint(char *);
int trddata(short *, int, TOKENFILE *);
int tread(short *, int, TOKENFILE *);
int twrdata(short *, int, TOKENFILE *);
int twrite(short *, int, TOKENFILE *);
int yesno(char *, char *);
int32_t getlong(char *);
TOKENFILE *tcreat(char *);
TOKENFILE *topen(char *);
void tclose(TOKENFILE *);
void tinit(TOKENFILE *);
void trdcar(int, TOKENFILE *);
void trdheader(TOKENFILE *);
void trdhitch(TOKENFILE *);
void twrcar(int, TOKENFILE *);
void twrheader(TOKENFILE *);
void twrhitch(TOKENFILE *);
void waitkp(void);
