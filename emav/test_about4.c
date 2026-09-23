#include <stdio.h>
#include <string.h>

extern int ar_dev_name(int di, char *name, int len);
extern int _ar_io_open(int di);

int main() {
    char buf[80];
    int res1 = _ar_io_open(6);
    printf("open res = %d\n", res1);
    int res = ar_dev_name(6, buf, 80);
    printf("ar_dev_name(6) = %d, name = %s\n", res, buf);
    return;
}
