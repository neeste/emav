/* calfile.c */

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <menu.h>
#include <token.h>
#include <util.h>
#include "savage.h"
#include "putt.h"

#ifndef O_BINARY
#define O_BINARY	0
#endif
#define OFLAG       (O_RDWR|O_CREAT|O_BINARY)
#define OPMOD       (O_RDONLY|O_BINARY)
#define PMODE       (S_IREAD|S_IWRITE)
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#define _close close
#define _lseek lseek
#define _open open
#define _read read
#define _write write 
#endif /* WIN32 */

#define FNSZ        42
#define PCMSZ       81
#define MSGSZ       16*3

int     getstim(char *, int);

char *probe_cal = "PROBE_CAL";
char *hear_cal = "HEAR_CAL";
double cal_ppc = 1, cal_vpc = 1;

//static double target_voltage = 0.3162;    /* 70dB SPL (80dB SPL = 1v) */
static int ntoken = 1;
static int tcount = 0;

/********************************* MAT **************************************/

/*............................ MAT WRITE ................................*/

#define NT          64

static int cfd = -1;
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
mat_wr1_d(int fd, char *nam, double p)
{
    mat_wr(fd, nam, &p, 1, 0, 0, 0);
}

static void
mat_wr1_s(int fd, char *nam, short p)
{
    mat_wr(fd, nam, &p, 1, 3, 0, 0);
}

static void
mat_wr_a(int fd, char *nam, char *p)
{
    short t[NT];
    int i, nw;
    int32_t hdr[5] = {1, 1, 0, 0, 0};

    hdr[0] = encode_mopt(3, 1);
    hdr[2] = strlen(p);
    hdr[4] = strlen(nam) + 1;
    _write(fd, hdr, 20);
    _write(fd, nam, (int) hdr[4]);
    for (i = 0; i < hdr[2]; i++) {
        t[i % NT] = p[i];
        if (((i + 1) % NT) == 0)
            _write(fd, t, NT * sizeof(short));
    }
    nw = i % NT;
    if (nw != 0)
        _write(fd, t, nw * sizeof(short));
}

static void
mat_wr_cf2d(int fd, char *nam, float *p, int n)
{
    double t[NT];
    int i, j, nw;
    int32_t hdr[5] = {0, 1, 1, 1, 0};

    hdr[0] = encode_mopt(0, 0);
    hdr[1] = n;
    hdr[4] = strlen(nam) + 1;
    _write(fd, hdr, 20);
    _write(fd, nam, (int) hdr[4]);
    for (j = 0; j < 2; j++) {
        for (i = 0; i < n; i++) {
            t[i % NT] = p[i * 2 + j];
            if (((i + 1) % NT) == 0)
                _write(fd, t, NT * sizeof(double));
        }
        nw = i % NT;
        if (nw != 0)
            _write(fd, t, nw * sizeof(double));
    }
}

/*............................ MAT READ ................................*/

static int
mat_rd_hdr(int fd, char *typ, int32_t * hdr, char *nam)
{
    int nc = 0;

    if (_read(fd, hdr, 20) == 20) {
        if (hdr[0] < 0 || hdr[0] > 9999)
            return (0);
        typ[0] = (hdr[0] / 1000) % 10;
        typ[1] = (hdr[0] / 100) % 10;
        typ[2] = (hdr[0] / 10) % 10;
        typ[3] = (hdr[0] / 1) % 10;
        nc = (int) hdr[4];
        if (nc <= 0 || nc > 80)
            return (0);
        nc = _read(fd, nam, nc);
    }

    return (nc);
}

static int
mat_rd(int fd, char *nam, void *p, int n, int dtyp, int txt, int cx)
{
    char    type[4];
    int     a = 1, sz, ne;
    char   *b = (char *) (&a);
    int32_t    hdr[5];

    if (!mat_rd_hdr(fd, type, hdr, nam))
        return (0);
    if ((type[0] != b[1]) || (type[2] != dtyp) || (type[3] != txt))
        return (0);
    if (hdr[3] != cx)
        return (0);
    ne = (short) (hdr[1] * hdr[2]);
    if (ne <= 0)
        return (0);
    sz = cx ? bsiz[dtyp] * 2 : bsiz[dtyp];
    if (n < ne) {
        _read(fd, p, n * sz);
        _lseek(fd, (int32_t) (ne - n) * sz, 1);
    } else {
        _read(fd, p, ne * sz);
        n = ne;
    }
    
    return (n);
}

static int
mat_rd_d(int fd, char *nam, double *p, int n)
{
    return (mat_rd(fd, nam, p, n, 0, 0, 0));
}

static int
mat_rd_f(int fd, char *nam, float *p, int n)
{
    return (mat_rd(fd, nam, p, n, 1, 0, 0));
}

static int
mat_rd_s(int fd, char *nam, short *p, int n)
{
    return (mat_rd(fd, nam, p, n, 3, 0, 0));
}

static void
mat_rd_text(int fd, char *p, int n)
{
    short   t[NT];
    int     i, nr, ne;

    ne = n;
    for (i = 0; i < ne; i++) {
        if ((i % NT) == 0) {
            nr = ((i + NT) > ne) ? (ne - i) : NT;
            _read(fd, t, nr * sizeof(short));
        }
        p[i] = (char) t[i % NT];
    }
    p[ne] = '\0';
}

static int
mat_rd_cd2f(int fd, char *nam, float *p, int n)
{
    char    type[4];
    double  t[NT];
    int     i, j, nr, ne;
    int     a = 1;
    char   *b = (char *) (&a);
    int32_t    hdr[5];

    if (!mat_rd_hdr(fd, type, hdr, nam))
        return (0);
    if ((type[0] != b[1]) || (type[2] != 0) || (type[3] != 0))
        return (0);
    if (hdr[3] == 0)
        return (0);
    ne = (short) (hdr[1] * hdr[2]);
    if (ne <= 0)
        return (0);
    for (j = 0; j < 2; j++) {
        for (i = 0; i < ne; i++) {
            if ((i % NT) == 0) {
                nr = ((i + NT) > ne) ? (ne - i) : NT;
                _read(fd, t, nr * sizeof(double));
            }
	    if (i < n) {
		p[i * 2 + j] = (float) t[i % NT];
	    }
        }
    }
    return (ne);
}

static int
mat_open(char *fn, char *msg, int hear)
{
    char    nam[80];
    float   rt;
    short   nt, np;

    cfd = _open(fn, OPMOD, PMODE);    /* open the file */
    if (cfd <= 0) {
	strcpy(msg, "Can't open the file");
	return (0);
    } else if (!mat_rd_s(cfd, nam, &nt, 1)) {
	strcpy(msg, "Not a valid CAL file");
	_close(cfd);
    	return(0);
    } else if (hear && strcmp(nam, hear_cal)) {
	strcpy(msg, "Not a valid HEARING calibration file");
	_close(cfd);
	return (0);
    } else if (strcmp(nam, probe_cal) && strcmp(nam, hear_cal)) {
	strcpy(msg, "Not a valid PUTT calibration file");
	_close(cfd);
	return (0);
    } else if (!mat_rd_f(cfd, nam, &rt, 1)) {
	_close(cfd);
        return(0);
    } else if (!mat_rd_s(cfd, nam, &np, 1)) {
	_close(cfd);
        return (0);
    }

    rate = (int32_t) rt;
    buflen = np;
    ntoken = nt;

    return (1);
}

static void
mat_info(char *hpt, char *sfn, char msg[][16], short *tm, short *ct)
{
    char nam[80], type[4];
    int dtyp, text, nbyt, nt;
    int32_t ofst, nsmp, hdr[5];

    nt = 0;
    ofst = 0;
    for (;;) {
        if (_lseek(cfd, ofst, 0) < 0)
            break;
        if (!mat_rd_hdr(cfd, type, hdr, nam))
            break;
        if (hdr[1] <= 0 || hdr[2] <= 0 || hdr[4] <= 0)
            break;
        if (ofst == 0)
            strcpy(hpt, nam);
        dtyp = type[2];
        text = type[3];
        nsmp = hdr[1] * hdr[2];
        nbyt = (int) (bsiz[dtyp] * nsmp);
        if (hdr[3] == 1)                        // complex data
            nbyt *= 2;
        if (strncmp(nam, "tok", 3) == 0) {      // count tokens
            nt++;
        } else if (strcmp(nam, "stm1") == 0) {
            if (text) {				// stimulus file name?
		mat_rd_text(cfd, sfn, (int) nsmp);
		getstim(sfn, buflen);
	    } else {
		strcpy(sfn, "~cal");
		_read(cfd, outbuf, nbyt);
	        memcpy(stimulus, outbuf, nbyt);
	    }
        } else if (strcmp(nam, "mesg") == 0) {
            _read(cfd, msg, nbyt);
        } else if (strcmp(nam, "time") == 0) {
            _read(cfd, tm, nbyt);
        }
	ofst += 20 + hdr[4] + nbyt;
    }
    *ct = nt;
}

static void
mat_read(int tn)
{
    char nam[80], type[4], toknam[20], carnam[20];
    float car[8], adsen, mpsen;
    int dtyp, nbyt, ncar = 0;
    int32_t ofst, nsmp, hdr[5];
    static int nbc = (int) (8 * sizeof(float));

    sprintf(toknam, "tok%d", tn);
    sprintf(carnam, "car%d", tn);
    ofst = 0;
    for (;;) {
        if (_lseek(cfd, ofst, 0) < 0)
            break;
        if (!mat_rd_hdr(cfd, type, hdr, nam))
            break;
        if (hdr[1] <= 0 || hdr[2] <= 0 || hdr[4] <= 0)
            break;
        dtyp = type[2];
        nsmp = hdr[1] * hdr[2];
        nbyt = (int) (bsiz[dtyp] * nsmp);
        if (hdr[3] == 1)                        /* complex data */
            nbyt *= 2;
        if (strcmp(nam, toknam) == 0) {
            _read(cfd, accbuf, nbyt);
        } else if (strcmp(nam, carnam) == 0) {
            ncar = (nbyt > nbc) ? nbc : nbyt;
            _read(cfd, car, ncar);
            break;
        }
	ofst += 20 + hdr[4] + nbyt;
    }
    if (ncar == nbc) {
        adsen = car[6];
        mpsen = car[7];
    } else {
        adsen = (1 / (car[0] * car[2] * Sen.MP));
        mpsen = Sen.MP;
    }
    swp1set = (int) car[0];
    cal_ppc = car[2];               /* Pa/cnt */
    cal_vpc = car[5];               /* V/cnt  */
    if (cal_vpc > 1 || cal_vpc < 1e-12) /* fix files before 29-Jul-96 */
        cal_vpc = 9.65e-6;
    Sen.AD = adsen;                 /* cnt/V  */
    Sen.MP = mpsen;                 /* Pa/V  */
}

static void
mat_close()
{
    if (cfd >= 0) {
        _close(cfd);
        cfd = -1;
    }
}

/********************************* TOK **************************************/

#ifdef WR_TOKEN_FILE

/*............................ TOK WRITE ................................*/

static TOKENFILE *t = NULL;

/* tok_init - initialize tokenfile structure */

static void
tok_init()
{
    t->header.power10 = 1;
    t->header.isf = (int) (rate / 10);
    t->hitch.tokens = ntoken;
    t->hitch.curtok = tcount;
    t->hitch.toksiz = buflen;
    memcpy(t->hitch.comment, &patient, sizeof(PATIENT));
    t->hitch.target_level = (float) (target_voltage / Sen.MP);
    t->hitch.mode = 0;
    tinit(t);
    if (tcount < ntoken) {
	strncpy(t->car.tokcmt, "", FNAMESIZE);
	strncpy(t->car.sname, ilstim, COMNTSIZE);
	t->car.reps = nsets * swp1set;
	t->car.atten1 = (int) (attenu.att1 * 100.);
	t->car.atten2 = (int) (attenu.att2 * 100.);
	t->car.rejt1 = 0;
	t->car.rejt2 = buflen;
	t->car.rejlo = -upperv;
	t->car.rejhi = upperv;
	t->car.swprej = 0;
	t->car.toktime = 0;
	t->car.temp = probe.temp;
	t->car.diacav = probe.diacav;
        t->car.adsen = Sen.AD;
        t->car.mpsen = Sen.MP;
	twrcar(tcount + 1, t);
    }
}

/* tok_creat - create a new tokenfile */

static void
tok_creat(char *fn, char *hpt)
{
    t = tcreat(fn);
    if (t == NULL) {
        remove(fn);
        t = tcreat(fn);
    }
    strcpy(t->header.pattern, hpt);
    tok_init();
}

static double
rescale(float *fbuf, short *ibuf, int n, int maxfs)
{
    double  scale, maxval = 0, absval;
    int     i;

    for (i = 0; i < n; i++) {
	absval = fabs(fbuf[i]);
	if (absval > maxval)
	    maxval = absval;
    }
    if (maxval <= maxfs || maxval <= 0) {
	for (i = 0; i < n; i++)
	    ibuf[i] = (int) fbuf[i];
    } else {
	scale = ((double) maxfs) / ((double) maxval);
	for (i = 0; i < n; i++)
	    ibuf[i] = nint(fbuf[i] * scale);
    }
    return (maxval);
}

static int
findmax(intbuf, n)
int    *intbuf;
int     n;
{
    int     i;
    int     maxval = 0, absval;

    for (i = 0; i < n; i++) {
	absval = abs(intbuf[i]);
	if (absval > maxval)
	    maxval = absval;
    }
    return (maxval);
}

/* store the token */

static void
tok_store(int c, float *p)
{
    int     tmax, reps;
    double  amax, scale;

    if (c == 0)
	return;
    amax = rescale(p, outbuf, buflen, MAXPOSINT);
    tmax = findmax(outbuf, buflen);
    reps = c * swp1set;
    if(tmax == 0.0) 
    	scale = 1 / (Sen.MP * Sen.AD * reps);
    else 
    	scale = ((float) amax / tmax) / (Sen.MP * Sen.AD * reps);
    t->car.dccomp = 0;
    t->car.maxval = (int32_t) amax;
    t->car.tokmax = tmax;
    strncpy(t->car.tokcmt, patient.Comment, PCMSZ);
    strncpy(t->car.sname, ilstim, FNAMESIZE);
    t->car.reps = reps;
    t->car.atten1 = (int) (attenu.att1 * 100.);
    t->car.atten2 = (int) (attenu.att2 * 100.);
    t->car.swprej = 0;
    t->car.toktime = time(NULL);
    t->car.rejt1 = 0;
    t->car.rejt2 = buflen;
    t->car.rejlo = -upperv;
    t->car.rejhi = upperv;
    t->car.scale = scale;
    t->car.temp = probe.temp;
    t->car.diacav = probe.diacav;
    t->car.adsen = Sen.AD;
    t->car.mpsen = Sen.MP;

    twrite(outbuf, tcount, t);
}

/* tok_shut - copy message & runtime, then close tokenfile */

static void
tok_shut(char *msg, int rt)
{
    memcpy(t->hitch.outmsg, msg, MSGSZ);
    t->hitch.time = rt;
    twrhitch(t);
    tclose(t);
    t = NULL;
}

/* tok_quit - close tokenfile for program termination */

static void
tok_quit()
{
    if (t != NULL) {
        tclose(t);
        t = NULL;
    }
}

#endif /* WR_TOKEN_FILE */

/*............................ TOK READ .................................*/

static TOKENFILE *tfp = NULL;

static int
tok_open(char *fn, char *msg, int hear)
{
    tfp = topen(fn);
    if (tfp == NULL) {
	strcpy(msg, "Can't open the file");
	return (0);
    } else if (tfp->header.stype != 11111) {
	strcpy(msg, "Not a valid TOKEN file");
	tclose(tfp);
	return (0);
    } else if (hear && strcmp(tfp->header.pattern, hear_cal)) {
	strcpy(msg, "Not a valid HEARING calibration file");
	tclose(tfp);
	return (0);
    } else if (strcmp(tfp->header.pattern, probe_cal)
	    && strcmp(tfp->header.pattern, hear_cal)) {
	strcpy(msg, "Not a valid PUTT calibration file");
	tclose(tfp);
	return (0);
    }

    buflen = (int) tfp->hitch.toksiz;
    rate = (int32_t) (tfp->header.isf
        * pow(10.0, (double) tfp->header.power10));
    ntoken = tfp->hitch.tokens;

    return (1);
}

static void
tok_info(char *hpt, char *sfn, char msg[][16], short *tm, short *ct)
{
    memcpy(hpt, tfp->header.pattern, 12);
    strncpy(sfn, tfp->car.sname, FNAMESIZE);
    memcpy(msg, tfp->hitch.outmsg, MSGSZ);
    *tm = tfp->hitch.time;
    *ct = tfp->hitch.curtok;
}

static void
scaleback(int tokmax, int32_t maxval, float *out)
{
    int     i;
    float   scale;

    if (tokmax > 0)
	scale = (float) maxval / tokmax;
    else
	scale = 1.0;
    for (i = 0; i < buflen; i++)
	out[i] = scale * outbuf[i];
}

static void
tok_read(int tn)
{
    double ad_sen = (MAXPOSINT / 10.0), mp_sen = 1;
    struct CAR *c = &tfp->car;

    tread(outbuf, tn, tfp);
    swp1set = c->reps;
    scaleback(c->tokmax, c->maxval, accbuf);
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
    Sen.AD = (float) ad_sen;        /* cnt/V  */
    Sen.MP = (float) mp_sen;        /* Pa/V  */
}

static void
tok_close()
{
    if (tfp != NULL) {
        tclose(tfp);
        tfp = NULL;
    }
}

/********************************* CAL **************************************/

/*............................ CAL READ .................................*/

static int matfmt = 0;

int
cal_open(char *fn, char *msg, int hear)
{
    int i;

    matfmt = 0;
    i = tok_open(fn, msg, hear);
    if (!i) {
        i = mat_open(fn, msg, hear);
        matfmt = 1;
    }
    return (i);
}

void
cal_info(char *hpt, char *sfn, char msg[][16], short *tm, short *ct)
{
    if (matfmt)
        mat_info(hpt, sfn, msg, tm, ct);
    else
        tok_info(hpt, sfn, msg, tm, ct);
}

void
cal_read(int tn)
{
    if (matfmt)
        mat_read(tn);
    else
        tok_read(tn);
}

void
cal_close()
{
    if (matfmt)
        mat_close();
    else
        tok_close();
}

/******************************* THEV *************************************/

static char *ld_ext= "THL";
static char *src_ext = "THS";
static char *src_thv = "SRC_THV";
static char *ld_thv = "LD_THV";

int
thev_src_sav(char *fn, char *tfn, float *zs, float *ps, float *px, 
    int nf, double dc, double tc, int nc, float *lc, float *sr)
{
    double df;
    int    fd;
    
    df = (double) rate / buflen;
    strncpy(tfn, fn, FNSZ);
    newext(tfn, src_ext);
    fd = _open(tfn, OFLAG, PMODE);    /* open the file */
    if (fd == -1) {
        return (1);
    }
    mat_wr1_s(fd, src_thv, (short) nf);
    mat_wr1_d(fd, "rate", (double) rate);
    mat_wr1_d(fd, "df", df);
    mat_wr_cf2d(fd, "zs", zs, nf + 1);
    mat_wr_cf2d(fd, "ps", ps, nf + 1);
    mat_wr_cf2d(fd, "px", px, nf + 1);
    mat_wr1_d(fd, "dc", dc);
    mat_wr1_d(fd, "tc", tc);
    mat_wr(fd, "lc", lc, nc, 1, 0, 0);
    mat_wr_cf2d(fd, "sr", sr, 1);
    _close(fd);

    return (0);
}

int
thev_src_read(char *fn, float *zs, float *ps, float *px, int n)
{
    char    nam[80];
    double  df, dc, tc, rt = 0;
    float   lc[8];
    int     fd, status = 0;
    short   nf;

    fd = _open(fn, OPMOD, PMODE);
    if (fd < 0)
	return (0);
    if (mat_rd_s(fd, nam, &nf, 1)
        && (strcmp(src_thv, nam) == 0)
        && mat_rd_d(fd, nam, &rt, 1)
        && mat_rd_d(fd, nam, &df, 1)
        && mat_rd_cd2f(fd, nam, zs, n)
        && mat_rd_cd2f(fd, nam, ps, n)
        && mat_rd_cd2f(fd, nam, px, n)
        && mat_rd_d(fd, nam, &dc, 1)
        && mat_rd_d(fd, nam, &tc, 1)
        && mat_rd_f(fd, nam, lc, 8))
        status = nf;
    _close(fd);
    rate = (int32_t) rt;

    return (status);
}

int
thev_ld_sav(char *fn, char *tsf, float *zl, float *pl, float *pr, int nf)
{
    char tfn[80];
    double df;
    int    fd;
    
    df = (double) rate / buflen;
    strncpy(tfn, fn, FNSZ);
    newext(tfn, ld_ext);
    fd = _open(tfn, OFLAG, PMODE);    /* open the file */
    if (fd == -1) {
        return (1);
    }
    mat_wr1_s(fd, ld_thv, (short) nf);
    mat_wr1_d(fd, "rate", (double) rate);
    mat_wr1_d(fd, "df", df);
    mat_wr_cf2d(fd, "zl", zl, nf);
    mat_wr_cf2d(fd, "pl", pl, nf);
    mat_wr_cf2d(fd, "pr", pr, nf);  // added Jun-2008
    mat_wr_a(fd, "tsf", tsf);
    _close(fd);

    return (0);
}

/****************************** PUTT ****************************************/

#ifdef PUTT

int
thev_ld_read(char *fn, float *zl, float *pl, int n)
{
    char    nam[80];
    double  df, rt = 0;
    int     fd, status = 0;
    short   nf;

    fd = _open(fn, OPMOD, PMODE);
    if (fd < 0)
	return (0);
    if (mat_rd_s(fd, nam, &nf, 1)
        && (strcmp(ld_thv, nam) == 0 || strcmp(src_thv, nam) == 0)
        && mat_rd_d(fd, nam, &rt, 1)
        && mat_rd_d(fd, nam, &df, 1)
        && mat_rd_cd2f(fd, nam, zl, n)
        && mat_rd_cd2f(fd, nam, pl, n))
        status = nf;
    _close(fd);
    rate = (int32_t) rt;

    return (status);
}

static int mfd = -1;

static void
mat_wr1_f(int fd, char *nam, float p)
{
    mat_wr(fd, nam, &p, 1, 1, 0, 0);
}

static void
mat_wr1_l(int fd, char *nam, int32_t p)
{
    mat_wr(fd, nam, &p, 1, 2, 0, 0);
}

static void
mat_wr_f(int fd, char *nam, float *p, int n)
{
    mat_wr(fd, nam, p, n, 1, 0, 0);
}

static void
mat_wr_s(int fd, char *nam, short *p, int n)
{
    mat_wr(fd, nam, p, n, 3, 0, 0);
}

static void
mat_wr_v(int fd, char *nam, void *p, int nb)
{
    mat_wr(fd, nam, p, nb / 4, 1, 1, 0);
}

static void
mat_creat(char *fn, char *hpt)
{
    mfd = _open(fn, OFLAG, PMODE);    /* open the file */
    if (mfd == -1)
        return;
    mat_wr1_s(mfd, hpt, (short) ntoken);
    mat_wr1_f(mfd, "rate", (float) rate);
    mat_wr1_s(mfd, "npts", (short) buflen);
    mat_wr_v(mfd, "info", &patient, 132);
}

static void
mat_store(int c, float *p)
{
    char    nam[20];
    float   car[8];
    int     reps;

    reps = c * swp1set;
    cal_ppc = 1 / (Sen.AD * Sen.MP * reps);                 /* Pa/cnt */
    cal_vpc = pow(10.0, -0.05 * attenu.att1) / Sen.DA[0];   /* V/cnt  */

    car[0] = (float) reps;
    car[1] = attenu.att1;
    car[2] = (float) cal_ppc;
    car[3] = probe.temp;
    car[4] = probe.diacav;
    car[5] = (float) cal_vpc;
    car[6] = Sen.AD;
    car[7] = Sen.MP;

    sprintf(nam, "tok%d", tcount); mat_wr_f(mfd, nam, p, buflen);
    sprintf(nam, "car%d", tcount); mat_wr_f(mfd, nam, car, 8);
    sprintf(nam, "stm%d", tcount); mat_wr_s(mfd, nam, stimulus, buflen);
    sprintf(nam, "tim%d", tcount); mat_wr1_l(mfd, nam, (int32_t) time(NULL));
}

static void
mat_shut(char *msg, int rt)
{
    mat_wr_v(mfd, "mesg", msg, MSGSZ);
    mat_wr1_s(mfd, "time", (short) rt);
    _close(mfd);
    mfd = -1;
}

static void
mat_quit()
{
    if (mfd >= 0)
        _close(mfd);
    mfd = -1;
}

static void
mat_pati(PATIENT *p)
{
    char nam[80], type[4];
    int dtyp, nbyt;
    int32_t ofst, nsmp, hdr[5];

    ofst = 0;
    for (;;) {
        if (_lseek(cfd, ofst, 0) < 0)
            break;
        if (!mat_rd_hdr(cfd, type, hdr, nam))
            break;
        if (hdr[1] <= 0 || hdr[2] <= 0 || hdr[4] <= 0)
            break;
        dtyp = type[2];
        nsmp = hdr[1] * hdr[2];
        nbyt = (int) (bsiz[dtyp] * nsmp);
        if (hdr[3] == 1)                   /* complex data */
            nbyt *= 2;
        if (strcmp(nam, "info") == 0) {
            _read(cfd, p, nbyt);
            break;
        }
	ofst += 20 + hdr[4] + nbyt;
    }
}

static void
tok_pati(PATIENT *p)
{
    memcpy(p, tfp->hitch.comment, sizeof(PATIENT));
}

/*............................ CAL WRITE ................................*/

void
cal_creat(char *fn, char *hpt, int nt)
{
    tcount = 0;
    ntoken = nt;
    mat_creat(fn, hpt);
}

void
cal_store(int c, float *p)
{
    tcount++;
    mat_store(c, p);
}

void
cal_shut(char *msg, int rt)
{
    mat_shut(msg, rt);
}

void
cal_quit()
{
    mat_quit();
}

void
cal_pati(PATIENT *p)
{
    if (matfmt)
        mat_pati(p);
    else
        tok_pati(p);
}

#endif /* PUTT */

/****************************************************************************/
