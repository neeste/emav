#include <windows.h>
#include <stdio.h>
int main() {
#ifdef _WIN32_WINNT
    printf("_WIN32_WINNT is 0x%04X\n", _WIN32_WINNT);
#else
    printf("_WIN32_WINNT is undefined\n");
#endif
    return 0;
}
