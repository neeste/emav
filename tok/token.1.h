/*                  A Structure for managing token files                */

#define FNAMESIZE 42
#define COMNTSIZE 82
#define TOKENFILE struct _tokenfile

struct HEADER	{		/* word locs*/
	short blank1[5];		/* 1-5   */
	short blocks;		/* 6     */
	short blank2[52];		/* 7-58 */
	short chans;		/* 59    */
	short blank3;		/* 60    */
	short power10;		/* 61    */
	short isf;		/* 62    */
	short ftype;		/* 63    */
	short stype;		/* 64    */
	char pattern[128];	/* 65-128 */
	short epoch;		/* 129   */
	long cal[14];		/* 130-157 */
	char label[196];	/* 158-255 */
	short lablen;		/* 256 words */
};

struct HITCH {			/* bytes locs */
	short tokens;		/* 1 - 2 */
	short toksiz;           /* 3 - 4 */
	short xtoksiz;	        /* 5 - 6 */
	short curtok;		/* 7 - 8 */
	char intval[6];		/* 9 - 14 */
	char delay[6];		/* 15 - 20 */
	char dur[6];		/* 21 - 26 */
	char comment[COMNTSIZE];/* 27 - 108 */
	char lastname[13];	/* 109 - 121 */
	char firstname[12];	/* 122 - 133 */
	char ear;		/* 124 - 134 */
	char patientID[12];	/* 135 - 146 */
	char Dob[12];		/* 147 - 158 */
	float target_level;	/* 159 - 162 */
	float stab;		/* 164 - 167 */
	float rejlmt;		/* 168 - 171 */
	float resid_vpp;	/* 172 - 175 */
	short time;		/* 176 - 179 */
	char outmsg[3][16];	/* 180 - 227 */
	short mode;		/* 228 - 229 */
};

struct CAR {			/* words locs*/
	short tokeno;		/* 1 */
	long block;		/* 2 - 3 */
	long index;		/* 4 - 5 */
	short rep;		/* 6 */
	short reps;		/* 7 */
	short swprej;		/* 8 */
	short target;		/* 9 */
	long toksize;           /* 10 - 11 */
	short atten1;		/* 12 */
	short atten2;		/* 13 */
	long dccomp;		/* 14 - 15 */
	long maxval;		/* 16 - 17 */
	short tokmax;		/* 18 */
	long toktime;		/* 19 - 20 */
	char sname[FNAMESIZE];	/* 21 - 41 */
	char tokcmt[COMNTSIZE];	/* 42 - 82 */
	short rejt1;		/* 83 */
	short rejt2;		/* 84 */
	short rejlo;		/* 85 */
	short rejhi;		/* 86 */
	char tag[6];		/* 87 - 89 */
	float scale;		/* 90 - 91 */
	float temp;		/* 92 - 93 */
	float diacav;		/* 94 - 95 */
	float lencav;		/* 96 - 97 */
	float adsen;		/* 98 - 99 */
	float mpsen;		/* 100 - 101 */
};

TOKENFILE {	
	short fd;		
	char fname[FNAMESIZE];	
	struct HEADER header;	
	struct HITCH hitch;	
	char htext[512-sizeof(struct HITCH)];		/* place holder */
	struct CAR car;		
	char ctext[512-sizeof(struct CAR)];		/* place holder */
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
long getlong(char *);
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
void waitkp();
