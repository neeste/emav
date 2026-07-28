// util.h - prototypes for utilities library
#include <stdint.h>

int bessel(float *, float *, float *, int, int);
int butter(float *, float *, float *, int, int);
int count_files(void);
int genfn(char *c, char *e, char *f, int *count);
int nint(double x);
int rdils(char *fnam, short *buff, int32_t blen, int32_t *rate, int32_t *npts);
int today_file(char *fn);
void bwfilt(float *ht, int npts, double r, int npole, int hpf);
void fillinblank(char *s, int n);
void float2long(int32_t *lht, int n);
void get_date(int *m, int *d, int *y);
void long2float(int32_t *lht, int n); 
void newext(char *f, char *e);
void take_files_bwt(int start, int end, char *files);
void trim(char *s);

#ifdef FILE
void file_flush(FILE *fp);
#endif // FILE

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif
