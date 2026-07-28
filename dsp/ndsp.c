/* ndsp.c - DSP function calls (for no DSP) */

#include "dsp.h"

int ad_type = 0;        /* Will A/D clock fail to synchronize? */
int dsp_select;     /* Which DSP is installed ? */
int dsp_err_msg = 0;    /* Allow DSP error messages ? */

/*
 * dsptest - test which DSP card is present
 */
void
dsptest(char *dsp_file, char *file0, char *file1, char *file2)
{
    dsp_select = -1;
    dsp_err_msg = 1;
}

/*
 * dspinit - Initialize.
 */
int
dspinit(char *dsp_file)
{
    dsp_err_msg = 1;
    return (1);
}

/*
 * dspseta - Set Address.
 */
int
dspseta(int num)
{
    return (1);
}

/*
 * dspssr - Set Sample Rate.
 */
int
dspssr(int inp_rate, int out_rate, int sh_delay, int io_sync)
{
    return (1);
}

/*
 * dspdarr - Download array.
 */
int
dspdarr(unsigned int ofst, int nwords, short *parr)
{
    return (1);
}

/*
 * dspuarr - Upload array.
 */
int
dspuarr(unsigned int ofst, int nwords, short *parr)
{
    return (1);
}

/*
 * dsprst - Reset TMS32020.
 */
int
dsprst(int set)
{
    return (1);
}

/*
 * dspdget - Get 320 Status.
 */
int
dspdget(void)
{
    return (1);
}

/*
 * dspdread - Read Immediate.
 */
int
dspdread(short *data)
{
    return (1);
}

char *
dspdev(void)
{
    return ("[No DSP]");
}

double
dspsets(double s)
{
    return (1);
}

/* dspsarr - scale DSP array */

void
dspsarr(double s, int c)
{
}

/* dspadjrt - adjust a sample rate to nearest possble value */

int32_t
dspadjrt(int32_t r)
{
    return (r);
}

/* dspsetrt - set DSP sample rate */

int32_t
dspsetrt(int32_t r)
{
    return (r);
}

/* dspsatt - set DSP attenuator */

double
dspsatt(double a)
{
    return (1);
}

/* dspgreg - set SMA size */

int
dspgreg(int g)
{
    return (g);
}

/* dspsethpf - set DSP high-pass filter */

void
dspsethpf(float *f)
{
}

/* dspapi - return soundcard API */

char *
dspapi(void)
{
    return ("No DSP");
}

