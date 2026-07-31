/* dsp.h */
#include <stdint.h>

#define DAC_DELAY 	0

char *dspapi(void);
char *dspdev(void);
char *dspname(int);
double dspclk(void);
double dspsets(double);
double dspsatt(double);
int dspinit(char *);
int dspgreg(int);
int dspseta(int);
int dspssr(int, int, int, int);
int dspdarr(unsigned int, int, short *);
int dspuarr(unsigned int, int, short *);
int dsprst(int);
int dspdget(void);
int dspdread(short *);
int32_t dspadjrt(int32_t);
int32_t dspsetrt(int32_t);
void dspsarr(double, int);
void dspinpflt(float **a, float **b, int *nfo);
void dspvfs(float *, float *);
void dspclr(void);
void dspcnt(int);
void dspcho(int, int);
void dspend(void);
