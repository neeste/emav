/* ================================= file.c ============================ */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <fcntl.h>
#include <time.h>
#include <dsp.h>
#include <menu.h>
#include <token.h>
#include <util.h>
#include "savage.h"
#include "color.h"
#include "version.h"
#include "screen.h"
#include "emav.h"

#ifdef WIN32
#include <io.h>
#include <direct.h>
#else
#include <unistd.h>
#define _access access
#define _chdir  chdir
#define _getcwd getcwd
#endif /* WIN32 */

#define limit(min,val,max)	((val<min)?min:((val>max)?max:val))
#define TICKNUM		(CLOCKS_PER_SEC/3)

extern char tokstr[], line[], swp_file[];
extern char *tmp_dat_file, *tmp_tok_file, *not_saved;
extern char *absci[], *dpsigtyp[], *dpfrqtyp[];
extern float limit_mv;
extern int nab, ncfn;
extern int dfgc, sig_type, nnsb, num_oct;
extern int swp1set, teoae_counter, dpoae_counter;
extern TOGGLETYPE xtype, dpftype, datafmt, Sbin, Suppr, r_mode, stim_unit;
extern TOKENFILE *t;
extern WIND w_stim_fft;

/* Exit the program */
int
Exit_wind(char *no_use)
{
    pgm_exit();
    return (27);
}

char    o_file_name[MAXNAME] = {0};
char    cali_file[MAXNAME] = {0};
char    dpoae_fn[MAXNAME] = {0};

static char curdir[MAXPATH] = {0};
static char ofiledir[MAXPATH] = {0};
static char *sf[] = {"*.TOK", "*.DAT", "*.SWP", "*.CAL", "*.PRB"};
static int (*rd_file[]) () = {
    rd_teoae_file, rd_dpoae_file, rd_dpswp_file, rd_dpcal_file, rd_probe_file
};

static TOGGLETYPE filetypes = {
    1,
    {"TEOAE", "DPOAE", "DPSWP", "DPCAL", "PROBE", NULL},
};
int     fileflag = -1;

/* save the opened data file, put in new patient info */
int
savefile(char *no_use)
{
    TOKENFILE *tfp;
    FILE   *dfp, *dfpi;
    int     renflg = 0;
    char    tmpfn[16];
    static char *kw[] = {
	"LastName", "FirstName", "Ear", "ID", "Comment", "DOB", ""
    };

    if (fileflag == 0) {
	if (!strcmp(file_name, not_saved)) {
	    genfn("T", "TOK", file_name, &teoae_counter);
	    teoae_counter++;
	    write_counter_file();
	    tfp = topen(tmp_tok_file);
	    renflg = 1;
	} else {
	    tfp = topen(file_name);
	}
	if (tfp == NULL)
	    return (0);
	memcpy(tfp->hitch.comment, &patient, sizeof(PATIENT));
	twrhitch(tfp);
	tclose(tfp);
	if (renflg)
	    rename(tmp_tok_file, file_name);
        decide(0, 2, "TEOAE file saved as", file_name);
    } else if (fileflag == 1) {
	if (!strcmp(file_name, not_saved)) {
	    strcpy(tmpfn, "OUT.DAT");
	    genfn("D", "DAT", file_name, &dpoae_counter);
	    dpoae_counter++;
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
	    decide(0, 3, "Can't open file", file_name,
		"Press any key to continue");
	    fclose(dfpi);
	    if (renflg)
		rename(tmpfn, file_name);
	    return (0);
	}
	fgets(line, MAXLINE, dfpi);
	trim(line);
	get_token(1);
	if (strcmp(tokstr, ";DPOAE")) {
	    decide(0, 3, "Not a DPOAE file",
		";DPOAE signature missing",
		"Press any key to continue");
	    fclose(dfp);
	    fclose(dfpi);
	    if (renflg)
		rename(tmpfn, file_name);
	    return (0);
	}
	fprintf(dfp, "%s\n", line);
	fprintf(dfp, ";LastName:  %s\n;FirstName: %s\n",
	    patient.Lastname, patient.Firstname);
	fprintf(dfp, ";Ear: %5s\n;ID: %s\n", patient.Ear == 'R' ?
	    "Right" : (patient.Ear == 'L' ? "Left" : "?"), patient.ID);
	fprintf(dfp, ";DoB: %s\n", patient.Dob);
	fprintf(dfp, ";Threshold: %s\n", patient.Threshold);
	fprintf(dfp, ";Comment: %s\n", patient.Comment);
	while (fgets(line, MAXLINE, dfpi) != NULL) {
	    trim(line);
	    if (line[0] != ';')
		fprintf(dfp, "%s\n", line);
	    else {
		line[0] = ' ';
		get_token(1);
		if (!check_list(tokstr, kw)) {
		    line[0] = ';';
		    fprintf(dfp, "%s\n", line);
		}
	    }
	}
	fclose(dfpi);
	fclose(dfp);
	remove(tmpfn);
        decide(0, 2, "DPOAE file saved as", file_name);
	strcpy(o_file_name, file_name);
	rd_dpoae_file();
    } 
    return (27);
}

/* restart from the beginning of the program and clear screen */
int
Start_New()
{
    defpar();
    read_init_file();
    read_args();
    Sen = Sys;
    dis_units(Sen.AD, Sen.MP);
    patient.Comment[0] = '\0';
    patient.Threshold[0] = '\0';
    patient.Lastname[0] = '\0';
    patient.Firstname[0] = '\0';
    patient.ID[0] = '\0';
    patient.Dob[0] = '\0';
    cali_file[0] = 0;
    getstim("", buflen);
    mainmenu_exit();
    set_trailer((void (*)(void))0);
    return (27);
}

MENUITEM file_opt[] = {
    {"&Open...", NULL, NONE, 0, 0, 1, file_items},
    {"&New", NULL, NONE, 0, 0, 1, Start_New},
    {"Change &Directory...", NULL, NONE, 0, 0, 1, chg_dir},
    {"&Print Screen  (^P)", NULL, NONE, 0, 0, 1, prn_screen_select},
    {"P&rinter Setup...", NULL, NONE, 0, 0, 1, prn_setup},
    {"File &Save", NULL, NONE, 0, 0, 0, savefile},
    {"&Create List File", NULL, NONE, 0, 0, 1, create_list},
    {"Create &Load File", NULL, NONE, 0, 0, 1, create_load},
    {"&Extract SFOAE", NULL, NONE, 0, 0, 1, extract_sfoae},
    {"&About EMAV... (F1)", NULL, NONE, 0, 0, 1, display_version},
    {"E&xit          (^C)", NULL, NONE, 0, 0, 1, Exit_wind},
    {NULL, NULL, NONE, 0, 0, 0, NULL}
};

/* when toggles the file type, generic filename changes accordingly */
int
toggle_type(char **s)
{
    strcpy(o_file_name, ofiledir);
    strcat(o_file_name, sf[filetypes.at]);
    return (0);
}

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

/* save thr directory info of a filename */
int
savedir(char **s)
{
    struct PATH path;

    split_path(*s, path.drive, path.dir, path.name, path.ext);
    strcpy(ofiledir, path.drive);
    strcat(ofiledir, path.dir);
    return (0);
}

MENUITEM sub_file[] = {
    {"&Open File", NULL, NONE, 0, 0, 1, open_file},
    {"File &Type =", (char *) &filetypes, TOGGLE, 0, 0, 1, toggle_type},
    {"File &Name :=", o_file_name, STRING, 127, 0, 1, savedir},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

/* this function trails from  the open_file ITEM in the FILE menu */
void
file_trailer(void)
{
    strcpy(o_file_name, ofiledir);
    strcat(o_file_name, sf[filetypes.at]);
    simple_submenu(menu_step(2), txtpar.menu_height * 2, sub_file);
}

int
file_items(char *no_use)
{
    set_trailer(file_trailer);
    return (27);
}

static int
setcwd(char **s)
{
    _chdir(curdir);
    _getcwd(curdir, MAXPATH);
    return (0);
}

MENUITEM sub_chdir[] = {
    {"&Current Directory =", curdir, STRING, MAXPATH, 0, 1, setcwd},
    {NULL, NULL, NONE, 0, 0, 1, NULL}
};

/* Change Directory in FILE menu */
int
chg_dir(char *no_use)
{
    _getcwd(curdir, MAXPATH);
    simple_submenu(menu_step(3), txtpar.menu_height * 3, sub_chdir);
    return (27);
}

int
File_wind()
{
    return (simple_submenu(0, txtpar.menu_height, file_opt));
}

/* when read the scaled data from a file, use this function to scale the
   int buffer back to long buffer */
void
scaleback(int tokmax, int32_t maxval, short *sbuf, int32_t *lbuf)
{
    int     i;
    float   scale;

    if (tokmax > 0)
	scale = (float) maxval / tokmax;
    else
	scale = 1.0;
    for (i = 0; i < buflen; i++)
	lbuf[i] = (int32_t) (scale * (float) sbuf[i] + 0.5);
}

/* check if it is a TEOAE file */
int
check_teoae_file(char *fn, int flag)
{
    int ok = 0;
    TOKENFILE *tfp;

    tfp = topen(fn);
    if (!tfp) {
	if (flag)
	    decide(0, 2, "Can't open file", o_file_name);
    } else if (tfp->header.stype != 11111 || strcmp(tfp->header.pattern, "TEOAE")) {
        tclose(tfp);
	if (flag)
	    decide(0, 3, o_file_name, "is not a valid TEOAE token file.",
		"Press any key to continue.");
    } else {
        tclose(tfp);
	ok = 1;
    }
    return (ok);
}

void
set_dis_units_car(TOKENFILE *tfp)
{
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
}

extern int xbegin, xrange;

/* read in a TEOAE file */
int
rd_teoae_file()
{
    TOKENFILE *tfp;
    int     i, j, k, sweeps = 0;
    extern float resid_vpp;

    if (!check_teoae_file(o_file_name, 1)) {
	return (0);
    }
    tfp = topen(o_file_name);
    rate = (int32_t) (tfp->header.isf
	* pow(10.0, (double) tfp->header.power10));
    target_voltage = tfp->hitch.target_level * Sen.MP;
    resid_vpp = tfp->hitch.resid_vpp;
    memcpy(&patient, tfp->hitch.comment, sizeof(PATIENT));
    strcpy(file_name, o_file_name);
    buflen = (int) tfp->hitch.toksiz;
    tread(outbuf, 2, tfp);
    attenu.att1 = (float) (tfp->car.atten1 / 100.0);
    attenu.att2 = (float) (tfp->car.atten2 / 100.0);
    scaleback(tfp->car.tokmax, tfp->car.maxval, outbuf, accbuf_a);
    tread(outbuf, 3, tfp);
    i = tfp->car.reps;
    if (i <= 0) {
	tclose(tfp);
	return (0);
    }
    out_msg(tfp->hitch.time, tfp->hitch.outmsg);
    scaleback(tfp->car.tokmax, tfp->car.maxval, outbuf, accbuf_b);
    tread(outbuf, 1, tfp);
    set_dis_units_car(tfp);
    swp1set = tfp->car.reps;
    stim_title(tfp->car.sname);
    scaleback(tfp->car.tokmax, tfp->car.maxval, outbuf, savbuf);
    memcpy(outbuf, savbuf, buflen * sizeof(int32_t));
    sweeps = i / swp1set;
    draw_w(&w_info);
    xbegin = 0;
    xrange = buflen;
    show_A_B(sweeps, 0, 1, 0);
    text_color(w_stim.wfgc, w_stim.wbgc);
    j = w_stim.xbot - 5 * 8;
    k = w_stim.ybot - txtpar.font_height;
    gprintf(j, k, "%3.0f%%", tfp->hitch.stab);
    limit_mv = tfp->hitch.rejlmt;
    pre_dis_ind("Artifact", "Limit");
    dis_artifact();
    tclose(tfp);
    fileflag = 0;

    return (sweeps);
}

int
check_dpoae_file(char *fn, int flag)
{
    int     ok = 0;
    FILE   *fpt;

    fpt = fopen(fn, "rt");
    if (!fpt) {
	if (flag) {
	    decide(0, 3, "Can't open file", fn,
		"Press any key to continue");
	}
    } else {
        fgets(line, MAXLINE, fpt);
	get_token(1);
	if (strcmp(tokstr, ";DPOAE")) {
	    fclose(fpt);
	    if (flag) {
		decide(0, 3, "Not a DPOAE file",
		    ";DPOAE signature missing", 
		    "Press any key to continue");
	    }
	} else {
	    fclose(fpt);
	    ok = 1;
	}
    }
    return (ok);
}

int
skip_num(char *s, int n)
{
    int i = 0;

    while (n-- > 0) {
        while(s[i] && s[i] <= ' ')      /* skip spaces */
            i++;
        while(s[i] && s[i] > ' ')      /* skip non-spaces */
            i++;
    }
    return (i);
}

int
set_level_unit(char *s)
{
    int c, u = 0;

    c = toupper(s[0]);
    if(c == 'D')				// dB = SPL
        u = 0;
    else if (s[0] == 'S' && s[1] == 'P')	// SPL
        u = 0;
    else if (s[0] == 'S' && s[1] == 'L')	// SL
        u = 1;
    else if (c == 'V')				// V
        u = 2;
    else if (c == 'F')				// FPL
        u = 3;
    else if (s[0] == 'S' && s[1] == 'I')	// SIL
        u = 4;

    return (u);
}

int
rd_dpoae_file()
{
    char    outmsg[3][16] = {{0}};
    char   *start;
    char    comment[3][40];
    float   f, f2, f1, pst[4], nst[2], tm, lev[14];
    float   pdp[NEX], ndp[NEX], adp[NEX];
    int     i, j, k, flag, swp, tim, cmt, nd, cal_flg, plot_yet, idp, sf;
    int     dpf_old, datafmt_old, sbin_old, nnsb_old, sigt_old, suppr_old;
    FILE   *fpt;
    static char *kw[] = {
	"sweeps", "stimulus", "CalibratePhase", "calibrate", "list",
	"LastName", "FirstName", "ID", "Ear", "DOB", "Comment",
	"FileName", "Date", "Time", "Accepted", "DP_freq", 
	"Units", "Level_Unit", "MP_sensitivity", "MP_transfer",
	"Threshold", "limit", ""
    };

    if (!check_dpoae_file(o_file_name, 1)) {
	return (0);
    }
    fpt = fopen(o_file_name, "rt");
    strcpy(file_name, o_file_name);
    o_file_name[0] = o_file_name[40] = o_file_name[80] = 0;
    dpf_old = dpftype.at;
    datafmt_old = datafmt.at;
    sbin_old = Sbin.at;
    suppr_old = Suppr.at;
    nnsb_old = nnsb;
    sigt_old = sig_type;
    stim_unit.at = 0;
    plot_yet = 0;
    num_oct = 0;
    flag = 0;
    swp = 0;
    tim = 0;
    cmt = -1;
    initial_dp();
    while (fgets(line, MAXLINE, fpt) != NULL) {
	if (line[0] == ';') {
	    line[0] = ' ';
	    proc_hdr();
	    get_token(1);
	    if (strlen(tokstr) == 0)
		flag = 0;
	    start = atline();
	    k = check_list(tokstr, kw);
	    get_token(0);
            j = check_list(tokstr, absci);
	    switch (k) {
	    case 1:		/* sweeps */
		swp = atoi(tokstr);
		break;
	    case 2:		/* stimulus */
                get_file_name();
		stim_title(tokstr);
		break;
	    case 3:		/* CalibratePhase */
		/* disambiguate "calibrate" */
		break;
	    case 4:		/* calibrate */
		strncpy(o_file_name, tokstr, 39);
		break;
	    case 5:		/* list */
		strncpy(&o_file_name[40], tokstr, 39);
		break;
	    case 6:		/* LastName */
		strncpy(patient.Lastname, start, 12);
		break;
	    case 7:		/* FirstName */
		strncpy(patient.Firstname, start, 11);
		break;
	    case 8:		/* ID */
		strncpy(patient.ID, start, 11);
		break;
	    case 9:		/* Ear */
		k = toupper(tokstr[0]);
		if (k == 'L')
		    patient.Ear = 'L';
		else if (k == 'R')
		    patient.Ear = 'R';
		break;
	    case 10:		/* Dob */
		strncpy(patient.Dob, start, 11);
		break;
	    case 11:		/* Comment */
		if (cmt < 0) {
		    strncpy(patient.Comment, start, 69);
		    cmt = 0;
		} else if (cmt < 3) {
		    strncpy(comment[cmt], start, 39);
		    cmt++;
		}
		break;
	    case 12:		/* FileName */
		strncpy(&o_file_name[80], tokstr, 39);
		break;
	    case 13:		/* Date */
		strncpy(outmsg[0], tokstr, 15);
		break;
	    case 14:		/* Time */
		tim = atoi(tokstr);
		break;
	    case 15:		/* Accepted */
		strncpy(outmsg[1], start, 15);
		break;
	    case 16:		/* DP_freq */
		i = check_list(tokstr, dpfrqtyp);
		if (i > 0)
		    dpftype.at = i - 1;
		break;
	    case 17:		/* Units */
	    case 18:		/* Level_Unit */
		stim_unit.at = set_level_unit(tokstr);
		break;
	    case 19:		/* MP_sensitivity */
		Sen.MP = (float) atof(tokstr);
		break;
	    case 20:		/* MP_transfer */
		strncpy(Sen.MPfn,tokstr,41);
		break;
	    case 21:		/* Threshold */
		strncpy(patient.Threshold, start, 11);
		break;
	    case 22:		/* limit */
		limit_mv = (float) atof(tokstr);
		break;
	    }
	} else if (strlen(line) > 12) {
	    if (datafmt.at == 0) {
	        sscanf(line, "%f %f %f %f %f %f %f", 
		    &f2, &f1, &pst[1], &pst[0], &tm, &pdp[0], &ndp[0]);
		adp[0] = 0;
		lev[12] = lev[13] = -99;
	    } else {
                if (datafmt.at == 3)
		    nd = 5 + 4;
		else
		    nd = 5 + 3 * num_oct;
	        sscanf(line, "%f %f %f %f %f", &f2, &f1, &pst[1], &pst[0], &tm);
	        for (i = 0; i < nd; i++) {
	            j = skip_num(line, 5 + i * 3);
	            sscanf(line + j, "%f %f %f", &pdp[i], &ndp[i], &adp[i]);
	        }
                j = skip_num(line, 5 + nd * 3);
	        sscanf(line + j, "%f %f %f %f", &nst[0], &tm, &nst[1], &tm);
                j = skip_num(line, 5 + nd * 3 + 5);
	        sscanf(line + j, "%f", &pst[2]);
                j = skip_num(line, 5 + nd * 3 + 9);
	        sscanf(line + j, "%f", &pst[3]);
	    }
	    if (xtype.at == 1) {
		f = f1;
	    } else if (xtype.at == 2) {
		f = (float) dpfreq(f1, f2);
	    } else {
		f = f2;
	    }
	    idp = dpindx();
            lev[0] = pst[1];
            lev[1] = pst[0];
            lev[2] = pdp[idp];
            lev[3] = ndp[idp];
            if (num_oct >= 1) {
                lev[4] = pdp[5];
                lev[5] = pdp[6];
                lev[6] = pdp[7];
                lev[7] = ndp[7];
            }
            if (num_oct >= 2) {
                lev[8] = pdp[8];
                lev[9] = pdp[9];
                lev[10] = pdp[10];
                lev[11] = ndp[10];
            }
            lev[12] = pst[2];
            lev[13] = pst[3];
	    if (datafmt.at == 4)
	        sf = 2;
	    else if (f1 == f2)
	        sf = 1;
	    else
	        sf = 0;
	    show_dp(f, lev, flag, flag, -1, sf, plot_yet++);
	    flag++;
	}
    }
    fclose(fpt);

    sprintf(outmsg[2], "%d sweeps", swp);
    i = draw_w(&w_info);
    k = dis_patient() + 2;
    if (o_file_name[80]) {
        newext(&o_file_name[80], "BIN");
        if (_access(&o_file_name[80], 0) == 0) {
            gprintf(i, k, "Binary file:  %s", &o_file_name[80]);
            k += txtpar.font_height;
        }
    }
    if (o_file_name[40]) {
	gprintf(i, k, "List file:  %s", &o_file_name[40]);
	k += txtpar.font_height;
    }
    gprintf(i, k, "Calibration:  %s", o_file_name);
    k += txtpar.font_height;
    gprintf(i, k, "Fd frequency:  %s", dpfrqtyp[dpftype.at]);
    if (cmt > 0) {
	for (swp = 0; swp < cmt; swp++) {
	    k += txtpar.font_height;
	    gprintf(i, k, comment[swp]);
	}
    }
    if (*o_file_name) {
        if (cal_read(o_file_name, 0)) {
	    cal_disp(o_file_name, 0);
	    thev_adj_show(o_file_name);
            cal_flg = 0;
	} else {
            cal_flg = 1;
	}
    } else {
        cal_flg = 1;
    }
    if (cal_flg) {
	draw_w(&w_stim);
	draw_w(&w_stim_fft);
    }
    dpftype.at = dpf_old;
    datafmt.at = datafmt_old;
    Sbin.at = sbin_old;
    Suppr.at = suppr_old;
    nnsb = nnsb_old;
    sig_type = sigt_old;
    draw_w(&w_spec);
    out_msg(tim, outmsg);
    pre_dis_ind("Artifact", "Limit");
    dis_artifact();
    fileflag = 1;

    return (1);
}

int
rd_dpswp_file()
{
    strcpy(swp_file, o_file_name);
    r_mode.at = 2;

    return (1);
}

int
rd_dpcal_file()
{
    w_spec.ybot = ypix - 1;
    if (cal_read(o_file_name, 0))
 	cal_disp(o_file_name, 1);
    fileflag = 2;

    return (1);
}

int
check_probe_file(char *fn, int flag)
{
    if (!cal_read(fn, 0)) {
	if (flag) {
	    decide(0, 3, "Can't open file", fn,
		"Press any key to continue");
	}
	return (0);
    }
    return (1);
}

int
rd_probe_file()
{
    thev_source(o_file_name);
    fileflag = 2;

    return (1);
}

int
eventintime()
{
    int32_t    timer;
    int     rv;

    rv = 0;
    timer = clock() + TICKNUM;
    while (clock() < timer) {
	rv = check_event();
	if (rv)
	    break;
    }
    return (rv);
}

int
open_file(char *use2chk)
{
    char   *files, *beg;
    int     saved, i, j, k, nf, tmp, acpt, end, rc, x, y, cw;
    int     xx = 0, yy = 0, hold, status, mx, my;
    FILE   *fpt;
    PATIENT P = {"", "", "", "", 0, "", ""};
    TOKENFILE *tfp;
    WIND    w;
    static char lastfn[MAXPATH] = {0};
    static char *kw[] = {
	"LastName", "FirstName", "Ear", "ID", "DOB", "Threshold", 
	"Comment", ""
    };
    static int nrow = 15;
    static int start = 0, ptr = 0;

    file_opt[0].inputlen = 0;
    nf = count_files();
    if (nf == 0) {
	decide(0, 3, "Warning: File not found!",
	    o_file_name,
	    "Press any key to continue");
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
	    "Press any key to continue");
	return (0);
    }
    take_files_bwt(start, end, files);
    cw = txtpar.font_width;
    w.xtop = menu_step(2);
    w.xbot = w.xtop + (ncfn + 3) * cw;
    w.ytop = 6 * txtpar.menu_height;
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
	w.ybot = w.ytop + 6 * txtpar.font_height;
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
	    if (filetypes.at == 1) {
		fpt = fopen(line, "rt");
		if (fpt == NULL) {
		    status = 1;
		} else {
		    fgets(line, MAXLINE, fpt);
		    get_token(1);
		    if (strcmp(tokstr, ";DPOAE")) {
			status = 2;
			fclose(fpt);
		    } else {
			status = 4;
			k = 0;
			while (fgets(line, MAXLINE, fpt) != NULL) {
			    if (line[0] == ';') {
				line[0] = ' ';
				get_token(1);
				beg = atline();
				while(*beg == ' ')
				    beg++;
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
				case 5:	/* Birthdate */
				    strncpy(P.Dob, beg, 11);
				    break;
				case 6:	/* Threshold */
				    strncpy(P.Threshold, beg, 11);
				    break;
				case 7:	/* Comment */
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
	    } else if (filetypes.at == 0) {
		tfp = topen(line);
		if (tfp == NULL)
		    status = 1;
		else if (tfp->header.stype != 11111 ||
		    strcmp(tfp->header.pattern, "TEOAE")) {
		    tclose(tfp);
		    status = 3;
		} else {
		    status = 4;
		    memcpy(&P, tfp->hitch.comment, sizeof(PATIENT));
		    tclose(tfp);
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
		sprintf(line, "%-26s", "Not a valid TEOAE file.");
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
		sprintf(beg, "Threshold: %-15s", P.Threshold);
		beg += 27;
		sprintf(beg, "Comment: %-17s", P.Comment);
		break;
	    }
	    k = yy;
	    beg = (char *) line;
            txtpar.text_wind_len = 27;
	    for (i = 0; i <= 5; i++) {
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
	case ESC:
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
	    set_trailer((void (*)(void)) rd_file[filetypes.at]);
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
    if (files)
        free(files);
    return (rc);
}

int
extract_sfoae()
{
    char    msg[MAXLINE];
    char    sfoae_file[MAXNAME];

    strcpy(sfoae_file, file_name);
    if (dp2sf(file_name, sfoae_file, msg)) {
        strcpy(o_file_name, sfoae_file);
	set_trailer((void (*)(void)) rd_dpoae_file);
    } else {
        decide(0, 3, "Extract SFOAE:", file_name, msg);
    }
    return (27);
}
