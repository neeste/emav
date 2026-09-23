#include <stdio.h>
#include <string.h>

extern char* dspdev(void);
extern char* dspapi(void);

int main() {
    printf("DSP API: %s\n", dspapi());
    printf("DSP DEV: %s\n", dspdev());
    return;
}
