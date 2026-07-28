/* binmat.c - BIN to MAT file converter */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>

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

    ns = (int32_t)(strlen(nam) + 1);
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
mat_wr_l(int fd, char *nam, int32_t *p, int n)
{
    mat_wr(fd, nam, p, n, 2, 0, 0);
}

static void
mat_wr_f(int fd, char *nam, float *p, int n)
{
    mat_wr(fd, nam, p, n, 1, 0, 0);
}

/****************************************************************************/

#define MAXNPTS	    8192
#define MAXRESP	     500

static int32_t accbuf_a1[MAXNPTS], accbuf_a2[MAXNPTS];
static int32_t accbuf_b1[MAXNPTS], accbuf_b2[MAXNPTS];
static float f1[MAXRESP], f2[MAXRESP], f3[MAXRESP], f4[MAXRESP], nswp[MAXRESP];
static float L1[MAXRESP], L2[MAXRESP], L3[MAXRESP], L4[MAXRESP], time[MAXRESP];
static int verbose = 0;

struct BINhdr {
    unsigned short total;	/* total number of waves           */
    unsigned short wav_len;	/* wave form length in points      */
    float samp_rate;		/* sample rate                     */
    float int2volt;		/* long to volt conversion scale   */
    float volt2pas;		/* volt to pascal conversion scale */
    unsigned short swp1set;	/* sweeps per set */
    unsigned short nic;		/* number of input channels */
    char reserved[12];
} bh;

struct BINwav {
    float f2, L2;
    float f1, L1;
    unsigned short N, T;
    float f3, L3;
    unsigned short f4;
    short L4;
} wh;

static FILE *fbin = NULL;

static int
open_bin_file(char *fn)
{
    char    bfn[256];

    strcpy(bfn, fn);
    newext(bfn, "BIN");
    if(_access(bfn, 0) != 0) {
        fbin = NULL;
	bh.total = 0;
    } else {
	fbin = fopen(bfn, "rb");
	fread(&bh, sizeof(struct BINhdr), (size_t)(1), fbin);
	if (bh.total <= 0 || bh.wav_len <= 0 || bh.samp_rate <= 0) {
	    fclose(fbin);
	    fbin = NULL;
	    bh.total = 0;
	}
    }
    return (bh.total);
}

int
read_bin_file(int r)
{
    int bsz, nic, rsz, ard, brd;

    bsz = bh.wav_len;
    nic = (bh.nic < 2) ? 1 : 2;
    rsz = sizeof(struct BINwav) + 2 * 4 * bsz * nic;
    fseek(fbin, sizeof(struct BINhdr) + (int32_t) r * rsz, 0);
    fread(&wh, sizeof(struct BINwav), (size_t)(1), fbin);
    ard = fread(accbuf_a1, 4, (size_t)(bsz), fbin);
    if (nic >= 2)
        fread(accbuf_a2, 4, (size_t)(bsz), fbin);
    brd = fread(accbuf_b1, 4, (size_t)(bsz), fbin);
    if (nic >= 2)
	fread(accbuf_b2, 4, (size_t)(bsz), fbin);

    return ((ard == bsz) && (brd == bsz));
}

void
dp_bin_mat(char *fn)
{
    char name[8];
    int i, nresp, mfd;

    if (open_bin_file(fn)) {
        printf("%s: nic = %d\n", fn, bh.nic);
        newext(fn, "mat");
        mfd = _open(fn, OFLAG, PMODE);    /* open the file */
        if (mfd == -1)
            return;
        mat_wr1_s(mfd, "DPOAE_BIN", bh.total);
        mat_wr1_f(mfd, "rate", (float) bh.samp_rate);
        mat_wr1_s(mfd, "npts", bh.wav_len);
        mat_wr1_f(mfd, "int2volt", bh.int2volt);
        mat_wr1_f(mfd, "volt2pas", bh.volt2pas);
	for (i = 0; i < MAXRESP; i++) {
            if (!read_bin_file(i))
		break;
	    f1[i] = wh.f1;
	    L1[i] = wh.L1;
	    f2[i] = wh.f2;
	    L2[i] = wh.L2;
	    f3[i] = wh.f3;
	    L3[i] = wh.L3;
	    f4[i] = wh.f3;
	    L4[i] = wh.L3;
	    nswp[i] = wh.N;
	    time[i] = wh.T;
	    if (verbose)
	        printf("%3d : f2,f1,L2,L1 =%6.0f%6.0f%4.0f%4.0f\n" ,
		    i + 1, wh.f2, wh.f1, wh.L2, wh.L1);
	    if (bh.nic < 2) {
		sprintf(name, "A%03d", i + 1);
		mat_wr_l(mfd, name, accbuf_a1, bh.wav_len);
		sprintf(name, "B%03d", i + 1);
		mat_wr_l(mfd, name, accbuf_b1, bh.wav_len);
	    } else {
		sprintf(name, "A1%03d", i + 1);
		mat_wr_l(mfd, name, accbuf_a1, bh.wav_len);
		sprintf(name, "B1%03d", i + 1);
		mat_wr_l(mfd, name, accbuf_b1, bh.wav_len);
		sprintf(name, "A2%03d", i + 1);
		mat_wr_l(mfd, name, accbuf_a2, bh.wav_len);
		sprintf(name, "B2%03d", i + 1);
		mat_wr_l(mfd, name, accbuf_b2, bh.wav_len);
	    }
	}
	nresp = i;
        mat_wr_f(mfd, "f1", f1, nresp);
        mat_wr_f(mfd, "L1", L1, nresp);
        mat_wr_f(mfd, "f2", f2, nresp);
        mat_wr_f(mfd, "L2", L2, nresp);
        mat_wr_f(mfd, "f3", f3, nresp);
        mat_wr_f(mfd, "L3", L3, nresp);
        mat_wr_f(mfd, "f4", f3, nresp);
        mat_wr_f(mfd, "L4", L3, nresp);
        mat_wr_f(mfd, "nswp", nswp, nresp);
        mat_wr_f(mfd, "time", time, nresp);
        _close(mfd);
    }
}

void
usage()
{
    printf("usage:\n");
    printf("    binmat [-v] file.bin ...\n");
    printf("\n");
    printf("where file is a DPOAE binary file generated by EMAV.\n");
    exit(0);
}

int
main(int ac, char **av)
{
    char    dfn[80];

    if (ac < 2)
	usage();
    while (ac > 1) {
	if (av[1][0] == '-') {
	    if (av[1][1] == 'v') {
		verbose++;
	    }
	} else {
	    strcpy(dfn, av[1]);
	    dp_bin_mat(dfn);
	}
	ac--;
	av++;
    }
    printf("\n");
    return(0);
}
