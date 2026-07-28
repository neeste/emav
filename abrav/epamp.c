/* epamp.c */

#include <math.h>
#include <stdio.h>
#ifdef WIN32
#include <windows.h>
#include <io.h>
#else /* WIN32 */
#include <unistd.h>
#endif /* WIN32 */

static double lpf_vars[] = {30,  100, 300, 500, 1000, 1500, 3000, 5000};
static double gain_vars[] = {30, 50, 75, 100, 150, 200, 250, 300};

void
msdelay(int msec)
{
#ifdef WIN32
    Sleep(msec);
#else /* WIN32 */
    usleep(msec * 1000);
#endif /* WIN32 */
}

/*
 * The Navigator E amp/AD/stim board uses 32 contiguous ports.
 * A DIP switch allows the base address to be relocated.
 * The standard base address is 0x300.
 *
 * offset     chip         description
 * ------   --------     ------------------------------------
 *  00-03   U57,8255     amplifier controls
 *  04-07   U58,8255     preamp and A/D controls
 *  08-0B   U59,8255     stimulator
 *  0C-0F   U60,8255     stimulator
 *  10-13   U51,741S244  input port (redundant)
 *  14-17   U62,8253     timer, stimulator
 *  18-1B   U35,8253     timer, A/D
 *  1C-1F   U46,AD7226   quad D/A converter
 *
 * High-pass cutoff frequency codes:
 *  0 = 0.1 Hz
 *  1 = 0.3 Hz
 *  2 = 1.0 Hz
 *  3 = 3.0 Hz
 *  4 =  10 Hz
 *  5 =  30 Hz
 *  6 = 100 Hz
 *  7 = 300 Hz
 *  8 = DC (no high pass)
 *
 * Low-pass cutoff frequency codes:
 *  0 =    30 Hz
 *  1 =   100 Hz
 *  2 =   150 Hz
 *  3 =   300 Hz
 *  4 =   500 Hz
 *  5 =   750 Hz
 *  6 = (1000 Hz, use 9 instead)
 *  7 =    70 Hz
 *  8 =  (300 Hz, use 3 instead)
 *  9 =  1000 Hz
 *  A =  1500 Hz
 *  B =  3000 Hz
 *  C =  5000 Hz
 *  D =  7500 Hz
 *  E = 10000 Hz
 *  F =   700 Hz
 *
 * Amp gain factor codes:
 *  0 =  10
 *  1 =  50
 *  2 =  75
 *  3 = 100
 *  4 = 150
 *  5 = 200
 *  6 = 240
 *  7 = 300
 *  8 =   1
 *  9 =   5
 *  A =   7.5
 *  B =  10
 *  C =  15
 *  D =  20
 *  E =  24
 *  F =  30
 *
 * AC coupling codes:
 *  0 =  AC
 *  1 =  DC
 *
 * Preamp gain factor codes:
 *  0 =  1000
 *  1 =  50
 *
 */

/*  Opt-Amp init codes
 *  
 * Gain
 * 0 = 30
 * 1 = 50
 * 2 = 75
 * 3 = 100
 * 4 = 150
 * 5 = 200
 * 6 = 250
 * 7 = 300
 * 
 * High Pass Filter
 * 0 = 1
 * 1 = 10
 * 2 = 30
 * 3 = 50
 * 4 = 100
 * 5 = 150
 * 6 = 300
 * 7 = 500
 * 
 * Low Pass Filter
 * 0 = 30
 * 1 = 100
 * 2 = 300
 * 3 = 500
 * 4 = 1000
 * 5 = 1500
 * 6 = 3000
 * 7 = 5000
 *
 * Notch filter codes:
 *  0 =  in
 *  1 =  out
 *
 */

void epamp_init(int gain, int hpf,int lpf, int notch)
{
#ifdef WIN32
    int rval;
    char string[50];
    char string2[50];
    char *rstring, *set_str;
    HGLOBAL clip_rval;
    HGLOBAL clip_mem = NULL;

    clip_mem = GlobalAlloc(GHND, 50);
    if(clip_mem == NULL)
    {
        //error getting global memory...abort!
        return;
    }
    if(_access("OPTIAMP.EXE", 0) == -1)
        return;
    rval = WinExec("OPTIAMP.EXE",SW_HIDE);      //start opti-amp program
    if(rval == ERROR_FILE_NOT_FOUND || rval == 0)
    {
        //couldn't start optiamp program...aborting!
        GlobalFree(clip_mem);
        return;
    }

    //set gain section
    sprintf(string, "OPTIAMP-CH1-GAIN=%d", gain+1);
    set_str = (char *) GlobalLock(clip_mem);
    memcpy(set_str, string, 50);
    GlobalUnlock(clip_mem);
    OpenClipboard(NULL);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, clip_mem);
    CloseClipboard();
    Sleep(1000);

    //make sure setting was recieved by opti-amp program
    if(IsClipboardFormatAvailable(CF_TEXT))
    {
        OpenClipboard(NULL);
        clip_rval = GetClipboardData(CF_TEXT);
        rstring = (char *) GlobalLock(clip_rval);
        sprintf(string2, "OK-%s", string);
        rval = strcmp(string2, rstring);
        if(rval)
        {
            //didn't werk...abort and stuff
            GlobalUnlock(clip_rval);
            CloseClipboard();
            GlobalFree(clip_mem);
            return;
        }
        GlobalUnlock(clip_rval);
        CloseClipboard();
    }
    else
        return;

    //high pass gain section
    clip_mem = GlobalAlloc(GHND, 50);
    set_str = (char *) GlobalLock(clip_mem);
    sprintf(string, "OPTIAMP-CH1-HP=%d", hpf+1);
    memcpy(set_str, string, 50);
    GlobalUnlock(clip_mem);
    OpenClipboard(NULL);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, clip_mem);
    CloseClipboard();
    Sleep(1000);

    //make sure setting was recieved by opti-amp program
    if(IsClipboardFormatAvailable(CF_TEXT))
    {
        OpenClipboard(NULL);
        clip_rval = GetClipboardData(CF_TEXT);
        rstring = (char *) GlobalLock(clip_rval);
        sprintf(string2, "OK-%s", string);
        rval = strcmp(string2, rstring);
        if(rval)
        {
            //didn't werk...abort and stuff
            GlobalUnlock(clip_rval);
            CloseClipboard();
            GlobalFree(clip_mem);
            return;
        }
        GlobalUnlock(clip_rval);
        CloseClipboard();
    }
    else
        return;

    //low pass gain section
    clip_mem = GlobalAlloc(GHND, 50);
    set_str = (char *) GlobalLock(clip_mem);
    sprintf(string, "OPTIAMP-CH1-LP=%d", lpf+1);
    GlobalUnlock(clip_mem);
    memcpy(set_str, string, 50);
    OpenClipboard(NULL);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, clip_mem);
    CloseClipboard();
    Sleep(1000);

    //make sure setting was recieved by opti-amp program
    OpenClipboard(NULL);
    clip_rval = GetClipboardData(CF_TEXT);
    rstring = (char *) GlobalLock(clip_rval);
    //rstring = (char *) clip_rval;
    sprintf(string2, "OK-%s", string);
    rval = strcmp(string2, rstring);
    if(rval)
    {
        //didn't werk...abort and stuff
        GlobalUnlock(clip_rval);
        CloseClipboard();
        GlobalFree(clip_mem);
        return;
    }
    GlobalUnlock(clip_rval);
    CloseClipboard();

    //notch filter section
    clip_mem = GlobalAlloc(GHND, 50);
    set_str = (char *) GlobalLock(clip_mem);
    if(notch == 0)
        sprintf(string, "OPTIAMP-CH1-SWITCH=OFF");
    else
        sprintf(string, "OPTIAMP-CH1-SWITCH=ON");
    GlobalUnlock(clip_mem);
    memcpy(set_str, string, 50);
    OpenClipboard(NULL);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, clip_mem);
    CloseClipboard();
    Sleep(1000);

    //make sure setting was recieved by opti-amp program
    OpenClipboard(NULL);
    clip_rval = GetClipboardData(CF_TEXT);
    rstring = (char *) GlobalLock(clip_rval);
    //rstring = (char *) clip_rval;
    sprintf(string2, "OK-%s", string);
    rval = strcmp(string2, rstring);
    if(rval)
    {
        //didn't werk...abort and stuff
        GlobalUnlock(clip_rval);
        CloseClipboard();
        GlobalFree(clip_mem);
        return;
    }
    GlobalUnlock(clip_rval);
    CloseClipboard();

    //save defaults section
    clip_mem = GlobalAlloc(GHND, 50);
    set_str = (char *) GlobalLock(clip_mem);
    sprintf(string, "OPTIAMP-SAVE");
    GlobalUnlock(clip_mem);
    memcpy(set_str, string, 50);
    OpenClipboard(NULL);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, clip_mem);
    CloseClipboard();
    Sleep(1000);

    clip_mem = GlobalAlloc(GHND, 50);
    strcpy(string,"OPTIAMP-CLOSE");
    rstring = (char *) GlobalLock(clip_mem);
    memcpy(rstring, string, 50);
    GlobalUnlock(clip_mem);
    OpenClipboard(NULL);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, clip_mem);
    CloseClipboard();


    GlobalFree(clip_mem);
#endif // WIN32
}

double
epamp_lpf(double f)
{
    double md, ad;
    int i, mi, nv;
	
    mi = 0;
    if (f > 0) {
        md = fabs(log(f / lpf_vars[0]));
	nv = sizeof(lpf_vars) / sizeof(double);
        for (i = 1; i < nv; i++) {
            ad = fabs(log(f / lpf_vars[i]));
            if (md > ad) {
                md = ad;
                mi = i;
            }
        }
    }
    return (lpf_vars[mi]);
}

double epamp_agn(double g)
{
    double md, ad;
    int i, mi, nv;

    mi = 0;
    if (g > 0) {
        md = fabs(log(g / gain_vars[0]));
	nv = sizeof(gain_vars) / sizeof(double);
        for (i = 1; i < nv; i++) {
            ad = fabs(log(g / gain_vars[i]));
            if (md > ad) {
                md = ad;
                mi = i;
            }
        }
    }
    return (gain_vars[mi]);
}
