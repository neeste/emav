/* tokmat.c  - TOK to MAT file converter */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <token.h>

#define MAXNPTS	    8192
#define MSGSZ       16*3
#define MAXPOSINT   32767

#ifndef WIN32
#include <unistd.h>
#define _access access
#define _close close
#define _open open
#define _write write 
#endif /* WIN32 */
#ifndef O_BINARY
#define O_BINARY  0
#endif
#define PMODE       (S_IREAD|S_IWRITE)
#define OFLAG       (O_RDWR|O_CREAT|O_BINARY)

typedef struct {
    char Comment[82];
    char Lastname[13];
    char Firstname[12];
    char Ear;
    char ID[12];
    char Dob[12];
} PATIENT;

static double cal_ppc = 1, cal_vpc = 1;
static float accbuf[MAXNPTS];
static int32_t rate = 0;
static short iflg = 0;
static short ftyp = 0;
static short npts = 0;
static short ntok = 0;
static short swp1set = 0;
static short outbuf[MAXNPTS];
static TOKENFILE *tfp = NULL;

/* give a new extension to a filename */
void
newext(char *f, char *e)
{
    while (*f != '.' && *f != '\0')
	f++;
    *f++ = '.';
    while (*e != '\0')
	*f++ = *e++;
    *f++ = '\0';
}

static int
tok_open(char *fn, char *msg)
{
    int i;
    static char *calstr[] = {"TEOAE"};
    static short nstr = sizeof(calstr) / sizeof(calstr[0]);
    
    tfp = topen(fn);
    if (tfp == NULL) {
	strcpy(msg, "Can't open the file");
	return (0);
    } else if (tfp->header.stype != 11111) {
	strcpy(msg, "Not a valid TOKEN file");
	tclose(tfp);
	return (0);
    }
    ftyp = 0;
    for (i = 0; i < nstr; i++)
        if (strcmp(tfp->header.pattern, calstr[i]) == 0)
            ftyp = i + 1;
    if (ftyp == 0) {
	strcpy(msg, "Unknown TOKEN file type");
	tclose(tfp);
	return (0);
    }
    npts = (int) tfp->hitch.toksiz;
    rate = (int32_t) (tfp->header.isf
        * pow(10.0, (double) tfp->header.power10));
    ntok = tfp->hitch.tokens;

    return (1);
}

static void
scaleback(int tokmax, int32_t maxval, short *sbuf, float *fbuf)
{
    int     i;
    float   scale;

    if (tokmax > 0)
	scale = (float) maxval / tokmax;
    else
	scale = 1.0;
    for (i = 0; i < npts; i++)
	fbuf[i] = scale * sbuf[i];
}

static void
tok_read(int tn)
{
    double ad_sen = (MAXPOSINT / 10.0), mp_sen = 1;
    struct CAR *c = &tfp->car;

    tread(outbuf, tn, tfp);
    swp1set = c->reps * 2;
    scaleback(c->tokmax, c->maxval, outbuf, accbuf);
    if ((c->adsen > 1e3) && (c->adsen < 1e9) && 
        (c->mpsen > 1e-6) && (c->mpsen < 1e6)) {
        ad_sen = c->adsen;
        mp_sen = c->mpsen;
    } else if (c->tokmax > 0) {
        mp_sen = ((double) c->maxval / c->tokmax) /
            (c->reps * c->scale * ad_sen);
    }
    cal_vpc = 1 / ad_sen;           /* V/cnt  */
    cal_ppc = cal_vpc / mp_sen;     /* Pa/cnt */
}

/********************************* MAT **************************************/

/*............................ MAT WRITE ................................*/

#define NT          64

static int bsiz[5] = {8, 4, 4, 2, 4};

/*
 *  matlab header -
 *	type = MOPT where:
 *	M=0 for pc; 1 for sun
 *	O=0 col or 1 row;
 *	P= [0,1,2,3,4]=>[r*8,r*4,I*4,I*2,I*4 unsigned integers]
 *	T= 0 for matrix or 1 for text (stored as r*4 numbers 0<i<255).
 */
static int32_t
encode_mopt(int p, int t)
{
    int     a = 1;
    char   *b = (char *) (&a);

    return (b[1] * 1000 + p * 10 + t);    
}

static void
mat_wr(int fd, char *nam, void *p, int n, int dtyp, int txt, int cx)
{
    int     sz;
    int32_t    ns, hdr[5];

    ns = strlen(nam) + 1;
    hdr[0] = encode_mopt(dtyp, txt);
    hdr[1] = n; 		/* rows */
    hdr[2] = 1;			/* cols */
    hdr[3] = cx;		/* 0 for real, 1 for complex */
    hdr[4] = ns;	        /* name length (including null byte) */
    sz = bsiz[dtyp];
    if (cx)
        sz *= 2;
    _write(fd, hdr, 20);
    _write(fd, nam, (int) ns);
    _write(fd, p, n * sz);
}

static void
mat_wr1_s(int fd, char *nam, short p)
{
    mat_wr(fd, nam, &p, 1, 3, 0, 0);
}

/****************************************************************************/

static void
mat_wr1_f(int fd, char *nam, float p)
{
    mat_wr(fd, nam, &p, 1, 1, 0, 0);
}

static void
mat_wr_f(int fd, char *nam, float *p, int n)
{
    mat_wr(fd, nam, p, n, 1, 0, 0);
}

/****************************************************************************/

void
tok_mat(char *fn)
{
    char   s[9];
    int i, mfd;

    newext(fn, "mat");
    mfd = _open(fn, OFLAG, PMODE);    /* open the file */
    if (mfd == -1)
        return;
    mat_wr1_s(mfd, "TEOAE_TOK", ntok);
    mat_wr1_f(mfd, "rate", (float) rate);
    mat_wr1_s(mfd, "npts", npts);
    for (i = 1; i <= ntok; i++) {
	sprintf(s, "tok%d", i);
	tok_read(i);
	mat_wr_f(mfd, s, accbuf, npts);
    }
    mat_wr1_s(mfd, "reps", swp1set);
    mat_wr1_f(mfd, "ppc", cal_ppc);
    mat_wr1_f(mfd, "vpc", cal_vpc);
    _close(mfd);
}

void
usage()
{
    printf("usage:\n");
    printf("    tokmat file.cal ...\n");
    printf("\n");
    printf("where file is a TEOAE tok file generated by EMAV.\n");
    exit(0);
}

int
main(int ac, char **av)
{
    char    dfn[80], msg[80];

    if (ac < 2)
	usage();
    while (ac > 1) {
	if (av[1][0] == '-') {
	    if (av[1][1] == 'i') {
		iflg++;
	    }
	} else {
	    strcpy(dfn, av[1]);
	    if (tok_open(dfn, msg) == 0) {
                printf("File %s: %s\n", dfn, msg);
            } else {
                printf("File %s: ntok=%d, size=%d, rate=%d\n", 
                    dfn, ntok, npts, rate);
                if (ftyp == 1)
                    tok_mat(dfn);
                tclose(tfp);
            }
	}
	ac--;
	av++;
    }
    printf("\n");
    return (0);
}
