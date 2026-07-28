/* file.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <dsp.h>
#include <menu.h>
#include <util.h>
#include "savage.h"
#include "color.h"
#include "screen.h"
#include "putt.h"

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#define _access access
#endif /* WIN32 */

#define limit(min,val,max)  ((val<min)?min:((val>max)?max:val))

char   *atline();
float   spl2hl(float, float);
int     count_files(void);
int     check_list(char *, char **);
int     dis_patient(void);
int	dis_stim(int load, void *p, enum TYPE type);
int	display_version(void);
int     file_items(char *);
int 	go_on(char*);
int     printer_ready(void);
int     savedir(char **);
int     thev_source(char *, char *);
void    take_files_bwt(int, int, char *);
void	cal_quit();
void    cal_pati(PATIENT *);
void    cal_read(int);
void    cal_close();
void	check_data_type();
void	data_trailer();
void    initial();
void    out_msg(int, char[][16], int);
void    set_binary(int);
void    show_cond(char *, int);
void    thev_reload(char *, char *, int);

extern char tokstr[], line[], exedir[];
extern char thsfn[], ite1fn[], ite2fn[];
extern double cal_ppc, cal_vpc;
extern int dfgc, sig_type, rclflg, ncfn;
extern int swp1set, hear_tth_cnt;
extern TOGGLETYPE levref, thresh_cnct, cond_thl;
extern WIND w_stim_fft;

int
Quit_wind(char *no_use)
{
    extern char tmpbatchfile[];

    cal_quit();
    if (tmpbatchfile[0]) remove(tmpbatchfile);
    end_gr();
    dsprst(TRUE);	/* Assert the DSP reset	 */
    pgm_exit();
    return (0);
}

static char ofiledir[120] = {0};
static char *sf[] = {"*.TTH", "*.CAL"};
static void (*rd_and_show[3]) () = {
    rd_hearing_file, rd_cal_file
};

static TOGGLETYPE filetypes = {
    0,
    { "HEARING", "CALIBRATION", NULL }
};

int     fileflag = 0;
static int open_end = 0;

int
savefile(char *no_use)
{
    FILE   *dfp, *dfpi;
    int     renflg = 0;
    char    tmpfn[16];
    static char *kw[] = {
	"LastName", "FirstName", "Ear", "ID", "Comment", "DOB", 
	"Thev_source", ""
    };

    if (fileflag) {
	if (!strcmp(file_name, "Not Saved")) {
	    strcpy(tmpfn, "OUT.DAT");
	    genfn("H", "TTH", file_name, &hear_tth_cnt);
	    hear_tth_cnt++;
	    write_counter_file();
	} else {
	    genfn("@", "$&$", tmpfn, &renflg);
	    rename(file_name, tmpfn);
	    renflg = 1;
	}
	dfpi = fopen(tmpfn, "rt");
	if (dfpi == NULL) {
	    if (renflg)
		rename(tmpfn, file_name);
	    return (0);
	}
	dfp = fopen(file_name, "wt");
	if (dfp == NULL) {
	    decide(0, 2, "Can't open file",
		"Press any key or mouse to continue");
	    fclose(dfpi);
	    if (renflg)
		rename(tmpfn, file_name);
	    return (0);
	}
	fgets(line, MAXLINE, dfpi);
	get_token(1);
	if (strcmp(tokstr, ";HEARING")) {
	    decide(0, 3, "Not a HEARING file",
		";HEARING signature missing",
		"Press any key or mouse to continue");
	    fclose(dfp);
	    fclose(dfpi);
	    if (renflg)
		rename(tmpfn, file_name);
	    return (0);
	}
	fputs(line, dfp);
	fprintf(dfp, ";LastName:  %s\n;FirstName: %s\n",
	    patient.Lastname, patient.Firstname);
	fprintf(dfp, ";Ear: %5s\n;ID: %s\n", patient.Ear == 'R' ?
	    "Right" : (patient.Ear == 'L' ? "Left" : "?"), patient.ID);
	fprintf(dfp, ";DoB: %s\n", patient.Dob);
	fprintf(dfp, ";Comment: %s\n", patient.Comment);
        if (*thsfn)
            fprintf(dfp, ";Thev_source=%s\n", thsfn);
	while (fgets(line, MAXLINE, dfpi) != NULL) {
	    if (line[0] != ';') {
		fputs(line, dfp);
	    } else {
		line[0] = ' ';
		get_token(1);
		if (!check_list(tokstr, kw)) {
		    line[0] = ';';
		    fprintf(dfp, "%s", line);
		}
	    }
	}
	fclose(dfpi);
	fclose(dfp);
	remove(tmpfn);
    }
    decide(0, 2, "File is saved in", file_name);
    return (1);
}

int
Start_New()
{
    defpar();
    read_init_file();
    read_args();
    Sen = Sys;
    dis_units(Sen.AD, Sen.MP);
    patient.Comment[0] = 0;
    patient.Lastname[0] = 0;
    patient.Firstname[0] = 0;
    patient.ID[0] = 0;
    patient.Dob[0] = 0;
    cali_file[0] = 0;
    mainmenu_exit();
    set_trailer(NULL);
    return(27);
}

MENUITEM file_opt[] = {
    {"&Open...", NULL, NONE, 0, 0, 1, file_items},
    {"&New", NULL, NONE, 0, 0, 1, Start_New },
    {"&Print Screen ( ^W )", NULL, NONE, 0, 0, 1, prn_screen_select},
    {"P&rinter Setup...", NULL, NONE, 0, 0, 1, prn_setup},
    {"File &Save", NULL, NONE, 0, 0, 0, savefile},
    {"&About PUTT... ( F1 )", NULL, NONE, 0, 0, 1, display_version},
    {"E&xit ( ^C )", NULL, NONE, 0, 0, 1, Quit_wind},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

int
toggle_type(char **s)
{
    strcpy(o_file_name, ofiledir);
    strcat(o_file_name, sf[filetypes.at]);
    return (0);
}

MENUITEM sub_file[] = {
    {"&Open File", NULL, NONE, 0, 0, 1, open_file},
    {"File &Type =", (char *) &filetypes, TOGGLE, 0, 0, 1, toggle_type},
    {"File &Name :=", o_file_name, STRING, 127, 0, 1, savedir},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

/* split the complete path name into drive, dir, name & ext */
void
split_path(char *s, char *drv, char *dir, char *nam, char *ext)
{
#ifdef WIN32
    _splitpath(s, drv, dir, nam, ext);
#else 
    if (strchr(s, ':')) {
	do {
	    *drv++ = *s++;
	} while (*s != ':');
    }
    *drv = '\0'; 
    if (strchr(s, '/') || strchr(s, '\\')) {
	do {
	    *dir++ = *s++;
	} while (strchr(s, '/') || strchr(s, '\\'));
    }
    *dir = '\0'; 
    while (*s && *s != '.') {
	*nam++ = *s++;
    }
    *nam = '\0'; 
    while (*s) {
	*ext++ = *s++;
    }
    *ext = '\0'; 
#endif
}

int
savedir(char **s)
{
    struct PATH path;

    split_path(*s, path.drive, path.dir, path.name, path.ext);
    strcpy(ofiledir, path.drive);
    strcat(ofiledir, path.dir);
    return (0);
}

void
file_trailer(void)
{
    int c;
    
    strcpy(o_file_name, ofiledir);
    strcat(o_file_name, sf[filetypes.at]);
    open_end = txtpar.menu_height * (3 + num_items(sub_file, &c));
    simple_submenu(menu_step(2), txtpar.menu_height * 3, sub_file);
}

int
file_items(char *no_use)
{
    set_trailer(file_trailer);
    return (27);
}

int
File_wind()
{
    return (simple_submenu(0, txtpar.menu_height, file_opt));
}

#define ATTRIB (_A_NORMAL | _A_RDONLY | _A_HIDDEN)

int
eventintime()
{
    int32_t    timer;
    int     rv;

    timer = clock();
    timer += CLOCKS_PER_SEC / 3;
    rv = 0;
    while (clock() < timer) {
	rv = check_event();
	if (rv)
	    break;
    }
    return (rv);
}

int
go_on(char *nouse)
{
	return(27);
}

int
open_file(char *use2chk)
{
    WIND    w;
    FILE   *fpt;
    int     saved, i, j, k, nf, tmp, acpt, end, rc, x, y, cw;
    int     xx = 0, yy = 0, hold, status, mx, my;
    PATIENT P = {"", "", "", 0, "", ""};
    char   *files, *beg, fn[MAXNAME];
    static int nrow = 15;
    static int start = 0, ptr = 0;
    static char lastfn[MAXNAME] = {0};
    static char *kw[] = {
	"LastName", "FirstName", "Ear", "ID", "DOB", "Comment", 
	""
    };

    file_opt[0].inputlen = 0;
    nf = count_files();
    if (nf == 0) {
	decide(0, 2, "Warning, File not found!",
	    "Press any key or mouse to continue");
	return ('N');
    }
    if (strcmp(lastfn, o_file_name)) {
	ptr = start = 0;
	strcpy(lastfn, o_file_name);
    }
    end = limit(1, start + nrow, nf);
    files = (char *) malloc((ncfn + 1) * (end - start));
    if (files == NULL) {
	decide(0, 2, "Error allocating space for filename holder",
	    "Press any key or mouse to continue");
	return (0);
    }
    take_files_bwt(start, end, files);
//    k = nrow;
    cw = txtpar.font_width;
    if (open_end == 0) {
        open_end = txtpar.menu_height * 6;
    }
    w.xtop = menu_step(2);
    w.xbot = w.xtop + (ncfn + 3) * cw;
    w.ytop = open_end + 2;
    w.ybot = w.ytop + nrow * txtpar.font_height;
    wind_color(&w, menucolor.afg, menucolor.abg);
    saved = save_w(&w);
    x = draw_w(&w) + cw;
    j = w.ytop + txtpar.font_height;
    hold = 0;
    if (use2chk != NULL) {
	xx = w.xbot;
	yy = w.ybot;
	w.xtop = xx + 20;
	w.xbot = w.xtop + 28 * cw;
	w.ybot = w.ytop + 5 * txtpar.font_height;
	wind_color(&w, menucolor.mfg, menucolor.mbg);
	hold = save_w(&w);
	draw_w(&w);
	w.xbot = xx;
	w.ybot = yy;
	xx = w.xtop + 4;
	yy = j;
	w.xtop = menu_step(2);
    }
    do {
	for (k = start; k < end; k++) {
	    if (k == ptr)
		text_color(menucolor.mfg, menucolor.mbg);
	    else
		text_color(menucolor.afg, menucolor.abg);
	    tmp = k - start;
	    y = j + tmp * txtpar.font_height;
	    txtpar.text_wind_len = ncfn + 1;
	    gprintf(x, y, files + tmp * (ncfn + 1));
	}
	if (use2chk != NULL && !eventintime()) {
	    status = 0;
	    text_color(menucolor.mfg, menucolor.mbg);
	    strcpy(line, ofiledir);
	    strcat(line, files + (ptr - start) * (ncfn + 1));
	    trim(line);
	    if (filetypes.at == 0) {
	    	fpt = fopen(line, "rt");
	    	if (fpt == NULL) {
	    	    status = 1;
	    	} else {
	    	    fgets(line, MAXLINE, fpt);
	    	    get_token(1);
	    	    if (strcmp(tokstr, ";HEARING")) {
	    	        status = 5;
			fclose(fpt);
	    	    } else {
			status = 4;
			k = 0;
			while (fgets(line, MAXLINE, fpt) != NULL) {
			    if (line[0] == ';') {
				line[0] = ' ';
				get_token(1);
				beg = atline();
				i = check_list(tokstr, kw);
				switch (i) {
				case 1:	/* LastName */
				    strncpy(P.Lastname, beg, 12);
				    break;
				case 2:	/* FirstName */
				    strncpy(P.Firstname, beg, 11);
				    break;
				case 3:	/* Ear */
				    i = toupper(beg[0]);
				    if (i == 'L')
					P.Ear = 'L';
				    else if (i == 'R')
					P.Ear = 'R';
				    break;
				case 4:	/* ID */
				    strncpy(P.ID, beg, 11);
				    break;
				case 5:
				    strncpy(P.Dob, beg, 11);
				    break;
				case 6:	/* Comment */
				    if (!k) {
					strncpy(P.Comment, beg, 26);
					k = 1;
				    }
				    break;
				}
			    }
			}
			fclose(fpt);
		    }
		}
	    } else if (filetypes.at == 1) {
	    	strcpy(fn, line);
		if (!cal_open(fn, line, 0)) {
		    status = 3;
		} else {
		    status = 4;
		    cal_pati(&P);
		    cal_close();
		}
	    }
	    beg = (char *) line;
	    for (i = 0; i < 5; i++) {
                beg[0] = 0;
		fillinblank(beg, 26);
		beg += 27;
    	    }
	    switch (status) {
	    case 1:
		sprintf(line, "%-26s", "Can't open the file.");
		break;
	    case 2:
		sprintf(line, "%-26s", "Not a valid DPOAE file.");
		break;
	    case 3:
//		sprintf(line, "%-26s", "Not a valid TEOAE file.");
		break;
	    case 4:
		beg = (char *) line;
		sprintf(beg, "Last Name : %-14s", P.Lastname);
		beg += 27;
		sprintf(beg, "First Name: %-14s", P.Firstname);
		beg += 27;
		sprintf(beg, "Ear: %c, ID: %-14s", P.Ear, P.ID);
		beg += 27;
		sprintf(beg, "Birthdate: %-15s", P.Dob);
		beg += 27;
		sprintf(beg, "Comment: %-17s", P.Comment);
		break;
	    case 5:
	        sprintf(line, "%-26s", "Not a valid HEARING file");
	        break;
	    }
	    k = yy;
	    beg = (char *) line;
            txtpar.text_wind_len = 27;
	    for (i = 0; i < 5; i++) {
		fillinblank(beg, 26);
		gprintf(xx, k, "%s", beg);
		beg += 27;
		k += txtpar.font_height;
	    }
	}
	acpt = 0;
	tmp = getevent();
	if (pgm_done())
	    break;
	if (tmp == 3)
	    return (0);
	switch (tmp) {
	case 27:
	    acpt = -1;
	    break;
	case 9:
	    acpt = -2;
	    break;
	case SINGLE_CLICK:
	    mouse_position(&mx, &my);
	    if (mx > w.xtop && mx < w.xbot && my > w.ytop &&
		my < w.ybot) {
		tmp = start + (my - w.ytop) / txtpar.font_height;
		if (tmp < end) {
		    acpt = (ptr == tmp);
		    ptr = tmp;
		}
	    } else {
		acpt = -1;
	    }
	    break;
	case 13:		/* return */
	    acpt = 1;
	    break;
	case FN | 72:		/* arrow up */
	    if (ptr > start) {
		ptr--;
	    } else {
		start = limit(0, start - 1, nf - 1);
		end = limit(1, start + nrow, nf);
		ptr = start;
		take_files_bwt(start, end, files);
	    }
	    break;
	case FN | 80:		/* arrow down */
	    if (ptr < end - 1) {
		ptr++;
	    } else {
		end = limit(1, end + 1, nf);
		start = limit(0, end - nrow, nf - 1);
		ptr = end - 1;
		take_files_bwt(start, end, files);
	    }
	    break;
	case FN | 75:		/* arrow left */
	    break;
	case FN | 77:		/* arrow right */
	    break;
	case FN | 73:		/* page up */
	    if (ptr > start) {
		ptr = start;
	    } else {
		start = limit(0, start - nrow, nf - 1);
		end = limit(1, start + nrow, nf);
		ptr = start;
		take_files_bwt(start, end, files);
	    }
	    break;
	case FN | 117:		/* control end */
	    end = nf;
	    start = limit(0, end - nrow, nf);
	    ptr = end - 1;
            take_files_bwt(start, end, files);
            break;
	case FN | 81:		/* page down */
	    if (ptr < end - 1) {
		ptr = end - 1;
	    } else {
		end = limit(1, end + nrow, nf);
		start = limit(0, end - nrow, nf - 1);
		ptr = end - 1;
		take_files_bwt(start, end, files);
	    }
	    break;
	case FN | 119:		/* control home */
	    start = ptr = 0;
	    end = limit(1, start + nrow, nf);
            take_files_bwt(start, end, files);
	    break;
	case FN | 71:		/* home */
	    ptr = start;
	    break;
	case FN | 79:		/* end */
	    ptr = end - 1;
	    break;
	}
    } while (acpt == 0);

    if (acpt > 0) {
	strcpy(o_file_name, ofiledir);
	strcat(o_file_name, files + (ptr - start) * (ncfn + 1));
	trim(o_file_name);
        if (use2chk != NULL)
            set_trailer((void (*)(void)) rd_and_show[filetypes.at]);
	rc = 27;
    } else if (acpt == -2) {
	rc = 'N';
    } else {
	rc = 0;
    }
    if (hold) {
	yy = w.xtop;
	w.xtop = xx - 4;
	close_w(&w);
	w.xtop = yy;
    }
    if (saved)
	close_w(&w);
    free(files);
    return (rc);
}

void 
rd_cal_file()
{
    init_wind();
    thev_source(o_file_name, o_file_name);
}

int
check_cal_file(char *fn)
{
    if (!cal_open(fn, line, 0)) {
        return (0);
    }
    cal_close();
    return(1);
}

int
check_hearing_file(char *fn, int flag)
{
    FILE *fpt;

    fpt = fopen(fn, "rt");
    if (fpt == NULL) {
    	if(flag) {
            decide(0, 2, "Can't open file",
	        "Press any key or mouse to continue");
        }
	return(0);
    }
    fgets(line, MAXLINE, fpt);
    get_token(1);
    if (strcmp(tokstr, ";HEARING")) {
    	if(flag) {
    	    decide(0, 3, "Not a HEARING file",
	     ";HEARING signature missing", 
	     "Press any key or mouse to continue");
        }
        fclose(fpt);
        return(0);
    }
    fclose(fpt);
    return(1);
}

void
rd_hearing_file()
{
    int     i, j, k, cflg, swp, tim, cmt, otim, itecnt, iteacc, shwflg, datfmt;
    float   spl, sil, fpl, cond;
    float   f, fzer[2], fnot[2];
    char    outmsg[3][16] = {{0}};
    char   *start, msg[80];
    char    comment[3][40];
    FILE *fpt;
    static char otherfn[80] = {0};
    static char *kw[] = {
	"sweeps", "stimulus", "calibrate", "list",
	"LastName", "FirstName", "ID", "Ear", "DOB", "Comment",
	"FileName", "Date", "Elapsed_Time", "Accepted", 
	"Thev_Source", "Notch", "Fz1__", "Other_Ear", "DATfmt", ""
    };

    if (!check_hearing_file(o_file_name, 1)) {
        return;
    }
    fpt = fopen(o_file_name, "rt");
    init_wind();
    strcpy(file_name, o_file_name);
    o_file_name[0] = o_file_name[40] = 0;
    cflg = 0;
    datfmt = 1;
    itecnt = iteacc = shwflg = 0;
    swp = 0;
    tim = 0;
    otim = 0;
    cmt = -1;
    fnot[0] = fnot[1] = fzer[0] = fzer[1] = 0;
    ite1fn[0] = ite2fn[0] = otherfn[0] = '\0';
    initial();
    while (fgets(line, MAXLINE, fpt) != NULL) {
	if (line[0] == ';') {
	    line[0] = ' ';
	    get_token(1);
	    if (strlen(tokstr) == 0)
		cflg = 0;
	    start = atline();
	    k = check_list(tokstr, kw);
	    get_token(0);
	    switch (k) {
	    case 1:		/* sweeps */
		swp = atoi(tokstr);
		break;
	    case 2:		/* stimulus */
		stim_title(tokstr);
		break;
	    case 3:		/* calibrate */
                iteacc = (_access(tokstr, 0) == 0);
                if (iteacc) {
                    if (rclflg)
                        thev_reload(tokstr, thsfn, 0);
		    show_cond(tokstr, shwflg++);
		}
		if (++itecnt == 1)
                    strncpy(ite1fn, tokstr, 39);
		else if (itecnt == 2)
                    strncpy(ite2fn, tokstr, 39);
		break;
	    case 4:		/* list */
		strncpy(&o_file_name[40], tokstr, 39);
		break;
	    case 5:		/* LastName */
		strncpy(patient.Lastname, start, 12);
		break;
	    case 6:		/* FirstName */
		strncpy(patient.Firstname, start, 11);
		break;
	    case 7:		/* ID */
		strncpy(patient.ID, start, 11);
		break;
	    case 8:		/* Ear */
		k = toupper(tokstr[0]);
		if (k == 'L')
		    patient.Ear = 'L';
		else if (k == 'R')
		    patient.Ear = 'R';
		break;
	    case 9:		/* Dob */
		strncpy(patient.Dob, start, 11);
		break;
	    case 10:		/* Comment */
		if (cmt < 0) {
		    strncpy(patient.Comment, start, 81);
		    cmt = 0;
		} else if (cmt < 3) {
		    strncpy(comment[cmt], start, 39);
		    cmt++;
		}
		break;
	    case 11:		/* FileName */
		strcpy(file_name, tokstr);
		break;
	    case 12:		/* Date */
		strncpy(outmsg[0], tokstr, 15);
		break;
	    case 13:		/* Time */
		tim = atoi(tokstr);
		break;
	    case 14:		/* Accepted */
		strncpy(outmsg[1], start, 15);
		break;
	    case 15:		/* thev_source */
		strncpy(thsfn, tokstr, 39);
		break;
	    case 16:		/* Notch */
                if (itecnt > 0 && itecnt <= 2)
		    fnot[itecnt - 1] = (float) atof(tokstr);
		break;
	    case 17:		/* Fz1__ */
                if (itecnt > 0 && itecnt <= 2)
		    fzer[itecnt - 1] = (float) atof(tokstr);
		break;
	    case 18:		/* Other_Ear */
	        strncpy(otherfn, tokstr, 79);
	        break;
	    case 19:		/* DATfmt */
		datfmt = atoi(tokstr);
	        break;
	    }
	} else if (strlen(line) > 12) {
	    if (datfmt == 2) {	    // new data format
                sscanf(line, "%f %f %f %f", &f, &spl, &sil, &fpl);
	    } else {		    // old data format
                sscanf(line, "%f %f %f", &f, &spl, &cond);
	        if (iteacc && cond_thl.at)
	        	cond = (float) thev_cond(f);
		sil = (float) (spl + cond + 10 * log10(rho_c));
		fpl = -400;
	    }
            show_hearing_result(f, spl, sil, fpl, cflg);
	    cflg = thresh_cnct.at;
	}
    }
    fclose(fpt);

    sprintf(outmsg[2], "%d sweeps", swp);
    i = draw_w(&w_info);
    j = dis_patient() + 2;
    j += txtpar.font_height;
    gprintf(i, j, "probe:  %s", thsfn);
    if (*ite1fn) {
        j += txtpar.font_height;
        gprintf(i, j, "ite_1:  %s  fnz=%5.0f %5.0f", 
            ite1fn, fnot[0], fzer[0]);
    }
    if (*ite2fn) {
        j += txtpar.font_height;
        gprintf(i, j, "ite_2:  %s  fnz=%5.0f %5.0f", 
            ite2fn, fnot[1], fzer[1]);
    }

    while(*otherfn) {
        if (check_hearing_file(otherfn, 1)) {
            fpt = fopen(otherfn, "rt");
            gprintf(i, j, "Other_file:  %s", otherfn);
            j += txtpar.font_height;
            otherfn[0] = '\0';
            cflg = 0;
            while (fgets(line, MAXLINE, fpt) != NULL) {
	        if (line[0] == ';') {
	            line[0] = ' ';
	            get_token(1);
	            if (strlen(tokstr) == 0)
		        cflg = 0;
	            start = atline();
	            k = check_list(tokstr, kw);
	            get_token(0);
	            switch (k) {
	            case 8:		/* Ear */
		        k = toupper(tokstr[0]);
		        if (k == 'L')
		            patient.Ear = 'L';
		        else if (k == 'R')
		            patient.Ear = 'R';
		        break;
	            case 13:		/* Time */
		        otim = atoi(tokstr);
		        break;
	            case 19:		/* DATfmt */
		        datfmt = atoi(tokstr);
		        break;
		    }
                } else if (strlen(line) > 12) {
		    if (datfmt == 2) {
			sscanf(line, "%f %f %f %f", &f, &spl, &sil, &fpl);
		    } else {
			sscanf(line, "%f %f %f", &f, &spl, &cond);
			sil = (float) (spl + cond + 10 * log10(rho_c));
			fpl = -400;
		    }
	            show_hearing_result(f, spl, sil, fpl, cflg);
	            cflg = thresh_cnct.at;
                }
            }
            fclose(fpt);
        }
    }

    if (*ite1fn || *ite2fn) {
        display_reset = 1;
	if (*ite1fn) {
	    if (cal_open(ite1fn, msg, 1)) {
                cal_read(1);
                dis_units(1 / cal_vpc, cal_vpc / cal_ppc);
 	        dfgc = scrn_c[C_WAVA];
	        dis_stim(0, accbuf, FLOAT);
	        cal_close();
                display_reset = 0;
            }
	}
        if (*ite2fn) {
	    if (cal_open(ite2fn, msg, 1)) {
                cal_read(1);
                dis_units(1 / cal_vpc, cal_vpc / cal_ppc);
 	        dfgc = scrn_c[C_WAVB];
	        dis_stim(0, accbuf, FLOAT);
	        cal_close();
                display_reset = 0;
            }
	}
        dfgc = scrn_c[C_WAVE];
    } else {
	draw_w(&w_stim);
	draw_w(&w_stim_fft);
    } 
    fileflag = 1;
    out_msg(tim, outmsg, otim);
    pre_dis_ind("Artifact", "Limit");
    dis_artifact();
}
