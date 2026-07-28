/* fixswp.c */

#include <stdio.h>
#include <stdlib.h>

main(int ac, char **av)
{
    char    key[4], hs[14];
    float   lv, gn = -20000;
    int     c;
    FILE   *ifp;
    FILE   *ofp;

    if (ac < 2) {
	printf("usage:  fixswp filename.swp\n");
	exit(0);
    }
    ifp = fopen(av[1], "rb");
    if (ifp == NULL) {
	printf("can't open %s\n", av[1]);
	exit(0);
    }
    ofp = fopen("record.swp", "wb");
    fread(key, 4, 1, ifp);
    fread(&lv, 4, 1, ifp);
    fread(hs, 14, 1, ifp);
    printf("opened %s: key=%s, lv=%.1f\n", av[1], key, lv);
    fwrite(key, 4, 1, ofp);
    fwrite(&lv, 4, 1, ofp);
    fwrite(hs, 14, 1, ofp);
    fwrite(&gn, 4, 1, ofp);
    while ((c = getc(ifp)) != EOF) {
	putc(c, ofp);
    }
    fclose(ifp);
    fclose(ofp);
}
