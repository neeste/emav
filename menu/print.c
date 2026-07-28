/* print.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#include "menu.h"
#ifdef _MSC_CODE
#include <dos.h>
#include <bios.h>
#endif /* _MSC_CODE */

#define MAXLINE		400
#define MAXPATH		256
#define MAXTOK	        80

static char  label[MAXTOK] = "Boys Town National Research Hospital";
static char  pr_port[MAXPATH] = "screen.ps";
static char *pgm_nam = "BTNRH EmAv";
static char *title = NULL;
static int xpix, ypix;
static TOGGLETYPE prntypes = {
    2,
    {"PostScript", "PCL", "Color PostScript", NULL},
};
static TOGGLETYPE orient = {
    1,
    {"Landscape", "Portrait", NULL},
};
static MENUITEM prn_opt[] = {
    {"Printer &Port :", pr_port, STRING, 80, 0, 1, NULL},
    {"Printer &Type =", (char *) &prntypes, TOGGLE, 0, 0, 1, NULL},
    {"&Orientation =", (char *) &orient, TOGGLE, 0, 0, 1, NULL},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

int
prn_setup(char *no_use)
{
    return (simple_submenu(menu_step(3), txtpar.menu_height * 4, prn_opt));
}

static int
prn_status()
{
    return (0);
}

static int
check_sys_call()
{
    int     i, ret;

    ret = 0;
    for (i = 0; i < (int) strlen(pr_port); i++) {
	if (pr_port[i] == '!') {
	    ret = i + 1;
	    break;
	}
    }
    return (ret);
}

static void
display_label()
{
    int     fg, bg, wid;

    if (label[0] == 0)
	return;
    rd_text_color(&fg, &bg);
    text_color(txtpar.cbg, txtpar.cbg);
    mainmenu_clear();
    wid = txtpar.text_wind_len;
    text_color(menucolor.afg, txtpar.cbg);
    txtpar.text_wind_len = strlen(label) + 1;
    gprintf(4, txtpar.font_height, label);
    text_color(fg, bg);
    txtpar.text_wind_len = wid;
}

static void
erase_label()
{
    gr_rectf(4, 0, 
	strlen(label) * txtpar.font_width + 4, txtpar.font_height - 1, 
	txtpar.cbg);
}

static void
colorps_pr(char *name)
{
    char *bb;
    int     i, j, xx, yy, c;
    time_t  tt;
    FILE   *fpt;
    static char *fgco[8]= {
	"000", "009", "080", "064", "900", "606", "840", "333"
    };

    fpt = fopen(name, "wt");
    if (fpt == NULL)
        return;
    xx = xpix;
    yy = ypix;
    tt = time(&tt);
    bb = orient.at ? "0 324 612 792" : "0 0 612 792";
    fprintf(fpt, "%%!PS-Adobe-2.0 EPSF-2.0\n");
    fprintf(fpt, "%%%%Creator: %s\n", pgm_nam);
    fprintf(fpt, "%%%%CreationDate: %s", ctime(&tt));
    fprintf(fpt, "%%%%Title: %s\n", title ? title : "");
    fprintf(fpt, "%%%%BoundingBox: %s\n", bb);
    fprintf(fpt, "%%%%Pages 1\n");
    fprintf(fpt, "%%%%EndComments\n");
    fprintf(fpt, "%%%%Page: 1 1\n");
    fprintf(fpt, "/picstr %d string def\n", ypix);
    if (orient.at) {
	fprintf(fpt, "606 324 translate 90 rotate 0.75 0.75 scale\n");
    }
    fprintf(fpt, "36 36 moveto currentpoint translate 540 720 scale\n");
    fprintf(fpt, "%d %d 4 ", yy, xx);		    // dimensions of source image
    fprintf(fpt, "[%d 0 0 %d 0 %d] ", yy, -xx, xx); // map unit square to source
    fprintf(fpt, "{currentfile picstr readhexstring pop}\n");
    fprintf(fpt, "false 3 colorimage\n");
    for (i = 0; i < xx; i++) {
	for (j = yy - 1; j >= 0; j--) {
	    c = gr_getpix(i, j);
	    if (c < 8) {
		fputs("FFF", fpt);
	    } else {
		fputs(fgco[c & 7], fpt);
	    }
	}
	fputs("\n", fpt);
    }
    fputs("showpage\n", fpt);
    fclose(fpt);
}

static void
postscript_pr(char *name)
{
    char *bb;
    int     i, j, xx, yy;
    time_t  tt;
    FILE   *fpt;

    fpt = fopen(name, "wt");
    if (fpt == NULL)
        return;
    xx = xpix;
    yy = ypix;
    tt = time(&tt);
    bb = orient.at ? "0 324 612 792" : "0 0 612 792";
    fprintf(fpt, "%%!PS-Adobe-2.0 EPSF-2.0\n");
    fprintf(fpt, "%%%%Creator: %s\n", pgm_nam);
    fprintf(fpt, "%%%%CreationDate: %s", ctime(&tt));
    fprintf(fpt, "%%%%Title: %s\n", title ? title : "");
    fprintf(fpt, "%%%%BoundingBox: %s\n", bb);
    fprintf(fpt, "%%%%Pages 1\n");
    fprintf(fpt, "%%%%EndComments\n");
    fprintf(fpt, "%%%%Page: 1 1\n");
    fprintf(fpt, "/picstr %d string def\n", ypix);
    if (orient.at) {
	fprintf(fpt, "606 324 translate 90 rotate 0.75 0.75 scale\n");
    }
    fprintf(fpt, "36 36 moveto currentpoint translate 540 720 scale\n");
    fprintf(fpt, "%d %d 4 ", yy, xx);		    // dimensions of source image
    fprintf(fpt, "[%d 0 0 %d 0 %d] ", yy, -xx, xx); // map unit square to source
    fprintf(fpt, "{currentfile picstr readhexstring pop}\n");
    fprintf(fpt, "image\n");
    for (i = 0; i < xx; i++) {
	for (j = yy - 1; j >= 0; j--) {
	    fputs((gr_getpix(i, j) < 8) ? "F" : "0", fpt);  // white or black
	}
	fputs("\n", fpt);
    }
    fputs("showpage\n", fpt);
    fclose(fpt);
}

#ifdef _MSC_CODE
static void
set_binary(int fh)
{
    union _REGS r;

    r.h.ah = 0x44;		/* DOS function 44 */
    r.h.al = 0x00;		/* sub function 0 = get info */
    r.x.bx = fh;		/* file handle */
    (void) _intdos(&r, &r);
    if (r.h.dl & 0x80) {	/* is this a character device ? */
	r.h.dh = 0;		/* make sure this is zero */
	r.h.dl |= 0x20;		/* set raw mode */
	r.h.ah = 0x44;		/* DOS function 44 */
	r.h.al = 0x01;		/* sub function 0 = set info */
	r.x.bx = fh;		/* file handle */
	(void) _intdos(&r, &r);
    }
    (void) _setmode(fh, (int) _O_BINARY);	/* do binary output */
}
#endif /* _MSC_CODE */

static int
pcl_resolution(double xres, double yres)
{
    int r;

    if (xres > yres)
	r = (int) xres;
    else
	r = (int) yres;
    if (r < 75)
	r = 75;
    else if (r < 150)
	r = 150;
    else
	r = 300;

    return (r);
}

static void
pcl_pr(char *name)
{
    FILE   *fpt;
    int     i, j, k, r, m1, m2;
    int     nl;
    char   *scan_line;
    static unsigned char m[8] = {
    	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
    };

    fpt = fopen(name, "wb");
    if (fpt == NULL)
        return;
#ifdef _MSC_CODE
    set_binary(_fileno(fpt));
#endif /* _MSC_CODE */
    fprintf(fpt, "\033E");	/* reset */
    fprintf(fpt, "\033&l0E");	/* top margin = 0 */
    if (orient.at == 0) {
	r = pcl_resolution(xpix / 10.5, ypix / 8.0);
	m1 = (int) (0.5 * (11. - xpix / (double) r) * 720); // vert. margin (dp)
	m2 = (int) (0.5 * (8.5 - ypix / (double) r) * 720); // horz. margin (dp)
        fprintf(fpt, "\033&a%dV", m1);	/* move cursor vert. */
	fprintf(fpt, "\033&a%dH", m2);	/* move cursor horz. */
	fprintf(fpt, "\033*t%dR", r);	/* resolution (dpi) */
	fprintf(fpt, "\033*r1A");	/* start graphics at cursor  */
	nl = (ypix + 7) / 8;
	scan_line = (char *) malloc(nl);
	for (i = 0; i < xpix; i++) {
	    for (k = 0; k < nl; k++)
		scan_line[k] = 0;
	    for (j = 0; j < ypix; j++) {
		if (gr_getpix(i, ypix - 1 - j) >= 8)
		    scan_line[j / 8] |= m[j % 8];
	    }
	    fprintf(fpt, "\033*b%dW", nl);
	    for (k = 0; k < nl; k++)
		putc(scan_line[k], fpt);
	}
    } else {
	r = pcl_resolution(xpix / 8.0, ypix / 10.5);
	m1 = 720;                                  // = 1-inch vert. margin (dp)
	m2 = (int) (0.5 * (8.5 - xpix / (double) r) * 720); // horz. margin (dp)
        fprintf(fpt, "\033&a%dV", m1);	/* move cursor vert. */
	fprintf(fpt, "\033&a%dH", m2);	/* move cursor horz. */
	fprintf(fpt, "\033*t%dR", r);	/* resolution (dpi) */
	fprintf(fpt, "\033*t150R");	/* resolution = 150 dpi */
	fprintf(fpt, "\033*r1A");	/* start graphics at cursor  */
	nl = (xpix + 7) / 8;
	scan_line = (char *) malloc(nl);
	for (i = 0; i < ypix; i++) {
	    for (k = 0; k < nl; k++)
		scan_line[k] = 0;
	    for (j = 0; j < xpix; j++) {
		if (gr_getpix(j, i) >= 8)
		    scan_line[j / 8] |= m[j % 8];
	    }
	    fprintf(fpt, "\033*b%dW", nl);
	    for (k = 0; k < nl; k++)
		putc(scan_line[k], fpt);
	}
    }
    fprintf(fpt, "\033E");	/* reset */
    free(scan_line);
    fclose(fpt);
}

void
prn_screen(void)
{
    char *name, *line = NULL;
#ifdef linux
    char tmpnam[256];
#endif /* linux */
    int ps, sysflg;
    static char *prn_stat[] = {
        "Ready"
	"Not ready or busy",
	"I/O error",
	"Out of paper",
    };

    ps = prn_status();
    if (ps) {
	(void) decide(0, 3, "Printer Port", pr_port, prn_stat[ps]);
	return;
    }
    display_label();
    check_event();
    sysflg = check_sys_call();
    if (sysflg == 0) {
        name = pr_port;
    } else {
#ifdef linux
	strcpy(tmpnam, "/tmp/prnscrXXXXXX");
	mkstemp(tmpnam);
	name = tmpnam;
#else /* linux */
        name = tmpnam(NULL);
#endif /* linux */
        line = (char *) malloc(MAXLINE);
	strcpy(line, (char *) pr_port + sysflg);
	strcat(line, " ");
	strcat(line, name);
	strcat(line, " > nul");
    }
    gr_scrsiz(&xpix, &ypix);		/* get screen size */
#ifdef linux
    gr_savpix();
#endif /* linux */
    if (prntypes.at == 0) {
        postscript_pr(name);
    } else if (prntypes.at == 1) {
        pcl_pr(name);
    } else {
        colorps_pr(name);
    }
    erase_label();
    if (sysflg) {
	(void) system(line);
	(void) remove(name);
	free(line);
    }
    gr_beep();
    (void) mainmenu_show(NULL);
}

int
prn_screen_select(char *no_use)
{
    set_trailer(prn_screen);
    return (27);
}

void
set_prn_label(char *s)
{
    strncpy(label, s, MAXTOK);
    label[MAXTOK - 1] = '\0';
}

void
set_prn_port(char *s)
{
    strncpy(pr_port, s, MAXPATH);
    pr_port[MAXPATH - 1] = '\0';
}

void
set_prn_orient(int o)
{
    orient.at = o;
}

void
set_prn_title(char *t)
{
    title = t;
}

void
set_prn_type(int t)
{
    prntypes.at = t;
}
