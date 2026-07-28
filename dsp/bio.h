/* bio.h - bank I/O header file */
#include <stdint.h>

#define MAXNDC 8	/* maximum number of I/O channels */
#define limit(mn,aa,mx) (((aa)<(mn))?(mn):((aa)>(mx))?(mx):(aa))

#ifdef WIN32
char   *wa_devnam();
double  wa_adjust_rate(double);
double  wa_atten_input(double);
double  wa_atten_output(double);
double  wa_set_rate(double);
int     wa_check_bank();
int     wa_devsel(int);
int     wa_devsel_byname(char *);
int     wa_open(int *, double *, double *);
int     wa_reset_io();
int     wa_test();
void    wa_bank_get(int, int, int);
void    wa_bank_put(int, int, int, int, int);
void    wa_bank_ready(int);
void    wa_begin_io();
void    wa_close();
void    wa_get_vfs(double *, double *);
void    wa_info(char *);
void    wa_set_ndc(int *);
void    wa_set_vfs(double *, double *);
void    wa_stop_io();
#endif /* WIN32 */

typedef struct {
    int32_t accept;
    int32_t avmode[MAXNDC];
    int32_t resp_i[MAXNDC];
    int32_t stim_i[MAXNDC];
    int32_t *bank_b;
    int32_t *resp_a[MAXNDC];
    int32_t *resp_b[MAXNDC];
    int32_t *stim_b[MAXNDC];
} BIO;

extern BIO bio_;
