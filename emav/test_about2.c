#include <stdio.h>
#include <string.h>

extern int ar_dev_name(int di, char *name, int len);

int main() {
    char buf[80];
    int res = ar_dev_name(6, buf, 80);
    printf("ar_dev_name(6) = %d, name = %s\n", res, buf);
    return 0;
}
