/* sfextr.c - extract SFOAE from DPOAE DAT file */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <menu.h>
#include <util.h>
#include "emav.h"
#include "savage.h"

#define MAXARG 128
#define MAXLIN 512

typedef struct {double x, y;} complex;

extern int dp_items, dp_repeat, sig_type, nnsb;

static char    line[MAXLIN];
static char    w[MAXARG];
static char    d[MAXARG];
static double  v[MAXARG];
static double  pi = 3.1415927;
static int     accnpts = 0;
static FILE *fbin = NULL;

static int
scan_line(char *s, int n)
{
    char *b, *p;
    int i;

    for (i = 0; *s >= ' ' && i < n; i++) {
	b = s;
	while (*s == ' ')
	    s++;
	v[i] = atof(s);
	p = NULL;
	while (*s > ' ') {
	    if (*s == '.')
		p = s + 1;
	    s++;
	}
	w[i] = (char)((int) (s - b));
	d[i] = (char)(p ? (int) (s - p) : 0);
    }
    return (i);
}

static void
open_bin(char *fn)
{
    char *bfn = line;
    struct BINhdr bhdr;

    strcpy(bfn, fn);
    newext(bfn, "BIN");
    fbin = fopen(bfn, "rb");
    if (fbin != NULL) {
	fread(&bhdr, sizeof(struct BINhdr), (size_t)(1), fbin);
	dp_items = bhdr.total;
	dp_repeat = 1;
	buflen = bhdr.wav_len;
	rate = (int32_t) bhdr.samp_rate;
	Sen.AD = 1 / bhdr.int2volt;
	Sen.MP = 1 / bhdr.volt2pas;
	if (dp_items <= 0 || buflen <= 0 || rate <= 0) {
	    fclose(fbin);
	    fbin = NULL;
	} else {
	    change_nic(bhdr.nic);
	    accnpts = (bhdr.nic < 2) ? buflen : buflen * 2;
	    swp1set = bhdr.swp1set;
	}
    }
}

static void
read_bin(int ild, int ipd, int itm, int chn)
{
    double  f1, f2, fd;
    float   pdp[1], ndp[1], adp[1];
    int32_t    wavsiz;
    struct BINwav wavhdr;

    wavsiz = sizeof(struct BINwav) + 2 * 4 * accnpts;
    fseek(fbin, sizeof(struct BINhdr) + (int32_t) itm * wavsiz, 0);
    fread(&wavhdr, sizeof(struct BINwav), (size_t)(1), fbin);
    fread(accbuf_a, 4, (size_t)(accnpts), fbin);
    fread(accbuf_b, 4, (size_t)(accnpts), fbin);
    acc_sets = wavhdr.N / swp1set;
    f1 = wavhdr.f1;
    f2 = wavhdr.f2;
    fd = dpfreq(f1, f2);
    fft_A_B(accbuf_a, accbuf_b, 1, acc_sets, 1, chn);
    get_level(fd, nnsb, sig_type, f2 - f1, pdp, ndp, adp);
    v[ild] = pdp[0];
    v[ipd] = adp[0];
}

static void
close_bin()
{
    if (fbin) {
	fclose(fbin);
	fbin = NULL;
    }
}

static complex
vec_dat(int ild, int ipd, int itm)
{
    complex u;
    double a, p;

    if (fbin)
	read_bin(ild, ipd, itm, 0);
    a = pow(10.0, v[ild] / 20);
    p = v[ipd] * (pi / 180);
    u.x = a * cos(p);
    u.y = a * sin(p);

    return (u);
}

static double
pmean(double a, double b)
{
    return (10 * log10((pow(10.0, a / 10) + pow(10.0, b / 10)) / 2)); 
}

int
dp2sf(char *ifn, char *ofn, char *msg)
{
    double a, n1 = 0, n2 = 0;
    int   i, n, count = 0;
    complex u = {0,0}, s = {0,0};
    FILE *ifp, *ofp;
    static int ild = 5, ind = 6, ipd = 7;

    if (toupper(ifn[5]) != 'D') {
	strcpy(msg, "Not a DPOAE file.");
	return (0);
    }
    ifp = fopen(ifn, "r");
    if (ifp == NULL) {
	strcpy(msg, "Can't open input file.");
	return (0);
    }
    strcpy(ofn, ifn);
    ofn[5] = 'S';
    ofp = fopen(ofn, "w");
    if (ofp == NULL) {
	strcpy(msg, "Can't open output file.");
	return (0);
    }
    open_bin(ifn);
    count = 0;
    while (fgets(line, MAXLIN, ifp) != NULL) {
	if (line[0] == ';') {
	    if (strncmp(line, ";Items=", 7) == 0) {
		i = atoi(line + 7);
	        fprintf(ofp, ";Items=%d\n", i / 2);
	    } else if (strncmp(line + 41, "2*F1-F2", 7) == 0) {
	        strncpy(line + 41, "SFOAE  ", 8);
	        fputs(line, ofp);
	    } else {
	        fputs(line, ofp);
	    }
	} else {
	    n = scan_line(line, MAXARG);
	    if (v[0] != v[1]) {
	        fputs(line, stderr);
                fprintf(stderr, "v[0]=%.6g v[1]=%.6g\n", v[0], v[1]);
		fclose(ofp);
		remove(ofn);
		strcpy(msg, "Error: f1 != f2.");
		return (0);
	    }
	    // assumes first stimulus condition has NO suppressor
	    if ((count % 2) == 0) {	// without suppressor
		u = vec_dat(ild, ipd, count);
		n1 = v[ind];
	    } else {	// with suppressor
		s = vec_dat(ild, ipd, count);
		s.x = u.x - s.x;
		s.y = u.y - s.y;
		a = s.x * s.x + s.y * s.y;
		if (a < 1e-40) {
		    v[ild] = -400; 
		    v[ipd] = 0;
		} else {
		    v[ild] = 10 * log10(a); 
		    v[ipd] = atan2(s.y, s.x) * (180 / pi);
		}
		n2 = v[ind];
		v[ind] = pmean(n1, n2);
		for (i = 0; i < n; i++) {
		    fprintf(ofp, "%*.*f", w[i], d[i], v[i]);
		}
	        fprintf(ofp, "\n");
	    }
	    count++;
	}
    }
    close_bin();
    fclose(ifp);
    fclose(ofp);
    return (1);
}
