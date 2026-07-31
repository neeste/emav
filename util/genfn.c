/* genfn.c */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(_WIN32) || defined(WIN32)
#include <io.h>
#else
#include <unistd.h>
#define _access access
#endif /* WIN32 */

void
get_date(int *m, int *d, int *y)
{
    time_t    clk;
    struct tm *tm_;

    clk = time(&clk);
    tm_ = localtime(&clk);
    *y = tm_->tm_year % 100;
    *m = tm_->tm_mon;
    *d = tm_->tm_mday;
}

/* generate a unique filename with date info */
int
genfn(char *c, char *e, char *f, int *count)
{
    int     i, y, m, d;

    get_date(&m, &d, &y);
    snprintf(f, 64, "%02d%c%02d%.1s00.%s", y, m + 'A', d, c, e);
    c = f + 6;
    e = c + 1;
    for (i = *count; i < 100; i++) {	/* 00 ... 99 */
        *c = i / 10 + '0';
        *e = i % 10 + '0';
	if (_access(f, 0) != 0) {
	    *count = i;
	    return (0);
	}
    }
    for (i = *count - 100; i < 26 * 26; i++) {	/* file numbers AA ... ZZ */
	*c = 'A' + i / 26;
	*e = 'A' + i % 26;
	if (_access(f, 0) != 0) {
	    *count = i + 100;
	    return (0);
	}
    }
    *c = '#';
    *e = '#';
    return (1);
}

#ifdef TEST
/* Here's an example of how to use genfn(). */
int
main(int ac, char **av)
{
    char fn[80];
    int  i;
    FILE *fp;

    genfn("T", "TOK", fn, &i);
    fp = fopen(fn, "w");
    fprintf(fp, "%s\n", fn);
    fclose(fp);
    printf("genfn: '%s'\n", fn);
    return(0);
}
#endif /* TEST */
