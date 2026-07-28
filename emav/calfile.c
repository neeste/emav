/* calfile.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <menu.h>
#include <token.h>
#include <fftlib.h>
#include "savage.h"
#include "emav.h"
#include "screen.h"

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#define _access access
#define _close close
#define _lseek lseek
#define _open open
#define _read read
#define _write write 
#define _hypot hypot
#endif /* WIN32 */
#ifndef O_BINARY
#define O_BINARY  0
#endif
#define PMODE       (S_IREAD|S_IWRITE)
#define OFLAG       (O_RDWR|O_CREAT|O_BINARY)
#define OPMOD       (O_RDONLY|O_BINARY)

extern char stmfn[FNAMESIZE];
extern float cal_attn;
extern int cal_sets;
extern struct _probe probe;

static char *calkey = "EMAV_CAL";
static int calfmt = 0;
static int calcav = 0;
static int ntoken = 0;

/********************************* MAT **************************************/

#define NT 64

static int bsiz[5] = {8, 4, 4, 2, 4};

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

/*..........................................................................*/

static int
mat_read(char *fn, int icav)
{
    char    type[4], nam[80], c1[20], c2[20];
    double  f8;
    float   f4;
    int     i, tpc, cfd, status = 1, dsiz, dtyp, text, itok = 0;
    int32_t    hdr[5];
    short   i2;

    // defaults
    numadc = 1;
    numcav = 1;
    numsrc = 2;
    cal_sets = 2;
    cal_attn = 20;
    tpc = 0;

    cfd = _open(fn, OPMOD, PMODE);    /* open the file */
    if (cfd < 0) {
	return (status);
    }
    if (mat_rd_hdr(cfd, type, hdr, nam)) {
	dtyp = type[2];
	dsiz = hdr[1] * hdr[2];
	if (strcmp(nam, calkey) == 0 && dtyp == 3 && dsiz == 1) {
	    _read(cfd, &i2, bsiz[dtyp]);
	    while(mat_rd_hdr(cfd, type, hdr, nam)) {
		dtyp = type[2];
		text = type[3];
		dsiz = hdr[1] * hdr[2];
		tpc = numsrc * numadc;	    // tokens per cavity
		sprintf(c1, "cal%02d", icav * tpc + 1);
		sprintf(c2, "cal%02d", icav * tpc + 2);
		if (strcmp(nam, "rate") == 0 && dtyp == 1 && dsiz == 1) {
		    _read(cfd, &f4, bsiz[dtyp]);
		    rate = (int32_t) f4;
		} else if (strcmp(nam, "npts") == 0 && dtyp == 3 && dsiz == 1) {
		    _read(cfd, &i2, bsiz[dtyp]);
		    buflen = i2;
		} else if (strcmp(nam, "reps") == 0 && dtyp == 3 && dsiz == 1) {
		    _read(cfd, &i2, bsiz[dtyp]);
		    swp1set = i2;
		} else if (strcmp(nam, "attn") == 0 && dtyp == 1 && dsiz == 1) {
		    _read(cfd, &f4, bsiz[dtyp]);
		    cal_attn = f4;
		} else if (strcmp(nam, "adsn") == 0 && dtyp == 1 && dsiz == 1) {
		    _read(cfd, &f4, bsiz[dtyp]);
		    Sen.AD = f4;
		} else if (strcmp(nam, "da1sn") == 0 && dtyp == 1 && dsiz == 1) {
		    _read(cfd, &f4, bsiz[dtyp]);
		    Sen.DA[0] = f4;
		} else if (strcmp(nam, "da2sn") == 0 && dtyp == 1 && dsiz == 1) {
		    _read(cfd, &f4, bsiz[dtyp]);
		    Sen.DA[1] = f4;
		} else if (strcmp(nam, "mpsn") == 0 && dtyp == 1 && dsiz == 1) {
		    _read(cfd, &f4, bsiz[dtyp]);
		    Sen.MP = f4;
		} else if (strcmp(nam, "stmfn") == 0 && dtyp == 0 &&
	    	    dsiz < FNAMESIZE && text) {
		    for (i = 0; i < dsiz; i++) {
			_read(cfd, &f8, bsiz[dtyp]);
			stmfn[i] = (char) f8;
		    }
		} else if (strcmp(nam, c1) == 0 && dtyp == 2 && tpc > 0) {
		    _read(cfd, accbuf_a, bsiz[dtyp] * dsiz);
		    itok++;
		} else if (strcmp(nam, c2) == 0 && dtyp == 2 && tpc > 1) {
		    _read(cfd, accbuf_b, bsiz[dtyp] * dsiz);
		    itok++;
		} else if (strcmp(nam, "stim") == 0 && dtyp == 3) {
		    _read(cfd, stmbuf, bsiz[dtyp] * dsiz);
		} else if (strcmp(nam, "nadc") == 0 && dtyp == 3 && dsiz == 1) {
		    _read(cfd, &i2, bsiz[dtyp]);
		    numadc = i2;
		} else if (strcmp(nam, "ncav") == 0 && dtyp == 3 && dsiz == 1) {
		    _read(cfd, &i2, bsiz[dtyp]);
		    numcav = i2;
		} else if (strcmp(nam, "nsrc") == 0 && dtyp == 3 && dsiz == 1) {
		    _read(cfd, &i2, bsiz[dtyp]);
		    numsrc = i2;
		} else if (strcmp(nam, "diam") == 0 && dtyp == 1 && dsiz == 1) {
		    _read(cfd, &f4, bsiz[dtyp]);
		    diacav = f4;
		} else if (strcmp(nam, "temp") == 0 && dtyp == 1 && dsiz == 1) {
		    _read(cfd, &f4, bsiz[dtyp]);
		    tmpcav = f4;
		} else if (strcmp(nam, "sets") == 0 && dtyp == 3 && dsiz == 1) {
		    _read(cfd, &i2, bsiz[dtyp]);
		    cal_sets = i2;
		} else {	    // skip over unknown variables
		    _lseek(cfd, bsiz[dtyp] * dsiz, SEEK_CUR);
		}
	    }
	}
    }
    _close(cfd);
    if (itok == tpc) {
	status = 0;
    }

    return (status);
}

/*............................ MAT WRITE ................................*/

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
    hdr[1] = txt ? 1 : n;	/* rows */
    hdr[2] = txt ? n : 1;	/* cols */
    hdr[3] = cx;		/* 0 for real, 1 for complex */
    hdr[4] = ns;	        /* name length (including null byte) */
    sz = bsiz[dtyp];
    if (cx)
        sz *= 2;
    _write(fd, hdr, 20);
    _write(fd, nam, (int) ns);
    _write(fd, p, n * sz);
}

/*..........................................................................*/

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
mat_wr_s(int fd, char *nam, short *p, int n)
{
    mat_wr(fd, nam, p, n, 3, 0, 0);
}

static void
mat_wr1_s(int fd, char *nam, short p)
{
    mat_wr(fd, nam, &p, 1, 3, 0, 0);
}

static void
mat_wr_t(int fd, char *nam, char *s)
{
    double *p;
    int i, n;

    n = strlen(s);
    p = (double *) calloc(n, sizeof(double));
    for (i = 0; i < n; i++)
	p[i] = s[i];
    mat_wr(fd, nam, p, n, 0, 1, 0);
    free(p);
}

static void
mat_wr1_d(int fd, char *nam, double p)
{
    mat_wr(fd, nam, &p, 1, 0, 0, 0);
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

/*..........................................................................*/

int
mat_save(char *fn, int icav)
{
    char    type[4], nam[80], cn[20], dn[20];
    int     mfd, npts, ntok = 0, dtyp, dsiz, i;
    float   scale;
    int32_t    hdr[5], *accbuf[2];
    short   i2;

    if (numsrc <= 0 || numcav <= 0) {		// sanity check
	return (1);
    }
    if (icav == 0 && _access(fn, 0) == 0) {	// remove existing file
        remove(fn);
    }
    mfd = _open(fn, OFLAG, PMODE);		// open the file
    if (mfd < 0) {
        return (1);
    }
    accbuf[0] = accbuf_a;
    accbuf[1] = accbuf_b;
    if (icav == 0) {
	ntok = numsrc;
	npts = buflen;
        scale = 1 / (2 * swp1set * Sen.AD * Sen.MP);
	mat_wr1_s(mfd, calkey, (short) ntok);
	mat_wr1_f(mfd, "rate", (float) rate);
	mat_wr1_s(mfd, "npts", (short) npts);
	mat_wr1_s(mfd, "reps", (short) swp1set);
	mat_wr1_f(mfd, "attn", cal_attn);
	mat_wr1_f(mfd, "adsn", Sen.AD);
	for (i = 0; i < numsrc; i++) {
	    sprintf(dn, "da%dsn", i + 1);
	    mat_wr1_f(mfd, dn, Sen.DA[i]);
	}
	mat_wr1_f(mfd, "mpsn", Sen.MP);
	mat_wr1_f(mfd, "scale", scale);
	mat_wr_t(mfd, "stmfn", stmfn);
	mat_wr_s(mfd, "stim", stmbuf, npts);
	mat_wr1_s(mfd, "ncav", (short) numcav);
	mat_wr1_s(mfd, "nsrc", (short) numsrc);
	mat_wr1_s(mfd, "nadc", (short) numadc);
	if (numcav > 1) {
	    mat_wr1_f(mfd, "diam", diacav);
	    mat_wr1_f(mfd, "temp", tmpcav);
	}
	for (i = 0; i < numsrc; i++) {
	    sprintf(cn, "cal%02d", i + 1);
	    mat_wr_l(mfd, cn, accbuf[i], npts);
	}
    } else if (icav > 0) {
	if (!mat_rd_hdr(mfd, type, hdr, nam)) {
	    _close(mfd);	// close the file
	    return (1);
	}
	dtyp = type[2];
	dsiz = hdr[1] * hdr[2];
	if (strcmp(nam, calkey) == 0 && dtyp == 3 &&  dsiz == 1) {
	    _read(mfd, &i2, bsiz[dtyp]);
	    ntok = i2;
	} else {
	    _close(mfd);	// close the file
	    return (1);
	}
	if (ntok != icav * numsrc) {
	    _close(mfd);	// close the file
	    return (1);
	}
	sprintf(cn, "cal%02d", ntok);
	npts = buflen;
	while(mat_rd_hdr(mfd, type, hdr, nam)) {
	    dtyp = type[2];
	    dsiz = hdr[1] * hdr[2];
	    _lseek(mfd, bsiz[dtyp] * dsiz, SEEK_CUR);	// skip data
	    if (strcmp(nam, cn) == 0) {
		break;
	    }
	}
	for (i = 0; i < numsrc; i++) {
	    sprintf(cn, "cal%02d", ++ntok);
	    mat_wr_l(mfd, cn, accbuf[i], npts);
	}
    }
    // optional variable at end of file
    mat_wr1_s(mfd, "sets", (short) acc_sets);
    _lseek(mfd, 0L, 0);
    // rewrite ntok at beginning of file
    mat_wr1_s(mfd, calkey, (short) ntok);
    _close(mfd);	// close the file

    return (0);
}

static void
mat_update(char *fn, int nadc)	    // adjust nadc
{
    char    type[4], nam[80];
    int     mfd, dtyp, dsiz;
    int32_t    hdr[5], p = 0;

    mfd = _open(fn, OFLAG, PMODE);  // open the file
    if (mfd < 0) {
        return;
    }
    numadc = nadc;
    while(mat_rd_hdr(mfd, type, hdr, nam)) {
        dtyp = type[2];
        dsiz = hdr[1] * hdr[2];
        if (strcmp(nam, "nadc") == 0) {
	    _lseek(mfd, p, SEEK_SET);			    // step back
	    mat_wr1_s(mfd, "nadc", (short) numadc);	    // update nadc
	    break;
	} else {
	    p = _lseek(mfd, bsiz[dtyp] * dsiz, SEEK_CUR);   // skip data
	}
    }
    _close(mfd);		    // close the file
}

/****************************************************************************/

/********************************* TOK **************************************/

/*............................ TOK READ .................................*/

static int
tok_read(char *cal_file_name)
{
    int     status = 0;
    short  *sbuf;
    TOKENFILE *tfp;

    tfp = topen(cal_file_name);
    if (tfp == NULL) {
	status = 1;
    } else if (tfp->header.stype != 11111 ||
        strcmp(tfp->header.pattern, "DPOAE.CAL")) {
	tclose(tfp);
	status = 2;
    } else {
        rate = (int32_t) (tfp->header.isf * pow(10.0, (double) tfp->header.power10));
        buflen = (int) tfp->hitch.toksiz;
        ntoken = (int) tfp->hitch.tokens;
	//
	// read & scaleback two tokens
	sbuf = (short *) calloc(buflen, sizeof(short));
        tread(sbuf, 1, tfp);
        scaleback(tfp->car.tokmax, tfp->car.maxval, sbuf, accbuf_a);
        tread(sbuf, 2, tfp);
        scaleback(tfp->car.tokmax, tfp->car.maxval, sbuf, accbuf_b);
	free(sbuf);
	//
        swp1set = tfp->car.reps;
        acc_sets = 1;
	strncpy(stmfn, tfp->car.sname, FNAMESIZE);
	//
	if ((tfp->car.adsen > 1e3) && (tfp->car.adsen < 1e9) &&
	    (tfp->car.mpsen > 1e-6) && (tfp->car.mpsen < 1e6)) {
	    Sen.AD = tfp->car.adsen;
	    Sen.MP = tfp->car.mpsen;
	} else if (tfp->car.tokmax > 0) {
            Sen.AD = (float) MAXPOSINT / 10;
	    Sen.MP = ((float) tfp->car.maxval / tfp->car.tokmax) /
		(tfp->car.reps * tfp->car.scale * Sen.AD);
	} else {
            Sen.AD = (float) MAXPOSINT / 10;
	    Sen.MP = 1;
	}
	dis_units(Sen.AD, Sen.MP);
	//
        tclose(tfp);
    }
    return (status);
}

/*............................ TOK SAVE .................................*/

extern TOKENFILE *t;

static int
tok_save(char *fn)
{
    if (_access(fn, 0) == 0) {
	remove(fn);
    }
    t = tcreat(fn);
    if (!t) {
	decide(0, 1, "Can't create calibration file.");
    } else {
	strcpy(t->header.pattern, "DPOAE.CAL");
	tok_init(2, 0);
	tok_store(1, accbuf_a);
	tok_store(1, accbuf_b);
	tclose(t);
	return (0);
    }
    return (1);
}

/********************************* CAL **************************************/

void
cal_disp(char *calfn, int dm)
{
    double  tms;
    int32_t   *buf;
    int     c, rs, itsv, ntok;

    itsv = itime1;
    itime1 = buflen;
    ntok = numsrc * numadc;
    dis_units(Sen.AD, Sen.MP);
    for (c = 0; c < ntok; c++) {
        rs = c ? 0 : 1;
        dfgc = c ? scrn_c[C_CHNB] : scrn_c[C_CHNA];
        buf = c ? accbuf_b : accbuf_a;
        if (dm) {
            if (rs) {
                w_stim.ytop = w_stim_fft.ytop = txtpar.menu_height + 1;
                draw_w(&w_stim);
                draw_w(&w_stim_fft);
                draw_w(&w_info);
                draw_w(&w_sig);
                draw_w(&w_spec);
                cal_mesg(calfn);
            }
            w_sig.ybot -= txtpar.font_height;
            disply(cal_sets * swp1set * 2, buf, buflen, rs);
            w_sig.ybot += txtpar.font_height;
            tms = 1000 * (double) buflen / rate;
            show_xlabel(0.0, tms, 0, "ms");
            fft_stim((int32_t *) buf, cal_sets, 1);
            show_fft(fft_xrange, fft_yrange, 0, dfgc, rs);
        } else {
            display_reset = rs;
            stim_title(stmfn);
            dis_stim(0, buf, 1, swp1set * 2, cal_sets);
        }
    }
    itime1 = itsv;
}

void
cal_mesg(char *calfn)
{
    int     i, j;
    struct PATH path;

    i = draw_w(&w_msg);
    j = w_msg.ytop + txtpar.font_height + 4;
    gprintf(i, j, "Filename:");
    j += txtpar.font_height;
    split_path(calfn, path.drive, path.dir, path.name, path.ext);
    gprintf(i, j, "  %s%s", path.name, path.ext);
    j += txtpar.font_height;
    gprintf(i, j, "Stimulus:");
    j += txtpar.font_height;
    split_path(stmfn, path.drive, path.dir, path.name, path.ext);
    gprintf(i, j, "  %s%s", path.name, path.ext);
    j += txtpar.font_height;
    gprintf(i, j, "cal_swps = %d", swp1set);
    j += txtpar.font_height;
    gprintf(i, j, "cal_sets = %d", cal_sets);
    j += txtpar.font_height;
    gprintf(i, j, "cal_attn = %.0f", cal_attn);
    j += txtpar.font_height;
}

/*............................ CAL READ .................................*/

int
cal_read(char *fn, int icav)
{
    if (tok_read(fn) == 0) {
	calfmt = 1;
    } else if (mat_read(fn, icav) == 0) {
	calfmt = 2;
    } else {
        if (_access(fn, 0) == 0) {
	    decide(0, 2, "Not a valid DPOAE calibration file", fn);
	}
        calfmt = 0;
    }
    acc_sets = cal_sets;
    calcav = icav + 1;
    return (calfmt);
}

/*............................ CAL SAVE .................................*/

static int matmod = 1;

int
cal_save(char *fn, int icav)
{
    if (matmod)
	return (mat_save(fn, icav));

    return (tok_save(fn));
}
void
cal_update(char *fn, int nadc)
{
    if (matmod)
	mat_update(fn, nadc);
}

/******************************* THEV *************************************/

static char *src_thv = "SRC_THV";
static char *ld_thv = "LD_THV";

int
thev_src_sav(char *tfn, 
    float *zs1, float *ps1, float *px1, float *lc1, 
    float *zs2, float *ps2, float *px2, float *lc2, 
    int nf, int nc, double dc, double tc, double at, short sets)
{
    double df;
    int    fd;
    
    fd = _open(tfn, OFLAG, PMODE);    /* open the file */
    if (fd == -1) {
        return (1);
    }
    df = (double) rate / buflen;
    mat_wr1_s(fd, src_thv, (short) nf);
    mat_wr1_d(fd, "rate", (double) rate);
    mat_wr1_d(fd, "df", df);
    mat_wr1_d(fd, "dc", dc);
    mat_wr1_d(fd, "tc", tc);
    mat_wr_cf2d(fd, "zs1", zs1, nf);
    mat_wr_cf2d(fd, "zs2", zs2, nf);
    mat_wr_cf2d(fd, "ps1", ps1, nf);
    mat_wr_cf2d(fd, "ps2", ps2, nf);
    mat_wr_cf2d(fd, "px1", px1, nf);
    mat_wr_cf2d(fd, "px2", px2, nf);
    mat_wr(fd, "lc1", lc1, nc, 1, 0, 0);
    mat_wr(fd, "lc2", lc2, nc, 1, 0, 0);
    mat_wr1_d(fd, "attn", at);
    mat_wr1_s(fd, "sets", sets);
    _close(fd);

    return (0);
}

int
thev_src_read(char *fn, 
	      float *zs1, float *ps1, float *px1, 
	      float *zs2, float *ps2, float *px2)
{
    char    nam[80];
    double  df, dc, tc, attn, rt = 0;
    float   lc[MAXNCAV];
    int     fd, status = 0;
    short   nf, sets;

    cal_sets = 2;
    cal_attn = 20;
    fd = _open(fn, OPMOD, PMODE);
    if (fd < 0)
	return (0);
    if (mat_rd_s(fd, nam, &nf, 1) 
	&& (strcmp(src_thv, nam) == 0)
        && mat_rd_d(fd, nam, &rt, 1)
	&& mat_rd_d(fd, nam, &df, 1)
	&& mat_rd_d(fd, nam, &dc, 1)
	&& mat_rd_d(fd, nam, &tc, 1)
	&& mat_rd_cd2f(fd, nam, zs1, nf)
	&& mat_rd_cd2f(fd, nam, zs2, nf)
	&& mat_rd_cd2f(fd, nam, ps1, nf)
	&& mat_rd_cd2f(fd, nam, ps2, nf)
	&& mat_rd_cd2f(fd, nam, px1, nf)
	&& mat_rd_cd2f(fd, nam, px2, nf)
	&& mat_rd_f(fd, nam, lc, MAXNCAV)
	&& mat_rd_f(fd, nam, lc, MAXNCAV)) {
	status = nf;
    }
    if (mat_rd_d(fd, nam, &attn, 1)) {
        cal_attn = (float) attn;
    }
    if (mat_rd_s(fd, nam, &sets, 1)) {
        cal_sets = sets;
    }
    _close(fd);
    rate = (int32_t) rt;

    return (status);
}

int
thev_ld_sav(char *tfn, float *z0, 
    float *zl1, float *pl1, float *pr1,
    float *zl2, float *pl2, float *pr2,
    int nf, double at, short sets)
{
    double df;
    int    fd;
    
    fd = _open(tfn, OFLAG, PMODE);    /* open the file */
    if (fd == -1) {
        return (1);
    }
    df = (double) rate / buflen;
    mat_wr1_s(fd, ld_thv, (short) nf);
    mat_wr1_d(fd, "rate", (double) rate);
    mat_wr1_d(fd, "df", df);
    mat_wr_cf2d(fd, "z0", z0, nf);
    mat_wr_cf2d(fd, "zl1", zl1, nf);
    mat_wr_cf2d(fd, "pl1", pl1, nf);
    mat_wr_cf2d(fd, "pr1", pr1, nf);
    mat_wr_cf2d(fd, "zl2", zl2, nf);
    mat_wr_cf2d(fd, "pl2", pl2, nf);
    mat_wr_cf2d(fd, "pr2", pr2, nf);
    mat_wr1_d(fd, "attn", at);
    mat_wr1_s(fd, "sets", sets);
    _close(fd);

    return (0);
}

int
thev_ld_read(char *fn, float *z0,
    float *zl1, float *pl1, float *pr1,
    float *zl2, float *pl2, float *pr2,
    int n)
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
        && mat_rd_cd2f(fd, nam, z0, n)
        && mat_rd_cd2f(fd, nam, zl1, n)
        && mat_rd_cd2f(fd, nam, pl1, n)
        && mat_rd_cd2f(fd, nam, pr1, n)
        && mat_rd_cd2f(fd, nam, zl2, n)
        && mat_rd_cd2f(fd, nam, pl2, n)
        && mat_rd_cd2f(fd, nam, pr2, n))
        status = nf;
    _close(fd);
    rate = (int32_t) rt;

    return (status);
}

/****************************************************************************/

int
mptrans_open(char *fn, int *n)
{
    char    type[4], nam[80];
    int     fd, dsiz, dtyp, np = 0;
    int32_t    hdr[5];

    fd = _open(fn, OPMOD, PMODE);    /* open the file */
    if (fd > 0) {
        while(mat_rd_hdr(fd, type, hdr, nam)) {
	    dtyp = type[2];
	    //text = type[3];
	    dsiz = hdr[1] * hdr[2];
	    if (strcmp(nam, "resp") == 0 && dtyp == 1) {
	        np = dsiz;
	    } else {	    // skip over unknown variables
	        _lseek(fd, bsiz[dtyp] * dsiz, SEEK_CUR);
	    }
	}
    }
    if (fd > 0 && n > 0) {
        _lseek(fd, 0L, 0);	// rewind
    } else {
	_close(fd);
    }
    *n = np;
    return (fd);
}

int
mptrans_read(int fd, float *fr, float *mg, float *ph)
{
    char    type[4], nam[80];
    double  fmax, df, sens_mp = Sys.MP;
    float   *r = NULL;
    int     i, ii, ir, nf, dsiz, dtyp, n = 0;
    int32_t    hdr[5];

    if (fd > 0) {
        while(mat_rd_hdr(fd, type, hdr, nam)) {
	    dtyp = type[2];
	    //text = type[3];
	    dsiz = hdr[1] * hdr[2];
	    if (strcmp(nam, "resp") == 0 && dtyp == 1) {
	        n = dsiz;
	        r = (float *) calloc(n + 2, sizeof(float));
	        _read(fd, r, n * sizeof(float));
	    } else if (strcmp(nam, "fmax") == 0 && dtyp == 0 && dsiz == 1) {
	        _read(fd, &fmax, bsiz[dtyp]);
	    } else if (strcmp(nam, "sens_mp") == 0 && dtyp == 0 && dsiz == 1) {
	        _read(fd, &sens_mp, bsiz[dtyp]);
		Sys.MP = (float) sens_mp;
	    } else {	    // skip over unknown variables
	        _lseek(fd, bsiz[dtyp] * dsiz, SEEK_CUR);
	    }
	}
	if (n && r) {
	    nf = n / 2;
	    df = fmax / nf;
	    fast(r, n);
	    for (i = 0; i <= nf; i++) {
		ir = i * 2;
		ii = i * 2 + 1;
		fr[i] = (float) (i * df);
		mg[i] = (float) (_hypot(r[ir], r[ii]) * sens_mp);
                ph[i] = (float) atan2(r[ii], r[ir]);
	    }
	}
    }
    _close(fd);
    return (n);
}

/****************************************************************************/
