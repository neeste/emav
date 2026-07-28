/* swppol.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int
chk_swp_fmt(char *key)
{
    int     f;
    static char *key_str[] = {"SWP", "SW1", "SW2", "SW3", "SW4", "SW5", "SW6"};
    static int nfmt = sizeof(key_str) / sizeof(key_str[0]);

    for (f = 0; f < nfmt && strcmp(key, key_str[f]); f++)
	continue;
    return ((f < nfmt) ? f : -1);
}

void
swppol(char *fn, int rflg)
{
    char    key[4], hs[14];
    float   lv, gn;
    int     swpfmt;
    FILE   *fp;

    fp = fopen(fn, "r+b");
    if (fp == NULL) {
        printf("can't open %s\n", fn);
        return;
    }
    fread(key, 4, 1, fp);
    fread(&lv, 4, 1, fp);
    fread(hs, 14, 1, fp);
    swpfmt = chk_swp_fmt(key);
    printf("opened %s: key=%s, lv=%.1f fmt=%d\n", fn, key, lv, swpfmt);
    if (swpfmt >= 5) {
        fread(&gn, 4, 1, fp);
    } else {
        gn = 2e5;
    }
    if (rflg) {
        if (swpfmt >= 5) {
            rewind(fp);
            gn = -gn;
            fwrite(key, 4, 1, fp);
            fwrite(&lv, 4, 1, fp);
            fwrite(hs, 14, 1, fp);
            fwrite(&gn, 4, 1, fp);
            printf("*** polarity has been reversed ***\n");
        } else {
            printf("*** polarity cannot be reversed ***\n");
        }
    }
    fclose(fp);
    printf("    epa_gain = %.1f with %s polarity\n", fabs(gn), 
        (gn < 0) ? "reversed" : "normal");
}

void
main(int ac, char **av)
{
    int     rflg = 0;

    while (ac > 2 && av[1][0] == '-') {
        if (av[1][1] == 'r') {
            rflg = 1;
        }
        ac--;
        av++;
    }
    if (ac < 2) {
	printf("usage:  swppol [-r] filename.swp\n");
	printf("option:  -r     reverse polarity\n");
	exit(0);
    }
    while (ac > 1) {
        swppol(av[1], rflg);
        ac--;
        av++;
    }
}
