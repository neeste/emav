#include <stdio.h>
#include <stdarg.h>

void my_decide(int set, int item, char *s, ...) {
    va_list ap;
    char *addr;
    int c;

    printf("Start first loop\n");
    addr = s;
    va_start(ap, s);
    for (c = 0; c < item; c++) {
        printf("c=%d, addr=%p\n", c, addr);
        if (addr) printf("String: %s\n", addr);
        addr = va_arg(ap, char *);
    }
    va_end(ap);

    printf("Start second loop\n");
    addr = s;
    va_start(ap, s);
    for (c = 0; c < item; c++) {
        printf("n=%d, addr=%p\n", c, addr);
        if (addr) printf("String: %s\n", addr);
        addr = va_arg(ap, char *);
    }
    va_end(ap);
    printf("Done\n");
}

int main() {
    my_decide(0, 6, "VERSION", "PGM_NAME", "COPYRIGHT", "BTNRH", "dspapi()", "dspdev()");
    return;
}
